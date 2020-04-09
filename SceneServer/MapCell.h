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


#ifndef __MAP_CELL_H
#define __MAP_CELL_H

class MapMgr;
class MapSpawn;
struct CellSpawns;

class MapCell
{
	friend class MapMgr;

public:
	MapCell();
	~MapCell();

	typedef hash_set<Object*> ObjectSet;

public:
	void Init(uint32 x, uint32 y, MapMgr* mapmgr);
	
	//Object Managing
	void AddObject(Object* obj); 
	void RemoveObject(Object* obj);
	void RemoveAllObjects(bool systemShutDown);

	bool HasObject(Object* obj) { return (_objects.find(obj) != _objects.end()); }
	bool HasPlayers() { return ((_playerCount > 0) ? true : false); }
	
	inline size_t GetObjectCount() { return _objects.size(); }
	inline uint32 GetPlayerCount() { return _playerCount; }

	uint16 GetPositionX() { return _cell_posX; }
	uint16 GetPositionY() { return _cell_posY; }

	inline ObjectSet::iterator Begin() { return _objects.begin(); }
	inline ObjectSet::iterator End() { return _objects.end(); }

	inline bool IsActive() { return _active; }
	inline bool IsLoaded() { return _loaded; }
	inline void SetPermanentActivity(bool val) { _forcedActive = val; }
	inline bool IsForcedActive() { return _forcedActive; }

	//State Related
	void SetActivity(bool state);

	//Object Loading Managing
	void LoadObjects(CellSpawns *sp);

	// �����Ƿ��Ծ��
	bool HaveActivityMonsters();
	bool HaveWaypointMovementCreatureNpc();

	// �����������
	ObjectSet _respawnObjects;

private:
	bool _forcedActive;				// ǿ�ƻ�Ծ��cell
	uint16 _cell_posX, _cell_posY;	// cell������

	ObjectSet _objects;				// cell�ϵĶ���
	bool _active;					// �Ƿ��Ծ��
	bool _loaded;					// �Ƿ��Ѷ�ȡ��

	uint16 _playerCount;			// cell�ϵ��������
	MapMgr* _mapmgr;
};

#endif
