// GatewayServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

//threading
#include "UpdateThread.h"
#include "threading/UserThreadMgr.h"
//config
#include "config/ConfigEnv.h"

//vld�ڴ�й¶���Թ���
#ifdef _WIN64
#include "vld/vld.h"
#endif // _WIN64
#include <signal.h>

//network
#include "LogonCommHandler.h"
#include "SceneCommHandler.h"
#include "GameSocket.h"

TextLogger * Crash_Log = NULL;
TextLoggerThread *pTextLoggerThread = NULL;
GatewayScript* g_pGatewayScript = NULL;

const uint64 ILLEGAL_DATA_64 = 0xFFFFFFFFFFFFFFFF;
const uint32 ILLEGAL_DATA_32 = 0xFFFFFFFF;

GatewayServer g_local_server_conf;

bool m_stopEvent = false;
bool m_ShutdownEvent = false;
bool m_ReloadPackConfigEvent = false;
uint32 m_ShutdownTimer = 7000;

void OnCrash(bool Terminate)
{
	return ;
}

void _OnSignal(int s)
{
	switch (s)
	{
#ifndef _WIN64
	case SIGHUP:
		break;
	case SIGPIPE:
		sLog.outError("Error Receive SIGPIPE");
		break;
#endif
	case SIGINT:
	case SIGTERM:
	case SIGABRT:
#ifdef _WIN64
	case SIGBREAK:
#endif
		m_stopEvent = true;
		break;
	}
	signal(s, _OnSignal);
}

void _HookSignals()
{
	signal(SIGINT, _OnSignal);
	signal(SIGTERM, _OnSignal);
	signal(SIGABRT, _OnSignal);
#ifdef _WIN64
	signal(SIGBREAK, _OnSignal);
#else
	signal(SIGHUP, _OnSignal);
	signal(SIGPIPE, _OnSignal);
#endif
}
void _UnhookSignals()
{
	signal(SIGINT, 0);
	signal(SIGTERM, 0);
	signal(SIGABRT, 0);
#ifdef _WIN64
	signal(SIGBREAK, 0);
#else
	signal(SIGHUP, 0);
	signal(SIGPIPE, 0);
#endif
}

void InitPacketForbidden()
{
	if(!Config.PacketConfig.SetSource("forbidpack.conf"))
		return;

	sLog.outString("�������ݰ���������");

	std::string forbidden = Config.PacketConfig.GetStringDefault( "PackOpcode", "forbid", "" );
	if ( forbidden.find( ';' ) == std::string::npos )
	{
		return;
	}
	
	/*char str_forbid[1200];
	snprintf( str_forbid, 1200, forbidden.c_str() );
	char * current_ptr = str_forbid;
	uint32 temp_opcode = 0;
	uint32 result = 0;

	memset( GameSocket::m_packetforbid, 0, sizeof(GameSocket::m_packetforbid) );
	for ( uint32 i = 0; i < NUM_MSG_TYPES; ++i )
	{
	result = sscanf( current_ptr, "%u", &temp_opcode );
	if ( result != 1 )
	{
	break;
	}
	if ( temp_opcode < NUM_MSG_TYPES )
	{
	GameSocket::m_packetforbid[temp_opcode] = 1;
	sLog.outString( "���ݰ�%u������", temp_opcode );
	}
	current_ptr = strchr( current_ptr, ';' );
	if ( current_ptr == NULL )
	{
	break;
	}
	++current_ptr;
	if ( *current_ptr == '\0' )
	{
	break;
	}
	}*/
}
// 1�������ı���¼�߳�
// 2��������������
// 3�����������������߳�
// 4����������̨�߳�
// 5��������Ϸ������
// 6��������֤������
// 7���������ؼ���������
// 8�����ط�������ʼ�������˿ںţ�7767��
// 9������ʱ�䣨��������̨�̸߳��¡��׽��ֿ��������µȣ�
// 10���ر����ط�����
// 11��ɾ����־
// 12���ر���������
// 13���ر��ı���־�߳�
// 14��ɾ��������־ָ��
// 15������winsock��̻���
// 16���رտ���̨�߳�

#ifdef _WIN64
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#else
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#endif

