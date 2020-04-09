#include "stdafx.h"
#include "MasterCommClient.h"
#include "SceneCommHandler.h"

//---HandleServerCMD����Ҫ�ı�����ͷ�ļ�
extern bool m_ShutdownEvent;
extern uint32 m_ShutdownTimer;
//---HandleServer

masterpacket_handler MasterServerPacketHandler[SERVER_MSG_OPCODE_COUNT];
extern GatewayServer g_local_server_conf;

CMasterCommClient::CMasterCommClient(SOCKET fd, const sockaddr_in *peer) : TcpSocket(fd, 5485760, 5485760, true, peer, RecvPacket_head_server2server, 
																					 SendPacket_head_server2server, PacketCrypto_none, PacketCrypto_none)
{
	m_lastPingUnixTime = m_lastPongUnixTime = time(NULL);
	ping_ms_time = 0;
	authenticated = 0;
	latency_ms = 0;
	m_nThisGateId = 0;
}

CMasterCommClient::~CMasterCommClient(void)
{
	sLog.outString("~CMasterCommClient %p", this);

	_sendQueueLock.Acquire();
	WorldPacket *pServerPck = NULL;
	while(pServerPck = _sendQueue.Pop())
	{
		if (pServerPck != NULL)
			DeleteWorldPacket(pServerPck);
	}
	_sendQueueLock.Release();

	while(pServerPck = _recvQueue.Pop())
	{
		if (pServerPck != NULL)
			DeleteWorldPacket(pServerPck);
	}
	while(pServerPck = _busyQueue.Pop())
	{
		if (pServerPck != NULL)
			DeleteWorldPacket(pServerPck);
	}
}

void CMasterCommClient::OnUserPacketRecved(WorldPacket *packet)
{
	uint32 serverOp = packet->GetServerOpCode();
	if (serverOp != 0)
	{
		if(serverOp < SERVER_MSG_OPCODE_COUNT)
			++sSceneCommHandler.m_masterServerPacketCounter[serverOp];
	}
	else
	{
		uint32 op = packet->GetOpcode();

#ifdef _WIN64
		ASSERT(op < SERVER_MSG_OPCODE_COUNT);
#endif // _WIN64

		packet->SetServerOpcodeAndUserGUID(op, packet->GetUserGUID());
		++sSceneCommHandler.m_masterServerPacketCounter[op];
	}
	
	sSceneCommHandler.g_totalRecvDataFromMasterServer += packet->size();
	serverOp = packet->GetServerOpCode();
	// ����������ÿ�����Ĵ���
	if (serverOp == SMSG_MASTER_2_GATE_AUTH_RESULT || serverOp == SMSG_MASTER_2_GATE_PONG )
	{
		HandleCentrePakcets(packet);
		DeleteWorldPacket(packet);
		packet = NULL;
	}
	else
	{
		_recvQueue.Push(packet);
	}
}

void CMasterCommClient::OnDisconnect()
{
	if(m_nThisGateId != 0)
	{
		sLog.outString("ERROR:���� �� ���� �����������ж�OnDisconnect.\n");
		sSceneCommHandler.CentreConnectionDropped();		// handlerȥ����
		m_nThisGateId = 0;
	}
}

void CMasterCommClient::SendPacket(WorldPacket *data)
{	
	//����GAMERCMSG_SESSIONING����Ҫָ��servercode֮�⣬�����İ�ֱ��ʹ��worldpacket�����opcode
	if(data == NULL)
		return;

	uint32 serverOp = data->GetServerOpCode();
	if(serverOp != CMSG_GATE_2_MASTER_SESSIONING)
	{
		uint32 opCode = data->GetOpcode();
		if (opCode == 0 && serverOp != 0)
		{
			data->SetOpcode(serverOp);
			opCode = serverOp;
		}
		ASSERT(opCode < SERVER_MSG_OPCODE_COUNT);
		sSceneCommHandler.m_masterServerPacketCounter[opCode]++;
	}
	else
		sSceneCommHandler.m_masterServerPacketCounter[CMSG_GATE_2_MASTER_SESSIONING]++;
	
	sSceneCommHandler.g_totalSendDataToMasterServer += data->size();
	
	const uint32 pckSize = data->size();
	const uint32 ret = SendingData((data->size() ? (void*)data->contents() : NULL), pckSize, data->GetOpcode(), serverOp, data->GetUserGUID(), data->GetErrorCode(), data->GetPacketIndex());
	switch (ret)
	{
	case Send_tcppck_err_none:
		sSceneCommHandler.g_totalSendDataToSceneServer += pckSize;
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			sLog.outError("[����->����]�����ͻ���(����%u,pckSize=%u)����,׼��ʹ�û�ѹ����", GetWriteBufferSpace(), pckSize);
			WorldPacket *pck = NewWorldPacket(data);
			_sendQueueLock.Acquire();
			_sendQueue.Push(pck);
			_sendQueueLock.Release();
		}
		break;
	default:
		{
			sLog.outError("[����->����]����ʱ������������(err=%u),serverOp=%u,op=%u,packLen=%u,userGuid="I64FMTD"", ret, 
				data->GetServerOpCode(), data->GetOpcode(), pckSize, data->GetUserGUID());
		}
		break;
	}
}

