#include "stdafx.h"
#include "GameSession.h"

// �����ɫ�������ء������ƶ��������ͼ���͵�Э��
void GameSession::HandleSessionPing(WorldPacket &recvPacket)
{

}

void GameSession::HandlePlayerMove(WorldPacket & recvPacket)
{
	Player *plr = GetPlayer();
	if (!plr->IsInWorld())
		return;

	pbc_rmessage *msg2003 = sPbcRegister.Make_pbc_rmessage("UserProto002003", (void*)recvPacket.contents(), recvPacket.size());
	if (msg2003 == NULL)
		return ;

	FloatPoint srcPos, destPos;
	srcPos.m_fX = pbc_rmessage_real(msg2003, "srcPosX", 0);
	srcPos.m_fY = pbc_rmessage_real(msg2003, "srcPosY", 0);
	destPos.m_fX = pbc_rmessage_real(msg2003, "destPosX", 0);
	destPos.m_fY = pbc_rmessage_real(msg2003, "destPosY", 0);

	// дһ������������·���Ƿ����
	if (srcPos.m_fX <= 0.0f || srcPos.m_fY <= 0.0f || destPos.m_fX <= 0.0f || destPos.m_fY <= 0.0f)
		return ;
	if (srcPos.m_fX > mapLimitWidth || srcPos.m_fY > mapLimitLength || destPos.m_fX > mapLimitWidth || destPos.m_fY > mapLimitLength)
		return ;
	// �����������뵱ǰ�������̫Զ,���޷�����
	float dist = plr->CalcDistance(srcPos.m_fX, srcPos.m_fY);
	if (dist >= 8.0f)
	{
		sLog.outError("[�ƶ�]���:%s(guid="I64FMTD")�����ƶ�(���:%.1f,%.1f)�뵱ǰ����(%.1f,%.1f)����(%.1f)̫Զ,����������", plr->strGetGbkNameString().c_str(), plr->GetGUID(),
						srcPos.m_fX, srcPos.m_fY, plr->GetPositionX(), plr->GetPositionY(), dist);
		return ;
	}

	// ���ԴĿ����Ŀ��������ȣ�˵��ֹͣ�ƶ���ĳ��˵�������ƶ����߽���ת��
	bool stopMoving = false;
	if (srcPos == destPos)
		stopMoving = true;

	if (stopMoving)
	{
		// ֹͣ�ƶ�
		plr->StopMoving(false, destPos);
	}
	else
	{
		uint8 moveMode = OBJ_MOVE_TYPE_START_MOVING;
		plr->StartMoving(srcPos.m_fX, srcPos.m_fY, destPos.m_fX, destPos.m_fY, moveMode);
	}

	DEL_PBC_R(msg2003);
}

void GameSession::HandlePlayerSceneTransfer(WorldPacket & recvPacket)
{
	Player *plr = GetPlayer();
	if (!plr->IsInWorld())
		return ;

	pbc_rmessage *msg2005 = sPbcRegister.Make_pbc_rmessage("UserProto002005", (void*)recvPacket.contents(), recvPacket.size());
	if (msg2005 == NULL)
		return ;

	uint32 destScene = pbc_rmessage_integer(msg2005, "destSceneID", 0, NULL);
	FloatPoint destPos;
	destPos.m_fX = pbc_rmessage_integer(msg2005, "destPosX", 0, NULL);
	destPos.m_fY = pbc_rmessage_integer(msg2005, "destPosY", 0, NULL);

	plr->HandleSceneTeleportRequest(ILLEGAL_DATA_32, destScene, destPos);
	DEL_PBC_R(msg2005);
}

/*void GameSession::HandleGameLoadedOpcode(WorldPacket & recvPacket)
{
	Player *plr = GetPlayer();

	sLog.outString("���%s loading��ͼ��� ׼�����볡��%u(%.1f,%.1f)", plr->strGetGbkNameString().c_str(), plr->GetMapId(), plr->GetPositionX(), plr->GetPositionY());	
	plr->AddToWorld();

	// �����Ҳ�δ�ɹ����볡����
	if ( !plr->IsInWorld() )
	{
		sLog.outError("��� %s("I64FMTD") loading��ͼ%u(instance=%u)ʧ�� ���Ͽ�", plr->strGetGbkNameString().c_str(), plr->GetGUID(), plr->GetMapId(), plr->GetInstanceID());
		LogoutPlayer(false);
		Disconnect();
		return;
	}

	// ������Ҷ���Ĵ�����Ϣ���ͻ���
	pbc_wmessage *msg2002 = sPbcRegister.Make_pbc_wmessage("UserProto002002");
	if (msg2002 == NULL)
		return ;
	pbc_wmessage *creationBuffer = pbc_wmessage_message(msg2002, "playerCreationData");
	if (creationBuffer == NULL)
		return ;

	plr->BuildCreationDataBlockToBuffer(creationBuffer, plr);
	WorldPacket packet(SMSG_002002, 256);
	sPbcRegister.EncodePbcDataToPacket(packet, msg2002);
	uint32 packSize = packet.size();
	SendPacket(&packet);

	DEL_PBC_W(msg2002);
	sLog.outString("��� %s �ɹ����볡�� %u(InstanceID = %u),����BufferSize=%u", plr->strGetGbkNameString().c_str(), plr->GetMapId(), plr->GetInstanceID(), packSize);
	//if (!plr->IsLoaded())
	//{
	//	// ֪ͨ�����������Ѿ���һ�ν��뱾��������
	//	WorldPacket centrePacket(GAME_TO_CENTER_PLAYER_OPERATION);
	//	centrePacket << uint32(0) << sInfoCore.GetSelfSceneServerIndex();
	//	centrePacket << GetAccountId() << plr->GetGUID();
	//	centrePacket << uint32(STC_Player_op_first_loaded);
	//	SendServerPacket(&centrePacket);

	//	// ֪ͨ����������������Ѿ���һ�ν��뱾��������
	//	WorldPacket scenePacket(GAME_TO_GAME_SERVER_MSG);
	//	scenePacket << uint32(ILLEGAL_DATA) << sInfoCore.GetSelfSceneServerIndex();
	//	scenePacket << uint32(STS_Server_msg_op_player_first_loaded);
	//	scenePacket << plr->GetGUID();
	//	SendServerPacket(&scenePacket);
	//}
}*/
