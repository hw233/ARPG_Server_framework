#include "stdafx.h"
#include "MasterCommServer.h"
#include "GateCommHandler.h"
#include "PlayerMgr.h"

void CMasterCommServer::HandlePing(WorldPacket &recvData)
{
	WorldPacket pck(SMSG_MASTER_2_GATE_PONG, 20);
	SendPacket(&pck, SMSG_MASTER_2_GATE_PONG, 0);

	int32 diff = time(NULL) - m_lastPingRecvUnixTime - SERVER_PING_INTERVAL;
	if (diff >= 2)
		sLog.outString("MasterServer Pong delay %d sec", diff);

	// printf("center server pong %d sec\n", diff);
	m_lastPingRecvUnixTime = time(NULL);
}

void CMasterCommServer::HandleSceneServerConnectionStatus(WorldPacket &recvData)
{
	sPark.GetSceneServerConnectionStatus(recvData);
}

void CMasterCommServer::HandleAuthChallenge(WorldPacket &recvData)
{
	// sLog.outColor(TGREEN, "��������� ���յ���֤����..\n");
	unsigned char key[20];
	uint32 result = 1;
	recvData.read(key, 20);

	sLog.outString("Authentication request from %s, result %s.",(char*)inet_ntoa(m_peer.sin_addr), result ? "OK" : "FAIL");

	printf("Key: ");
	for(int i = 0; i < 20; ++i)
	{
		printf("%.2X", key[i]);
	}
	printf("\n");
	/* send the response packet */
	WorldPacket pck( SMSG_MASTER_2_GATE_AUTH_RESULT, 128 );
	pck << result;

	authenticated = result;
	if(authenticated)
	{	
		uint32 gatewayPort, globalServerID;
		recvData >> registername >> registeradderss >> gatewayPort >> globalServerID;
		gateserver_id = sInfoCore.GenerateGateServerID();
		sLog.outString("ע������[id=%u]������%s ��ַ%s,Port=%u", gateserver_id, registername.c_str(), registeradderss.c_str(), gatewayPort);
		pck << uint32(0);
		pck << gateserver_id;
		pck << registername;
		SendPacket( &pck, SMSG_MASTER_2_GATE_AUTH_RESULT, 0 );
	}
	else
	{
		SendPacket( &pck, SMSG_MASTER_2_GATE_AUTH_RESULT, 0 );
	}
}

void CMasterCommServer::HandleRequestRoleList(WorldPacket &recvData)
{
	uint64 guid = 0;
	uint32 sessionIndex = 0, gmFlag = 0;
	string agentName, accName;
	recvData >> guid >> agentName >> accName >> sessionIndex >> gmFlag;

	sLog.outString("[��½]����%s����˺�%s(����id=%u)�����ȡ�����б�", agentName.c_str(), accName.c_str(), GetGateServerId());

	bool bWaitAdd = false;				// �ӳټ���ı��
	CPlayerServant *pExist = sPlayerMgr.FindServant(agentName, accName);
	
	if ( pExist != NULL )
	{
		// ���ﴦ��������Բ�ͬ���ص�������Ϊͬһ���ص��ظ���½�����Ѿ�������������
		if (pExist->GetServerSocket() != this)		// ���Ը��ж�һ��Ϊ��
			pExist->DisConnect();
		else
		{
			if(pExist->GetServerSocket() != NULL && !pExist->IsWaitLastData())
			{
				sLog.outError("ͬһ����û�д����ظ���½�����󣿣�����%s(�˺���%s) ԭ����%u ��ǰ����%u", agentName.c_str(), accName.c_str(), pExist->GetServerSocket()->GetGateServerId(), GetGateServerId());
				pExist->DisConnect();
				// return ;
			}
			sLog.outError("ͬһ���ص�¼ʱ,�ɵ�servert(����%s,�˺���%s)��δ��ɾ��,��������%u", agentName.c_str(), accName.c_str(), GetGateServerId());
		}
		bWaitAdd  = true;
		sLog.outString("���(����%s,�˺���%s,guid="I64FMTD")�Ѿ�������Ϸ��,��������%u,��Ҫ��ʱ��½����", agentName.c_str(), accName.c_str(), pExist->GetCurPlayerGUID(), GetGateServerId());
	}

    ServantCreateInfo info;
	info.Init(guid, gmFlag, agentName, accName);
	
	CPlayerServant* pNew = sPlayerMgr.CreatePlayerServant( &info, sessionIndex, this );
    if ( pNew == NULL )
    {
        sLog.outError( "Error: fail to create new servant(guid="I64FMTD"), out of memory?", guid );
        return;
    }

	if ( bWaitAdd )
	{
		pNew->m_nReloginStartTime = getMSTime64();
		sPlayerMgr.AddPendingServant( pNew );
	}
	else
    {
		sPlayerMgr.AddServant( pNew );
    }
}

