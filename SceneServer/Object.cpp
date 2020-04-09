/****************************************************************************
 *
 * General Object Type File
 * Copyright (c) 2007 Team Ascent
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
 * 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
 *
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
 * ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
 * ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

#include "stdafx.h"
#include "GameSession.h"
#include "SkillHelp.h"

Object::Object() : m_mapId(0), m_objectUpdated(false), Active(false),
					m_mapMgr(NULL), m_mapCell(NULL), m_instanceId(0), m_direction(RandomUInt(255)), m_bMoving(false), m_nMovingUpdateTimer(0), m_nStartMovingTime(0)
{
	m_uint64Values = NULL;
	m_valuesCount = 0;
	m_objectsInRange.clear();
	m_inRangePlayers.clear();
	m_position.Init(1.0f,1.0f);
	m_moveDestPosition.Init();
	m_moveDirection.Init();
}

Object::~Object( )
{
	// for linux
	if (IsInWorld())
	{
		RemoveFromWorld(false);
	}
	m_instanceId = -1;

	sEventMgr.RemoveEvents(this);
}

bool Object::isInRange(Object* target, float range)
{
	float dist = CalcDistance( target );
	return( dist <= range );
}

float Object::StartMoving(float srcPosX, float srcPosY, float destPosX, float destPosY, uint8 moveMode)
{
	static const char* szDirection16Name[GDirection16_Count] = 
	{
		"��", "����ƫ��", "����", "����ƫ��",
		"��", "����ƫ��", "����", "����ƫ��",
		"��", "����ƫ��", "����", "����ƫ��",
		"��", "����ƫ��", "����", "����ƫ��"
	};

	// ��ʼ�ƶ������ƶ�������ó�true������������������player��
	m_bMoving = true;

	m_position.Init(srcPosX, srcPosY);
	m_moveDestPosition.Init(destPosX, destPosY);
	m_moveDirection = m_moveDestPosition - m_position;
	m_moveDirection.Norm();

	BoardCastMovingStatus(srcPosX, srcPosY, destPosX, destPosY, moveMode);
	
	float costTime = CalcDistance(destPosX, destPosY) / GetMoveSpeed();
	m_nMovingUpdateTimer = 0;
	m_nStartMovingTime = getMSTime64();
	// ���¼��㷽��
	float dir = SkillHelp::CalcuDirection(m_position.m_fX, m_position.m_fY, m_moveDestPosition.m_fX, m_moveDestPosition.m_fY);
	SetDirection(dir);
	uint32 dir16 = SkillHelp::CalcuDireciton16(dir);

	//if (isPlayer())
	//	sLog.outDebug("��ҵ�ǰ����: %.2f -> %s", dir, szDirection16Name[dir16]);

	sLog.outString("[��ʼ�ƶ�]����"I64FMTD" ��ʼ�ƶ�,��ǰ����(%.1f,%.1f),Ŀ������(%.1f,%.1f),Ԥ���ƶ�ʱ��%.2f��", GetGUID(), GetPositionX(), GetPositionY(), 
					m_moveDestPosition.m_fX, m_moveDestPosition.m_fY, costTime);

	return costTime;
}

void Object::StopMoving(bool systemStop, const FloatPoint &curPos)
{
	/*if (!m_bMoving)
		return ;*/

	if (systemStop)
	{
		sLog.outString("[ֹͣ�ƶ�]sys ����"I64FMTD",��ǰ����(%.1f,%.1f),Ŀ������(%.1f,%.1f),��ʱ%u ms", GetGUID(), GetPositionX(), GetPositionY(), 
						m_moveDestPosition.m_fX, m_moveDestPosition.m_fY, getMSTime64() - m_nStartMovingTime);
	}
	else
	{
		sLog.outString("[ֹͣ�ƶ�]user ����"I64FMTD",��ǰ����(%.1f,%.1f),Ŀ������(%.1f,%.1f),��ʱ%u ms", GetGUID(), GetPositionX(), GetPositionY(),
						m_moveDestPosition.m_fX, m_moveDestPosition.m_fY, getMSTime64() - m_nStartMovingTime);
	}

	float curDist = CalcDistance(curPos.m_fX, curPos.m_fY);

	if (curDist > 1.0f)
	{
		sLog.outError("���� "I64FMTD" �ͻ�������(%.1f, %.1f)����������(%.1f, %.1f)�в���(dist=%.1f)", GetGUID(), curPos.m_fX, curPos.m_fY, 
						GetPositionX(), GetPositionY(), curDist);
	}
	
	// ������������
	m_position = curPos;

	// �����ƶ�����
	m_bMoving = false;
	m_moveDestPosition.Init();
	m_moveDirection.Init();
	m_nStartMovingTime = 0;

	BoardCastMovingStatus(m_position.m_fX, m_position.m_fY, m_position.m_fX, m_position.m_fY, OBJ_MOVE_TYPE_STOP_MOVING);

	// ����lua��ֹͣ�ƶ�����
	ScriptParamArray params;
	params << GetGUID() << m_position.m_fX << m_position.m_fY;
	LuaScript.Call("OnMovingStopped", &params, NULL);
}

