#include "stdafx.h"

#include "md5.h"
#include "Util.h"
#include "zlib/zlib.h"
#include "ServerCommonDef.h"
#include "GameSocket.h"
#include "LogonCommHandler.h"
#include "SceneCommHandler.h"
#include "OpDispatcher.h"

extern bool m_ShutdownEvent;
extern GatewayServer g_local_server_conf;

#define WORLDSOCKET_SENDBUF_SIZE 32768
#define WORLDSOCKET_RECVBUF_SIZE 4096
#define MAX_CLIENTPACKET 2048

enum PacketTransferDest
{
	Transfer_dst_gateway	= 0x01,
	Transfer_dst_master		= 0x02,
	Transfer_dst_scene		= 0x04
};

//#define WORLDSOCKET_SENDBUF_SIZE 65535
//#define WORLDSOCKET_RECVBUF_SIZE 65535
//#define MAX_CLIENTPACKET 65535

//uint8 GameSocket::m_transpacket[NUM_MSG_TYPES];
//uint32 GameSocket::m_packetfrequency[NUM_MSG_TYPES];
//uint8 GameSocket::m_packetforbid[NUM_MSG_TYPES];
//uint32 GameSocket::m_disconnectcount = 0;

void GameSocket::InitTransPacket(void)
{
	//memset(m_transpacket, PROC_GAME, sizeof(uint8) * NUM_MSG_TYPES);
	//memset(m_packetforbid, 0, sizeof(uint8) * NUM_MSG_TYPES);

	////�⼸�����ݰ���Ҫ�������״̬��������ηַ�
	//m_transpacket[MSG_NULL_ACTION] = PROC_GATEWAY;

	////�հ�Ƶ�����ã��������ӵ����ݰ����ر��Ƕ����ݿ��в����������Ҫ�ܴ�ÿ�ε�½ֻ��һ�εģ����ó�0xFFFFFFFF
	//memset(m_packetfrequency, 0, sizeof(m_packetfrequency));

	//m_packetfrequency[CMSG_PING]			= 2000;
	//m_packetfrequency[CMSG_AUTH_SESSION]	= 5000;
	//m_packetfrequency[CMSG_MSG]				= 1000;
	//m_packetfrequency[CMSG_ENTERGAME]		= 0xFFFFFFFF;

	//m_packetfrequency[CMSG_CHAR_CREATE]		= 2000;
	//m_packetfrequency[CMSG_GET_CHARINFO]	= 2000;
	//m_packetfrequency[CMSG_PLAYER_PANEL_VIEW_REQ]= 1000;

}

string GameSocket::GenerateGameSocketLoginToken(const string &agent, const string &accName)
{
	char szData[512] = { 0 };
	uint32 rand_val = (rand() % 0xFF) * (rand() % 0xFF) * (rand() % 0xFF);
	uint32 time_val = UNIXTIME + getMSTime32();
	snprintf(szData, 512, "%s:%s:%u:%u", agent.c_str(), accName.c_str(), rand_val, time_val);
	string strData = szData;
	MD5 md5;
	md5.update(strData);

	return md5.toString();
}

uint64 GameSocket::GeneratePlayerHashGUID(const string& agent, const string& accName)
{
	string str = agent + accName;
	return BRDR_Hash(str.c_str());
}

GameSocket::GameSocket(SOCKET fd,const sockaddr_in * peer) : TcpSocket(fd, WORLDSOCKET_RECVBUF_SIZE, WORLDSOCKET_SENDBUF_SIZE, true, peer, 
																	RecvPacket_head_user2server, SendPacket_head_server2user, PacketCrypto_none, PacketCrypto_none)
{
	m_AuthStatus = GSocket_auth_state_none;
	
	m_lastPingRecvdMStime = getMSTime64();
	m_nSocketConnectedTime = m_lastPingRecvdMStime;
	m_ReloginNextMsTime = 0;
	m_SessionIndex = 0;
	m_nAuthRequestID = 0;
	m_PlayerGUID = 0;
	m_GMFlag = 0;
	m_uSceneServerID = 0;
	m_allowConnect = true;
	m_inPckBacklogState = false;

	m_nChatBlockedExpireTime = 0;

	if(!sSceneCommHandler.OnIPConnect(GetIP()))
	{
		m_allowConnect = false;
	}
#ifdef USE_FSM
	_sendFSM.setup(2059198199, 1501);
	_receiveFSM.setup(2059198199, 1501);
#endif
}

