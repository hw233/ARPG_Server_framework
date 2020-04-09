#include "stdafx.h"
#include "threading/UserThreadMgr.h"
#include "MasterCommClient.h"
#include "SceneCommHandler.h"
#include "GameSocket.h"
#include "UpdateThread.h"

extern uint32 m_ShutdownTimer;

void CMasterCommClient::HandlePong(WorldPacket *packet)
{
	latency_ms = getMSTime64() - ping_ms_time;
	if(latency_ms)
	{
		if(latency_ms>1000)
			sLog.outError("���������pingʱ��̫��, %u ms", latency_ms);
	}
	sLog.outDebug(">> masterserver latency: %ums", latency_ms);
	m_lastPongUnixTime = time(NULL);
}

void CMasterCommClient::HandleAuthResponse(WorldPacket *packet)
{
	uint32 result;
	(*packet) >> result;
	sLog.outColor(TYELLOW, "%d", result);
	if(result != 1)
		authenticated = 0xFFFFFFFF;
	else
	{
		authenticated = 1;
		uint32 error;
		string servername;
		(*packet) >> error >> m_nThisGateId >> servername;
		sLog.outColor(TNORMAL, "\n        >> ��������� `%s` ע���idΪ ", servername.c_str());
		sLog.outColor(TGREEN, "%u", m_nThisGateId);
		// ��handler��ע���id
		sSceneCommHandler.AdditionAckFromCentre(m_nThisGateId);
	}
}

void CMasterCommClient::HandleCloseGameSocket(WorldPacket *packet)
{
	uint64 guid;
	uint32 sessionindex;
	bool direct;
	*packet >> guid >> sessionindex >> direct;
	sLog.outString("[�Ự�ر�]�����֪ͨ�ر�GameSock,guid="I64FMTD",SessionIndex=%u,direct=%d", guid, sessionindex, direct);
	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID(guid);
	if (client == NULL)
	{
		sLog.outError("δ�ҵ�guid��Ӧ��GameSocket����,�Ự�ر�ʧ��");
		return ;
	}
	if (sessionindex != client->m_SessionIndex)
	{
		sLog.outError("GameSocket��ǰsessionIndex=%u�����index��ƥ��,�Ự�ر�ʧ��", client->m_SessionIndex);
		return ;
	}
	
	if(direct)		
	{
		// ֱ�ӹر�
		client->m_AuthStatus = GSocket_auth_state_ret_failed;
		client->Disconnect();
	}
	else
	{
		// ֪ͨ�ر�
		if (client->m_uSceneServerID > 0)
		{
			sSceneCommHandler.SendCloseSessionNoticeToScene(client->m_uSceneServerID, guid, sessionindex);
			sSceneCommHandler.SendCloseServantNoticeToMaster(guid, sessionindex, false);
		}
		else
		{
			sLog.outError("[�Ự�ر�]Warning:֪ͨ�رջỰʱ,�û�guid="I64FMTD"����������һ��������", guid);
			// ֱ�ӹر��������servant
			sSceneCommHandler.SendCloseServantNoticeToMaster(guid, sessionindex, true);
		}
	}
}

void CMasterCommClient::HandleChooseGameRoleResponse(WorldPacket *packet)
{
	uint64 tempGuid, realGuid = 0;
	uint32 targetSceneID = 0;
	(*packet) >> tempGuid >> realGuid >> targetSceneID;

	GameSocket * pSock = sSceneCommHandler.GetSocketByPlayerGUID( tempGuid );
	if ( pSock == NULL )
		return;
	
	// ��ʱguid����������guid,�Ž���guid���滻
	if (tempGuid != realGuid)
		sSceneCommHandler.UpdateGameSocektGuid(tempGuid, realGuid, pSock);

	if (packet->GetErrorCode() != 0)
	{
		// ѡ�ǳ���,ֱ�ӷ��ظ��ͻ���
	}
	else
	{
		// ֪ͨ������������Ϸ
		sSceneCommHandler.AddNewPlayerToSceneServer( pSock, packet, targetSceneID );
	}
}

void CMasterCommClient::HandlePlayerGameResponse(WorldPacket *packet)
{
	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID(packet->m_nUserGUID);
	if (client != NULL && client->IsConnected())
	{
		// sLog.outDebug("[�����Ӧ��]���Ͱ������%s, op=%u, size=%u", client->m_strAccountName.c_str(), packet->GetOpcode(), packet->size());
		client->SendPacket(packet);
	}
}

void CMasterCommClient::HandleChangeSceneServerMasterNotice(WorldPacket *packet)
{
	uint64 plrGuid = 0;
	uint32 targetSceneServerID = 0, targetMapID = 0;
	float posX = 0.0f, posY = 0.0f, height = 0.0f;
	(*packet) >> plrGuid >> targetSceneServerID >> targetMapID >> posX >> posY >> height;

	GameSocket * pSock = sSceneCommHandler.GetSocketByPlayerGUID( plrGuid );
	if ( pSock == NULL )
		return;

	// ֪ͨ������������Ϸ
	sSceneCommHandler.AddNewPlayerToSceneServer( pSock, packet, targetSceneServerID );

	// ֪ͨ��ҿ�ʼloading
	pSock->SendChangeSceneResult(0, targetMapID, posX, posY, height);
}

