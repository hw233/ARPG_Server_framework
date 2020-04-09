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
#include "ObjectPoolMgr.h"
#include "convert/strconv.h"

UpdateMask Player::m_visibleUpdateMask;

Player::Player () : m_session(NULL), m_gamestates(STATUS_ANY), m_pbcMovmentDataBuffer(NULL), m_pbcRemoveDataBuffer(NULL), m_pbcRemoveDataCounter(0),
					m_pbcCreationDataBuffer(NULL), m_pbcPropUpdateDataBuffer(NULL), m_bNeedSyncPlayerData(false)
{
	m_uint64Values = NULL;
	m_valuesCount = 0;
	ResetSyncObjectDatas();
}

Player::~Player ()
{
	SAFE_DELETE_ARRAY(m_uint64Values);
	m_valuesCount = 0;
}

void Player::Init(GameSession *session, uint64 guid, const uint32 fieldCount)		//���湹�캯��
{
	m_gamestates = STATUS_ANY;
	m_uint64Values = new uint64[fieldCount];
	m_valuesCount = fieldCount;
	m_updateMask.SetCount(fieldCount);
	
	memset(m_uint64Values, 0, fieldCount * sizeof(uint64));

	SetUInt64Value( OBJECT_FIELD_GUID, guid );
	SetUInt64Value( OBJECT_FIELD_TYPE, TYPEID_PLAYER );

	m_session = session;
	m_session->SetPlayer(this);

	ResetSyncObjectDatas();

	bProcessPending			= false;
	if (m_pbcPropUpdateDataBuffer == NULL)
		m_pbcPropUpdateDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002012");
	bMoveProcessPending		= false;
	if (m_pbcMovmentDataBuffer == NULL)
		m_pbcMovmentDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002004");
	bRemoveProcessPending	= false;
	if (m_pbcRemoveDataBuffer == NULL)
		m_pbcRemoveDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002010");
	bCreateProcessPending	= false;
	if (m_pbcCreationDataBuffer == NULL)
		m_pbcCreationDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002008");
	
	// ���û�������
	if (sInstanceMgr.GetDefaultBornMap() != 0)
	{
		SetMapId(sInstanceMgr.GetDefaultBornMap());
		SetPosition(sInstanceMgr.GetDefaultBornPosX(), sInstanceMgr.GetDefaultBornPosY());
	}
	else
	{
		SetMapId(1001);
		SetPosition(100, 100);
	}

	Object::Init();
}

void Player::Term()//������������
{
	// ɾ��������
	if (!m_Partners.empty())
	{
		Partner *p = NULL;
		for (PlayerPartnerMap::iterator it = m_Partners.begin(); it != m_Partners.end(); ++it)
		{
			p = it->second;
			p->Term();
			SAFE_DELETE(p);
		}
		m_Partners.clear();
	}

	memset(m_uint64Values, 0, m_valuesCount * sizeof(uint64));
	m_session = NULL;
	
	DEL_PBC_W(m_pbcMovmentDataBuffer);
	DEL_PBC_W(m_pbcCreationDataBuffer);
	DEL_PBC_W(m_pbcRemoveDataBuffer);
	m_pbcRemoveDataCounter = 0;
	DEL_PBC_W(m_pbcPropUpdateDataBuffer);

	ResetSyncObjectDatas();

	Object::Term();
}

string Player::strGetGbkNameString()
{
	if (m_NickName.length() <= 0)
		return m_NickName;

	CUTF82C *utf8_name = new CUTF82C(m_NickName.c_str());
	string str = utf8_name->c_str();
	delete utf8_name;

	return str;
}

uint32	Player::GetGameStates()
{ 
	//��״̬�ı��ʱ��ֱ���޸�״̬��Ȼ��Ϳ���ֱ�ӷ���m_gamestates��Ŀǰʵʱ��ȡ״̬
	/*m_gamestates = STATUS_ANY; 
	if(this->isDead())
	m_gamestates|=STATUS_DEAD;
	else
	m_gamestates|=STATUS_ALIVE;
	if(GetMapMgr()==NULL)
	m_gamestates|=STATUS_NOTINWORLD;*/
	return m_gamestates;
}

#define PLAYER_UPDATE_SPAN     50
void	Player::Update( uint32 p_time )
{
	if (!IsInWorld())		// ����������
		return ;

	UpdateMoving(p_time);

	// Unit::Update(p_time);	// �˴���Ѫ(AI�Ŀ���)

	// ���������Ե�buffer����ͬ��
	if (((++m_nSyncDataLoopCounter % 10) == 0) && m_bNeedSyncPlayerData)
		SyncUpdateDataMasterServer();
}