GameSocket::~GameSocket(void)
{

}

void GameSocket::SendPacket(WorldPacket * data)
{
	if(!data)
		return;

	if(!IsConnected())
		return;

	_HookSendWorldPacket(data);
	const uint32 opCode = data->GetOpcode();
	const uint32 packSize = data->size();
	
	//++sSceneCommHandler.m_clientPacketCounter[opCode];

	if (packSize >= 10240)		// ����10KB�Ļ�����Ҫѹ������з���
	{
		if (_OutPacketCompress(opCode, packSize, data->GetErrorCode(), (const void*)data->contents()))
			return ;
	}

	const uint32 ret = SendingData((packSize > 0 ? (void*)data->contents() : NULL), packSize, opCode, 0, 0, data->GetErrorCode(), 0);
	switch (ret)
	{
	case Send_tcppck_err_none:
		sLog.outString("[GameSocket����] ʵ�ʷ��� (plrGuid="I64FMTD"),opcode=%u,len=%u,err=%u)", m_PlayerGUID, opCode, packSize, data->GetErrorCode());
		sSceneCommHandler.g_totalSendBytesToClient += packSize;
		if (m_beStuck)
		{
			m_beStuck = false;
			sLog.outString("[GameSock]��ɫ(accName=%s)�����������ָ�����,���ÿռ�%u", m_strAccountName.c_str(), GetWriteBufferSpace());
		}
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			if(!m_beStuck)
			{
				sLog.outError("[GameSock]��ɫ(accName=%s)��������������,ʣ��ռ�%u,���ݰ�op=%u ����%u", m_strAccountName.c_str(), GetWriteBufferSpace(), opCode, packSize);
				m_beStuck = true;
			}
			WorldPacket *pck = NewWorldPacket(data);
			if (packSize > 0)
				pck->append((const uint8*)data, packSize);
			_queue.Push(pck);
		}
		break;
	default:
		sLog.outError("[GameSock]��ͻ���(accName=%s)��������������������,ret=%u", m_strAccountName.c_str(), ret);
		break;
	}
}

bool GameSocket::_OutPacketCompress(uint32 opcode, uint32 packSize, uint32 errCode, const void* data)
{
	uint32 destsize = compressBound(packSize);
	uint8 *buffer = new uint8[destsize];
	int cresult = compress(buffer, (uLongf*)&destsize, (const Bytef*)data, packSize);
	if(cresult!= Z_OK)
	{
		delete [] buffer;
		sLog.outError("ѹ������, Opcode =%u Error=%u",opcode,cresult);
		return false;
	}
	else
	{
		sLog.outDebug("��(opcode=%u) ԭ��%uѹ����%u", opcode, packSize, destsize);
		sSceneCommHandler.AddCompressedPacketFrenquency(opcode, packSize, destsize);
	}

	// ѹ�����ݺ��͵İ�����Ҫ��opcode����һ����ǣ�����ͻ�������
	const uint32 ret = SendingData(buffer, destsize, 1000000 + opcode, 0, 0, errCode, 0);

	//printf("ѹ��%d---%d\n",len,destsize + 4);
	delete [] buffer;

	return ret == Send_tcppck_err_none ? true : false;
}

