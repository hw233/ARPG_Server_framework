#pragma once

#include "ServerCommonDef.h"
#include "auth/Sha1.h"
#include "GameSocket.h"
#include "SceneCommClient.h"
#include "MasterCommClient.h"

struct tagGameSocketCacheData
{
	tagGameSocketCacheData() : plr_guid(0), gm_flag(0), relogin_valid_expire_time(0), session_index(0)
	{
	}
	uint64 plr_guid;
	uint32 gm_flag;
	uint32 session_index;
	uint32 relogin_valid_expire_time;
	string agent_name;
	string acc_name;
	string token_data;
};

typedef map<uint64, GameSocket*> GameSocketMap;

class CSceneCommHandler : public Singleton<CSceneCommHandler>
{
public:
	CSceneCommHandler(void);
	~CSceneCommHandler(void);

public:
	// Func
	CSceneCommClient* ConnectToSceneServer(string Address, uint32 Port, bool blockingConnect = true, int32 timeOutSec = 1);		// ʹ�����ط������׽���ȥ������Ϸ������
	CMasterCommClient* ConnectToCentreServer(string Address, uint32 Port);	// �������������

	void Startup();															// ����������������ȡ�ļ����ã���������Ϸ��������
	void OnGameServerConnectionModified();									// ���������ɹ����루��������+��֤�ɹ���
	void FillValidateGameServerInfo(WorldPacket &pck);						// �����Ч�ĳ�������Ϣ

	void ConnectionDropped(uint32 ID);										// ���ط�������ĳ�������Ͽ�����
	void CentreConnectionDropped();											// ���ط�����������������Ͽ�����
	void AdditionAck(uint32 ID, uint32 ServID);								// ��һ�����ط����������ע��
	void AdditionAckFromCentre(uint32 id);									// ���ط�����������ע��ɹ�
	
	//������Ϸ�������İ� add by tangquan 2011/11/11
	void UpdateSockets();													// ��ĳһ���ط�����ping����ʱ��ر���
	//���¿ͻ����������ݰ� add by tangquan 2011/11/11
	void UpdateGameSockets();												// ��ѯ���ÿ���ͻ����׽��֣������ʱ��֪ͨ��Ϸ���������ûỰ�ر�
	void UpdatePendingGameSockets();										// ����ÿ����������Ϸ��gamesocket
		
	void ConnectSceneServer(SceneServer * server);							// ��ĳ�����ط��������ӵ�����������
	void ConnectCentre(MasterServer *server);								// �ø����ط����ӵ����������
	
	void SendPacketToSceneServer(WorldPacket *data, uint32 uServerID);
	void SendPacketToMasterServer(WorldPacket *data);
	
	void LoadRealmConfiguration();											// ��ȡ�ļ����ã�������Ϸ�����������Լ����ط���������

	inline GameSocket* GetSocketByPlayerGUID(uint64 guid)					// ͨ����ɫguid�ҵ�һ���ͻ����׽���
	{
		GameSocket* sock = NULL;
		onlineLock.Acquire();
		GameSocketMap::iterator itr = online_players.find(guid);
		if (itr != online_players.end())
		{
			sock = itr->second;
		}
		onlineLock.Release();
		return sock;
	}

	inline uint32 GetOnlinePlayersCount()
	{ 
		onlineLock.Acquire();
		uint32 count = online_players.size(); 
		onlineLock.Release();
		return count;
	}
	
	inline uint32 GenerateSessionIndex()
	{
		uint32 uRet = 0;
		sessionIndexLock.Acquire();
		uRet = ++m_nSessionIndexHigh;
		sessionIndexLock.Release();
		return uRet;
	}


	inline void SetIPLimit(uint32 limit) { m_nIPPlayersLimit = limit; }

	void UpdateGameSocektGuid(uint64 tempGuid, uint64 realGuid, GameSocket *sock);		// ����GameSock��guid����
	// ��������ұ������һ���µ����
	void AddNewPlayerToSceneServer( GameSocket * playerSocket, ByteBuffer *enterData, uint32 targetSceneServerID );
	// ���ͻ�ѹ��
	void UpdateClientSendQueuePackets();
	// ����ͻ��˽��ջ���
	void UpdateClientRecvQueuePackets();
	// �ر����пͻ����׽��֣����ı� 9.10�޸ģ�����������Ϸ�������رգ�
	void CloseAllGameSockets(uint32 serverid = 0);
	//�ر����з���˶˿�
	void CloseAllServerSockets();
	//����ȫ����Ϣ
	void SendGlobaleMsg(WorldPacket *packet);
	// ����ȫ����Ϣ�����ֵ����
	void SendGlobalMsgToPartialPlayer(WorldPacket &rawPacket);

