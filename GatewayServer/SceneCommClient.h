#pragma once

#include "ServerCommonDef.h"

class CSceneCommClient : public TcpSocket
{
public:
	CSceneCommClient(SOCKET fd,const sockaddr_in * peer);
	~CSceneCommClient(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	virtual void OnDisconnect();					// �����ӱ��ر�ʱ

	void SendPing();								// ����Ϸ����������ping������
	void SendChallenge(uint32 serverid);			// ��������ʱ������Ϸ������������֤������
	
	static void InitPacketHandlerTable();			// ��ʼ�����ݰ������

	// ��Ϣ������
	void HandleSceneServerPacket(WorldPacket *pPacket);				// �������ݣ���������Handle��ͷ�ķ���
	
	void HandleAuthResponse(WorldPacket *pPacket);					// �õ���֤���
	void HandlePong(WorldPacket *pPacket);							// �õ�ping�Ľ��
	void HandleGameSessionPacketResult(WorldPacket *pPacket);		// �õ���Ϸ���������ص���Ϣ��������Ϣת������Ӧ�Ŀͻ����׽��֣�
	void HandleEnterGameResult(WorldPacket *pPacket);
	void HandleChangeSceneServerNotice(WorldPacket *pPacket);		// ��Ϸ��֪ͨĳ�����Ҫ�л�����

	void HandleSavePlayerData(WorldPacket *pPacket);				// ������Ϸ���������ı��������Ϣ������Ҫ�Ǵ������߱���
	void HandleSendGlobalMsg(WorldPacket *pPacket);					// gameserverҪ����ȫ����Ϣ
	void HandleTransferPacket(WorldPacket *pPacket);				// ������Ϸ������ת���İ�

	// ���ڷ��͵ĺ���
	void SendPacket(WorldPacket *data);								// �������ݵ����ͻ�����
	
	// �����ڷ��ͻ�����е�Packets����
	void UpdateServerSendQueuePackets();
	// �����ڽ��ջ�����е�Packets
	void UpdateServerRecvQueuePackets();
	
public:
	uint32 m_lastPingUnixtime;						
	uint32 m_lastPongUnixtime;

	uint64 ping_ms_time;						
	uint32 latency_ms;						// �ӳ�
	uint32 _id;								// ���˿����Ӷ�Ӧsceneserver��id������ʶ��ͬ��scenerserver

	uint32 authenticated;					// �Ƿ�ͨ����֤
	bool m_registered;

public:
	FastQueue<WorldPacket*, DummyLock>	_sendQueue;		// �����Ͷ���
	Mutex								_sendQueueLock;	// ������

	FastQueue<WorldPacket*, Mutex>		_recvQueue;
	FastQueue<WorldPacket*, Mutex>		_busyQueue;//����ʱ����Ҫ�ܳ������ݰ�

};

typedef void (CSceneCommClient::*scenepacket_handler)(WorldPacket *);

