#include "stdafx.h"
#include "SceneCommClient.h"
#include "SceneCommHandler.h"
#include "FloatPoint.h"

void CSceneCommClient::HandleAuthResponse(WorldPacket *pPacket)
{
	uint32 result;
	(*pPacket) >> result;
	sLog.outColor(TYELLOW, "%d", result);

	if(result != 1)
	{
		authenticated = 0xFFFFFFFF;
	}
	else
	{
		uint32 realmlid;
		uint32 error;
		string realmname;
		(*pPacket) >> error >> realmlid >> realmname;

		sLog.outColor(TNORMAL, "\n        >> ���������� `%s` ע���idΪ ", realmname.c_str());
		sLog.outColor(TGREEN, "%u", realmlid);

		sSceneCommHandler.AdditionAck(_id, realmlid);//_idΪ��Ϸ��������id��realmlid�Ǳ�gateserver����Ϸ��������ע���id��

		authenticated = 1;
	}
}

void CSceneCommClient::HandlePong(WorldPacket *pPacket)
{
	uint32 count;
	*pPacket >> count;		// ����Ϸ���õ�������������

	sSceneCommHandler.SetGameServerOLPlayerNums(_id, count);
	latency_ms = getMSTime64() - ping_ms_time;

	if(latency_ms > 1000)
		sLog.outError("��Ϸ������%d��pingʱ��̫��,%ums", _id, latency_ms);

	m_lastPongUnixtime = time(NULL);
	sLog.outDebug(">> scene server %d latency: %ums", _id, latency_ms);
}

void CSceneCommClient::HandleGameSessionPacketResult(WorldPacket *pPacket)
{
	// �ڴ˺����У���ȡ��Ϸ������������ĳ��ҵ���Ϣ
	// ���ҽ�������ҳ�����������Ϸ��Ϣ����
	// ����ȡ���û���AccountID���ҳ���Ӧ��GameSocket�׽���
	// ����ֱ�ӵ���GameSocket��SendPacket����
	uint64 userGuid = pPacket->GetUserGUID();
	GameSocket *pSocket = sSceneCommHandler.GetSocketByPlayerGUID(userGuid);
	if (pSocket != NULL && pSocket->m_uSceneServerID == this->_id)//���ı� 3.30 ��ֹ���ݰ�����û�������ߵ����
	{
		pSocket->SendPacket(pPacket);
	}
	else
	{
		if (pSocket != NULL)
		{
			// �鿴���������Ϣ�İ����ǿ��Կ��߷��͵�
			/*if (pPacket->GetOpcode() == SMSG_FRIEND_ROLE_PANEL)
			pSocket->SendPacket(pPacket);
			else*/
				sLog.outError("�ͻ���socket �û�id="I64FMTD" ���� %d ��, ���� %d ��,�޷�������Ϣ�� pck_OP=%u", userGuid, _id, pSocket->m_uSceneServerID, pPacket->GetOpcode());
		}
		else
			sLog.outDebug("�ͻ���socket�Ѿ��Ͽ� �û�id="I64FMTD" ����%d��, �޷�������Ϣ�� pck_OP=%u", userGuid, _id, pPacket->GetOpcode());
	}
}

void CSceneCommClient::HandleSendGlobalMsg(WorldPacket *pPacket)		// gameserverҪ����ȫ����Ϣ
{
	WorldPacket pck(*pPacket);
	sSceneCommHandler.SendGlobaleMsg(&pck);		
}

