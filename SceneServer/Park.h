/****************************************************************************
 *
 * Main World System
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

#ifndef __PARK_H
#define __PARK_H

#include "threading/RWLock.h"

class CSceneCommServer;
class WorldPacket;
class GameSession;
class Player;
class EventableObjectHolder;

// Slow for remove in middle, oh well, wont get done much.
typedef std::set<GameSession*> SessionSet;
struct tagQueueSessionInfo
{
	tagQueueSessionInfo(GameSession *s, ByteBuffer *data) : session(s), plrData(data) 
	{
	}
	GameSession *session;
	ByteBuffer *plrData;
};

class Park : public Singleton<Park>//, public EventableObject
{
public:
	Park();
	~Park();

	/** Reloads the config and sets all of the setting variables 
	 */
	void Rehash(bool load);

	GameSession* FindSession(uint64 plrGuid);
	void CreateSession(CSceneCommServer *pSock, WorldPacket &recvData);		// ����һ���Ự������player����Ϣ��
	void AddSession(uint64 plrGuid, GameSession *s);

	void AddGlobalSession(GameSession *session);
	//Session��ɫ��¼��UpdateSessions����ɾ������ɵ�¼�Ự����
	//����ɫ�˳�ʱ��m_sessions�Żᱻɾ����
	void RemoveGlobalSession(GameSession *session);
	void DeleteSession(uint64 plrGuid, GameSession *session, bool gcToPool = true);

	void OnSocketClose(CSceneCommServer* sock);

	uint32 GetSessionCount();
	inline uint32 GetPlayerLimit() const { return m_playerLimit; }
	void SetPlayerLimit(uint32 limit) { m_playerLimit = limit; }

	inline time_t GetGameTime() const { return m_gameTime; }

	void SetInitialWorldSettings();
	void DailyEvent();

	void SendGlobalMessage( WorldPacket *packet );
	void SendSystemBoardCastMessage(const string &strMessage, const uint32 location, const uint32 repeatCount, uint32 interval, bool castAtOnce);
	void SendChannelMessage(uint32 channel, uint32 guid, const string &msg, uint32 minLevel = 0, uint32 maxLevel = 0xFFFFFFFF);		//���������Ƶ��������Ϣ
	void SendPacketToChannel(uint32 channel, uint32 guid, ByteBuffer &packet, uint32 minLevel = 0, uint32 maxLevel = 0xFFFFFFFF);

	// update the world server every frame
	void Update(time_t diff);
	void UpdateSessions(uint32 diff);
	void SyncPlayerCounterDataToMaster(uint32 diff);
	void SceneServerChangeProc(GameSession *session, uint32 targetSceneID, const FloatPoint &targetPos, uint32 ret, uint32 destSceneServerID = 0xffffffff);

	Mutex queueMutex;
	void SaveAllPlayers();
	void PrintAllSessionStatusOnProgramExit();
	void ShutdownClasses();

	// ����Ự�Ĺ��̣������ݰ����͵���Ӧ��Session��
	void SessioningProc(WorldPacket *recvData, uint64 plrGuid);
	void InvalidSession(uint64 plrGuid, uint32 sessionIndex, bool ignoreindex = false);				//ʹ��ҵ�sessionʧЧ��Ȼ�����update���Ƴ�

	uint32 flood_lines;
	uint32 flood_seconds;
	bool flood_message;

	void PrintDebugLog();
	void LoadLanguage();
	
	// ��ȡ������id(1,2,3,4,5,��)
    inline uint32 GetServerID()
    {
        return m_ServerGlobalID;
    }
    inline void SetServerID( uint32 serverid )
    {
        m_ServerGlobalID = serverid;
        m_PlatformID = m_ServerGlobalID / 10000;
    }
    inline uint32 GetPassMinuteToday()
    {
        return m_pass_min_today;
    }
    inline void SetPassMinuteToday(uint32 data)
    {
        m_pass_min_today = data;
    }

	//ƽ̨�����
	void LoadPlatformNameFromDB();
	std::string GetPlatformNameById(uint32 platformId = 0); //Ĭ�ϻ�ȡ����ƽ̨����

	// ��ͼ����ͳ�����
	void OnPlayerEnterMap(uint32 mapID);
	void OnPlayerLeaveMap(uint32 mapID);

public:
	// lua�������
	static int getServerGlobalID(lua_State *l);
	static int getSelfSceneServerID(lua_State *l);
	static int getCurUnixTime(lua_State *l);
	static int getSessionByGuid(lua_State *l);
	static int setPlayerVisibleUpdateMaskBits(lua_State *l);
	static int setMonsterVisibleUpdateMaskBits(lua_State *l);
	static int setPartnerVisibleUpdateMaskBits(lua_State *l);
	static int trySendPacket(lua_State *l);

	LUA_EXPORT(Park)

protected:
	// update Stuff : use diff
	time_t _UpdateGameTime()
	{
		// Update Server time
		time_t thisTime = time(NULL);
		m_gameTime += thisTime - m_lastTick;
		m_lastTick = thisTime;
		return m_gameTime;
	}

private:
	EventableObjectHolder *eventholder;
	typedef HM_NAMESPACE::hash_map<uint64, GameSession*> SessionMap;
	SessionMap m_sessions;			// map<��ɫguid, session����>
    RWLock m_sessionlock;

    uint32 m_pass_min_today;        // ����ڼ�����

public:
	// �ȴ����뵽���¶��еĶԻ���Ŀǰ��Ҫ�����ظ���¼�����
	Mutex _queueSessionMutex;
	map<uint64, tagQueueSessionInfo> _queueSessions;

protected:
    uint32		m_PlatformID;
    uint32		m_ServerGlobalID;
	Mutex		m_globalSessionMutex;//FOR GLOBAL !
	SessionSet	m_globalSessions;

	uint32 m_playerLimit;
	time_t m_gameTime;
	time_t m_lastTick;

	uint32 m_ClassUpdateTime[15];
	uint32 m_ClassUpdateMaxTime[15];
	uint32 m_TotalUpdate;
	uint64 _CheckStartMsTime;
	
	void _CheckUpdateStart();
	void _CheckUpdateTime(uint32 classenum);

	//ƽ̨����
	std::map<uint32, string> m_platformNames;

	// �������ͬ��������������ݽṹ��
	uint32 m_mapPlayerCountSyncTimer;					// ÿ30�����һ��ͬ��
	std::map<uint32, int32> m_mapPlayerCountModifier;	// map<��ͼid,�仯��>
};

#define sPark Park::GetSingleton()

#endif
