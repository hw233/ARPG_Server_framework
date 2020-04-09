#pragma once

#include "auth/hohoCrypt.h"
#include "auth/FSM.h"

class WorldPacket;

enum CLIENTSTATUS
{
	eCSTATUS_AUTHFAILED = -2,
	eCSTATUS_ATTACK = -1,
	eCSTATUS_CONNECTED = 0,
	eCSTATUS_SEND_AUTHREQUEST = 1,
	eCSTATUS_SEND_ENTERGAME = 2,
	eCSTATUS_ENTERGAME_SUCESS = 3,
	eCSTATUS_SEND_CHARINFO = 4,
	eCSTATUS_SEND_CREATECHAR = 5,
};

enum GameSocketAuthStatus
{
	GSocket_auth_state_none				= 0x00,			// 
	GSocket_auth_state_sended_packet	= 0x01,			// �Ѿ���������֤��
	GSocket_auth_state_ret_succeed		= 0x02,			// �õ��˻Ự��֤���(��֤�ɹ�)
	GSocket_auth_state_ret_failed		= 0x04,			// �õ��˻Ự��֤���(��֤ʧ��)
};

class GameSocket : public TcpSocket
{
public:
	GameSocket(SOCKET fd, const sockaddr_in * peer);
	~GameSocket(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }

	// �԰�����ѹ���󷢳�
	bool _OutPacketCompress(uint32 opcode, uint32 packSize, uint32 errCode, const void* data);
	
	void SendPacket(WorldPacket * data);
	void OnConnect();
	void OnDisconnect();

	// ���´����͵�ʵ�ʵ����ݰ�
	void UpdateQueuedPackets();
	// �Ự��֤�ɹ�
	void SessionAuthedSuccessCallback(uint32 sessionIndex, string &platformAgent, string &accName, uint32 gmFlag, string existSessionToken);
	// �Ự��֤ʧ��
	void SessionAuthedFailCallback(uint32 errCode);
	// ֪ͨ�л��������
	void SendChangeSceneResult(uint32 errCode, uint32 tarMapID = 0, float dstPosX = 0.0f, float dstPosY = 0.0f, float dstHeight = 0.0f);

	// ���´����͵�ʵ�ʵ����ݰ�
	bool InPacketBacklogState();
	void TryToLeaveBackLogState();
	void TryToEnterBackLogState();
	
	inline uint64 GetPlayerGUID() { return m_PlayerGUID; }
	inline const char* szGetPlayerAccountName() { return m_strAccountName.c_str(); }
	inline uint32 GetPlayerLine() { return m_uSceneServerID; }
	inline string& GetPlayerIpAddr() { return mClientIpAddr; } 
	inline bool hadSendedAuthPacket() { return (m_AuthStatus & GSocket_auth_state_sended_packet) > 0 ? true : false; }
	inline bool hadSessionAuthSucceeded() { return (m_AuthStatus & GSocket_auth_state_ret_succeed) > 0 ? true : false; }
	inline bool hadGotSessionAuthResult() 
	{ 
		bool bRet = false;
		if ((m_AuthStatus & GSocket_auth_state_ret_failed) || (m_AuthStatus & GSocket_auth_state_ret_succeed))
			bRet = true;
		return bRet;
	}

public:
	static void InitTransPacket(void);
	static string GenerateGameSocketLoginToken(const string &agent, const string &accName);

	/* �����������ѡ��ɫ��������Ϸ֮ǰ,�ǲ��������guid��
	*  ���������һ����ʱ��guid��Ϊ��ֵ����gateway-master����ͨ��
	*  �ȵ�ѡ�ǳɹ�������Ϸ�󣬻�ʹ�ý�ɫguid��ȫ��Ψһ������Ϊ��ʽ��key
	*/
	static uint64 GeneratePlayerHashGUID(const string& agent, const string& accName);

public:
	uint64 m_lastPingRecvdMStime;		// �ϴν������Կͻ���ping��ʱ��
	uint32 m_SessionIndex;				// ����Ϸ������ͬ���ĶԻ�����
	string m_strClientAgent;			// �ͻ������Ե�������
	
	uint32 m_uSceneServerID;			// ����Ӧ��Ϸ�߼�������(�ڼ���)
	uint32 m_nChatBlockedExpireTime;	// ���Ե���ʱ��

	// ��֤���
	uint8  m_AuthStatus;				// socket��½����֤״̬
	uint32 m_nAuthRequestID;			// �ڱ����ؽ��̽��е�������־Ψһ�ĵ�½��֤����
	string m_strAuthSessionKey;			// �Ựkey����phpSocket���ɵġ������ڳ��ε�½��֤��
	string m_strLoginToken;				// ��½���ƣ����ڶ����ص�½��

	std::string m_strAccountName;		// ����˺���
	uint64 m_PlayerGUID;				// ��GameSocket��Ӧ�Ľ�ɫguid(�������ʽѡ�ǽ�����Ϸ��,��idΪ��ʽguid,����Ϊ����ͨ��hash�㷨���ɵ�һ��guid)
	uint32 m_GMFlag;					// gmֵ
	uint64 m_ReloginNextMsTime;			// �����ص�½��ʱ��

	//uint64 m_packrecvtime[NUM_MSG_TYPES];	// �ϴ��հ��¼�
	uint32 m_checkcount;					// �հ��������������һ����Ŀ��Ͽ�

	// static uint32 m_packetfrequency[NUM_MSG_TYPES];	// �ͻ����հ�Ƶ�ʿ���
	// static uint32 m_disconnectcount;					// �Ͽ���������
	// static uint8 m_transpacket[NUM_MSG_TYPES];		// �ͻ��˰�ת������
	// static uint8 m_packetforbid[NUM_MSG_TYPES];		// �����ݰ��Ƿ����Σ�������ʱ����һЩ����ģ��

	uint32 m_nSocketConnectedTime;		// socket�����ʱ��,����ͳ�������ʧ
	int32 mClientStatus;				// �ͻ���״̬
	
private:
	bool	m_beStuck;				// ������������
	bool	m_allowConnect;

	bool	m_inPckBacklogState;	// ���ڰ���ѹ״̬
	Mutex	m_backlogStateLock;		// ��ѹ״̬�ı����

	FastQueue<WorldPacket*, Mutex> _pckBacklogQueue;	// ����ѹ����

#ifdef USE_FSM
	CFSM	_sendFSM;
	CFSM	_receiveFSM;
#endif

	FastQueue<WorldPacket*, Mutex> _queue;//�����Ͷ���

	std::string mClientVersion;			// �ͻ��˰汾��
	std::string mClientLog;				// �ͻ���ʵʱ��־
	std::string mClientEnv;				// �ͻ��˻���
	std::string mClientIpAddr;			// �ͻ���id��ַ
	
private:
	void _HandleUserAuthSession(WorldPacket* recvPacket);	// ������֤�Ự
	void _HandlePing(WorldPacket* recvPacket);				// ����ͻ���ping
	void _HandlePlayerSpeek(WorldPacket *recvPacket);

	void _HookRecvedWorldPacket(WorldPacket *recvPacket);	// �����Ѿ����ܵ������ݰ�
	bool _HookSendWorldPacket(WorldPacket* recvPacket);     // ����׼�����ͳ�ȥ�����ݰ�
	void _CheckRevcFrenquency(uint32 packopcode);		//����հ�Ƶ��

	void _EnterBacklogState();
	void _LeaveBacklogState();
};
