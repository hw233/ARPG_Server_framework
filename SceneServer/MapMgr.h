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


#ifndef __MAPMGR_H
#define __MAPMGR_H

#include "MapInfo.h"
#include "MapCell.h"
#include "MapObjectStorage.h"
#include "CellHandler.h"

class MapCell;
class MapSpawn;
class Object;
class GameSession;
class Player;
class Monster;
class Instance;

enum MapMgrTimers
{
	MMUPDATE_OBJECTS		= 0,
	MMUPDATE_SESSIONS		= 1,
	MMUPDATE_FIELDS			= 2,
	MMUPDATE_IDLE_OBJECTS	= 3,
	MMUPDATE_ACTIVE_OBJECTS = 4,
	MMUPDATE_COUNT			= 5
};

enum ObjectActiveState
{
	OBJECT_STATE_NONE		= 0,
	OBJECT_STATE_INACTIVE	= 1,
	OBJECT_STATE_ACTIVE		= 2,
};

typedef hash_set<Object* > ObjectSet;
typedef hash_set<Object* > UpdateQueue;
typedef hash_set<Player*  > PUpdateQueue;
typedef hash_set<Player*  > PMoveInfoQueue;
typedef hash_set<Player*  > PCreationQueue;
typedef hash_set<Player*  > PlayerSet;
typedef hash_set<Monster* > MonsterSet;
typedef list<uint64> MonsterGuidList;
typedef list<Monster* > MonsterList;
typedef hash_map<uint64, Monster*> MonsterHashMap;

typedef HM_NAMESPACE::hash_map<uint64, Player*> PlayerStorageMap;

#define RESERVE_EXPAND_SIZE 1024

#define SCENEMAP_SYNC_TO_CENTRE_FRE_SEGMENT		5

/************************************************************************/
/* һ����ͼʵ����Ӧһ��MapMgr                                           */
/************************************************************************/
class MapMgr : public CellHandler <MapCell>, public EventableObject
{
	friend class MapCell;

public:
		
	//This will be done in regular way soon
	Mutex m_objectinsertlock;
	ObjectSet m_objectinsertpool;
	void AddObject(Object *obj);

	//////////////////////////////////////////////////////////
	// Local (mapmgr) storage of players for faster lookup
	////////////////////////////////
	CMapObjectStorage<Player> m_PlayerStorage;
	Player* GetPlayer(const uint64 guid);

	/////////////////////////////////////////////////////////
	// Local (mapmgr) storage/generation of monster
	/////////////////////////////////////////////
	CMapObjectStorage<Monster> m_MonsterStorage;
	Monster* CreateMonster(uint32 entry);
	Monster* GetMonsterByEntry(uint32 entry);
	Monster* GetMonster(const uint64 guid);
	inline const uint32 GetAddMonsterCount()
	{
		return m_nAddMonsterCount;
	}
	uint32 m_nAddMonsterCount;

	//////////////////////////////////////////////////////////
	// Lookup Wrappers
	///////////////////////////////////
	Object* GetObject(const uint64 & guid);

	MapMgr(MapSpawn *spawn, uint32 mapid, uint32 instanceid);
	~MapMgr();

	void Init();
	void Term(bool bShutDown);
	void Update(uint32 timeDiff);
	void RemoveAllObjects(bool bShutDown);
	void Destructor();

	//���object��������
	void PushObject(Object* obj);
	void RemoveObject(Object* obj, bool free_guid);

	void ChangeObjectLocation( Object* obj, bool same_map_teleport = false ); // update inrange lists

	//! Mark object as updated
	void ObjectUpdated(Object* obj);							// �ŵ�Ҫ�������ݵĶ�����
	void UpdateCellActivity(uint32 x, uint32 y, int radius);
	bool GetMapCellActivityState(uint32 posx, uint32 posy);

	// ������صĽӿڣ��������ͺ��Ƿ����߿��Կ�������������߶ȾͲ������˰�
	inline uint8 GetTerrianType(float x, float y) { return 0; }
	inline uint8 GetWalkableState(float x, float y) { return 0; }

	void AddForcedCell(MapCell * c);
	void RemoveForcedCell(MapCell * c);

	void PushToMoveProcessed(Player* plr);
	void PushToProcessed(Player* plr);				// �ŵ�Ҫ������µĶ�����
	void PushToCreationProcessed(Player *plr);		// ����Ҷ����Ʒ����������ݶ���

	// �Ƴ��������
	uint32 GetInstanceLastTurnBackTime(Player *plr);// ��ȡ��һص�����ʵ�������ʱ��
	void TeleportPlayers();

	//��ȡ�ӿ�
	inline bool HasPlayers() { return !m_PlayerStorage.empty(); }
	inline uint32 GetMapId() { return _mapId; }
	inline uint32 GetInstanceID() { return m_instanceID; }
	inline uint32 GetDungeonID() { return iInstanceDungeonID; }
	inline MapInfo *GetMapInfo() { return pMapInfo; }

	virtual int32 event_GetInstanceID() { return m_instanceID; }

