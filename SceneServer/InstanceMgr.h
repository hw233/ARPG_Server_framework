/*
 * Ascent MMORPG Server
 * Copyright (C) 2005-2010 Ascent Team <http://www.ascentemulator.net/>
 *
 * This software is  under the terms of the EULA License
 * All title, including but not limited to copyrights, in and to the AscentNG Software
 * and any copies there of are owned by ZEDCLANS INC. or its suppliers. All title
 * and intellectual property rights in and to the content which may be accessed through
 * use of the AscentNG is the property of the respective content owner and may be protected
 * by applicable copyright or other intellectual property laws and treaties. This EULA grants
 * you no rights to use such content. All rights not expressly granted are reserved by ZEDCLANS INC.
 *
 */

#ifndef __WORLDCREATOR_H
#define __WORLDCREATOR_H

#define MAX_INSTANCE_ID		90000000				// ͨ��instancemgr�����instanceid���Ϊ9ǧ�򣬳����˴�1000��ʼ���
#define MAX_MONSTER_GUID	(0x00000002540BE400)	// ���Ĺ���id��ʮ����:10001000000�����ܳ�����ҵ���Сid

enum InstanceGroupSymbolType
{
	Instance_group_type_none	        = 0,		// ��
	Instance_group_type_personal        = 1,		// ���˸���
	Instance_group_type_team	        = 2,		// ��Ӹ���
	Instance_group_type_guild	        = 3,		// ����ʵ��
};

enum INSTANCE_ABORT_ERROR
{
	INSTANCE_ABORT_ERROR_ERROR			= 0x00,
	INSTANCE_ABORT_FULL					= 0x01,
	INSTANCE_ABORT_NOT_FOUND			= 0x02,
	INSTANCE_ABORT_TOO_MANY				= 0x03,
	INSTANCE_ABORT_ENCOUNTER			= 0x04,
	INSTANCE_ABORT_NON_CLIENT_TYPE		= 0x05,
	INSTANCE_ABORT_NOT_IN_RAID_GROUP	= 0x07,

	INSTANCE_OK = 0x10,
};

enum OWNER_CHECK
{
	OWNER_CHECK_ERROR		= 0,
	OWNER_CHECK_EXPIRED		= 1,
	OWNER_CHECK_NOT_EXIST	= 2,
	OWNER_CHECK_NO_GROUP	= 3,
	OWNER_CHECK_DIFFICULT	= 4,
	OWNER_CHECK_MAX_LIMIT	= 5,
	OWNER_CHECK_MIN_LEVEL	= 6,
	OWNER_CHECK_WRONG_GROUP	= 7,
	OWNER_CHECK_OK			= 10,
	OWNER_CHECK_GROUP_OK	= 11,
	OWNER_CHECK_SAVED_OK	= 12,
};

class MapSpawn;
class MapMgr;
class Object;
class Player;

class Instance
{
public:
	Instance() : m_instanceId( 0 ), m_mapId( 0 ), m_mapMgr( NULL ), m_creatorGuid( 0 ), m_creatorGroup( Instance_group_type_none ),
		m_difficulty( 0 ), m_dungeonID(0), m_mapInfo( NULL )
	{
	}
	inline void InitData(uint64 creatorGUID, uint32 creatorGroup, uint32 difficulty, uint32 dungeonID)
	{
		m_creatorGuid = creatorGUID;
		m_creatorGroup = creatorGroup;
		m_difficulty = difficulty;
		m_dungeonID = dungeonID;
	}
	inline uint32 GetInstanceCreatorType()			// ���������ͣ���ң����ᣬ���飬ϵͳ��
	{
		return 0;
	}
	inline uint32 GetInstanceCreatorInfo()			// ������id�����id������id���ȡ���
	{
		return 0;
	}
	
	uint32 m_instanceId;
	uint32 m_mapId;
	MapMgr *m_mapMgr;
	uint64 m_creatorGuid;
	uint32 m_creatorGroup;
	uint32 m_difficulty;
	uint32 m_dungeonID;
	MapInfo *m_mapInfo;
};