bool Player::SafeTeleport(uint32 MapID, uint32 InstanceID, float X, float Y)
{
	// Lookup map info
	MapInfo * mi = sSceneMapMgr.GetMapInfo( MapID );
	if ( mi == NULL )
		return false;

	//are we changing instance or map?
	bool force_new_world = ( m_mapId != MapID || ( InstanceID != 0 && (uint32)m_instanceId != InstanceID ) );	//�����л������������
	return _Relocate( MapID, FloatPoint(X, Y), force_new_world, InstanceID );
}

bool Player::CanEnterThisScene(uint32 mapId)
{
	// �����ܷ���룬�����ýű�ʵ��
	return true;
}

bool Player::HandleSceneTeleportRequest(uint32 destServerIndex, uint32 destMapID, FloatPoint destPos)
{
	uint32 curMapID = GetMapId();
	uint32 changeRet = TRANSFER_ERROR_NONE;
	do 
	{
		if (!IsInWorld() )
		{
			changeRet = TRANSFER_ERROR_NOT_IN_WORLD;
			break;
		}
		//if (IsInCombat())
		//{
		//	changeRet = TRANSFER_ERROR_BADSTATE_COMBATING;
		//	break;
		//}
		if (destPos.m_fX >= mapLimitWidth || destPos.m_fY >= mapLimitLength)
		{
			changeRet = TRANSFER_ERROR_COORD;
			break;
		}
		if(!CanEnterThisScene(destMapID))
		{
			//GetSession()->SendPopUpMessage(POPUP_AREA_4, sLang.GetTranslate("Ŀ�곡��δ����"));
			changeRet = TRANSFER_ERROR_SCENE_NOT_OPEN;
			break;
		}
	} while (false);

	if (changeRet == TRANSFER_ERROR_NONE)
	{
		if (curMapID == destMapID)														// �����ͬ��ͼ���ͣ������ز���������
		{
			if (InSpecifiedScene(destMapID) && !destPos.IsSamePoint(0.0f, 0.0f))		// Ӧ�ü��Ŀ����Ƿ���Ե���
			{
				if ( CalcDistance( destPos.m_fX, destPos.m_fY ) < 3.0f )	// 3���ڵľ������ڲ�����
					return false;
			}
			else
				return false;
		}
		else																	// ���ؽ���������������ζ�Ҫ���������ؼ����������������
		{
			// ����Ƿ��ڱ��������ڵĳ����л�
			if (destPos.IsSamePoint(0.0f, 0.0f))		// ��ȡ���͵��Ŀ�곡������
				changeRet = sSceneMapMgr.TransferPlayer( curMapID, GetPositionNC(), destMapID, destPos );

			if (changeRet == TRANSFER_ERROR_NONE)
			{
				// ����Ƿ�Ϊ��Ч�ĵ�ͼ
				MapInfo * pminfo = sSceneMapMgr.GetMapInfo( destMapID );
				if ( pminfo == NULL || !pminfo->isNormalScene() )
				{
					changeRet = TRANSFER_ERROR_MAP;
				}
				else if(pminfo->enter_min_level > GetLevel())
				{
					//GetSession()->SendPopUpMessage(POPUP_AREA_4, sLang.GetTranslate("�����ȼ�����"));
					changeRet = TRANSFER_ERROR_LEVEL;
				}
				else
				{
					// ����Ƿ�Ϊ��Ч����
					if (!sSceneMapMgr.IsCurrentPositionMovable(destMapID, destPos.m_fX, destPos.m_fY))
						changeRet = TRANSFER_ERROR_COORD;
				}
			}
		}
	}

	if (changeRet == TRANSFER_ERROR_NONE)
	{
		if (destServerIndex == ILLEGAL_DATA_32)
		{
			if (sInstanceMgr.HandleSpecifiedScene(destMapID))
			{
				// �ڱ����������д���
				if ( !SafeTeleport( destMapID, 0, destPos.m_fX, destPos.m_fY ) )
				{
					sLog.outError( "��� %s ���͵� %u(%.1f,%.1f) ʧ��", strGetGbkNameString().c_str(), destMapID, destPos.m_fX, destPos.m_fY );
					changeRet = TRANSFER_ERROR_MAP;
				}
			}
			else
			{
				uint32 targetServerIndex = sInstanceMgr.GetHandledServerIndexBySceneID(destMapID);
				sLog.outString("��� %s ׼��ȥ�������� %u ����ĳ���%u (%.1f,%.1f)", strGetGbkNameString().c_str(), targetServerIndex, destMapID, destPos.m_fX, destPos.m_fY);
				changeRet = TRANSFER_ERROR_NONE_BUT_SWITCH;
			}
		}
		else
		{
			// ���Ŀ�곡�������Ǳ�����Ҳ����Ҫ���⴦��
			if (destServerIndex == sGateCommHandler.GetSelfSceneServerID())
			{
				// �ڱ����������д���
				if ( !SafeTeleport( destMapID, 0, destPos.m_fX, destPos.m_fY ) )
				{
					sLog.outError( "��� %s ���͵� %u(%.1f,%.1f) ʧ��", strGetGbkNameString().c_str(), destMapID, destPos.m_fX, destPos.m_fY );
					changeRet = TRANSFER_ERROR_MAP;
				}
			}
			else
			{
				sLog.outString("��� %s ׼��ȥ�������� %u ����ĳ���%u (%.1f,%.1f)", strGetGbkNameString().c_str(), destServerIndex, destMapID, destPos.m_fX, destPos.m_fY);
				changeRet = TRANSFER_ERROR_NONE_BUT_SWITCH;
			}
		}
	}

	// ֪ͨ��ȥ
	sPark.SceneServerChangeProc(GetSession(), destMapID, destPos, changeRet, destServerIndex);
	return true;
}

