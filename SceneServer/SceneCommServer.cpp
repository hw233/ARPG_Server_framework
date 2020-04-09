#include "stdafx.h"
#include "ObjectMgr.h"
#include "SceneCommServer.h"
#include "ObjectPoolMgr.h"

gamepacket_handler GamePacketHandlers[SERVER_MSG_OPCODE_COUNT];

CSceneCommServer::CSceneCommServer(SOCKET fd,const sockaddr_in * peer) : TcpSocket(fd, GATESOCKET_RECVBUF_SIZE, GATESOCKET_SENDBUF_SIZE, true, peer, RecvPacket_head_server2server, 
																				   SendPacket_head_server2server, PacketCrypto_none, PacketCrypto_none)
{
	int arg2 = 1024*1024;
	setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, (char*)&arg2, sizeof(int));
	setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, (char*)&arg2, sizeof(int));

	m_lastPingRecvUnixTime = time(NULL);
	sGateCommHandler.AddServerSocket(this);

	removed = false;
	authenticated = 0;
	registerPort = 0;
	associate_gateway_id = 0;
	src_platform_id = 0;
	_lastSendLowLevelPacketMsTime = getMSTime64();
}

CSceneCommServer::~CSceneCommServer(void)
{
	// ������������е�ʣ���ȫ��ɾ��(����еĻ�)
	WorldPacket* pck = NULL;

	// �����ն����е�ʣ���ȫ��ɾ��
	while(pck = _recvQueue.front())
	{
		if (pck != NULL)
		{
			DeleteWorldPacket(pck);
			_recvQueue.pop_front();
		}
	}
	while(pck = _lowrecvQueue.front())
	{
		if (pck != NULL)
		{
			DeleteWorldPacket(pck);
			_lowrecvQueue.pop_front();
		}
	}
	while (pck = _highPrioritySendQueue.Pop())
	{
		if (pck != NULL)
		{
			DeleteWorldPacket(pck);
		}
	}
	while (pck = _lowPrioritySendQueue.Pop())
	{
		if (pck != NULL)
		{
			DeleteWorldPacket(pck);
		}
	}
}

void CSceneCommServer::OnConnect()
{
	sLog.outString("���յ���������:%s������", GetIP());
}

void CSceneCommServer::OnDisconnect()
{
	if(!removed)
	{
		if (src_platform_id != 0)
			sGateCommHandler.RemovePlatformGatewaySocket(src_platform_id, this);
		else
			sGateCommHandler.RemoveServerSocket(this);

		removed = true;
	}
	sPark.OnSocketClose(this);
}

void CSceneCommServer::OnUserPacketRecved(WorldPacket *packet)
{
	// sLog.outDebug("CSceneCommServer OnUserPacketRecved called, packSize=%d", packet->size());

	sGateCommHandler.m_totalreceivedata += packet->size();
	sGateCommHandler.m_recvDataByteCountPer5min += packet->size();

	HandlePacket(packet);
}

void CSceneCommServer::SendPacket(WorldPacket *data, uint32 nServerOPCode, uint64 userGuid)
{
	if(!data)
	{
		return;
	}
	// ���data�Ǿֲ�����
	if (!data->m_direct_new)
	{
		data = NewWorldPacket(data);
	}

	const uint32 packSize = data->size();
	const uint32 ret = SendingData((packSize > 0 ? (void*)data->contents() : NULL), packSize, data->GetOpcode(), nServerOPCode, userGuid, data->GetErrorCode(), data->GetPacketIndex());
	switch (ret)
	{
	case Send_tcppck_err_none:
		sGateCommHandler.m_totalsenddata += packSize;
		sGateCommHandler.m_sendDataByteCountPer5min += packSize;
		DeleteWorldPacket(data);
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			sLog.outError("[����->����%u]����ʱ����������,���ݿ�ʼ��ѹ(pckSize=%u,����������:%u)", associate_gateway_id, packSize, GetWriteBufferSpace());
			if (data != NULL)
			{
				data->SetServerOpcodeAndUserGUID(nServerOPCode, userGuid);
				_highPrioritySendQueue.Push(data);
			}
		}
		break;
	default:
		sLog.outError("[����->����%u]����ʱ������������(err=%u),serverOp=%u,op=%u,packLen=%u,userGuid="I64FMTD"", associate_gateway_id, ret, 
						nServerOPCode, data->GetOpcode(), packSize, data->GetUserGUID());
		DeleteWorldPacket(data);
		break;
	}
}