	void LoadAllCells();

	//һ��������
	inline size_t GetPlayerCount() { return m_PlayerStorage.size(); }
	inline size_t GetMonsterCount() { return m_MonsterStorage.size(); }

    inline bool HasMapInfoFlag( uint32 flag )
    {
        if ( pMapInfo != NULL )
        {
            return pMapInfo->HasFlag( flag );
        }
        return false;
	}

	void _PerformObjectDuties();	//��������Ĺ�����update
	void _PerformUpdateMonsters(uint32 f_time);
	void _PerformUpdateMonstersForWin32(uint32 f_time, map<uint32, uint32> &update_times);
	void _PerformUpdateEventHolders(uint32 f_time);
	void _PerformUpdateFightPets(uint32 f_time);
	void _PerformUpdatePlayers(uint32 f_time);
	void _PerformUpdateGameSessions();
	void _PerformDropItemUpdate( uint32 diff );

	uint64 lastObjectUpdateMsTime;	// �����update��ʱ
	uint32 m_nNextDropItemUpdate;	// ������Ʒ��update��ʱ

	time_t InactiveMoveTime;		// �����Ĳ����ʱ���������ʱ��û������ȥ�ͻ�ж��
    uint32 iInstanceDifficulty;		// �����Ѷ�
	uint32 iInstanceDungeonID;		// ����id

	//����������ˢ��
	void EventRespawnMonster(Monster* m, MapCell * p);
	void ForceRespawnMonsterByEntry(uint32 monsterEntry);
	void ForceRespawnMonsterByGUID(uint64 monsterGUID);
	void ForceRespawnAllMonster();

	//������Ϣ���͵Ľӿ�
	void SendMessageToCellPlayers(Object* obj, WorldPacket * packet, uint32 cell_radius = 2);

	//ʵ����Ϣ��ָ��
	Instance * pInstance;

	// better hope to clear any references to us when calling this :P
	void InstanceShutdown()
	{
		pInstance = NULL;
	}

	static const uint32 ms_syncToCentreFreSegment[SCENEMAP_SYNC_TO_CENTRE_FRE_SEGMENT];

protected:
	//! Collect and send updates to clients
	void _UpdateObjects();
	// Sync Map Object data to master server
	void _UpdateSyncDataProcess();

private:
	//! Objects that exist on map
 	uint32 _mapId;//������id
	bool __isCellActive(uint32 x, uint32 y);							// ������������Ƿ��ǻ�Ծ��
	void __UpdateInRangeSet(Object* obj, Player* plObj, MapCell* cell);	// �����ڷ�Χ�ڵ�object����
	void __DeactiveMonsters();				// ȡ�������й���Ļ�Ծ״̬
	void __ActiveMonsters();				// ���ö����й���Ļ�Ծ״̬

private:
	/* Update System */
	Mutex m_updateMutex;				// use a user-mode mutex for extra speed
	UpdateQueue _updates;				// ��Ҫ�������ݵ�object����뵽�˶���
	PUpdateQueue _processQueue;			// ��ҵ����ݻ������Ｏ��һ���ͣ���������������˼
	PMoveInfoQueue _moveInfoQueue;		// ����Щ����Ҫ��֪ͨ������object�ƶ�
	PCreationQueue _creationQueue;		// �������ݵĶ��У�λ�ڼ����е���һ���update�����б����ʹ�����ϢЭ�����
	uint32 m_processFre;	     

	int32 m_syncToMasterFre;						// �������ͬ�����ݵ�Ƶ��
	map<uint32, PUpdateQueue > _syncToMasterQueue;	// ��Ҫ�������ͬ����Ϣ����Ҽ���map<ƽ̨id,��Ҷ���>

	/* Sessions */
	SessionSet MapSessions;				// ��ҵ�session�б���ҽ����ͼ��session��update���������ͼ��update����ִ����

	/* Map Information */
	MapInfo *pMapInfo;				// ��ͼ�ļ�Ҫ��Ϣ
	uint32 m_instanceID;			// �˵�ͼ��ʵ��id����ͨ������Ψһ��ʵ�����񸱱�֮�࣬���Ҫ�������ʵ����������������

	PlayerStorageMap::iterator __player_iterator;

public:
	EventableObjectHolder eventHolder;

	MonsterHashMap activeMonsters;		// ��Ծ�����б�
	MonsterGuidList pendingActiveMonsterGuids;		// �ȴ���ӻ�Ծ�Ĺ���id�б�
	MonsterGuidList pendingDeactiveMonsterGuids;	// �ȴ�ȡ����Ծ�Ĺ���id�б�

public:
	void SendPacketToPlayers( WorldPacket * pData, uint64 ignoreGuid = 0 );

public:
	PlayerStorageMap::iterator GetPlayerStorageMapBegin()
	{
		return m_PlayerStorage.begin();
	}
	PlayerStorageMap::iterator GetPlayerStorageMapEnd()
	{
		return m_PlayerStorage.end();
	}
};

#endif