void Object::BoardCastMovingStatus(float srcPosX, float srcPosY, float destPosX, float destPosY, uint8 mode)
{
	if (!IsInWorld())
		return ;

	uint64 guid = GetGUID();
	const uint32 mapID = GetMapId();
	float srcHeight = sSceneMapMgr.GetPositionHeight(mapID, srcPosX, srcPosY);
	float destHeight = sSceneMapMgr.GetPositionHeight(mapID, destPosX, destPosY);

	// ���͸��Լ�
	if (isPlayer())
	{
		TO_PLAYER(this)->PushMovementData(guid, srcPosX, srcPosY, srcHeight, destPosX, destPosY, destHeight, mode);
	}
	
	if (!HasInRangePlayers())
		return ;

	Player * plr = NULL;
	uint32 sendcount = 0;
	
	hash_set< Player * > * in_range_players = GetInRangePlayerSet();
	for ( hash_set< Player * >::iterator itr = in_range_players->begin(); itr != in_range_players->end() && sendcount < 150; ++itr )
	{
		plr = *itr;
		ASSERT( plr->GetSession() != NULL );
		if ( plr->CanSee( this ) )
		{
			++sendcount;
			plr->PushMovementData( guid, srcPosX, srcPosY, srcHeight, destPosX, destPosY, destHeight, mode );
		}
	}
}

void Object::UpdateMoving(uint32 timeDiff)
{
	if (!m_bMoving)
		return ;

	if (m_nMovingUpdateTimer > timeDiff)
	{
		m_nMovingUpdateTimer -= timeDiff;
		return ;
	}
	
	// ��ȡ����������updateʱ��(��һ��׼ȷ����ȡ������lastMovetimer�����ı�������¼)
	uint32 updateDelta = timeDiff - m_nMovingUpdateTimer + OBJ_MOVE_UPDATE_TIMER;
	m_nMovingUpdateTimer = OBJ_MOVE_UPDATE_TIMER;

	float disWithDest = CalcDistance(m_moveDestPosition);

	// ����update��ʱ�������ٶ�
	float dist = updateDelta * GetMoveSpeed() / 1000.0f;
	float newPosX = GetPositionX() + m_moveDirection.m_fX * dist;
	float newPosY = GetPositionY() + m_moveDirection.m_fY * dist;

	// ������ӱ�������,��ô����Ҷ�����˵,��Ҫֹͣ�ƶ���
	if (!sSceneMapMgr.IsCurrentPositionMovable(m_mapId, newPosX, newPosY))
	{
		if (isPlayer())
		{
			StopMoving(true, m_position);
			return ;
		}
	}

	float disWithNextPos = CalcDistance(newPosX, newPosY);
	if (disWithDest <= disWithNextPos)
	{
		// �Ѿ�����Ŀ�ĵ�
		SetPosition(m_moveDestPosition.m_fX, m_moveDestPosition.m_fY);
		StopMoving(true, m_moveDestPosition);
	}
	else
	{
		// �����ƶ�
		SetPosition(newPosX, newPosY);
		// sLog.outDebug("[�ƶ���]����("I64FMTD")�ƶ���,��ǰ����λ��[%.2f,%.2f]", GetGUID(), newPosX, newPosY);
		m_moveDirection = m_moveDestPosition - m_position;
		m_moveDirection.Norm();
	}
}

float Object::CalcDistance(Object* Ob)
{
	return CalcDistance(GetPositionX(), GetPositionY(), Ob->GetPositionX(), Ob->GetPositionY());
}

float Object::CalcDistance(float ObX, float ObY)
{
	return CalcDistance(GetPositionX(), GetPositionY(), ObX, ObY);
}

float Object::CalcDistance(Object* Oa, Object* Ob)
{
	return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), Ob->GetPositionX(), Ob->GetPositionY());
}