bool _LoadConfig()
{
	//ȡ�����ļ�
	cLog.Notice("Config", "Loading Config Files...\n");
	if(Config.GlobalConfig.SetSource(global_config_file))
	{
		cLog.Success("Config", ">> ../arpg2016_config/mainconfig.conf");
	}
	else
	{
		cLog.Error("Config", ">> ../arpg2016_config/mainconfig.conf");
		return false;
	}
	sSceneCommHandler.SetStartParm("host", Config.GlobalConfig.GetStringDefault("GateServer", "ListenHost", "0.0.0.0"));
	sSceneCommHandler.SetStartParm("domain",Config.GlobalConfig.GetStringDefault("GateServer", "Host", "127.0.0.1"));
	sSceneCommHandler.SetStartParm("port",Config.GlobalConfig.GetStringDefault("GateServer", "ListenPort", "16001"));
	sSceneCommHandler.SetStartParm("consolename",Config.GlobalConfig.GetStringDefault("GateServer","Name","����s1_16001"));

	sSceneCommHandler.SetStartParm("scount",Config.GlobalConfig.GetStringDefault("Global", "SceneServerCount", "1"));
	sSceneCommHandler.SetStartParm("packcheck",Config.GlobalConfig.GetStringDefault("GateServerSetting", "PackCheck", "10"));
	sSceneCommHandler.SetStartParm("sameip",Config.GlobalConfig.GetStringDefault("GateServerSetting", "SameIP", "3000"));
	sSceneCommHandler.SetStartParm("shutdowntimer", "10000");
	sSceneCommHandler.SetStartParm("screenloglv",Config.GlobalConfig.GetStringDefault("GateLogLevel","Screen","3"));
	sSceneCommHandler.SetStartParm("fileloglv",Config.GlobalConfig.GetStringDefault("GateLogLevel","File","1"));

	return true;
}

