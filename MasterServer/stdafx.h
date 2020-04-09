#pragma once

#ifndef MASTER_SERVER
#define MASTER_SERVER
#endif

#include "CommonTypes.h"
#include "MersenneTwister.h"
#include "log/NGLog.h"
#include "log/Log.h"
#include "log/TextLogger.h"
#include "network/Network.h"
#include "threading/Timer.h"
#include "threading/UserThreadMgr.h"
#include "database/mysql/DatabaseEnv.h"
#include "database/mongodb/MongoConnector.h"
#include "config/ConfigEnv.h"
#include "config/JsonFileReader.h"
#include "script/ScriptParamArray.h"
#include "database/mongodb/MongoConnector.h"
#include "database/mongodb/MongoTable.h"

#include "Singleton.h"
#include "BufferPool.h"
#include "WorldPacket.h"
#include "LanguageMgr.h"
#include "MasterScript.h"
#include "PbcDataRegister.h"

#include "PlayerCommonDef.h"
#include "BaseObject.h"
#include "SynchronizedObject.h"
#include "GameCommonDefine.h"
#include "DBConfigLoader.h"
#include "PbDataLoader.h"
#include "SceneMapMgr.h"

#include "Park.h"
#include "PlayerMgr.h"
#include "ObjectMgr.h"
#include "PlayerServant.h"
#include "MasterLuaTimer.h"

extern int g_serverDaylightBias;	// �ĸ�sb�����巢����ʱ�Ƶģ�
extern int g_serverTimeOffset;		// ʱ��

extern const uint64 ILLEGAL_DATA_64;
extern const uint32 ILLEGAL_DATA_32;

extern MongoConnector* MongoDB_Game;
#define MongoGameDatabase (*(MongoDB_Game->GetConnection()))

//extern MySQLDatabase * Database_Game;
//#define GameDatabase (*Database_Game)

//extern MySQLDatabase *Database_World;
//#define WorldDatabase (*Database_World)

extern MySQLDatabase * Database_Log;
#define LogDatabase (*Database_Log)

extern MasterScript* g_pMasterScript;
#define LuaScript (*g_pMasterScript)

// ����json�������ļ���ȡ·��
string strMakeJsonConfigFullPath(const char* fileName);
// ����lua�������ļ���ȡ·��
string strMakeLuaConfigFullPath(const char* fileName);