void CMasterCommClient::HandleEnterInstanceNotice(WorldPacket *packet)
{
	// ���������븱����ϵͳʵ������Ҫ�ڴ˴���gamesocket�������ѹ״̬
	//uint32 curSceneIndex, accID;
	//(*packet) >> curSceneIndex >> accID;
	////
	//GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( accID );
	//if (client != NULL)
	//{
	//	client->TryToEnterBackLogState();
	//	packet->SetServerAccountID(accID);
	//	// �����ݰ���������Ӧ�ĳ�����
	//	sSceneCommHandler.SendPacketToGameServer( packet, curSceneIndex );
	//}
}

void CMasterCommClient::HandleExitInstanceNotice(WorldPacket *packet)
{
	//uint32 curSceneIndex, accID;
	//(*packet) >> curSceneIndex >> accID;
	////
	//GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( accID );
	//if (client != NULL)
	//{
	//	client->TryToEnterBackLogState();
	//	packet->SetServerAccountID(accID);
	//	// �����ݰ���������Ӧ�ĳ�����
	//	sSceneCommHandler.SendPacketToGameServer( packet, curSceneIndex );
	//}
}

void CMasterCommClient::HandleSendGlobalMsg(WorldPacket *packet)
{
	sSceneCommHandler.SendGlobaleMsg(packet);
}

void CMasterCommClient::HandleSendPartialGlobalMsg(WorldPacket *packet)
{
	sSceneCommHandler.SendGlobalMsgToPartialPlayer(*packet);
}

void CMasterCommClient::HandleTranferPacket( WorldPacket * packet )
{
	uint64 userguid = 0;
	uint32 curLine = 0;
	*packet  >> curLine >> userguid;

	if ( curLine != 0 )
	{
		if (curLine == 0xffffffff)
		{
			DEBUG_LOG("ת����Ϣ", "ת����Ϣ��������Ϸ��");
		}
		packet->SetUserGUID(userguid);
		sLog.outDebug("GateWay Trans Server Packet(%u, size=%u) to LineID %u", uint32(packet->GetServerOpCode()), (uint32)(packet->size()), curLine);
		sSceneCommHandler.SendPacketToSceneServer( packet, curLine );
	}
}

void CMasterCommClient::HandleBlockChat( WorldPacket * packet )
{
	uint64 userguid = 0;
	uint32 block = 0;
	*packet >> userguid >> block;
	GameSocket * client = sSceneCommHandler.GetSocketByPlayerGUID( userguid );
	if ( client == NULL )
		return;

	client->m_nChatBlockedExpireTime = block;
}

void CMasterCommClient::HandleServerCMD( WorldPacket * packet )
{
	std::string cmd,s1,s2;
	int32 p1,p2,p3;
	(*packet)>>cmd>>p1>>p2>>p3>>s1>>s2;
	//if(cmd=="SHUTDOWN")
	//{
	//	m_ShutdownTimer = p1>=10000?p1+15000:m_ShutdownTimer+15000;
	//	m_ShutdownEvent = true;

	//}
	//if(cmd=="CANCLE_SHUTDOWN")
	//{
	//	m_ShutdownTimer = p1>10000?p1:m_ShutdownTimer;
	//	m_ShutdownEvent = false;
	//	WorldPacket data(SMSG_SERVER_MESSAGE,20);
	//	data << uint32(SERVER_MSG_SHUTDOWN_CANCELLED);
	//	sSceneCommHandler.SendGlobaleMsg(&data);
	//}
	//if ( cmd == "OPCODEBAN" )
	//{
	//	if ( p1 < NUM_MSG_TYPES )
	//	{
	//		GameSocket::m_packetforbid[p1] = 1;
	//		sLog.outString( "���ݰ�%u������", p1 );
	//	}
	//	return;
	//}
	//if(cmd=="INPUT")
	//{
	//	if(p1==1)
	//	{
	//		if(p2==sSceneCommHandler.GetStartParmInt("id"))
	//		{
	//			CUpdateThread* prun = (CUpdateThread*)sThreadMgr.GetThreadByType(THREADTYPE_CONSOLEINTERFACE);
	//			if(prun!=NULL)
	//				prun->m_input=p3;
	//		}
	//	}
	//	else if(sSceneCommHandler.GetStartParmInt("id")==0)
	//	{
	//		sSceneCommHandler.SendPacketToGameServer(packet, p2);
	//	}
	//	return;
	//	//�������Ҫת�������з�����
	//}
	//if(sSceneCommHandler.GetStartParmInt("id")==0)
	//{
	//	sSceneCommHandler.SendPacketToGameServer(packet, 0xFFFFFFFF);
	//}
}

