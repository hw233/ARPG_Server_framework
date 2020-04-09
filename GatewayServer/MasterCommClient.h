#pragma once

#include <RC4Engine.h>

class CMasterCommClient : public TcpSocket
{
public:
	CMasterCommClient(SOCKET fd, const sockaddr_in *peer);
	~CMasterCommClient(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	virtual void OnDisconnect();					// �����ӱ��ر�ʱ

	// ���ڷ��͵ĺ���
	void FASTCALL SendPacket(WorldPacket *data);	// �������ݵ����ͻ�����

	// �����ڷ��ͻ�����е�Packets����
	void UpdateServerSendQueuePackets();
	// �����ڽ��ջ�����е�Packets
	void UpdateServerRecvQueuePackets();

	void SendPing();
	void SendChallenge(uint32 serverId);
	static void InitPacketHandlerTable();		// ��ʼ�����ݰ������б�

	//��Ϊ�����������handle�Ĵ�����
	void HandleCentrePakcets(WorldPacket *packet);

	void HandlePong(WorldPacket *packet);
	void HandleAuthResponse(WorldPacket *packet);
	void HandleCloseGameSocket(WorldPacket *packet);				// �ر�һ��gamesocket
	void HandleChooseGameRoleResponse(WorldPacket *packet);			// ѡ�ǽ�����Ϸ�ķ���
	void HandlePlayerGameResponse(WorldPacket *packet);
	void HandleChangeSceneServerMasterNotice(WorldPacket *packet);	// �����֪ͨ���ؽ���Ŀ�곡���л�
	void HandleEnterInstanceNotice(WorldPacket *packet);			// ������븱����ϵͳʵ��
	void HandleExitInstanceNotice(WorldPacket *packet);				// �˳�������ϵͳʵ��
	void HandleSendGlobalMsg(WorldPacket *packet);
	void HandleSendPartialGlobalMsg(WorldPacket *packet);			// ����ȫ����Ϣ�����ֵ�socket
	void HandleTranferPacket( WorldPacket * packet );				// ֱ��ת������Ӧ��Ϸ������
	void HandleBlockChat( WorldPacket * packet );					// ���Դ��� by eeeo@2011.4.27
	void HandleServerCMD( WorldPacket * packet );					// �������������

public:
	uint32 m_lastPingUnixTime;
	uint32 m_lastPongUnixTime;
	uint64 ping_ms_time;
	uint32 latency_ms;			// �ӳ�
	uint32 authenticated;
	uint32 m_nThisGateId;	// �����ط����������������ע��õ���id

	//�Ѿ����÷��Ͷ��еģ�����ֱ�Ӹ�GameCommHandler���� by tangquan
	FastQueue<WorldPacket*, DummyLock> _sendQueue;		// �����Ͷ���
	Mutex							_sendQueueLock;		// ������
	FastQueue<WorldPacket*, Mutex> _recvQueue;
	FastQueue<WorldPacket*, Mutex> _busyQueue;//����ʱ����Ҫ�ܳ������ݰ����Ѿ�������by tangquan��
};

typedef void (CMasterCommClient::*masterpacket_handler)(WorldPacket *);