void GameSocket::UpdateQueuedPackets()
{
	//queueLock.Acquire();
	if(!_queue.HasItems())
	{
		//queueLock.Release();
		return;
	}

	WorldPacket * pck = NULL;
	while((pck = _queue.front()))
	{
		const uint32 pckSize = pck->size();
		const uint32 ret = SendingData((pckSize > 0 ? (void*)pck->contents() : NULL), pckSize, pck->GetOpcode(), 0, 0, pck->GetErrorCode(), 0);
		/* try to push out as many as you can */
		switch(ret)
		{
		case Send_tcppck_err_none:
			{
				sSceneCommHandler.g_totalSendBytesToClient += pckSize;
				DeleteWorldPacket(pck);
				_queue.pop_front();
				if (m_beStuck)
				{
					m_beStuck = false;
					sLog.outString("[GameSock]��ɫ(accName=%s)�������ָ�����,ʣ��ռ�:%u", m_strAccountName.c_str(), GetWriteBufferSpace());
				}
			}
			break;
		case Send_tcppck_err_no_enough_buffer:
			{
				if(!m_beStuck)
				{
					sLog.outError("[GameSock]��ɫ(accName=%s)��������������,ʣ��ռ�%u,���ݰ�op=%u ����%u", m_strAccountName.c_str(), GetWriteBufferSpace(), pck->GetOpcode(), pckSize);
					m_beStuck = true;
				}
				sLog.outError("[GameSock]��ɫ(accName=%s)��������������,ʣ��ռ�%u,ʣ��%u�������´η�", m_strAccountName.c_str(), GetWriteBufferSpace(), _queue.GetElementCount());
				return;
			}
			break;
		default:
			{
				sLog.outError("[GameSock]��ͻ���(accName=%s)��������������������,ret=%u,׼��������ͻ������(��%u��)", m_strAccountName.c_str(), ret, _queue.GetElementCount());
				/* kill everything in the buffer */
				while((pck == _queue.Pop()))
				{
					DeleteWorldPacket(pck);
					pck = NULL;
				}
				//queueLock.Release();
				return;
			}
			break;
		}
	}
	//queueLock.Release();
}

