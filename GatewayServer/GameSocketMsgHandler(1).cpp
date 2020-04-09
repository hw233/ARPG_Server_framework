#include "stdafx.h"
#include "GameSocket.h"
#include "LogonCommHandler.h"

void GameSocket::_HandleUserAuthSession(WorldPacket* recvPacket)
{
	cLog.Notice("����", "���յ����Կͻ��˵ĵ�¼����..\n");

	// �����˺ţ�sessionkey
	uint32 loginType;
	string accountName, authKey, clientVersion, agentName, netType, sysVersion, sysModel, phoneMac;
	
	pbc_rmessage *msg1003 = sPbcRegister.Make_pbc_rmessage("UserProto001003", (void*)recvPacket->contents(), recvPacket->size());
	if (msg1003 == NULL)
		return ;

	loginType = pbc_rmessage_integer(msg1003, "loginType", 0, NULL);
	accountName = pbc_rmessage_string(msg1003, "accountName", 0, NULL);
	authKey = pbc_rmessage_string(msg1003, "gateServerAuthKey", 0, NULL);
	clientVersion = pbc_rmessage_string(msg1003, "clientVersion", 0, NULL);
	agentName = pbc_rmessage_string(msg1003, "agentName", 0, NULL);
	netType = pbc_rmessage_string(msg1003, "netType", 0, NULL);
	sysVersion = pbc_rmessage_string(msg1003, "sysVersion", 0, NULL);
	sysModel = pbc_rmessage_string(msg1003, "sysModel", 0, NULL);
	phoneMac = pbc_rmessage_string(msg1003, "phoneMac", 0, NULL);

	sLog.outString("�û�����Ự��֤,accName=%s,loginType=%u,authKey=%s", accountName.c_str(), loginType, authKey.c_str());

	switch (loginType)
	{
	case 1:			// ���ε�½
		{
			m_strAuthSessionKey = authKey;
			// Send out a request for this account.
			m_nAuthRequestID = sLogonCommHandler.GenerateRequestID();
			if(m_nAuthRequestID == ILLEGAL_DATA_32 || !sLogonCommHandler.ClientConnected(accountName, this))
			{
				sLog.outError("�޷����Ϳͻ�����֤����,�����Ͽ�(�ʺ�=%s,authReqID=%u", accountName.c_str(), m_nAuthRequestID);
				Disconnect();
				return;
			}
		}
		break;
		//case 2:			// �����ص�
		//	{
		//		string strToken;
		//		try
		//		{
		//			*recvPacket >> strToken;
		//		}
		//		catch(ByteBuffer::error &)
		//		{
		//			sLog.outError("Incomplete copy of AUTH_SESSION recieved. accName=%s,login_type=1", account_name.c_str());
		//			Disconnect();
		//			return;
		//		}

		//		sLog.outString("[�Ự��֤] �˺� %s ������ж����ص�,recv_token=%s", account_name.c_str(), strToken.c_str());
		//		// �������غ���ģ����д���
		//		int32 nRet = sSceneCommHandler.CheckAccountReloginValidation(account_name, strToken);
		//		if (nRet == 0)
		//		{
		//			tagGameSocketCacheData *data = sSceneCommHandler.GetCachedGameSocketData(account_name);
		//			ASSERT(data != NULL);

		//			uint32 index = data->session_index;
		//			uint32 platformID = data->platform_id;
		//			uint32 accID = data->acc_id;
		//			uint32 gmFlag = data->gm_flag;
		//			string existToken = data->token_data;
		//			// ��ȡ�󽫸���Ϣ�Ƴ���
		//			sSceneCommHandler.RemoveReloginAccountData(account_name);

		//			SessionAuthedSuccessCallback(index, platformID, accID, account_name, gmFlag, existToken);
		//		}
		//		else
		//		{
		//			sLog.outError("[�Ự����]��֤ʧЧ(accName=%s),RET=%d", account_name.c_str(), nRet);
		//			// �����ص�ʧ�ܣ���Ҫ������ʽ�ĵ�½����
		//			SessionAuthedFailCallback(GameSock_auth_ret_err_need_relogin);
		//		}
		//	}
		//	break;
	default:
		{
			sLog.outError("Incomplete copy of AUTH_SESSION recieved.accName=%s,login_type=%u", accountName.c_str(), loginType);
			Disconnect();
			return;
		}
		break;
	}

	// �˴���ֵһ���˺���,��������
	m_strAccountName = accountName;
	m_strClientAgent = agentName;

	DEL_PBC_R(msg1003);
}

