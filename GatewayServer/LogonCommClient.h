/****************************************************************************
 *
 * General Object Type File
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

#ifndef __LOGON_COMM_CLIENT_H
#define __LOGON_COMM_CLIENT_H

#include "ServerCommonDef.h"

class LogonCommClient;
typedef void (LogonCommClient::*logonpacket_handler)(WorldPacket&);

//��������Ҫ�������غ͵�¼������֮�������ͱ������ӵĴ���û��������ݵĹ�ͨ
class LogonCommClient : public TcpSocket
{
public:
	LogonCommClient(SOCKET fd, const sockaddr_in * peer);
	~LogonCommClient();
	
public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	
	void SendPacket(WorldPacket *data);		// �������ݵ����ͻ�����
	void OnDisconnect();					// �����ӱ��ر�ʱ

	void SendPing();						// ���½����������ping������
	void SendChallenge();					// ��������ʱ�����½������������֤������

	void HandlePacket(WorldPacket & recvData);	// �������ݣ���������Handle��ͷ�ķ���

	void HandleAuthResponse(WorldPacket & recvData);		// �õ���֤���
	void HandlePong(WorldPacket & recvData);				// �õ�ping�Ľ��
	void HandleAccountAuthResult(WorldPacket & recvData);	// �õ����������֤��Ľ��

	uint32 last_ping_unixtime;						
	uint32 last_pong_unixtime;

	uint64 ping_ms_time;						
	uint32 latency;							// �ӳ�

	bool authenticated_ret;					// �Ƿ�ͨ����֤

public:
	static logonpacket_handler ms_packetHandler[SERVER_MSG_OPCODE_COUNT];
	static void InitPacketHandler();

};

#endif