void GameSocket::OnUserPacketRecved(WorldPacket *packet)
{
	// ���ͬ��ip����̫�࣬�ÿͻ��˿��ܲ��ᱻע��ɹ�������Ҫ�Ͽ�����
	if(!m_allowConnect)
	{
		DeleteWorldPacket(packet);
		Disconnect();
		return;
	}

	// �Կͻ������ݰ����д���
	const uint32 op_code = packet->GetOpcode();
	map<uint32, uint32>::iterator it = OpDispatcher::ms_OpcodeTransferDestServer.find(op_code);
	if (it != OpDispatcher::ms_OpcodeTransferDestServer.end())
	{
		if (it->second == Transfer_dst_gateway)
		{
			switch (op_code)
			{
			case CMSG_001003:
				_HandleUserAuthSession(packet);
				break;
			case CMSG_001011:
				_HandlePing(packet);
				break;
			default:
				break;
			}
			DeleteWorldPacket(packet);
		}
		else if (it->second == Transfer_dst_master)
		{
			_HookRecvedWorldPacket(packet);
			// ����ת�������������
			if (hadSessionAuthSucceeded())
			{
				packet->SetServerOpcodeAndUserGUID(CMSG_GATE_2_MASTER_SESSIONING, GetPlayerGUID());
				sSceneCommHandler.SendPacketToMasterServer(packet);
			}
			else
			{
				sLog.outError("�յ�δ�ɹ���֤���˺����ݰ�accName=%s, opcode=%u, size=%u", m_strAccountName.c_str(), op_code, packet->size());
			}
			DeleteWorldPacket(packet);
		}
		else if (it->second == Transfer_dst_scene)
		{
			_HookRecvedWorldPacket(packet);
			// ����ת������Ӧ�ĳ���������
			if (hadSessionAuthSucceeded())
			{
				if (m_uSceneServerID > 0)
				{
					packet->SetServerOpcodeAndUserGUID(CMSG_GATE_2_SCENE_SESSIONING, GetPlayerGUID());
					sSceneCommHandler.SendPacketToSceneServer(packet, m_uSceneServerID);
				}
				else
				{
					sLog.outError("��ҵ�ǰ�������κ�һ����,������������Э���������(accName=%s,op=%u,size=%u)", m_strAccountName.c_str(), op_code, packet->size());
				}
			}
			else
			{
				sLog.outError("�յ�δ�ɹ���֤���˺����ݰ�accName=%s, opcode=%u, size=%u", m_strAccountName.c_str(), op_code, packet->size());
			}
			DeleteWorldPacket(packet);
		}
		else
		{
			sLog.outError("[GameSocket]accName=%s,agent=%s,Lua��ת��ע���쳣op=%u,dst=%u", m_strAccountName.c_str(), m_strClientAgent.c_str(), op_code, it->second);
			DeleteWorldPacket(packet);
		}
	}
	else
	{
		sLog.outError("[GameSocket]accName=%s,agent=%s �յ��쳣����Ϣ��op=%u", m_strAccountName.c_str(), m_strClientAgent.c_str(), op_code);
		DeleteWorldPacket(packet);
	}
	//if(m_disconnectcount > 0)
	//	_CheckRevcFrenquency(op_code);

	// sLog.outString("[GameSocket]�յ������˺�["I64FMTD"]����Ϣ��,op=%u,size=%u", GetPlayerGUID(), packet->GetOpcode(), packet->size());
	//if ( op_code < NUM_MSG_TYPES )
	//{
	//	sSceneCommHandler.m_client_recv_pack_count[op_code]++;
	//	if ( m_packetforbid[op_code] )					// �������������Σ���ֱ��ɾ���˰�
	//	{
	//		DeleteWorldPacket(Packet);
	//		return;
	//	}
	//	else if (InPacketBacklogState())								// ������ڰ���ѹ״̬������ͼ�����ѹ������
	//	{
	//		// �����ѹ���У������հ����ж�һ��queue�Ĵ�С
	//		if (m_canPushInBacklogQueue[packet->GetOpcode()] > 0 && _pckBacklogQueue.GetElementCount() < 20)
	//		{
	//			_pckBacklogQueue.Push(packet);
	//			continue ;
	//		}
	//		else
	//		{
	//			DeleteWorldPacket(packet);
	//			continue;
	//		}
	//	}
	//}

	//switch(op_code)
	//{
	//case CMSG_PING:
	//	{
	//		_HandlePing(Packet);
	//		// ÿ5��ping(150��)֪ͨһ����Ϸ��,����Ϸ�����ָûỰPlayerEntity�Ļ�Ծ,������ж��
	//		if (m_pingCounter >= 5)
	//		{
	//			m_pingCounter = 0;
	//			if (hadSessionAuthSucceeded() && m_uSceneServerID > 0)
	//			{
	//				Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//				sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//			}
	//		}
	//		DeleteWorldPacket(Packet);
	//	}
	//	break;
	//case CMSG_AUTH_SESSION:
	//	{
	//		if ((m_AuthStatus == GSocket_auth_state_none) && (m_nAuthRequestID == 0) && !m_ShutdownEvent)
	//		{
	//			mClientStatus = eCSTATUS_SEND_AUTHREQUEST;
	//			mClientLog = "send auth request";
	//			_HandleAuthSession(Packet);
	//		}
	//		else
	//		{
	//			sLog.outError("�ظ��յ��˺� %u �ĻỰ��֤����,status=%u,reqID=%u", m_PlayerGUID, uint32(m_AuthStatus), m_nAuthRequestID);
	//			DeleteWorldPacket(Packet);
	//		}
	//	}
	//	break;
	//case CMSG_ENTERGAME:
	//	{
	//		mClientStatus = eCSTATUS_SEND_ENTERGAME;
	//		mClientLog = "request enter game";
	//		if (hadSessionAuthSucceeded() && m_uSceneServerID > 0)
	//		{
	//			// ���͸���Ϸ��
	//			Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//			sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//		}
	//		DeleteWorldPacket(Packet);
	//	}
	//	break;
	//case CMSG_MSG:
	//	{
	//		if (hadSessionAuthSucceeded())
	//			_HandlePlayerSpeek(Packet);
	//		DeleteWorldPacket(Packet);
	//	}
	//	break;
	//case CMSG_CHAR_CREATE:
	//	{
	//		std::string name;
	//		(*Packet) >> name;
	//	}
	//	break;
	//case CMSG_CHAR_RENAME:
	//	{
	//		std::string newname;
	//		(*Packet) >> newname;
	//		sLog.outString("receive rename request %s",newname.c_str());

	//		// 				if(sLang.containForbidWords(newname.c_str()))   //�����ּ��~~
	//		// 				{
	//		// 					WorldPacket packet(SMSG_CHAR_RENAME);
	//		// 					packet<<(uint8)2;
	//		// 					SendPacket(&packet);
	//		// 					sLog.outString("forbid words found %s",newname.c_str());
	//		// 				}
	//		// 				else
	//		// 				{
	//		if (hadSessionAuthSucceeded() && m_uSceneServerID > 0)
	//		{
	//			// ���͸���Ϸ��
	//			Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//			sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//		}

	//		/*				}*/
	//	}
	//	break;
	//default:
	//	{
	//		if (hadSessionAuthSucceeded() && op_code < NUM_MSG_TYPES)
	//		{
	//			//����������Ŀǰ���ڷ����������״̬��������m_transpacket����Ķ��壬�ַ����ݰ�����ͬ������ ���ı� 9��4��
	//			// ������Ϣ���ķַ�
	//			switch (m_transpacket[op_code])
	//			{
	//			case PROC_GAME:
	//				{
	//					HookWorldPacket(Packet);
	//					if (m_uSceneServerID > 0)	// ���ȷʵ����ĳ�����ڣ���������
	//					{
	//						Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//						sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//					}
	//					else
	//						Log.Debug("���ݰ�����","��Ϣ��:%d", op_code);
	//				}
	//				break;
	//			case PROC_GATEWAY:
	//				{
	//					HookWorldPacket(Packet);
	//					break;
	//				}
	//			}
	//		}
	//		DeleteWorldPacket(Packet);
	//		Packet = NULL;
	//	}
	//	break;
	//}
}

