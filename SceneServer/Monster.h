#ifndef _MONSTER_H_
#define _MONSTER_H_

#include "Object.h"
#include "MonsterDef.h"

class MapMgr;
class MapCell;
struct tagMonsterBornConfig;

class Monster : public Object
{
public:
	Monster(void);
	virtual ~Monster(void);

public:
	inline tagMonsterProto* GetProto()	{ return m_protoConfig; }
	inline void SetProto(tagMonsterProto *proto) { m_protoConfig = proto; }
	inline tagMonsterSpwanData* GetSpawnData() { return m_spawnData; }
	inline const uint8 GetAIState() { return m_AIState; }

public:
	void Initialize(uint64 guid);		// ��pool�г�ʼ��
	void Terminate();					// ɾ����pool

	virtual void gcToMonsterPool();
	virtual void Update( uint32 time );

	virtual void OnPushToWorld();
	virtual void OnPreRemoveFromWorld(MapMgr *mgr);
	virtual const float GetMoveSpeed();

public:
	bool Load( tagMonsterBornConfig *bornConfig, MapMgr *bornMapMgr );
	void Load( tagMonsterProto *proto_, float x, float y, float direction, MapMgr *bornMapMgr );
	void RemoveFromWorld(bool addrespawnevent, bool free_guid);
	void OnRemoveCorpse();
	void OnRespawn(MapMgr* m);
	void Despawn(uint32 delay, uint32 respawntime);
	void SafeDelete();
	void DeleteMe();//����ɾ��������safedelete�Ļص�

public:
	virtual void _SetCreateBits(UpdateMask *updateMask, Player* target) const;
	virtual void _SetUpdateBits(UpdateMask *updateMask, Player* target) const;
	// static
	static void InitVisibleUpdateBits();
	static UpdateMask m_visibleUpdateMask;

	// ��AI�йصĲ���
	Player* FindAttackablePlayerTarget(uint32 alertRange);

public:
	static int getMonsterObject(lua_State *l);

	int getBornPos(lua_State *l);
	int onMonsterJustDied(lua_State *l);
	int findAttackableTarget(lua_State *l);
	int isMovablePos(lua_State *l);
	int setAIState(lua_State *l);

	// ������lua
	LUA_EXPORT(Monster)

public:
	MapCell *m_respawnCell;				// ������cell

private:
	bool m_corpseEvent;					// �����¼��Ĵ�����
	uint8 m_AIState;					// ai״̬(����,ս��,׷����)
	
	tagMonsterProto		*m_protoConfig;
	tagMonsterSpwanData *m_spawnData;

	uint64 _fields[UNIT_FIELD_END];
};

#endif