// �����ڷ��Ͷ����е���Ϣ�����ȼ���ȫ���ͣ����ȼ��Ͳ��ַ��ͣ�
void CSceneCommServer::UpdateServerSendQueuePackets()
{
	if (!_highPrioritySendQueue.HasItems())
		return ;

	uint64 starttime = getMSTime64();
	uint32 pckCount = _highPrioritySendQueue.GetElementCount();
	uint32 curSendByte = sGateCommHandler.m_totalsenddata;
	WorldPacket* pServerPck = NULL;

	while (pServerPck = _highPrioritySendQueue.Pop())
	{
		const uint32 pckSize = pServerPck->size();
		const uint32 ret = SendingData((pckSize > 0 ? (void*)pServerPck->contents() : NULL), pckSize, pServerPck->GetOpcode(), 
										pServerPck->GetServerOpCode(), pServerPck->GetUserGUID(), pServerPck->GetErrorCode(), 0);
		
		if (ret == Send_tcppck_err_none)
		{
			sGateCommHandler.m_totalsenddata += pckSize;
			sGateCommHandler.m_sendDataByteCountPer5min += pckSize;
			++sGateCommHandler.m_totalSendPckCount;
			++sGateCommHandler.m_sendPacketCountPer5min;
			DeleteWorldPacket( pServerPck );
		}
		else if (ret == Send_tcppck_err_no_enough_buffer)
		{
			bool flag = true;
			if(GetWriteBufferSize() > 0)
			{
				LockWriteBuffer();
				Write((void*)&flag, 0);
				UnlockWriteBuffer();
			}
			_highPrioritySendQueue.Push(pServerPck);
			sLog.outError("[����->����%u]����ʱ����������,���ݿ�ʼ��ѹ(pckSize=%u,����������:%u),����%u�������´η���", associate_gateway_id, pckSize, GetWriteBufferSpace(), _highPrioritySendQueue.GetElementCount());
			// �����͵İ�̫���ˣ�ѡ������һЩ������(�ƶ�����˵������)
			/*uint32 throwcount = 0;
			while(pServerPck = _highPrioritySendQueue.front())
			{
				if(pServerPck->GetServerOpCode()==SMSG_SCENE_2_GATE_SESSIONING_RET && (pServerPck->GetOpcode()==SMSG_OBJECT_MOVE||pServerPck->GetOpcode()==SMSG_MSG))
				{
					_highPrioritySendQueue.pop_front();
					DeleteWorldPacket( pServerPck );
					++ throwcount;
				}
				else
				{
					break;
				}
			}
			sLog.outString("������֮�����ݰ�̫�࣬������%d��", throwcount);*/
			break;
		}
		else
		{
			_highPrioritySendQueue.Push(pServerPck);
			sLog.outError("[����->����%u]�����������������������(err=%u),��ɾ�������ȶ�����%u����", associate_gateway_id, ret, _highPrioritySendQueue.GetElementCount());
			while (pServerPck = _highPrioritySendQueue.Pop())
			{
				DeleteWorldPacket( pServerPck );
				pServerPck = NULL;
			}
		}
	}

	uint32 timeDiff = getMSTime64() - starttime;
	if (timeDiff > 2000)
	{
		sLog.outError("[����->����%u]�����ȷ������к�ʱ(%u ms)����,���ι�����%u����", associate_gateway_id, timeDiff, pckCount - _highPrioritySendQueue.GetElementCount());
	}
	//sLog.outString("[����->����%u]�����ȷ������б��ι�����%u����(%u�ֽ�)", associate_gateway_id, pckCount - _highPrioritySendQueue.GetElementCount(), 
	//				sGateCommHandler.m_totalsenddata - curSendByte);
}