void GameSocket::SessionAuthedSuccessCallback(uint32 sessionIndex, string &platformAgent, string &accName, uint32 gmFlag, string existSessionToken)
{
	// ��֤�ɹ�
	m_AuthStatus |= GSocket_auth_state_ret_succeed;

	// ���ɵ�½����(�Ѿ������˴��ڵ��������ݣ����������µ��������ݣ��������Ա�֤��Ҵ�ͷ��βֻʹ��һ��token����)
	if (existSessionToken.length() > 0)
		m_strLoginToken = existSessionToken;
	else
		m_strLoginToken = GameSocket::GenerateGameSocketLoginToken(platformAgent, accName);

	pbc_wmessage *msg1004 = sPbcRegister.Make_pbc_wmessage("UserProto001004");
	pbc_wmessage_string(msg1004, "reloginKey", m_strLoginToken.c_str(), 0);
	// ֪ͨ�ؿͻ���
	WorldPacket packet(SMSG_001004, 128);
	sPbcRegister.EncodePbcDataToPacket(packet, msg1004);
	SendPacket(&packet);

	DEL_PBC_W(msg1004);

	m_SessionIndex = sessionIndex;
	m_strAccountName = accName;
	m_PlayerGUID = GameSocket::GeneratePlayerHashGUID(m_strClientAgent, accName);
	m_GMFlag = gmFlag;

	if (m_nAuthRequestID > 0)
	{
		// ���֮ǰ��ע�ᣬ���Ƴ��Ѿ�ע�����������
		sSceneCommHandler.RemoveReloginAccountData(accName);
		sLog.outString("[�Ự��֤�ɹ�]reqID=%u,�˺�%s(guid="I64FMTD"),gm=%u,token=%s", m_nAuthRequestID, m_strAccountName.c_str(), m_PlayerGUID, m_GMFlag, m_strLoginToken.c_str());
		m_nAuthRequestID = 0;
	}
	else
		sLog.outString("[�Ự�����ɹ�]�˺�%s(guid="I64FMTD"),gm=%u,��token=%s", m_strAccountName.c_str(), m_PlayerGUID, m_GMFlag, m_strLoginToken.c_str());

	sSceneCommHandler.AddPendingGameList(this, m_PlayerGUID);
	// ע��������������
	sSceneCommHandler.RegisterReloginAccountTokenData(m_strClientAgent, accName, m_PlayerGUID, m_strLoginToken, sessionIndex, gmFlag);
}