void CMasterCommClient::UpdateServerSendQueuePackets()
{
	if (!_sendQueue.HasItems())
		return ;

	_sendQueueLock.Acquire();
	WorldPacket *pck = NULL;
	while (pck = _sendQueue.front())
	{
		const uint32 pckSize = pck->size();
		const uint32 ret = SendingData((pckSize ? (void*)pck->contents() : NULL), pckSize, pck->GetOpcode(), pck->GetServerOpCode(), pck->GetUserGUID(), pck->GetErrorCode(), pck->GetUserGUID());
		switch (ret)
		{
		case Send_tcppck_err_none:
			sSceneCommHandler.g_totalSendDataToSceneServer += pckSize;
			DeleteWorldPacket(pck);
			_sendQueue.pop_front();
			break;
		case Send_tcppck_err_no_enough_buffer:
			{
				// ����Ȼ��ѹ,�˳�����,���´�ѭ��
				_sendQueueLock.Release();
				return ;
			}
			break;
		default:
			{
				// ���з��������ش���,�Ѷ����еİ���ɾ����
				sLog.outError("[����->����]���з���ʱ������������(err=%u),׼��ɾ���������������ݰ�(%u��)", ret, _sendQueue.GetElementCount());
				while(pck = _sendQueue.Pop())
				{
					if (pck != NULL)
						DeleteWorldPacket(pck);
				}
				_sendQueueLock.Release();
				return ;
			}
			break;
		}
	}
	_sendQueueLock.Release();
}

void CMasterCommClient::UpdateServerRecvQueuePackets()
{
	//�˺���ʱ����������Լ1���ʱ���ڽ���
	WorldPacket *pRecvPck = NULL;

	uint32 busycount = 0;
	while (pRecvPck = _recvQueue.Pop())
	{
		if (pRecvPck != NULL)
		{
			HandleCentrePakcets(pRecvPck);		// ������������������İ�
			DeleteWorldPacket(pRecvPck);
			pRecvPck = NULL;
		}
		if(busycount++>1000)		//���µİ�̫������������update���ֲ��� ���ı� 7.9
			break;
	}
	if(busycount>0)
		m_lastPongUnixTime = time(NULL);
	busycount = 0;
	while (pRecvPck = _busyQueue.Pop())
	{
		if (pRecvPck != NULL)
		{
			HandleCentrePakcets(pRecvPck);		// ������������������İ�
			DeleteWorldPacket(pRecvPck);
			pRecvPck = NULL;
		}
		busycount++;
		if(busycount>2)
			break;
	}
}

void CMasterCommClient::SendPing()
{
	ping_ms_time = getMSTime64();

	WorldPacket pck(CMSG_GATE_2_MASTER_PING, 4);
	SendPacket(&pck);
	
	m_lastPingUnixTime = time(NULL);
}

void CMasterCommClient::SendChallenge(uint32 serverId)
{
	uint8 * key = (uint8*)sSceneCommHandler.m_sha1AuthKey.Intermediate_Hash;

	printf("Key:");
	sLog.outColor(TGREEN, " ");
	for(int i = 0; i < 20; ++i)
	{
		printf("%.2X ", key[i]);
	}
	sLog.outColor(TNORMAL, "\n");

	WorldPacket data(CMSG_GATE_2_MASTER_AUTH_REQ, 20);
	data.append(key, 20);
	data << g_local_server_conf.localServerName 
		<< g_local_server_conf.localServerAddress 
		<< g_local_server_conf.localServerListenPort 
		<< g_local_server_conf.globalServerID;

	SendPacket(&data);
}

void CMasterCommClient::HandleCentrePakcets(WorldPacket *packet)
{
	uint32 serverOp = packet->GetServerOpCode();
	if(serverOp >= SERVER_MSG_OPCODE_COUNT)
	{
		sLog.outError("�����յ�����������Ĵ����,op=%u,serverOp=%u,pckSize=%u,serverOp�쳣", packet->GetOpcode(), serverOp, packet->size());
	}
	else if (MasterServerPacketHandler[serverOp] == NULL)
	{
		sLog.outError("�����յ�����������Ĵ����,op=%u,serverOp=%u,pckSize=%u,�Ҳ�����ӦHandler", packet->GetOpcode(), serverOp, packet->size());
	}
	else
	{
		(this->*(MasterServerPacketHandler[packet->GetServerOpCode()]))(packet);
	}
}