// �������ping��Ϣ�ĺ���
void GameSocket::_HandlePing(WorldPacket* recvPacket)
{
	pbc_rmessage *msg1011 = sPbcRegister.Make_pbc_rmessage("UserProto001011", (void*)recvPacket->contents(), recvPacket->size());
	if (msg1011 == NULL)
		return ;
	
	uint64 pingSendTime = sPbcRegister.ReadUInt64Value(msg1011, "pingSendTime");
	m_lastPingRecvdMStime = getMSTime64();

	pbc_wmessage *msg1012 = sPbcRegister.Make_pbc_wmessage("UserProto001012");
	sPbcRegister.WriteUInt64Value(msg1012, "pingSendTime", pingSendTime);
	sPbcRegister.WriteUInt64Value(msg1012, "pingRecvTime", m_lastPingRecvdMStime);

	WorldPacket packet(SMSG_001012, 16);
	sPbcRegister.EncodePbcDataToPacket(packet, msg1012);
	SendPacket(&packet);
	
	//ping��ʱ��Ƶ�ʼ���������
	m_checkcount = 0;

	DEL_PBC_R(msg1011);
	DEL_PBC_W(msg1012);
}

void GameSocket::_HandlePlayerSpeek(WorldPacket *recvPacket)
{
	//if ( m_nChatBlockedExpireTime > 0 && UNIXTIME < m_nChatBlockedExpireTime )
	//{
	//	WorldPacket packet( SMSG_NOTIFICATION );
	//	packet << "";//���Է�����Ϣ
	//	SendPacket( &packet );
	//	return;
	//}

	//uint8 chat_type;
	//(*recvPacket) >> chat_type;

	//if (chat_type < CHAT_TYPE_COUNT)
	//{
	//	if (m_uSceneServerID > 0)	// ���ȷʵ����ĳ���������ڣ���������
	//	{
	//		recvPacket->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + recvPacket->GetOpcode());
	//		sSceneCommHandler.SendPacketToGameServer(recvPacket, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//	}
	//}
}

void GameSocket::_HookRecvedWorldPacket(WorldPacket *recvPacket)
{
	const uint32 op = recvPacket->GetOpcode();
	switch (op)
	{
	case CMSG_002005:		// ������г������ͣ���Ҫ������������У�
		{
			/*pbc_rmessage *msg2005 = sPbcRegister.Make_pbc_rmessage("UserProto002005", (void*)recvPacket->contents(), recvPacket->size());
			if (msg2005 == NULL)
				return ;
			uint32 destScene = pbc_rmessage_integer(msg2005, "destSceneID", 0, NULL);
			uint32 destPosX = pbc_rmessage_integer(msg2005, "destPosX", 0, NULL);
			uint32 destPosY = pbc_rmessage_integer(msg2005, "destPosY", 0, NULL);*/
		}
		break;
	default:
		break;
	}
}

bool GameSocket::_HookSendWorldPacket(WorldPacket* recvPacket)
{
	//switch(recvPacket->GetOpcode())
	//{
	//	//�ͻ��˵������������̷߳��ʳ�ͻ�����⣬δ�޸ĺ�֮ǰ��ʹ��
	//case SMSG_ENTERGAME:
	//	{
	//		mClientStatus = eCSTATUS_ENTERGAME_SUCESS;
	//		break;
	//	}
	//case SMSG_CHARINFO:
	//	{
	//		mClientStatus = eCSTATUS_SEND_CHARINFO;
	//		break;
	//	}
	//case CMSG_CHAR_CREATE:
	//	{
	//		mClientStatus = eCSTATUS_SEND_CREATECHAR;
	//		break;
	//	}
	//case MSG_NULL_ACTION://���ڽ��տͻ��˷��͵�״̬
	//	{
	//		(*recvPacket) >> mClientLog;
	//		if(mClientLog.find("exploreos")!=string::npos)
	//		{
	//			mClientEnv=mClientLog;
	//		}
	//		else if(mClientLog.find("error")!=string::npos)
	//		{
	//			Crash_Log->AddLineFormat("�˺�%u �ͻ��˴���:%s",m_PlayerGUID,mClientLog.c_str());
	//		}
	//		break;
	//	}
	//default:
	//	return false;
	//}

	return true;
}
