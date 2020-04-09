#pragma once

class CPlayerServant;
// class PlayerEntity;

class Park : public Singleton<Park>
{
public:
	Park(void);
	~Park(void);
public:
	inline void SetPlayerLimit(int limit) { m_nMaxPlayerLimit = limit; }
	inline int GetPlayerLimit() { return m_nMaxPlayerLimit; }
	inline uint32 GetPlatformID() { return m_PlatformID; }
	inline uint32 GetServerFullID() { return m_ServerGlobalID; }
	inline uint32 GetServerIndex() { return m_ServerIndex; }
	inline uint32 GetPassMinuteToday() { return m_pass_min_today; }
	inline void SetPassMinuteToday(uint32 data) { m_pass_min_today = data; }

public:
	void Update(uint32 diff);

	void Rehash(bool load = false);
	void Initialize();
	void SaveAllPlayers();
	void ShutdownClasses();

	// ���Է������
	void LoadLanguage();

	//ƽ̨�����
	void LoadPlatformNameFromDB();
	std::string GetPlatformNameById(uint32 platformId = 0);//Ĭ�ϻ�ȡ��ƽ̨����

	bool isValidablePlayerName(string& utf8_name);
	
    // �����������
	void LoadSceneMapControlConfig(bool reloadFlag = false);
	const uint32 GetHandledServerIndexBySceneID(uint32 sceneID);
	uint32 GetSceneIDOnPlayerLogin(uint32 targetMapID);					// �������ʱ��ѡһ�����ʵĳ�������������
	uint32 GetMinPlayerCounterServerIndex(uint32 defaultServerIndex);	// ��ȡ�������ٵĳ�������������
	bool IsValidationSceneServerIndex(uint32 serverIndex);				// �Ƿ�Ϊһ����Ч�ĳ���������
	
    // ��������������ͳ�Ƹ���
	void UpdatePlayerMapCounterData(uint32 serverIndex, WorldPacket &recvData);
	void GetSceneServerConnectionStatus(WorldPacket &recvData);

public:
	// lua�������
	static int getMongoDBName(lua_State *l);
	static int getServerGlobalID(lua_State *l);
	static int getCurUnixTime(lua_State *l);
	static int getSceneServerIDOnLogin(lua_State *l);

	LUA_EXPORT(Park)

public:
	uint32 flood_lines;
	uint32 flood_seconds;
	bool flood_message;

private:
    void _DeleteOldDatabaseLogs();

protected:
    uint32 m_PlatformID;
	uint32 m_ServerGlobalID;	// 
	uint32 m_ServerIndex;		// ���������(����)

	int m_nMaxPlayerLimit;			// �������ɵ������
    uint32 m_pass_min_today;        // ����ڼ����� 

	// ƽ̨����
	std::map<uint32, string> m_platformNames;
	// ��������
	std::map<uint32, set<uint32> > m_sceneControlConfigs;
	typedef std::map<uint32, int32> ScenePlayerCounterMap;
	// ��������������ͳ��
	std::map<uint32, ScenePlayerCounterMap> m_sceneServerMapPlayerCounter;		// map<����������,map<����id,��������>>
	// ����������ͳ��
	std::map<uint32, int32> m_sceneServerPlayerCounter;							// map<����������,�ó���������>
};

#define sPark Park::GetSingleton()