bool Player::_Relocate(uint32 mapid, const FloatPoint &v, bool force_new_world, uint32 instance_id)
{
	//are we changing maps?
	if ( force_new_world || m_mapId != mapid )	// ������������SafeTeleprot���ã�force_new_world���Ѱ�����ͼ�ж�
	{
		//Preteleport will try to find an instance (saved or active), or create a new one if none found.
		uint32 status = sInstanceMgr.PreTeleport( mapid, this, instance_id );
		if ( status != INSTANCE_OK )
		{
			// δ�ҵ����ʵĸ������д��� by eeeo@2011.7.14
			// ����Ӧ�����ϲ��߼���������ж�
			return false;
		}

		//did we get a new instanceid?
		if ( instance_id != 0 )
			SetInstanceID(instance_id);

		if ( IsInWorld() )  // ��������
		{
			bool savePos = true;
			if(savePos)
			{
				m_lastMapData.SetData(GetMapId(), GetPositionX(), GetPositionY());
			}
		}
		else
		{
			m_lastMapData.ClearData();
		}

		//remove us from this map
		if ( IsInWorld() )
			RemoveFromWorld();

		SetMapId( mapid );

		//if(GetPlayerTransferStatus() == TRANSFER_PENDING)
		//	sLog.outError("when plr %s safeteleport to map %u in state transpending", strGetGbkNameString().c_str(), mapid);

		////trandsfer_pending��ʾ������ڴ����У���û�ɹ�
		//SetPlayerTransferStatus( TRANSFER_PENDING );
	}

	// �޸�����
	SetPosition(v.m_fX, v.m_fY);

	return true;
}

void Player::EjectFromInstance()
{
	// �Ӹ������˳������ýű�����
}

void Player::AddToWorld()
{
	// If we join an invalid instance and get booted out, TO_PLAYER(this) will prevent our stats from doubling :P
	if ( IsInWorld() )
	{
		// sLog.outError( "Player %s is already InWorld(%u) when he is calling AddToWorld() !", strGetGbkNameString().c_str(), GetMapId());
		return;
	}

	Object::AddToWorld();

	/// Add failed.
	if ( m_mapMgr == NULL )
	{
		sLog.outError( "Adding player %s to (map=%u,instance=%u) failed.", strGetGbkNameString().c_str(), GetMapId(), GetInstanceID() );
		// eject from instance
		// EjectFromInstance();		// �ص�ԭ�ȵĳ�����
		return;
	}
	else
	{
		if ( GetSession() != NULL )
			GetSession()->SetInstance( m_mapMgr->GetInstanceID() );
	}
}