float Object::CalcDistance(Object* Oa, float ObX, float ObY)
{
	return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), ObX, ObY);
}

float Object::CalcDistance(float OaX, float OaY, float ObX, float ObY)
{
	float xdest = OaX - ObX;
	float ydest = OaY - ObY;
	return sqrtf( ydest*ydest + xdest*xdest );
}

float Object::CalcDistance(FloatPoint &pos)
{
	return pos.Dist(m_position);
}

float Object::CalcDistanceSQ(Object *obj)
{
	if(obj->GetMapId() != m_mapId) 
		return 250000.0f;				// enough for out of range
	return m_position.DistSqr(obj->GetPosition());
}

float Object::CalcDistanceWithModelSize(Object *obj)
{
	float modelDist = GetModelRadius() + obj->GetModelRadius();
	float dist = CalcDistance(obj);
	if(dist < modelDist)
		return 0.0f;

	return dist - modelDist;
}

float Object::CalcDistanceWithModelSize(float x, float y)
{
	float dist = CalcDistance(x, y);
	if(dist < GetModelRadius())
		return 0.0f;

	return dist - GetModelRadius();
}

void Object::SetPosition( float newX, float newY)
{
	bool updateMap = false;

	FloatPoint newPos(newX, newY);
	if(m_lastMapUpdatePosition.Dist(newPos) > 1.5f)	// ����1.5�׵ľ��룬update��Χ
		updateMap = true;

	m_position.Init(newX, newY);

#ifdef _WIN64
	ASSERT(m_position.m_fX >= 0.0f && m_position.m_fY >= 0.0f && m_position.m_fX < mapLimitWidth && m_position.m_fY < mapLimitLength);
#else
	if (m_position.m_fX < 0.0f)
		m_position.m_fX = 1.0f;
	if (m_position.m_fX >= mapLimitWidth)
		m_position.m_fX = mapLimitWidth - 1;
	if (m_position.m_fY < 0.0f)
		m_position.m_fY = 1.0f;
	if (m_position.m_fY >= mapLimitLength)
		m_position.m_fY = mapLimitLength - 1;
#endif

	if (IsInWorld() && updateMap)
	{
		m_lastMapUpdatePosition.Init(newX, newY);
		m_mapMgr->ChangeObjectLocation( this, true );
		
		//sLog.outDebug("Obj "I64FMTD" �ƶ��� ��ǰ����(%u, %u), Ŀ������(%u, %u)", GetGUID(), (uint32)GetPositionX(), (uint32)GetPositionY(), 
		//			uint32(m_moveDestPosition.m_fX), uint32(m_moveDestPosition.m_fY));
	}

	//ͬ����������
	if(isPlayer())
	{
		//CPet *pet = TO_PLAYER(this)->GetCurPet(); 
		//if(pet != NULL && !pet->IsServerControl())
		//{
		//	TO_PLAYER(this)->GetCurPet()->SetPosition(newX, newY);
		//}
	}
}

void Object::SetRespawnLocation(const FloatPoint &spawnPos, float spawnDir)
{
	m_spawnLocation = spawnPos;
	m_spawndir = spawnDir;
}

void Object::ModifyPosition(float modX, float modY)
{
	m_position.m_fX += modX;
	m_position.m_fY += modY;
	bool updateMap = false;

	FloatPoint newPos(m_position);
	if(m_lastMapUpdatePosition.DistSqr(newPos) > 4.0f)		/* 2.0f */
		updateMap = true;

	if (IsInWorld() && updateMap)
	{
		m_lastMapUpdatePosition = newPos;
		m_mapMgr->ChangeObjectLocation( this , false );
	}
}

bool Object::CheckPosition(uint16 x, uint16 y)
{
	return ((uint16)m_position.m_fX==x) && ((uint16)m_position.m_fY==y);
}

void Object::_Create( uint32 mapid, float x, float y, float dir )
{
	m_mapId = mapid;
	m_position.Init(x, y);
	m_spawnLocation.Init(x, y);
	m_lastMapUpdatePosition.Init(x,y);
	m_spawndir = dir;
	m_direction = dir;
}

// �ѱ�����Ĵ�������д��Ŀ����ҵĻ�������
void Object::BuildCreationDataBlockForPlayer( Player* target )
{
	pbc_wmessage *creationBuffer = target->GetObjCreationBuffer();
	pbc_wmessage *data = pbc_wmessage_message(creationBuffer, "createObjectList");
	if (data == NULL)
		return ;

	BuildCreationDataBlockToBuffer(data, target);
	// ��target�����ͼ�Ĵ����Ͷ���
	target->PushCreationData();
}

