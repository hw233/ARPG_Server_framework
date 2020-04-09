#include "stdafx.h"
#include "MasterCommServer.h"
#include "GateCommHandler.h"
#include "PlayerMgr.h"
#include "ObjectPoolMgr.h"

gatepacket_handler GatePacketHandler[SERVER_MSG_OPCODE_COUNT];

CMasterCommServer::CMasterCommServer(SOCKET fd, const sockaddr_in * peer) : TcpSocket(fd, 5485760, 5485760, true, peer, RecvPacket_head_server2server, SendPacket_head_server2server, 
																					  PacketCrypto_none, PacketCrypto_none)
{
	m_lastPingRecvUnixTime = time(NULL);
	removed = false;
	authenticated = 0;
	sInfoCore.AddServerSocket(this);
}

CMasterCommServer::~CMasterCommServer(void)
{
	WorldPacket *pck = NULL;
	while(pck = _recvQueue.Pop())
	{
		if (pck != NULL)
			DeleteWorldPacket(pck);
	}
	while(pck = _sendQueue.Pop())
	{
		if (pck != NULL)
			DeleteWorldPacket(pck);
	}
}

void CMasterCommServer::OnDisconnect()
{
	if (!removed)
	{
		sInfoCore.RemoveServerSocket(this);
		sLog.outError("���ط�����:%s�Ͽ�%s", registername.c_str(),registeradderss.c_str());
	}
	sPlayerMgr.OnGateCommServerDisconnect(this);
}

void CMasterCommServer::OnUserPacketRecved(WorldPacket *packet)
{	
	sInfoCore.m_totalreceivedata += packet->size();
	sInfoCore.m_recvDataByteCountPer5min += packet->size();
	
	HandlePacket(packet);
}

// ���ڷ��͵ĺ���
void CMasterCommServer::SendPacket(WorldPacket* data, uint32 serverOpcode, uint64 guid)
{
	if(data == NULL)
		return ;

	if (!data->m_direct_new)
		data = NewWorldPacket(data);

	const uint32 packSize = data->size();
	const uint32 ret = SendingData((packSize > 0 ? (void*)data->contents() : NULL), packSize, data->GetOpcode(), serverOpcode, guid, data->GetErrorCode(), data->GetPacketIndex());

	switch (ret)
	{
	case Send_tcppck_err_none:
		sInfoCore.m_totalsenddata += packSize;
		sInfoCore.m_sendDataByteCountPer5min += packSize;
		++sInfoCore.m_totalSendPckCount;
		++sInfoCore.m_sendPacketCountPer5min;
		DeleteWorldPacket( data );
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			sLog.outError("[����->����%u]����ʱ����������,���ݿ�ʼ��ѹ(pckSize=%u,����������:%u)", gateserver_id, packSize, GetWriteBufferSpace());
			if (data != NULL)
			{
				data->SetServerOpcodeAndUserGUID(serverOpcode, guid);
				_sendQueue.Push(data);
			}
		}
		break;
	default:
		sLog.outError("[����->����%u]����ʱ������������(err=%u),serverOp=%u,op=%u,packLen=%u,userGuid="I64FMTD"", gateserver_id, ret, 
						serverOpcode, data->GetOpcode(), data->size(), data->GetUserGUID());
		DeleteWorldPacket(data);
		break;
	}
}

// ���ʹ洢�ڻ������ڵ�Packets
void CMasterCommServer::UpdateServerSendQueuePackets()
{
	if (!_sendQueue.HasItems())
		return ;

	WorldPacket *pServerPck = NULL;
	uint64 starttime = getMSTime64();
	uint32 startPckCount = _sendQueue.GetElementCount();

	while (pServerPck = _sendQueue.Pop())
	{
		const uint32 pckSize = pServerPck->size();
		const uint32 ret = SendingData((pckSize > 0 ? (void*)pServerPck->contents() : NULL), pckSize, pServerPck->GetOpcode(), pServerPck->GetServerOpCode(), 
										pServerPck->GetUserGUID(), pServerPck->GetErrorCode(), 0);

		if (ret == Send_tcppck_err_none)
		{
			// ���ͳɹ���׼����������
			++sInfoCore.m_totalSendPckCount;
			++sInfoCore.m_sendPacketCountPer5min;
			DeleteWorldPacket( pServerPck );
		}
		else if (ret == Send_tcppck_err_no_enough_buffer)
		{
			// ����Ͷ��һ�·�������,�����Żض���,׼����һ��ѭ������
			bool flag = true;
			if(GetWriteBufferSize() > 0)
			{
				LockWriteBuffer();
				Write((void*)&flag, 0);
				UnlockWriteBuffer();
			}
			_sendQueue.Push(pServerPck);
			sLog.outError("[����->����%u]����ʱ����������,���ݿ�ʼ��ѹ(pckSize=%u,����������:%u),����%u�������´η���", gateserver_id, pckSize, GetWriteBufferSpace(), _sendQueue.GetElementCount());
			break;
		}
		else
		{
			_sendQueue.Push(pServerPck);
			sLog.outError("[����->����%u]�����������������������(err=%u),��ɾ��������%u����", gateserver_id, ret, _sendQueue.GetElementCount());
			while (pServerPck = _sendQueue.Pop())
			{
				DeleteWorldPacket( pServerPck );
				pServerPck = NULL;
			}
			break;
		}
	}

	uint32 timeDiff = getMSTime64() - starttime;
	if (timeDiff > 2000)
	{
		sLog.outError("[����->����%u]�������к�ʱ(%u ms)����,���ι�����%u����", gateserver_id, timeDiff, startPckCount - _sendQueue.GetElementCount());
	}
}