void GameSocket::SessionAuthedFailCallback(uint32 errCode)
{
	ASSERT(errCode != GameSock_auth_ret_success);

	mClientStatus = eCSTATUS_AUTHFAILED;
	mClientLog = "auth failed!";
	sLog.outError("[�Ự��֤ʧ��]accName=%s, errCode=%u", m_strAccountName.c_str(), uint32(errCode));
	m_AuthStatus |= GSocket_auth_state_ret_failed;

	// ֪ͨ�ؿͻ���
	WorldPacket packet(SMSG_001004, 4);
	packet.SetErrCode(errCode);
	SendPacket(&packet);
}

void GameSocket::SendChangeSceneResult(uint32 errCode, uint32 tarMapID /*= 0*/, float dstPosX /*= 0*/, float dstPosY /*= 0*/, float dstHeight /*= 0.0f*/)
{
	WorldPacket packet(SMSG_002006, 128);
	if (errCode != 0)
	{
		packet.SetErrCode(errCode);
	}
	else
	{
		pbc_wmessage *msg2006 = sPbcRegister.Make_pbc_wmessage("UserProto002006");
		if (msg2006 == NULL)
			return ;

		pbc_wmessage_integer(msg2006, "descSceneID", tarMapID, 0);
		pbc_wmessage_real(msg2006, "destPosX", dstPosX);
		pbc_wmessage_real(msg2006, "destPosY", dstPosY);
		pbc_wmessage_real(msg2006, "destPosHeight", dstHeight);

		sPbcRegister.EncodePbcDataToPacket(packet, msg2006);
		DEL_PBC_W(msg2006);
	}
	
	SendPacket(&packet);
}

bool GameSocket::InPacketBacklogState()
{
	bool bRet = false;

	m_backlogStateLock.Acquire();
	bRet = m_inPckBacklogState;
	m_backlogStateLock.Release();

	return bRet;
}

void GameSocket::TryToLeaveBackLogState()
{
	if (InPacketBacklogState())
		_LeaveBacklogState();
}

void GameSocket::TryToEnterBackLogState()
{
	if (!InPacketBacklogState())
		_EnterBacklogState();
}

void GameSocket::OnConnect()
{
	//�������ӵ�ʱ���ʼ���հ�ʱ��
	//memset(m_packrecvtime,0,sizeof(uint32)*NUM_MSG_TYPES);
	
	m_checkcount = 0;
	mClientStatus = 0;
	m_beStuck = false;
	mClientIpAddr = GetIP();
	
	sLog.outString("[GameSocket] %p Connected(ip=%s)", this, mClientIpAddr.c_str());
}