void Object::BuildCreationDataBlockToBuffer( pbc_wmessage *data, Player *target )
{
	sPbcRegister.WriteUInt64Value(data, "objectGuid", GetGUID());
	const uint32 objType = GetTypeID();
	pbc_wmessage_integer(data, "objectType", objType, 0);

	switch (objType)
	{
	case TYPEID_PLAYER:
		{
			Player *plr = TO_PLAYER(this);
			pbc_wmessage *plrData = pbc_wmessage_message(data, "plrData");
			if (plrData != NULL)
			{
				pbc_wmessage_string(plrData, "plrName", plr->strGetUtf8NameString().c_str(), 0);
			}
		}
		break;
	case TYPEID_MONSTER:
		{
			Monster *m = TO_MONSTER(this);
			pbc_wmessage *monsterData = pbc_wmessage_message(data, "monsterData");
			if (monsterData != NULL)
			{
				char szName[64] = { 0 };
				snprintf(szName, 64, "id:"I64FMTD"", m->GetGUID());
				// monsterData->set_monstername(m->GetProto()->m_MonsterName);
				pbc_wmessage_string(monsterData, "monsterName", szName, 0);
			}
		}
		break;
	case TYPEID_PARTNER:
		{
			Partner *p = TO_PARTNER(this);
			pbc_wmessage *partnerData = pbc_wmessage_message(data, "partnerData");
			if (partnerData != NULL)
			{
				pbc_wmessage_integer(partnerData, "partnerProtoID", p->GetEntry(), 0);
			}
		}
		break;
	default:
		break;
	}

	pbc_wmessage_real(data, "curPosX", m_position.m_fX);
	pbc_wmessage_real(data, "curPosY", m_position.m_fY);
	pbc_wmessage_real(data, "curPosHeight", sSceneMapMgr.GetPositionHeight(GetMapId(), m_position.m_fX, m_position.m_fY));
	pbc_wmessage_integer(data, "curDirection", (uint32)m_direction, 0);
	uint32 movingFlag = m_bMoving ? 1 : 0;
	pbc_wmessage_integer(data, "isMoving", movingFlag, 0);

	if (m_bMoving)
	{
		pbc_wmessage_real(data, "moveDestPosX", m_moveDestPosition.m_fX);
		pbc_wmessage_real(data, "moveDestPosY", m_moveDestPosition.m_fY);
		pbc_wmessage_real(data, "moveDestHeight", sSceneMapMgr.GetPositionHeight(GetMapId(), m_moveDestPosition.m_fX, m_moveDestPosition.m_fY));
		pbc_wmessage_integer(data, "moveType", uint32(OBJ_MOVE_TYPE_START_MOVING), 0);
	}

	// sLog.outDebug("[MSG2008]���һ��������������,��ǰ����%u��,byteSize=%u", creationBuffer->createobjectlist_size(), creationBuffer->ByteSize());
	// we have dirty data, or are creating for ourself.
	UpdateMask updateMask;
	updateMask.SetCount( m_valuesCount );
	_SetCreateBits( &updateMask, target );
	// this will cache automatically if needed
	_BuildValuesCreation( data, &updateMask );
}

bool Object::BuildValuesUpdateBlockToBuffer( pbc_wmessage *msg, Player *target )
{
	uint32 uRet = 0;
	UpdateMask updateMask;
	updateMask.SetCount( m_valuesCount );
	_SetUpdateBits( &updateMask, target );
	if (updateMask.GetSetBitCount() <= 0)
		return false;

	pbc_wmessage *data = pbc_wmessage_message(msg, "UpdateObjects");
	if (data == NULL)
		return false;

	sPbcRegister.WriteUInt64Value(data, "objGuid", GetGUID());
	_BuildValuesUpdate(data, &updateMask);
	
	return true;
}

bool Object::BuildValuesUpdateBlockForPlayer( Player *target )
{
	UpdateMask updateMask;
	updateMask.SetCount( m_valuesCount );
	_SetUpdateBits( &updateMask, target );
	if (updateMask.GetSetBitCount() <= 0)
		return false;

	pbc_wmessage *buffer = target->GetObjUpdateBuffer();
	pbc_wmessage *data = pbc_wmessage_message(buffer, "updateObjects");
	if (data == NULL)
		return false;

	sPbcRegister.WriteUInt64Value(data, "objGuid", GetGUID());
	_BuildValuesUpdate(data, &updateMask);

	return true;
}