// �������ڽ��ջ�������е�Packets
void CMasterCommServer::UpdateServerRecvQueuePackets()
{
	uint64 start_time = getMSTime64();
	uint32 packet_sum = 0;
	uint16 packet_op_counter[SERVER_MSG_OPCODE_COUNT] = { 0 };
	WorldPacket *pRecvPck = NULL;

	while (pRecvPck = _recvQueue.Pop())
	{
		if (pRecvPck == NULL)
			continue;

		// ������յ����������
		if (pRecvPck->GetServerOpCode() == CMSG_GATE_2_MASTER_SESSIONING)	// ���serveropcode����CENTRECMSG_PLAYERGAME_REQUEST��˵����servantȥ��������ɾ����servant��update�н���
		{
			HandlePlayerGameRequest(*pRecvPck);
			++packet_op_counter[CMSG_GATE_2_MASTER_SESSIONING];
		}
		else
		{
			uint32 pckOpcode = pRecvPck->GetOpcode();
			if(pckOpcode >= SERVER_MSG_OPCODE_COUNT)
			{
				sLog.outError("������յ���������%u�Ĵ����,op=%u,serverOp=%u,Opcode�쳣", gateserver_id, pckOpcode, pRecvPck->GetServerOpCode());
				DeleteWorldPacket( pRecvPck );
			}
			else if (GatePacketHandler[pckOpcode] == NULL)
			{
				// ����lua����
				ScriptParamArray params;
				params << pckOpcode;
				if (!pRecvPck->empty())
				{
					size_t sz = pRecvPck->size();
					params.AppendUserData((void*)pRecvPck->contents(), sz);
					params << sz;
				}
				LuaScript.Call("HandleServerPacket", &params, NULL);

				DeleteWorldPacket( pRecvPck );
			}
			else
			{
				++packet_op_counter[pckOpcode];
				(this->*(GatePacketHandler[pckOpcode]))(*pRecvPck);	// �������������ֱ��ɾ��
				DeleteWorldPacket(pRecvPck);
			}
		}
		++packet_sum;
	}

	uint32 time_diff = getMSTime64() - start_time;
	if (time_diff > 1000)
	{
		sLog.outError("����� ��������[%s]����ʱ��([%u]����,[%u]����)�ϳ�", registeradderss.c_str(), time_diff, packet_sum);
		for (int i = 0; i < SERVER_MSG_OPCODE_COUNT; ++i)
		{
			if (packet_op_counter[i] > 0)
				sLog.outString("����opcode = %u ���� %u ��", i, uint32(packet_op_counter[i]));
		}
	}
}

void CMasterCommServer::HandlePacket(WorldPacket *recvData)
{
	/*if(recvData->GetOpcode() == CMSG_GATE_2_MASTER_PING)
	{
		uint32 curTime = time(NULL);
		int32 diff = curTime - m_lastPingRecvUnixTime - SERVER_PING_INTERVAL;
		if (diff >= 2)
			sLog.outColor(TYELLOW, "GatewayServer: %s Ping delay %d sec\n", registername.c_str(), diff);
		m_lastPingRecvUnixTime = time(NULL);
	}*/
	_recvQueue.Push(recvData);
}

void CMasterCommServer::DeletePlayerSocket(uint64 userGuid, uint32 sessionindex, bool direct)
{
	WorldPacket packet(SMSG_MASTER_2_GATE_CLOST_SESSION_NOTICE, 4);
	packet << userGuid;
	packet << sessionindex;
	packet << direct;
	SendPacket(&packet, SMSG_MASTER_2_GATE_CLOST_SESSION_NOTICE, 0);
}