	CSceneCommClient* GetServerSocket(uint32 serverid);
	void SetGameServerOLPlayerNums(uint32 serverId, uint32 playerNum);
	void WriteGameServerOLNumsToPacket(WorldPacket &pack);

	// ���ѹ�������ݵ�ʹ��ͳ��
	void AddCompressedPacketFrenquency(uint16 opCode, uint32 length, uint32 compressed_length);
	void PrintCompressedPacketCount();

	bool OnIPConnect(const char* ip);
	void OnIPDisconnect(const char* ip);

	// �����������
	void RegisterReloginAccountTokenData(string agent_name, string &acc_name, uint64 guid, string &token_data, uint32 index, uint32 gm_flag);
	void RemoveReloginAccountData(string &acc_name);
	int32 CheckAccountReloginValidation(string &acc_name, string &recv_token);
	tagGameSocketCacheData* GetCachedGameSocketData(string &acc_name);

	// �ȴ���½�б�
	void AddPendingGameList(GameSocket *playerSocket, uint64 tempGuid);
	void RemoveDisconnectedSocketFromPendingGameList(uint64 guid);
	void SendCloseSessionNoticeToScene(uint32 scene_server_id, uint64 guid, uint32 session_index);
	void SendCloseServantNoticeToMaster(uint64 guid, uint32 session_index, bool direct);

public:
	SHA1Context m_sha1AuthKey;								// �������ط��ڳ���������֤ע��ʱ�õ���sha1��ϣֵ

	uint32 g_transpacketnum;
	uint32 g_transpacketsize;
	uint32 g_totalSendBytesToClient;
	uint32 g_totalRecvBytesFromClient;
	//uint32 m_clientPacketCounter[NUM_MSG_TYPES];

	// ���غ���Ϸ������������ͳ��
	uint32 g_totalSendDataToSceneServer;
	uint32 g_totalRecvDataFromSceneServer;
	uint32 m_sceneServerPacketCounter[SERVER_MSG_OPCODE_COUNT];
	// ���غ����������������ͳ��
	uint32 g_totalSendDataToMasterServer;
	uint32 g_totalRecvDataFromMasterServer;
	uint32 m_masterServerPacketCounter[SERVER_MSG_OPCODE_COUNT];
	void OutPutPacketLog();

private:
	std::vector<SceneServer*> m_SceneServerConfigs;				// ���泡�����������õļ���
	std::map<SceneServer*, CSceneCommClient*> m_sceneServers;	// �����볡��������ͨѶ���׽���ӳ���

	MasterServer* m_pMasterServerInfo;							// �������������������һЩ��Ϣ
	CMasterCommClient* m_pMasterServer;							// ����������������׽��ֱ�

	// ���ݽṹ�޸�Ϊ<ӳ�䣬AccountID>
	GameSocketMap	online_players;								// ����������ҵ��׽���ӳ���
	HM_NAMESPACE::hash_map<string, tagGameSocketCacheData> m_gamesock_login_token;	// ����������gamesocket�������ݣ������ص�½

	std::map<uint32, uint32>	m_ipcount;						// ͬip����������
	Mutex connectionLock;										// ���ӺͶϿ�����

	GameSocketMap	m_pendingGameSockets;					// ��֤�ɹ�׼����½��Ϸ��socket<��ɫguid��gamesocket����>

	Mutex mapLock;											// ��Ϸ������ͨѶ���׽���ӳ������
	Mutex onlineLock;										// ������ұ���
	Mutex reloginGameSocketLock;							// �����صǵ�socket��
	Mutex sessionIndexLock;									// sessionIndex��������
	Mutex pendingGameLock;									// �ȴ���½�����
	
	uint32 m_nSessionIndexHigh;								// sessionIndex
	uint32 m_nIPPlayersLimit;								// �����������������

	map<uint16, map<uint32, uint32> > m_compressPacketFrenc;// ѹ�����ݰ���ͳ�� 
	Mutex m_compressPackLock;

private:
	//ֻ����updategamesocket���̵߳���
	void __RemoveOnlineGameSocket(uint64 guid);												// ͨ���˻�ID�ҵ�һ���ͻ����׽��ֲ����Ƴ���
	void __AddGameSocketToOnlineList(uint64 guid, GameSocket *sock);						// ���һ��gamesock���������б�
	bool __CheckGameSocketOnlineAndDisconnect(const string &agent, const string &accName);	// ȷ��socket�Ƿ���ڣ����������Ͽ�

public:
	bool LoadStartParam(int argc, char* argv[]);
	void SetStartParm(const char* key,std::string val);
	const char* GetStartParm(const char* key);
	int GetStartParmInt(const char* key);

private:
	std::map<std::string, std::string> m_startParms;
};

#define sSceneCommHandler CSceneCommHandler::GetSingleton()