int RUNGATE(int argc, char* argv[])
{
#ifndef _WIN64
	SetEXEPathAsWorkDir();
#endif
	UNIXTIME = time(NULL);

	if(!_LoadConfig())
		return 0;

	std::string listenaddr;
	int	listenport=0;
	int SceneServerCount=0;

	if(sSceneCommHandler.LoadStartParam(argc,argv))
	{
		SceneServerCount = sSceneCommHandler.GetStartParmInt("scount");
		listenaddr = sSceneCommHandler.GetStartParm("domain");
		listenport = sSceneCommHandler.GetStartParmInt("port");
		char temp[256];
		sprintf(temp,"%s:%d",listenaddr.c_str(),listenport);
		listenaddr = temp;
	}
	else
	{
		return 0;
	}	

	if(SceneServerCount==0||listenport==0||listenaddr.length()<5)
	{
		cLog.Notice("Startup", "Start Param Missing,Process Will Exit!(try start with -domain [ip/domain] -port [listenport] -host [listenip] -scount [SceneServerCount])");
		return 0;
	}
	else
	{
		cLog.Notice("Startup", "ServerCount:%d  Domain:%s  ListenHost:%s, port:%d", SceneServerCount, listenaddr.c_str(), sSceneCommHandler.GetStartParm("host"), listenport);
	}

#ifdef _WIN64
	::SetConsoleTitle(sSceneCommHandler.GetStartParm("consolename"));
#endif // _WIN64


	// 1�������ı���¼�߳�
	pTextLoggerThread = new TextLoggerThread();
	launch_thread(pTextLoggerThread);

	cLog.Notice("Startup", "===============================================");
	cLog.Notice("Startup", "       the Gateway Server         ");
	cLog.Notice("Startup", "===============================================");
	cLog.Line();

	Crash_Log = new TextLogger("logs", "GateCrashLog", false);

#ifdef _WIN64
	::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#endif
	//GameSocket::m_disconnectcount = sSceneCommHandler.GetStartParmInt("packcheck");
	char szLogname[128];
	snprintf(szLogname, 128, "GateSLog_Port%d", listenport);
	//��ʼ��slog�ļ�¼�ȼ�
	sLog.Init(sSceneCommHandler.GetStartParmInt("fileloglv"), sSceneCommHandler.GetStartParmInt("screenloglv"), szLogname);

	sSceneCommHandler.SetIPLimit(sSceneCommHandler.GetStartParmInt("sameip"));

	cLog.Notice("Startup", "ͬIP�������� %d", sSceneCommHandler.GetStartParmInt("sameip"));
	m_ShutdownTimer = sSceneCommHandler.GetStartParmInt("shutdowntimer");//�ط��ӳ�

	// ��ʼ���ű�����
	g_pGatewayScript = new GatewayScript();
	int loadRet = g_pGatewayScript->LoadScript("GatewayScript/gateway_main.lua");
	ASSERT(loadRet == 0 && "LuaScript LoadFile failed");

	_HookSignals();

	// ע��pbc
	sPbcRegister.Init();
	RegistProtobufConfig(&(sPbcRegister));

	//worldpacketbuff��sqlִ���ַ����ط��䣬�������ʼ��֮ǰ
	g_worldPacketBufferPool.Init();
	g_sqlExecuteBufferPool.Init();

	// 2��������������
	CreateSocketEngine();

	// 3�����������������߳�
	SocketEngine::getSingleton().SpawnThreads();
	cLog.Line();
	cLog.Success("Net", "Gateway NetWork thread spawned\n");

	// 4����������̨�߳�
	cLog.Notice("Gateway", "Console threading..");
	CUpdateThread *pUpdateThread = new CUpdateThread(sThreadMgr.GenerateThreadId());
	launch_thread(pUpdateThread);

	// 5��������Ϸ�����������������
	sSceneCommHandler.Startup();

	// 6��������֤��¼������
	sLogonCommHandler.Startup();

	// 7���������ؼ���������
	GameSocket::InitTransPacket();

	InitPacketForbidden();
	
	// 8�����ط�������ʼ�������˿ںţ�14001��
	ListenSocket<GameSocket> *sListen = new ListenSocket<GameSocket>(); 
	uint32 sport = listenport>0?listenport:Config.GlobalConfig.GetIntDefault("GateServer", "ListenPort", 14001);
	string shost = sSceneCommHandler.GetStartParm("host");
	bool listnersockcreate = sListen->Open(shost.c_str(), sport);
	cLog.Notice("GateWay", "�����ؼ�����%u �Ŷ˿�..\n", sport);
	
	// 9������ʱ�䣨��������̨�̸߳��¡��׽��ֿ��������µȣ�
	uint64 start = getMSTime64();
	uint64 diff = 0;
	uint64 last_time = getMSTime64();
	uint64 etime;
	uint64 next_printout = 0;
	uint64 next_send = 0;

	// �����������
	srand(time(NULL));
	while (!m_stopEvent&&listnersockcreate)
	{
		if(m_ReloadPackConfigEvent)
		{
			InitPacketForbidden();
			m_ReloadPackConfigEvent = false;
		}
		UNIXTIME = time(NULL);
		UNIXMSTIME = getMSTime64();
		
		start = UNIXMSTIME;
		diff = start - last_time;

		sSceneCommHandler.UpdateClientRecvQueuePackets();

		uint64 loginUpdateStartTime = getMSTime64();
		sLogonCommHandler.UpdateSockets();
		uint64 curmstime = getMSTime64();
		if(curmstime - loginUpdateStartTime > 10000)	// 15���Ǽ��ޣ�Ӧ�ÿ�����1�����¡�����ͻ��˾ͻ�����ӳ�
			sLog.outError("���� ���¼�� ���ݰ�������10��%d���п��ܷ��ʱ�ɾ����GameSocket", curmstime - loginUpdateStartTime);

		if (curmstime - start > 15000)
			sLog.outError("�û����ݰ�������ʱ�䳬��15��(%d)��", curmstime - start);

		//ɾ���Ѿ�delete��socket
		sSocketDeleter.Update();			
		//����������Ϸ����������������δ���ӵģ�6������Ϸ��������
		//���Ѿ������ϵ�һ��ʱ�䷢��ping����жϳ�ʱ�Ƴ�
		sSceneCommHandler.UpdateSockets();	
		//���Ѿ����ӵĿͻ��˽��и��£����ж����ӳ�ʱ�Ƴ����������������Ŀͻ�������
		sSceneCommHandler.UpdateGameSockets();
		//�Եȴ�������Ϸ�Ŀͻ��˽��и���
		sSceneCommHandler.UpdatePendingGameSockets();

		sSceneCommHandler.UpdateClientSendQueuePackets();

		last_time = getMSTime64();
		etime = last_time - start;
		if (m_ShutdownEvent)
		{
			if (getMSTime64() >= next_printout)
			{
				if (m_ShutdownTimer > 60000.0f)
				{
					if (!((int)(m_ShutdownTimer) % 60000))
					{
						sLog.outString("Server shutdown in %i minutes..", (int)(m_ShutdownTimer / 60000.0f));
					}
				}
				else
				{
					sLog.outString("Server shutdown in %i seconds..", (int)(m_ShutdownTimer / 1000.0f));
				}
				next_printout = getMSTime64() + 2000;
			}
			if(getMSTime64() >= next_send)
			{
				int time = m_ShutdownTimer / 1000;
				if(time>15)
					time-=15;//��������ǰ�ط��ġ����Ի���30�����ؾ͵�����
				if (time > 0)
				{
					int hour = 0;
					int mins = 0;
					int secs = 0;
					if(time>3600)
					{
						hour = time / 3600;
						time = time % 3600;
					}
					if (time > 60)
					{
						mins = time / 60;
					}
					if (mins)
					{
						time -= (mins * 60);
					}
					secs = time;
					char str[64];
					if(hour>0)
						snprintf(str, 64, "Server shutdown in %02u:%02u:%02u",hour, mins, secs);
					else
						snprintf(str, 64, "Server shutdown in %02u:%02u", mins, secs);

					// �ط�֪ͨ
					/*WorldPacket data(SMSG_SERVER_MESSAGE,64);
					data << uint32(SERVER_MSG_SHUTDOWN_TIME);
					data << str;
					sSceneCommHandler.SendGlobaleMsg(&data);*/
				}
				if(m_ShutdownTimer>3600000)
					next_send = getMSTime64() + 1800000;
				else if(m_ShutdownTimer>180000)
					next_send = getMSTime64() + 30000;
				else
					next_send = getMSTime64() + 5000;
			}
			if(diff >= m_ShutdownTimer)
			{
				m_stopEvent = true;
			}
			else
			{
				m_ShutdownTimer -= diff;
			}
		}
		// ��һЩ�����ļ��
		// �������¿ͻ����׽��ֹ�����
		if (5 > etime)
		{
			Sleep(5 - etime);
		}
	}

	_UnhookSignals();
	pUpdateThread->SetThreadState(THREADSTATE_TERMINATE);
	sThreadMgr.RemoveThread(pUpdateThread);
	Sleep(600);
	delete pUpdateThread;
	
	// �ű��˳�
	g_pGatewayScript->LoadScript(NULL);
	SAFE_DELETE(g_pGatewayScript);

	// 10���ر����ط�����
	sListen->Disconnect();

	cLog.Notice("Close", "�������ѹر�..\n");
	
	// 11�����������������ѹ����ͳ�������ӡ����
	sSceneCommHandler.PrintCompressedPacketCount();
	
	// 12���ر���������
	sSceneCommHandler.CloseAllGameSockets(ILLEGAL_DATA_32);
	sSceneCommHandler.CloseAllServerSockets();
	sSceneCommHandler.OutPutPacketLog();
	Sleep(1000);
	SocketEngine::getSingleton().Shutdown();
	Sleep(1500);//�ȴ��ı�д��
	// 13���ر��ı���־�߳�
	TextLogger::Thread->Die();
	Sleep(1000);
	delete CSceneCommHandler::GetSingletonPtr();
	delete LogonCommHandler::GetSingletonPtr();
	// 14��ɾ��������־ָ��
	delete PbcDataRegister::GetSingletonPtr();
	delete ThreadMgr::GetSingletonPtr();
	printf("delete CLog::GetSingletonPtr()");
	delete CLog::GetSingletonPtr();
	printf("delete oLog::GetSingletonPtr()");
	delete oLog::GetSingletonPtr();
	//delete WorldLog::GetSingletonPtr();

	// 15������winsock��̻���
#ifdef _WIN64
	WSACleanup();
#endif
	
	printf("g_worldPacketBufferPool.Stats()\n");
	g_worldPacketBufferPool.Stats();
	g_worldPacketBufferPool.Destroy();

	printf("g_sqlExecuteBufferPool.Stats()\n");
	g_sqlExecuteBufferPool.Stats();
	g_sqlExecuteBufferPool.Destroy();
	
	printf("����ȫ�˳������������������\n");

	return 0;
}

int main(int argc, char* argv[])
{
	THREAD_TRY_EXECUTION
	{
		RUNGATE(argc,argv);
	}
	THREAD_HANDLE_CRASH
}
