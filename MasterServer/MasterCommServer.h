#pragma once

#include "auth/Sha1.h"
#include "ServerCommonDef.h"

class CMasterCommServer : public TcpSocket
{
	friend class CCentreCommHandler;
public:
	CMasterCommServer(SOCKET fd, const sockaddr_in * peer);
	virtual ~CMasterCommServer(void);

public:
	inline uint32 GetGateServerId()
	{
		return gateserver_id;
	}

public:
	void OnDisconnect();
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }

	// ���ڷ��͵ĺ���
	void SendPacket(WorldPacket* data, uint32 serverOpcode, uint64 guid);	// �������ݵ����ͻ�����
	
	// ���ʹ洢�ڻ������ڵ�Packets
	void UpdateServerSendQueuePackets();
	// �������ڽ��ջ�������е�Packets
	void UpdateServerRecvQueuePackets();

	static void InitPacketHandlerTable();		// ��ʼ�����ݰ������
	void HandlePacket(WorldPacket *recvData);
	void HandlePing(WorldPacket &recvData);		// 
	void HandleSceneServerConnectionStatus(WorldPacket &recvData);
	void HandleAuthChallenge(WorldPacket &recvData);
	void HandleRequestRoleList(WorldPacket &recvData);
	// void HandleEnterSceneNotice(WorldPacket &recvData);
	void HandleChangeSceneServerNotice(WorldPacket &recvData);
	void HandlePlayerGameRequest(WorldPacket &recvData);
	void HandleCloseServantRequest(WorldPacket &recvData);
	void HandleSceneServerPlayerMsgEvent(WorldPacket &recvData);
	void HandleSceneServerSystemMsgEvent(WorldPacket &recvData);

	void DeletePlayerSocket(uint64 userGuid, uint32 sessionindex, bool direct);
	
public:
	FastQueue<WorldPacket*, Mutex> _recvQueue;		// �հ�����
	FastQueue<WorldPacket*, Mutex> _sendQueue;		// �������� 

private:
	// ����һЩ������Ϣ
	uint32 m_lastPingRecvUnixTime;
	bool removed;
	
	uint32 gateserver_id;
	uint32 authenticated;

	std::string registername;
	std::string registeradderss;

};

typedef void (CMasterCommServer::*gatepacket_handler)(WorldPacket &);