//void CMasterCommServer::HandleEnterSceneNotice(WorldPacket &recvData)
//{
//	uint64 guid = 0;
//	uint32 sceneServerID = 0, mapid = 0, curPos = 0, enterTime = 0;
//	string loginIpAddr;
//
//	recvData >> guid >> sceneServerID >> enterTime >> mapid >> curPos >> loginIpAddr;
//	
//	CPlayerServant *pServant = sPlayerMgr.FindServant(guid);
//	if (pServant == NULL || pServant->GetPlayer() == NULL)
//	{
//		sLog.outError("δ�ҵ��Ϸ���servant��entity����(guid="I64FMTD"),���õ�½�ɹ���Ϣʧ��", guid);
//		return ;
//	}
//
//	PlayerEntity *plr = pServant->GetPlayer();
//	pServant->m_bHasPlayer = true;
//	pServant->m_nGameState = STATUS_GAMEING;
//
//	uint32 posX = Get_CoordPosX(curPos);
//	uint32 posY = Get_CoordPosY(curPos);
//
//	// �󶨸���ҵ�playerinfo
//	if (pServant->GetPlayerInfo() == NULL)
//	{
//		sLog.outError("��� %s("I64FMTD") ������Ϸǰδ����playerinfo", plr->strGetGbkName().c_str(), guid);
//		PlayerInfo *info = sPlayerMgr.GetPlayerInfo(guid);
//		ASSERT(info != NULL);
//		pServant->SetPlayerInfo(info);
//	}
//
//	PlayerInfo *info = pServant->GetPlayerInfo();
//	ASSERT(info != NULL && "��½ʱservant�Ҳ���playerinfo");
//
//	// ������һ�³�����������
//	if (pServant->GetCurSceneServerID() != 0)
//		sLog.outError("��� %s("I64FMTD") ���볡����ʱ,ԭ������������Ϊ0(%u)", plr->strGetGbkName().c_str(), guid, pServant->GetCurSceneServerID());
//
//	// ���ñ��ν��볡������ʱ��
//	pServant->SetSceneServerID(sceneServerID, enterTime, true);
//	
//	// ��login_timeΪ0ʱ�ż�¼���ε�½ʱ�䣬���߲����¼�ʱ
//	if (plr->GetPlaytimeValue(PLAYTIME_CUR_LOGIN) == 0)
//		plr->SetPlaytimeValue(PLAYTIME_CUR_LOGIN, UNIXTIME);
//
//	// Ϊinfo����servantָ��
//	info->SetServant(pServant);
//
//	sLog.outString("[��½�ɹ�]�����ȷ����� %s("I64FMTD")���볡����[%u](map%u,pos %u,%u)", plr->strGetGbkName().c_str(), guid, sceneServerID, mapid, posX, posY);
//
//}

void CMasterCommServer::HandleChangeSceneServerNotice(WorldPacket &recvData)
{
	// ��������л�������
	uint64 guid;
	uint32 changeTimer;
	recvData >> guid >> changeTimer;
	CPlayerServant *servant = sPlayerMgr.FindServant(guid);
	if (servant != NULL)
		servant->OnPlayerChangeSceneServer(changeTimer);
}

void CMasterCommServer::HandlePlayerGameRequest(WorldPacket &recvData)
{
	// ���뵽ÿ��servant��queue��
	sPlayerMgr.SessioningProc(&recvData, recvData.GetUserGUID());
}

void CMasterCommServer::HandleCloseServantRequest(WorldPacket &recvData)
{
	uint64 guid;
	uint32 sessionIndex;
	bool direct;
	recvData >> guid >> sessionIndex >> direct;

	// direct���Ϊfalse����ʾ�����Ѿ�֪ͨ��Ϸ���˳���
	sLog.outString("[�Ự�ر�]�յ����عر����֪ͨ(guid="I64FMTD"),sessionindex=%u,direct=%d", guid, sessionIndex, direct);
	sPlayerMgr.InvalidServant(guid, sessionIndex, this, false, false, direct);
}

void CMasterCommServer::HandleSceneServerPlayerMsgEvent(WorldPacket &recvData)
{
	uint32 scene_index, acc_id, player_guid, op_code;
	recvData >> scene_index >> scene_index >> acc_id >> player_guid >> op_code;

	CPlayerServant *pServant = sPlayerMgr.FindServant(acc_id);
	if (pServant == NULL)
		return ;
}

void CMasterCommServer::HandleSceneServerSystemMsgEvent(WorldPacket &recvData)
{
	uint32 serverID;
	uint16 opCode;
	recvData >> serverID >> opCode;
	switch (opCode)
	{
	default:
		break;
	}
}