MapMgr* Player::OnObjectGetInstanceFailed()
{
	MapMgr *ret = Object::OnObjectGetInstanceFailed();

	// ��ҪΪ��Ҵ������˸������ҵ��µ�ϵͳʵ������ҽ���
	//if (ret == NULL)
	//{
	//	Instance *instance = NULL;
	//	MapInfo *mapInfo = sSceneMapMgr.GetMapInfo(GetMapId());
	//	if (mapInfo == NULL)
	//		return NULL;

	//	if (mapInfo->isGuildScene())
	//		instance = sInstanceMgr.FindGuildInstance(GetMapId(), GetGuildID());
	//	else if (mapInfo->isSystemInstance())
	//		instance = sInstanceMgr.FindSystemInstance(GetMapId());
	//	else
	//		instance = sInstanceMgr.GetInstance(GetMapId(), GetInstanceID());

	//	if (instance != NULL)
	//	{
	//		// �Ѿ����ڸ�ʵ�����������Ƿ��ܽ���
	//		uint8 checkRet = sInstanceMgr.PlayerOwnsInstance(instance, this);
	//		if (checkRet >= OWNER_CHECK_OK)
	//		{
	//			// ���ʵ��������Ҫϵͳ�ٴ���
	//			if (instance->m_mapMgr == NULL && instance->m_mapInfo->isInstance())
	//				instance->m_mapMgr = sInstanceMgr.CreateInstance(instance);
	//			ret = instance->m_mapMgr;
	//		}
	//		
	//		else
	//			sLog.outError("��� %s ��������볡�� %u ������(ret=%u)", strGetGbkNameString().c_str(), GetInstanceID(), uint32(checkRet));
	//	}

	//	// ��Ӧ�ò�������Ҹ���ʵ��
	//	// ASSERT(ret != NULL);
	//}

	return ret;
}

void Player::OnPushToWorld()
{
	// Process create packet
	if( m_mapMgr != NULL )
		ProcessObjectPropUpdates();

	// Unit::OnPushToWorld();

	// ���ýű��Ľ��볡��
	// CALL_INSTANCE_SCRIPT_EVENT( m_mapMgr, OnPlayerEnter )( this );

	// ֪ͨ������Լ�����ĳ����
	WorldPacket centrePacket(SCENE_2_MASTER_PLAYER_MSG_EVENT, 128);
	centrePacket << uint32(0) << sGateCommHandler.GetSelfSceneServerID();
	centrePacket << GetGUID();
	centrePacket << uint32(STM_plr_event_enter_map);
	uint16 mapID = GetMapId(), posX = GetPositionX(), posY = GetPositionY();
	centrePacket << mapID << posX << posY << GetInstanceID();
	GetSession()->SendServerPacket(&centrePacket);

	sPark.OnPlayerEnterMap(GetMapId());
}

void Player::RemoveFromWorld()
{
	if(IsInWorld())
	{
		sPark.OnPlayerLeaveMap(GetMapId());
		Object::RemoveFromWorld(false);
	}
}

void Player::OnValueFieldChange(const uint32 index)
{
	// sLog.outDebug("���:%s ����value,index=%u, val="I64FMTD"..", strGetGbkNameString().c_str(), index, GetUInt64Value(index));
	if(IsInWorld() && m_visibleUpdateMask.GetBit(index))
	{
		m_updateMask.SetBit( index );
		if(!m_objectUpdated)
			SetUpdatedFlag();
	}
}

void Player::SyncUpdateDataMasterServer()
{
	ScriptParamArray params;
	params << GetGUID();

	LuaScript.Call("SyncPlayerDataToMaster", &params, NULL);

	// ����ͬ������
	ResetSyncObjectDatas();
}

void Player::ResetSyncObjectDatas()
{
	m_nSyncDataLoopCounter = 0;
	m_bNeedSyncPlayerData = false;
}

//===================================================================================================================
//  Set Create Player Bits -- Sets bits required for creating a player in the updateMask.
//  Note:  Doesn't set Quest or Inventory bits
//  updateMask - the updatemask to hold the set bits
//===================================================================================================================
void Player::_SetCreateBits(UpdateMask *updateMask, Player* target) const
{
	if(target == this)
	{
		Object::_SetCreateBits(updateMask, target);
	}
	else
	{
		for(uint32 index = 0; index < m_valuesCount; index++)
		{
			if(m_uint64Values[index] != 0 && Player::m_visibleUpdateMask.GetBit(index))
				updateMask->SetBit(index);
		}
	}
}

void Player::_SetUpdateBits(UpdateMask *updateMask, Player* target) const
{
	if(target == this)
	{
		Object::_SetUpdateBits(updateMask, target);
	}
	else
	{
		Object::_SetUpdateBits(updateMask, target);
		*updateMask &= Player::m_visibleUpdateMask;
	}
}

