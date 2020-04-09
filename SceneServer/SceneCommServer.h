#pragma once

#include "RC4Engine.h"
#include "ServerCommonDef.h"

enum ENUMMSGPRIORITY
{
	HIGH_PRIORITY = 1,
	LOW_PRIORITY
};

enum PLAYER_ENTER_GAME_RET 
{
	P_ENTER_RET_SUCCESS,					// �ɹ������Ự
	P_ENTER_RET_CREATE_SESSION_FAILED,		// �����Ự����ʧ��
	P_ENTER_RET_LOADDB_FAILED,				// ��ȡ����ʧ��
	P_ENTER_RET_DELAY_ADD,					// �ӳټ���
};

#define GATESOCKET_SENDBUF_SIZE 5485760
#define GATESOCKET_RECVBUF_SIZE 5485760

class CSceneCommServer : public TcpSocket
{
public:
	CSceneCommServer(SOCKET fd,const sockaddr_in * peer);
	~CSceneCommServer(void);

public:
	// Func
	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }

	void SendSessioningResultPacket(WorldPacket* pck, uint64 userGuid, ENUMMSGPRIORITY msgPriority = HIGH_PRIORITY);// ������ר���������ͻỰ��Ϣ
	//void DeletePlayerSocket(uint32 nPlayerAccountID);

	void HandlePacket(WorldPacket *recvData);	//���ݰ���������
	void HandlePing(WorldPacket &recvData);			
	
	void HandleGameServerConnectionStatus(WorldPacket &recvData);		// ����������״̬
	void HandleGameSessionPacket(WorldPacket* pRecvData);				// ������Ϸ�����е���Ϣ

	void HandleGatewayAuthReq(WorldPacket &recvData);				// ��������֤һ�����ط�����
	void HandleRequestEnterGame(WorldPacket &recvData);				// �����������Ϸ
	void HandleCloseSession(WorldPacket &recvData);					// gateserverҪ���Ƴ��Ի�
	// void HandleObjectDataSyncNotice(WorldPacket &recvData);			// ������������Ҫͬ���������ݵ�����
	void HandleServerCMD( WorldPacket & packet );					// �������������
	// void HandleMasterServerPlayerMsgEvent(WorldPacket &recvData);	// ��������������ͬ��������
	// void HandleMasterServerSystemMsgEvent(WorldPacket &recvData);	// ��������������ķ������Ϣ
	// void HandleSceneServerSystemMsgEventFromOtherScene(WorldPacket &recvData);	// �������������������ķ������Ϣ

	/************************************************/
	static void InitPacketHandlerTable();//��ʼ�����ݰ������
	/************************************************/

	//void HandleGlobalVar(WorldPacket &recvPacket);	//ȫ�ֱ�����أ���ǰ�������NPC��ȡ��Ʒ

	// ���ڷ��͵ĺ���
	void FASTCALL SendPacket(WorldPacket* data, uint32 nServerOPCode = 0 , uint64 userGuid = 0 );	// �������ݵ����ͻ�����

	// ���ʹ洢�ڻ������ڵ�Packets
	void UpdateServerSendQueuePackets();
	// ���͵����ȼ��ķ�������
	void UpdateServerLowSendQueuePackets();
	// �������ڽ��ջ�������е�Packets
	void UpdateServerRecvQueuePackets();
	// ����˼��������
	void ServerPacketProc(WorldPacket *recvPacket, uint32 *packetOpCounter);

public:
	// Variable
	uint32 m_lastPingRecvUnixTime;
	bool removed;
	uint32 associate_gateway_id;
	uint32 src_platform_id;			// Դƽ̨id
	uint32 authenticated;
	uint64 _lastSendLowLevelPacketMsTime;
	
	uint32 registerPort;
	std::string registername;
	std::string registeradderss;

public:
	FastQueue<WorldPacket*, Mutex> _recvQueue;					// �հ�����
	FastQueue<WorldPacket*, Mutex> _lowrecvQueue;				// �����ȴ������
	FastQueue<WorldPacket*, Mutex>	_highPrioritySendQueue;		// �����ȼ����Ͷ���
	FastQueue<WorldPacket*, Mutex>	_lowPrioritySendQueue;		// �����ȼ����Ͷ���

private:
	Mutex sendMutex;
};

typedef void (CSceneCommServer::*gamepacket_handler)(WorldPacket &);