extern TextLogger * Crash_Log;
void GameSocket::OnDisconnect()
{
	static const uint8 snGameSocketAuthFailedStatus = (GSocket_auth_state_sended_packet | GSocket_auth_state_ret_failed);
	
	sSceneCommHandler.OnIPDisconnect(GetIP());
	sLog.outString("[GameSocket] %p(%s) Disconnected(ip=%s),auth=%u,agent=%s,accName=%s,sindex=%u", this, szGetPlayerAccountName(), mClientIpAddr.c_str(),
					uint32(m_AuthStatus), m_strClientAgent.c_str(), m_strAccountName.c_str(), m_SessionIndex);

	if(m_nAuthRequestID != 0)
	{
		sLog.outError("δ��֤�ͻ��˶Ͽ���request:%u,accName=%s", m_nAuthRequestID, szGetPlayerAccountName());
		sLogonCommHandler.UnauthedSocketClose(m_nAuthRequestID);
		m_nAuthRequestID = 0;
	}

	// ��5����֮���뿪�Ļ���Ҫ���һ�¾������
	uint32 timeDiff = UNIXTIME - m_nSocketConnectedTime;
	if(timeDiff < 300)
	{
		// �����˻Ự��֤��
		if (hadSendedAuthPacket())
		{
			// �õ��˻Ự��֤���
			if (hadGotSessionAuthResult())
			{
				Crash_Log->AddLineFormat("RECORD:���(agent=%s,accName=%s,guid="I64FMTD")�뿪��status=%d lastlog=%s IP=%s Time=%u�� ����=%s,LineID=%u", m_strClientAgent.c_str(), m_strAccountName.c_str(), 
										m_PlayerGUID, mClientStatus, mClientLog.c_str(), GetIP(), timeDiff, mClientEnv.c_str(), m_uSceneServerID);
			}
			else
			{
				Crash_Log->AddLineFormat("RECORD:�ͻ���(agent=%s,accName=%s)�Ͽ�[%u��],Auth=[�ѷ��Ự��֤��δ�Ƚ������] status=%d IP=%s ����=%s", m_strClientAgent.c_str(), m_strAccountName.c_str(), timeDiff, 
										mClientStatus, GetIP(), mClientEnv.c_str());
			}
		}
		else
		{
			// û�з��ͻỰ��֤��
			Crash_Log->AddLineFormat("RECORD:�ͻ����뿪��Auth=[δ���ͻỰ��֤��]status=%d IP=%s Time=%u�� ����=%s", mClientStatus, GetIP(), timeDiff, mClientEnv.c_str());
		}
	}

	if (hadGotSessionAuthResult())
	{
		if (hadSessionAuthSucceeded() && m_SessionIndex > 0)
		{
			if (m_uSceneServerID > 0)
			{
				// ���������������,��֪ͨ����������,����ֱ��֪ͨ������رջỰ
				if (sSceneCommHandler.GetServerSocket(m_uSceneServerID) != NULL)
					sSceneCommHandler.SendCloseSessionNoticeToScene(m_uSceneServerID, m_PlayerGUID, m_SessionIndex);
				else
					sSceneCommHandler.SendCloseServantNoticeToMaster(m_PlayerGUID, m_SessionIndex, true);
			}
			else
				sSceneCommHandler.RemoveDisconnectedSocketFromPendingGameList(m_PlayerGUID);
			// if (m_strLoginToken.length() > 0)
				// sSceneCommHandler.RegisterReloginAccountTokenData(m_strAccountName, m_PlayerGUID, m_strLoginToken, m_SessionIndex, m_GMFlag);
		}

		m_AuthStatus = snGameSocketAuthFailedStatus;
		m_uSceneServerID = 0;
	}
}