typedef HM_NAMESPACE::hash_map<uint32, Instance*> InstanceMap;
typedef HM_NAMESPACE::hash_map<uint32, MapSpawn*> MapSpawnConfigMap;
/************************************************************************/
/*�����࣬����Ұ���ͼ�������ȶ����ڡ����硱��                          */
/************************************************************************/
class InstanceMgr : public Singleton<InstanceMgr>
{
	friend class MapMgr;   //Ϊ��ʹMapMgr�ܵ���InstanceMgr���һЩ˽�к���

public:
	InstanceMgr();	
	~InstanceMgr();

public:
	inline bool HandleSpecifiedScene(uint32 sceneID)
	{
		return m_selfServerHandleScenes.find(sceneID) == m_selfServerHandleScenes.end() ? false : true;
	}
	inline void AddHandleScene(const uint32 sceneID)
	{
		m_selfServerHandleScenes.insert(sceneID);
	}
	inline const uint32 GetDefaultBornMap()
	{
		return m_nServerDefaultBornMapID;
	}
	inline const float GetDefaultBornPosX()
	{
		return m_ptServerDefaultBornPos.m_fX;
	}
	inline const float GetDefaultBornPosY()
	{
		return m_ptServerDefaultBornPos.m_fY;
	}

public:
	void Initialize();		// ��ʼ��
	bool CanAddPlayerToSpecifiedWorld(Player *plr, uint32 targetMap, uint32 &targetInstance, uint32 targetDungeonData);
	uint32 PreTeleport(uint32 mapid, Player* plr, uint32 instanceid);   //���ͺ��������ͨ�������͡������ͼ(��������)
	
	MapMgr* GetNormalSceneMapMgr(Object* obj);
	MapMgr* GetInstanceMapMgr(uint32 MapId, uint32 InstanceId);

	Instance* GetInstance(uint32 mapID, uint32 instanceID);
	Instance* FindGuildInstance(uint32 mapID, uint32 guildID);
	Instance* FindSystemInstance(uint32 mapID);

	uint32 GenerateInstanceID();
	uint64 GenerateMonsterGuid();

	void LoadServerSceneConfigs();		// ��ȡ����������������
	void Load();						// �������е�ͼ(���³Ǹ���������)
	
	const uint32 GetHandledServerIndexBySceneID(const uint32 sceneID);
	// Has instance expired? Can player join?
	uint8 PlayerOwnsInstance(Instance * pInstance, Player* pPlayer);
	// update all mapmgrs
	void Update(uint32 diff);
	// delete all instances
	void Shutdown();

	MapMgr* GetMapMgr(uint32 mapId);
	InstanceMap* GetInstanceMap(uint32 mapId);
	InstanceMap* CreateInstanceMap(uint32 mapId);

	uint32 GetMapPlayerCount( uint32 mapid );
	Instance* CreateUndercity( uint32 mapid, Player *plr, uint32 dungeonID, uint32 difficulty );          // ��NPC�Ի�����ĸ�������Ե�һ��һ����
	MapMgr* CreateInstance(Instance *instance);
	Instance* newInstanceObject(uint32 mapId, MapInfo *mapInfo, uint32 instanceId, MapMgr *mgr = NULL);

private:
	void _SetServerDefaultBornConfig(uint32 mapID, float posX, float posY);
	void _CreateMap(uint32 mapid);       //������ͼ
	MapMgr* _CreateInstance(Instance * in);
	MapMgr* _CreateInstanceForSingleMap(uint32 mapid, uint32 instanceid);		// only used on main maps!
	bool _DeleteInstance(Instance * in);

	uint32 m_InstanceHigh;											// ��������ʵ����Ψһguid
	uint64 m_MonsterGuidHigh;										// ����Ψһguid
	Mutex m_monsterGuidLock;

	Mutex m_mapLock;
	MapSpawnConfigMap								m_mapSpawnConfigs;	// ��ͼ��������
	HM_NAMESPACE::hash_map<uint32, MapMgr*>			m_singleMaps;	// ��������ͨ�����糡����һ��mapidֻ��һ��mapmgr
	HM_NAMESPACE::hash_map<uint32, InstanceMap*>	m_instances;	// ��Ҵ����ĸ�����ϵͳ�ʵ���Ȼ��õ���ʵ��

	set<uint32> m_selfServerHandleScenes;						// ������������ĵ�ͼid
	map<uint32, set<uint32> > m_sceneControlConfig;				// ��ͼ��������<��Ϸ��id��set<����id> >

	uint32 m_nServerDefaultBornMapID;		// ��������Ĭ�ϵĵ�ͼid����ȫ�������Ĭ�Ͻ���ĵ�ͼid��
	FloatPoint m_ptServerDefaultBornPos;	// Ĭ�ϵĳ�����
};

#define sInstanceMgr InstanceMgr::GetSingleton()

#endif
