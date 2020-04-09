#pragma once

#include "MasterCommServer.h"

class CMasterCommServer;
class CPlayerServant;

struct OpcodeHandler
{
	uint16 status;
	void (CPlayerServant::*handler)(WorldPacket &recvPacket);
};

enum ServantStatus
{
	STATUS_NONE					= 0x00,		// ��
	STATUS_LOGIN				= 0x01,		// �ѵ�¼
	STATUS_GAMEING				= 0x02,		// ����Ϸ��
	STATUS_SERVER_SWITTHING		= 0x04,		// ������л�
};

enum RoleCreateError
{
	CREATE_ERROR_NONE,
	CREATE_ERROR_NAME			= 100801,
	CREATE_ERROR_LIMIT			= 100802,
	CREATE_ERROR_EXIST			= 100803,
	CREATE_ERROR_NAME_LEN		= 100804,
	CREATE_ERROR_CAREER			= 100805,
};

enum RoleLoginError
{
	LOGINERROR_NONE,
	LOGINERROR_ROLE,
	LOGINERROR_BANNED,
	LOGINERROR_TIMELIMIT,
};

struct tagSingleRoleInfo
{
	tagSingleRoleInfo() : m_nPlayerID(0), 
		m_strPlayerName(""), 
		m_nCareer(-1),
		m_nLevel(1)
	{

	}
	tagSingleRoleInfo(uint64 plrGuid, const char* plrName, uint32 level, int career, uint32 banned) : 
		m_nPlayerID(plrGuid),
		m_strPlayerName(plrName),
		m_nLevel(level),
		m_nCareer(career),
		m_nBanExpiredTime(banned)
	{
	}

	void Clear()
	{
		m_nPlayerID = 0;
		m_strPlayerName = "";
		m_nLevel = 0;
		m_nCareer = -1;
		m_nBanExpiredTime = 0;
	}

	uint64 m_nPlayerID;
	std::string m_strPlayerName;
	uint32 m_nLevel;
	int m_nCareer;				// ְҵ
	uint32 m_nBanExpiredTime;
};

#define MAX_ROLE_PER_ACCOUNT	1

struct tagRoleList			// �˻�ӵ�����н�ɫ��Ϣ
{
	tagRoleList() : m_strAccountName(""), m_strAgentName(""), m_nPlayerGuid(0)
	{
		m_vecRoles.clear();
	}
	void Clear()
	{
		m_strAccountName = "";
		m_strAgentName = "";
		m_nPlayerGuid = 0;
		m_vecRoles.clear();
	}

	const uint32 GetRolesNum() { return m_vecRoles.size(); }

	tagSingleRoleInfo* GetSingleRoleInfoByGuid(uint64 plrGuid)
	{
		tagSingleRoleInfo *ret = NULL;
		for (vector<tagSingleRoleInfo>::iterator it = m_vecRoles.begin(); it != m_vecRoles.end(); ++it)
		{
			if (it->m_nPlayerID == plrGuid)
			{
				ret = &(*it);
				break ;
			}
		}
		return ret;
	}

	void AddSingleRoleInfo(tagSingleRoleInfo &roleInfo)
	{
		m_vecRoles.push_back(roleInfo);
	}

	string m_strAccountName;				// �˻���
	string m_strAgentName;					// ������
	uint64 m_nPlayerGuid;					// ���guid�������δѡ���ɫ����ʱ����guidΪ���ظ���ƽ̨/�˺����ɵ�һ����ϣֵ�����ѡ���ɫ������Ϸ�󣬸�ֵΪ��ɫ��guid��
	std::vector<tagSingleRoleInfo> m_vecRoles;	// �����ɫ��Ϣ������
};

class ServantCreateInfo;

class CPlayerServant
{
	friend class CPlayerMgr;
public:
	CPlayerServant(void);
	~CPlayerServant(void);

	void Init( ServantCreateInfo * info, uint32 sessionIndex );
	void Term();
	void DisConnect();

