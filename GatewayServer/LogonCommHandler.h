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

#ifndef LOGONCOMMHANDLER_H
#define LOGONCOMMHANDLER_H

#include "ServerCommonDef.h"
#include "GameSocket.h"
#include "LogonCommClient.h"
#include "auth/Sha1.h"

class LogonCommHandler : public Singleton<LogonCommHandler>
{
private:
	map<uint32, GameSocket*> m_pendingAuthSockets;			// ���������δ��֤����ҵ��׽���ӳ���
	
	LoginServer				m_LoginServerData;				// ��½�����������¼����
	LogonCommClient*		m_pLoginServerSocket;			// ���½���������ӵ��׽���
														
	uint32 next_request;									// ��������½ʱ���õ���һ��������
	Mutex mapLock;											// ��½������ͨѶ���׽���ӳ������
	Mutex pendingLock;										// �������ӳ�����

public:	
	SHA1Context m_sha1AuthKey;								// �������ط��ڵ�½������֤ע��ʱ�õ���sha1��ϣֵ

	LogonCommHandler();
	~LogonCommHandler();

	void Startup();															// ����������������ȡ�ļ����ã���������½��������
	void ConnectionDropped();												// ĳ���ط������Ͽ�����
	void AdditionAck(uint32 ServID);										// ��һ�����ط����������ע��
	void UpdateSockets();													// ��ĳһ���ط�����ping����ʱ��ر���
	void ConnectLoginServer(LoginServer *server);							// �����ط��������ӵ���½������

	void LoadRealmConfiguration();											// ��ȡ�ļ����ã�������½�����������Լ����ط���������

	/////////////////////////////
	// Worldsocket stuff
	///////

	uint32 GenerateRequestID();
	bool ClientConnected(string &accountName, GameSocket *clientSocket);	// �ͻ����׽����������ط�������������֤����������½������
	
	void UnauthedSocketClose(uint32 id);		// һ��δͨ����֤�Ŀͻ����׽��ֱ��ر�
	void RemoveUnauthedSocket(uint32 id);		// �Ƴ�һ��δͨ����֤�Ŀͻ����׽���

	inline GameSocket* GetSocketByRequest(uint32 id)		// ͨ������ID�ҵ�һ���ͻ����׽���
	{
		//pendingLock.Acquire();
		GameSocket * sock;
		map<uint32, GameSocket*>::iterator itr = m_pendingAuthSockets.find(id);
		sock = (itr == m_pendingAuthSockets.end()) ? 0 : itr->second;
		//pendingLock.Release();
		return sock;
	}
	inline Mutex & GetPendingLock() { return pendingLock; }
	inline LoginServer* GetLoginServerData() { return &m_LoginServerData; }
};

#define sLogonCommHandler LogonCommHandler::GetSingleton()

#endif