void CSceneCommServer::UpdateServerLowSendQueuePackets()
{
	uint32 timeDiff = getMSTime64() - _lastSendLowLevelPacketMsTime;
	// 50����Ž���һ�η���
	if (timeDiff < 50)
		return ;

	_lastSendLowLevelPacketMsTime = getMSTime64();
	if (!_lowPrioritySendQueue.HasItems())
		return ;

	WorldPacket* pServerPck = NULL;
	uint32 pckCount = 0;
	while (pServerPck = _lowPrioritySendQueue.Pop())
	{
		const uint32 pckSize = pServerPck->size();
		const uint32 ret = SendingData((pckSize > 0 ? (void*)pServerPck->contents() : NULL), pckSize, pServerPck->GetOpcode(), 
							pServerPck->GetServerOpCode(), pServerPck->GetUserGUID(), pServerPck->GetErrorCode(), 0);

		if (ret == Send_tcppck_err_none)
		{
			sGateCommHandler.m_totalsenddata += pckSize;
			sGateCommHandler.m_sendDataByteCountPer5min += pckSize;
			++sGateCommHandler.m_totalSendPckCount;
			++sGateCommHandler.m_sendPacketCountPer5min;
			++sGateCommHandler.m_TimeOutSendCount;
			DeleteWorldPacket( pServerPck );
			// �����ȶ��еİ�,ÿ50���뷢100��
			if (++pckCount > 100)
				break;
		}
		else if (ret == Send_tcppck_err_no_enough_buffer)
		{
			bool flag = true;
			if(GetWriteBufferSize() > 0)
			{
				LockWriteBuffer();
				Write((void*)&flag, 0);
				UnlockWriteBuffer();
			}
			_lowPrioritySendQueue.Push(pServerPck);
			sLog.outError("[����->����%u]����ʱ����������,���ݿ�ʼ��ѹ(pckSize=%u,����������:%u),����%u�������´η���", associate_gateway_id, pckSize, GetWriteBufferSpace(), _lowPrioritySendQueue.GetElementCount());
			break;
		}
		else
		{
			_lowPrioritySendQueue.Push(pServerPck);
			sLog.outError("[����->����%u]�����������������������(err=%u),��ɾ�������ȶ�����%u����", associate_gateway_id, ret, _lowPrioritySendQueue.GetElementCount());
			while (pServerPck = _lowPrioritySendQueue.Pop())
			{
				DeleteWorldPacket( pServerPck );
				pServerPck = NULL;
			}
			break;
		}
	}

	timeDiff = getMSTime64() - _lastSendLowLevelPacketMsTime;
	if (timeDiff > 2000)
	{
		sLog.outError("[����->����%u]�����ȷ������к�ʱ(%u ms)����,���ι�����%u����", associate_gateway_id, timeDiff, pckCount - _lowPrioritySendQueue.GetElementCount());
	}
}

