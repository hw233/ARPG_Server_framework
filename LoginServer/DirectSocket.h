#pragma once

//�ͻ���������¼��֤
class DirectSocket : public TcpSocket
{
public:
	DirectSocket(SOCKET fd,const sockaddr_in * peer);
	~DirectSocket(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	
public:
	void OnConnect();
	void OnDisconnect();
	void SendPacket(WorldPacket * packet);
	
	uint32 last_recv;

private:
	// �����½
	void HandleLogin(WorldPacket &packet);
	// �����Զ�ע���˺�
	void HandleAccountAutoRegister(WorldPacket &packet);

	// ip��ַ
	string mClientIpAddr;
};