void CSceneCommClient::HandleEnterGameResult(WorldPacket *pPacket)
{
	uint64 guid = 0;
	uint32 enterRet = 0;
	
	(*pPacket) >> guid >> enterRet;
	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( guid );
	if ( client == NULL )
		return ;

	if ( enterRet != 0 )
	{
		// ��½ʧ��,д�����뷢�͸����
		pPacket->SetOpcode( SMSG_001010 );
		client->SendPacket( pPacket );
	}
	else
	{
		string roleName;
		uint32 sceneServerID = 0, mapId = 0, enterTime = 0, career = 0, level = 0, maxHp = 0, curHp = 0;
		FloatPoint curPos;
		float height = 0.0f;
		(*pPacket) >> sceneServerID >> enterTime >> roleName >> career >> level >> maxHp >> curHp >> mapId >> curPos.m_fX >> curPos.m_fY >> height;

		// �ҳ��ÿͻ����׽��ֲ����ͳ�ȥ
		pbc_wmessage *msg1010 = sPbcRegister.Make_pbc_wmessage("UserProto001010");
		sPbcRegister.WriteUInt64Value(msg1010, "roleGuid", guid);
		pbc_wmessage_string(msg1010, "roleName", roleName.c_str(), 0);
		pbc_wmessage_integer(msg1010, "career", career, 0);
		pbc_wmessage_integer(msg1010, "level", level, 0);
		pbc_wmessage_integer(msg1010, "curHP", curHp, 0);
		pbc_wmessage_integer(msg1010, "maxHp", maxHp, 0);
		pbc_wmessage_integer(msg1010, "mapID", mapId, 0);
		pbc_wmessage_real(msg1010, "curPosX", curPos.m_fX);
		pbc_wmessage_real(msg1010, "curPosY", curPos.m_fY);
		pbc_wmessage_real(msg1010, "curPosHeight", height);

		WorldPacket packet(SMSG_001010, 128);
		sPbcRegister.EncodePbcDataToPacket(packet, msg1010);
		client->SendPacket( &packet );
		DEL_PBC_W(msg1010);

		sLog.outString("֪ͨ ���guid="I64FMTD" �ɹ����볡���� %u(��ͼ%u: %.1f,%.1f)", guid, sceneServerID, mapId, curPos.m_fX, curPos.m_fY);
	
		//// ֪ͨ�������ҽ�������Ϸ
		WorldPacket sendData( CMSG_GATE_2_MASTER_ENTER_SCENE_NOTICE, 128 );
		sendData << guid << sceneServerID << enterTime << mapId << curPos.m_fX << curPos.m_fY << client->GetPlayerIpAddr();
		sSceneCommHandler.SendPacketToMasterServer( &sendData );

		// ������ڰ���ѹ״̬�����˳�
		if (client->InPacketBacklogState())
			client->TryToLeaveBackLogState();
	}
}

void CSceneCommClient::HandleChangeSceneServerNotice(WorldPacket *pPacket)
{
	uint64 guid = 0;
	uint32 ret = 0;
	(*pPacket) >> guid >> ret;

	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( guid );
	if (client == NULL)
		return ;

	switch (ret)
	{
	case TRANSFER_ERROR_NONE:
		{
			// �л��ɹ�, ֱ��֪ͨ�ͻ��˽���loading
			uint32 targetMapId = 0;
			float posX = 0.0f, posY = 0.0f, height = 0.0f;
			(*pPacket) >> targetMapId >> posX >> posY >> height;
			
			client->SendChangeSceneResult(0, targetMapId, posX, posY, height);
			client->TryToLeaveBackLogState();
		}
		break;
	case TRANSFER_ERROR_NONE_BUT_SWITCH:
		{
			// ��Ҫ�л�����������,��ʱ��֪ͨ�ͻ���,��֪ͨ���������
			WorldPacket centrePacket(CMSG_GATE_2_MASTER_CHANGE_SCENE_NOTICE, 32);
			centrePacket << guid << uint32(UNIXTIME);
			sSceneCommHandler.SendPacketToMasterServer(&centrePacket);
			client->TryToEnterBackLogState();
		}
		break;
	default:
		{
			// �����л��д�,ֱ�ӷ��ظ��ͻ���
			client->SendChangeSceneResult(ret);
			client->TryToLeaveBackLogState();
		}
		break;
	}
}

void CSceneCommClient::HandleSavePlayerData(WorldPacket *pPacket)
{
	//uint32 srcPlatformID, srcServerIndex, accountID, sessionIndex,nextServerIndex;
	//(*pPacket) >> srcPlatformID >> srcServerIndex >> srcServerIndex >> accountID >> sessionIndex>>nextServerIndex;

	////�¸�����Ϊ0��ʾ����
	//if (nextServerIndex == 0)
	//{
	//	sLog.outString("player logout account=%u sessionindex=%u", accountID, sessionIndex);	
	//	GameSocket *client = sSceneCommHandler.GetSocketByAccountID(accountID);
	//	if(client != NULL&&client->_SessionIndex==sessionIndex)
	//	{
	//		client->Authed = false;
	//		client->Disconnect();
	//		sLog.outDebug("receive Save And Logout,accountid��%u index=%u",accountID,sessionIndex);
	//	}
	//}
	////ת���������
	//sSceneCommHandler.SendPacketToMasterServer( pPacket, 0, pPacket->GetServerOpCode() );
}

void CSceneCommClient::HandleTransferPacket(WorldPacket *pPacket)//������Ϸ������ת���İ�
{
	sSceneCommHandler.g_transpacketnum++;;
	sSceneCommHandler.g_transpacketsize += pPacket->size();

	uint32 targetserver = 0;
	(*pPacket) >> targetserver;

	if ( targetserver == MASTER_SERVER_ID ) //���������������
	{
		sSceneCommHandler.SendPacketToMasterServer( pPacket );
	}
	else
	{
		sSceneCommHandler.SendPacketToSceneServer( pPacket, targetserver );
	}
}

