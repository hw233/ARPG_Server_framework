#pragma once

#include "auth/Sha1.h"
#include "ServerCommonDef.h"

class CMasterCommServer;
class WorldPacket;

class CCentreCommHandler : public Singleton<CCentreCommHandler>
{
public:
	CCentreCommHandler(void);
	~CCentreCommHandler(void);

public:
	inline Mutex& GetServerLocks()
	{
		return m_serverSocketLocks;
	}

public:
	uint32 GenerateGateServerID();
	void AddServerSocket(CMasterCommServer *pServerSocket);
	void RemoveServerSocket(CMasterCommServer *pServerSocket);
	void TimeoutSockets();												// ��ⳬʱ
	bool TrySendPacket(WorldPacket *packet, uint32 serverOpCode);		// ���Է���һ�����ݰ�
	void SendMsgToGateServers(WorldPacket *pack, uint32 serverOpcode);	// ����Ϣ�����������ط�����

	void UpdateServerSendPackets();
	void UpdateServerRecvPackets();

	uint32 m_totalsenddata;			// �ܷ���������
	uint32 m_totalreceivedata;		// �ܽ���������
	uint32 m_totalSendPckCount;		// �ܷ�����

	uint32 m_sendDataByteCountPer5min;	// 5���ӵķ���������
	uint32 m_recvDataByteCountPer5min;	// 5���ӵĽ���������
	uint32 m_sendPacketCountPer5min;	// 5���ӵķ�������

	SHA1Context m_gatewayAuthSha1Data;	// gateway����������֤��sha1����
	std::string m_phpLoginKey;			// ƽ̨��½key

private:
	set<CMasterCommServer*> m_serverSockets;		// ����������ӵ�����������׽���
	Mutex m_serverSocketLocks;						// 
	uint32 m_nGateServerIdHigh;						// ���
	Mutex m_serverIdLock;
};

#define sInfoCore CCentreCommHandler::GetSingleton()