void Object::BuildRemoveDataBlockForPlayer( Player *target )
{
	uint64 guid = GetGUID();
	uint32 tp = GetTypeID();

	target->PushOutOfRange(guid, tp);
}

void Object::ClearUpdateMask()
{
	m_updateMask.Clear();
	m_objectUpdated = false;
}

void Object::SetUpdatedFlag()
{
	if (!m_objectUpdated && m_mapMgr != NULL)
	{
		m_mapMgr->ObjectUpdated(this);
		m_objectUpdated = true;
	}
}

void Object::CheckErrorOnUpdateRange(uint32 cellX, uint32 cellY, uint32 mapId)
{
	// sLog.outError("(OBJ type=%u,guid=%u)�޷���ö�Ӧ����(%u,%u)�ĵ�ͼcell(mapid=%u)", (uint32)GetTypeID(), GetGUID(), cellX, cellY, mapId);
}

void Object::OnValueFieldChange(const uint32 index)
{
	if(IsInWorld())
	{
		m_updateMask.SetBit( index );

		if(!m_objectUpdated)
			SetUpdatedFlag();
	}
}
//=======================================================================================
//  Creates an update block with the values of this object as
//  determined by the updateMask.
//=======================================================================================
void Object::_BuildValuesCreation( pbc_wmessage *createData, UpdateMask *updateMask )
{
	WPAssert( updateMask && updateMask->GetCount() == m_valuesCount );
	uint32 bc;
	uint32 values_count;
	if( m_valuesCount > ( 2 * 0x20 ) )//if number of blocks > 2->  unit and player+item container
	{
		bc = updateMask->GetUpdateBlockCount();
		values_count = (uint32)min( bc * 32, m_valuesCount );
	}
	else
	{
		bc = updateMask->GetBlockCount();
		values_count = m_valuesCount;
	}

	for( uint32 index = 0; index < values_count; ++index )
	{
		if( updateMask->GetBit( index ) )
		{
			pbc_wmessage *data = pbc_wmessage_message(createData, "propList");
			if (data != NULL)
			{
				pbc_wmessage_integer(data, "propIndex", index, 0);
				sPbcRegister.WriteUInt64Value(data, "propValue", m_uint64Values[index]);
				// sLog.outDebug("BuildCreateValue,objID="I64FMTD", index=%u,value="I64FMTD".", GetGUID(), index, m_uint64Values[index]);
			}
			updateMask->UnsetBit( index );
		}
	}
}

void Object::_BuildValuesUpdate( pbc_wmessage *propData, UpdateMask *updateMask )
{
	WPAssert( updateMask && updateMask->GetCount() == m_valuesCount );
	uint32 bc;
	uint32 values_count;
	if( m_valuesCount > ( 2 * 0x20 ) )//if number of blocks > 2->  unit and player+item container
	{
		bc = updateMask->GetUpdateBlockCount();
		values_count = (uint32)min( bc * 32, m_valuesCount );
	}
	else
	{
		bc = updateMask->GetBlockCount();
		values_count = m_valuesCount;
	}

	for( uint32 index = 0; index < values_count; ++index )
	{
		if( updateMask->GetBit( index ) )
		{
			pbc_wmessage *data = pbc_wmessage_message(propData, "updateProps");
			if (data != NULL)
			{
				pbc_wmessage_integer(data, "propIndex", index, 0);
				sPbcRegister.WriteUInt64Value(data, "propValue", m_uint64Values[index]);
			}

			updateMask->UnsetBit( index );
		}
	}
}

void Object::_SetUpdateBits(UpdateMask *updateMask, Player* target) const
{
	*updateMask = m_updateMask;
}

void Object::_SetCreateBits(UpdateMask *updateMask, Player* target) const
{
	for(uint32 i = 0; i < m_valuesCount; ++i)
	{
		if(m_uint64Values[i] != 0)
			updateMask->SetBit(i);
	}
}