void CSceneCommServer::UpdateServerRecvQueuePackets()
{
	uint64 starttime = getMSTime64();
	WorldPacket* pRecvPck = NULL;
	uint32 highPackCount = 0, lowPackCount = 0;
	uint32 highPackOpCount[SERVER_MSG_OPCODE_COUNT]  = { 0 };
	uint32 lowPackOpCount[SERVER_MSG_OPCODE_COUNT] = { 0 };

	while (pRecvPck = _recvQueue.front())
	{
		if (pRecvPck != NULL)
		{
			++highPackCount;
			ServerPacketProc(pRecvPck, highPackOpCount);
			_recvQueue.pop_front();
		}
	}

	//�����ȼ���ÿ��update���¶��ٸ��� ���ı� 8.7
	pRecvPck = _lowrecvQueue.front();
	if (pRecvPck != NULL)
	{
		++lowPackCount;
		ServerPacketProc(pRecvPck, lowPackOpCount);
		_lowrecvQueue.pop_front();
	}

	uint32 timeDiff = getMSTime64() - starttime;
	if(timeDiff > 1000)
	{
		sLog.outString("������ ��������[%s]���ݰ�ʱ��̫��(%u ms, highPckCount=%u, lowPckCount=%u)", registeradderss.c_str(), timeDiff, highPackCount, lowPackCount);
		for (int i = 0; i < SERVER_MSG_OPCODE_COUNT; ++i)
		{
			if (highPackOpCount[i] > 0)
				sLog.outString("[high] opcode = %u, packet_count=%u", i, highPackOpCount[i]);
		}
		for (int i = 0; i < SERVER_MSG_OPCODE_COUNT; ++i)
		{
			if (lowPackOpCount[i] > 0)
				sLog.outString("[low] opcode = %u, packet_count=%u", i, lowPackOpCount[i]);
		}
	}
}

void CSceneCommServer::ServerPacketProc(WorldPacket *recvPacket, uint32 *packetOpCounter)
{
	if (recvPacket->GetServerOpCode() == CMSG_GATE_2_SCENE_SESSIONING)
	{
		++packetOpCounter[CMSG_GATE_2_SCENE_SESSIONING];
		HandleGameSessionPacket(recvPacket);
	}
	else
	{
		uint32 pckOpCode = recvPacket->GetOpcode();
		if(pckOpCode >= SERVER_MSG_OPCODE_COUNT)
		{
			sLog.outError("�������յ���������%u�Ĵ����,op=%u,serverOp=%u,Opcode�쳣", associate_gateway_id, pckOpCode, recvPacket->GetServerOpCode());
			DeleteWorldPacket( recvPacket );
		}
		else if (GamePacketHandlers[pckOpCode] == NULL)
		{
			// ����lua����
			ScriptParamArray params;
			params << pckOpCode;
			if (!recvPacket->empty())
			{
				size_t sz = recvPacket->size();
				params.AppendUserData((void*)recvPacket->contents(), sz);
				params << sz;
			}
			LuaScript.Call("HandleServerPacket", &params, NULL);

			DeleteWorldPacket( recvPacket );
		}
		else
		{
			++packetOpCounter[pckOpCode];
			(this->*(GamePacketHandlers[pckOpCode]))(*recvPacket);
			DeleteWorldPacket( recvPacket );
		}
	}
}

// �˺���Ҫע����Ϣ�����ȼ�
void CSceneCommServer::SendSessioningResultPacket(WorldPacket* pck, uint64 userGuid, ENUMMSGPRIORITY msgPriority /* = HIGH_PRIORITY */)
{
	if(pck->size() > 65535)
		sLog.outError("�������������ش���");
	
	if ( !pck->m_direct_new )
	{
		WorldPacket* packet = NewWorldPacket(pck->GetOpcode(), pck->GetErrorCode(), pck->size(), false);
		if(!pck->empty())
			packet->append(pck->contents(),pck->size());
		pck = packet;
	}
	else
	{
		// ���ֵ�packet��Ȼ��ʹ��NewWorldPacket���䡣
		// printf("����new ������packet��%d", pck->GetOpcode());
	}

	// cLog.Success("�������ݰ�������", "���ID��%d ���� %d \n", nPlayerAccountID,data->GetOpcode());
	// SendPacket(data, GAMERSMSG_SESSION_RESULT, nPlayerAccountID);
	// �˴����Ự������Ự������
	pck->SetServerOpcodeAndUserGUID( SMSG_SCENE_2_GATE_SESSIONING_RET, userGuid );
	
	if (msgPriority == HIGH_PRIORITY)
	{
		_highPrioritySendQueue.Push(pck);
	}
	else
	{
		_lowPrioritySendQueue.Push(pck);
	}
}