void GameSocket::_CheckRevcFrenquency(uint32 packopcode)
{
	//if(packopcode<NUM_MSG_TYPES && m_packetfrequency[packopcode]>0)
	//{
	//	if(UNIXMSTIME-m_packrecvtime[packopcode]<m_packetfrequency[packopcode])
	//	{
	//		m_checkcount++;
	//		if(m_checkcount>m_disconnectcount)
	//		{			
	//			sLog.outDebug("���%d��������%d,Ӧ����ֹ",m_PlayerGUID,packopcode);
	//			//Disconnect();
	//		}
	//		if(m_packetfrequency[packopcode]==0xFFFFFFFF && m_packrecvtime[packopcode]>0)
	//		{
	//			//�ϸ�����һ�εİ�
	//			sLog.outError("���%d�ظ�����%d,ֱ�Ӷ���",m_PlayerGUID,packopcode);
	//			Disconnect();
	//		}
	//	}
	//	m_packrecvtime[packopcode] = UNIXMSTIME;
	//}
}

void GameSocket::_EnterBacklogState()
{
	m_backlogStateLock.Acquire();
#ifdef _WIN64
	ASSERT(m_inPckBacklogState == false && "GameSocket�������ѹ״̬ʱ��ǲ�Ϊfalse");
#else
	if (m_inPckBacklogState)
		sLog.outError("[��ѹ̬�쳣]GameSock(guid="I64FMTD") ����[����]��ѹ̬ʱ��Ϊfalse,��������%u����", m_PlayerGUID, _pckBacklogQueue.GetElementCount());
#endif

	if (_pckBacklogQueue.HasItems())
	{
		sLog.outError("[��ѹ̬�쳣]��(guid="I64FMTD")�����ѹ̬ʱʣ��%u�����ݰ�δ����", m_PlayerGUID, _pckBacklogQueue.GetElementCount());
		_pckBacklogQueue.Clear();
	}
	sLog.outString("GameSocket (guid="I64FMTD") Enter BacklogState", m_PlayerGUID);
	m_inPckBacklogState = true;
	m_backlogStateLock.Release();
}

void GameSocket::_LeaveBacklogState()
{
	m_backlogStateLock.Acquire();
#ifdef _WIN64
	ASSERT(m_inPckBacklogState == true && m_uSceneServerID > 0 && "GameSocket�������ѹ״̬ʱ��ǲ�Ϊfalse");
#else
	if (!m_inPckBacklogState)
		sLog.outError("[��ѹ̬�쳣] GameSocket(guid="I64FMTD") ����[�뿪]��ѹ̬ʱ��Ϊtrue", m_PlayerGUID);
#endif

	uint32 elementCount = _pckBacklogQueue.GetElementCount();
	if (_pckBacklogQueue.HasItems())
	{
		WorldPacket *pck = NULL;
		while (pck = _pckBacklogQueue.Pop())
		{
			// ������Ϣ���ķַ�
			/*switch (m_transpacket[pck->GetOpcode()])
			{
			case PROC_CENTRE:
			{
			HookWorldPacket(pck);
			pck->SetServerCodeAndId(CMSG_GATE_2_MASTER_SESSIONING, m_PlayerGUID);
			sSceneCommHandler.SendPacketToMasterServer(pck);
			}
			break;
			case PROC_GAME:
			{
			HookWorldPacket(pck);
			pck->SetOpcode(m_uPlatformServerID * MAX_CLIENT_OPCODE + pck->GetOpcode());
			pck->SetServerCodeAndId(CMSG_GATE_2_SCENE_SESSIONING, m_PlayerGUID);
			sSceneCommHandler.SendPacketToGameServer(pck, m_uSceneServerID);
			}
			break;
			case PROC_GATEWAY:
			{
			HookWorldPacket(pck);
			break;
			}
			}*/
			DeleteWorldPacket(pck);
			pck = NULL;
		}
	}

	sLog.outString("GameSocket (guid="I64FMTD") Leave BacklogState,Sended %u packets", m_PlayerGUID, elementCount);
	// �뿪��ѹ״̬
	m_inPckBacklogState = false;
	m_backlogStateLock.Release();
}