void Object::AddToWorld()
{
	// ������Ѱ����ͨ������mapmgr�ö������
	MapMgr* mapMgr = sInstanceMgr.GetNormalSceneMapMgr(this);
	// ���Ǽ�����ͨ��������ôѰ����Ҹ�����ϵͳʵ�������ö������
	if ( mapMgr == NULL )
		mapMgr = OnObjectGetInstanceFailed();
	
	// �����Ȼ�Ҳ���mapmgr˵������
	if (mapMgr == NULL)
	{
		sLog.outError( "Couldn't get (map=%u, instance=%u) for Object "I64FMTD"", GetMapId(), GetInstanceID(), GetGUID() );
		return ;
	}

	m_mapMgr = mapMgr;

	mapMgr->AddObject(this);

	//// correct incorrect instance id's
	m_instanceId = m_mapMgr->GetInstanceID();
}

MapMgr* Object::OnObjectGetInstanceFailed()
{
	// ��ͼ�Ӹ���ʵ����Ѱ�ҵ���object�����ĵ�ͼ
	return sInstanceMgr.GetInstanceMapMgr(GetMapId(), GetInstanceID());
}

//Unlike addtoworld it pushes it directly ignoring add pool
//this can only be called from the thread of mapmgr!!!
void Object::PushToWorld(MapMgr* mgr)
{
	ASSERT(mgr != NULL);

	if(mgr == NULL)
	{
		// Reset these so session will get updated properly.
		if(isPlayer())
		{
			cLog.Error("Object","Kicking Player %s due to empty MapMgr;",TO_PLAYER(this)->strGetGbkNameString().c_str());
			GameSession *session = TO_PLAYER(this)->GetSession();
			session->LogoutPlayer(false);
			session->Disconnect();
		}
		return; //instance add failed
	}

	m_mapId = mgr->GetMapId();
	m_instanceId = mgr->GetInstanceID();
	m_mapMgr = mgr;
	m_event_Instanceid = mgr->event_GetInstanceID();
	OnPrePushToWorld();

	mgr->PushObject(this);
	
    event_Relocate();
	OnPushToWorld();
}

void Object::Init()
{
	// ��ʼ��һ�����ߵĶ���
	m_bMoving = false;
	m_moveDestPosition.Init();
	m_moveDirection.Init();
	m_nStartMovingTime = 0;
	
	m_instanceId = 0;
	m_deathState = ALIVE;
}

void Object::Delete()
{
	if(IsInWorld())
		RemoveFromWorld(true);
	delete this;
}

void Object::RemoveFromWorld(bool free_guid)
{
	ASSERT(m_mapMgr != NULL);
	MapMgr* m = m_mapMgr;
	m_mapMgr = NULL;

	if (isMoving())
		StopMoving(true, GetPositionNC());

	OnPreRemoveFromWorld(m);
	sEventMgr.RemoveEvents(this);	// ���������ж�ʱ��ɱ��
	m_event_Instanceid = -1;	// ���worldupdate��
	m->RemoveObject(this, free_guid);
	//// update our event holder
	event_Relocate();
}

bool Object::CanActivate()
{
	switch(GetTypeID())
	{
	case TYPEID_MONSTER:
		{
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void Object::Activate(MapMgr* mgr)
{
	switch(GetTypeID())
	{
	case TYPEID_MONSTER:
		// sLog.outDebug("activeMonsters ��ӹ���id="I64FMTD"", GetGUID());
		mgr->pendingActiveMonsterGuids.push_back(GetGUID());
		break;
	default:
		break;
	}

	Active = true;
}

void Object::Deactivate(MapMgr* mgr)
{
	switch(GetTypeID())
	{
	case TYPEID_MONSTER:
		{
			mgr->pendingDeactiveMonsterGuids.push_back(GetGUID());
			// sLog.outDebug("activeMonsters �Ƴ�����id="I64FMTD"", GetGUID());
		}
		break;
	default:
		break;
	}

	Active = false;
}

void Object::SendMessageToSet(WorldPacket *data, bool bToSelf)
{
	if(bToSelf && isPlayer())
	{
		Player *plr = TO_PLAYER(this);
		if (plr->GetSession() != NULL)
			plr->GetSession()->SendPacket(data);		
	}

	if(!IsInWorld() || !HasInRangePlayers())
		return;

	hash_set<Player*>::iterator itr = m_inRangePlayers.begin();
	hash_set<Player*>::iterator it_end = m_inRangePlayers.end();
	for(; itr != it_end; ++itr)
	{
		if ((*itr)->GetSession())
			(*itr)->GetSession()->SendPacket(data);
	}
}

const char* Object::szGetUtf8Name() const
{
	static const char *szUnknowObject = "Unknow Object";
	return szUnknowObject;
}