	void FillRoleListInfo();		// ����ɫ�б���Ϣ
	void WriteRoleListDataToBuffer(pbc_wmessage *msg);
	void LogoutServant(bool saveData);						// ������ߣ��ǳ������棩
	void ResetCharacterInfoBeforeTransToNextSceneServer();	// ���͵���һ��������ǰҪ������

public:
	inline const uint32 GetGMFlag() { return m_nGmFlag; }
	inline bool IsBeDeleted() { return m_bDeleted; }
	inline bool IsWaitLastData(){ return m_deleteTime>0;}
	inline void SetWaitLastData(){ if(m_deleteTime==0) m_deleteTime=UNIXTIME;}
	inline bool HasPlayer() { return m_bHasPlayer; }
	inline const uint32 GetCurSceneServerID() { return m_nCurSceneServerID; }
	inline const time_t GetLastSetServerIndexTime() { return m_lastTimeSetServerIndex; }
    inline tagRoleList& GetPlayerRoleList() { return m_RoleList; }
	inline const uint64 GetCurPlayerGUID() { return m_RoleList.m_nPlayerGuid; }
	inline void ResetCurPlayerGUID(uint64 realGuid) { m_RoleList.m_nPlayerGuid = realGuid; }
	inline std::string strGetAccountName() { return m_RoleList.m_strAccountName; }
	inline const char* szGetAccoundName() { return m_RoleList.m_strAccountName.c_str(); }
	inline std::string strGetAgentName() { return m_RoleList.m_strAgentName; }
	inline const char* szGetAgentName() { return m_RoleList.m_strAgentName.c_str(); }
	inline bool IsInGameStatus(uint32 status) { return (m_nGameState & status) > 0; }

	inline void QueuePacket(WorldPacket *packet) { _recvQueue.Push(packet); }
	inline void SetCommSocketPtr(CMasterCommServer *pSock) { m_pServerSocket = pSock; }
	inline CMasterCommServer* GetServerSocket() { return m_pServerSocket; }

	inline bool HasGMFlag() { return m_gm; }

public:
	int Update(uint32 p_time);
	bool SetSceneServerID(uint32 curSceneServerIndex, const uint32 setIndexTimer, bool forceSet);
	void OnPlayerChangeSceneServer(uint32 changeIndexTimer);
	void ResetSyncObjectDatas();

	void SendServerPacket(WorldPacket *packet);//Ϊ�˷�ֹ����
	void SendResultPacket(WorldPacket *packet);
	
public:
	// ������Lua�õĺ���
	int getAccountName(lua_State *l);
	int getAgentName(lua_State *l);
	int getRoleCount(lua_State *l);
	int getSessionIndex(lua_State *l);
	int getCurSceneServerID(lua_State *l);
	int addNewRole(lua_State *l);
	int setRoleListInfo(lua_State *l);
	int onEnterSceneNotice(lua_State *l);

	int sendServerPacket(lua_State *l);
	int sendResultPacket(lua_State *l);

	LUA_EXPORT(CPlayerServant)

public:
	bool m_bHasPlayer;						// �Ƿ���player����Ϸ�У�������Ϸǰ���л���ͼ������ʱ�˱�Ƕ�Ϊfalse
	uint32 m_nCurSceneServerID;				// ��ǰ���ڵĳ����������������κγ�����ʱ��������Ϸǰ���л�������ʱ��Ϊ0��
	time_t m_lastTimeSetServerIndex;		// ����ϴ����ó�����������ʱ��

	uint32 m_nGameState;					// ��Ϸ״̬���ޣ��ѵ�¼����Ϸ�У�
	uint64 m_nReloginStartTime;				// �ظ���½��ʱ�䣨��ͬ���ط��������ظ���½���󣬱�����������֮������ٴν�����½��

	bool m_bVerified;						// �Ƿ��Ѿ����й���֤
	bool m_bPass;							// �Ƿ���ͨ�������֤
	bool m_bLogoutFlag;						// �ǳ����

	uint32 m_nGmFlag;
	uint32 m_nSessionIndex;					// �Ựid�����롢���ء���Ϸ����һ�£�

	// ��������ͬ��
	ByteBuffer m_syncObjectDataBuffer;		// ������
	uint32 m_nSyncDataObjectCount;			// ����
	uint32 m_nSyncDataLoopCounter;			// ѭ��������������ÿ��update��ͬ�����ݣ�

	bool	m_newPlayer;					// ֻ���½�ɫ����Ҫ������ʧ��־

protected:
	bool m_bDeleted;						// �Ƿ�ɾ������
	uint32 m_deleteTime;					// ɾ����ʱ���������ȴ���Ϸ���������߰��ĳ�ʱ���
    bool   m_gm;                            // �ڲ���Ա��ʶ

	// ������հ�
	FastQueue<WorldPacket*, Mutex> _recvQueue;	// ������ҷ�����Ҫ����������������Ϣ�������������
	Mutex deleteMutex;
	
	CMasterCommServer *m_pServerSocket;
	tagRoleList m_RoleList;
};
