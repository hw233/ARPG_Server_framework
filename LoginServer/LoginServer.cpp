// sharedtest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <signal.h>

//database
#include "threading/UserThreadMgr.h"
#include "config/ConfigEnv.h"
#include "network/Network.h"

//threading
#include "WorldRunnable.h"

//network
#include "LogonCommServer.h"
#include "DirectSocket.h"

//accountmgr
#include "AccountCache.h"

//vld�ڴ�й¶���Թ���
#ifdef _WIN64
#include "vld/vld.h"
#endif

#ifdef _WIN64
TextLogger * Crash_Log = NULL;
#endif 

WorldRunnable *WorldRun_Thread = NULL;
// MySQLDatabase * Database_Data = NULL;
MySQLDatabase * Database_Log = NULL;
MongoConnector * MongoDB_Data = NULL;

bool m_stopEvent = false;

const uint64 ILLEGAL_DATA_64 = 0xFFFFFFFFFFFFFFFF;
const uint32 ILLEGAL_DATA_32 = 0xFFFFFFFF;

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

void OnCrash(bool Terminate)
{

}

bool _StartDB()
{
	string hostname, username, password, database,charset;
	int port = 0;
	int type = 1;
	bool result = false;

	result = Config.GlobalConfig.GetString("MongoDatabase", "Username", &username);
	Config.GlobalConfig.GetString("MongoDatabase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("MongoDatabase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("MongoDatabase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("MongoDatabase", "Port", &port);

	if(result == false)
	{
		sLog.outError("sql: mainconfig.conf�����ļ���ȱ��MongoDatabase����.");
		return false;
	}
	
	// ��ʼ��mongo����
	mongo::client::initialize();

	MongoDB_Data = new MongoConnector();
	if (MongoDB_Data == NULL)
		return false;

	int ret = MongoDB_Data->ConnectMongoDB(hostname.c_str(), port);
	if (ret == 0)
	{
		cLog.Success("Database", "MongoDB Connections established.");
	}
	else
	{
		cLog.Error("Database", "One or more errors occured while connecting to MongoDB..");
		SAFE_DELETE(MongoDB_Data);
		mongo::client::shutdown();
		return false;
	}
	
	// Configure Main Database
	/*result = Config.GlobalConfig.GetString("GameDatabase", "Username", &username);
	Config.GlobalConfig.GetString("GameDatabase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("GameDatabase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("GameDatabase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("GameDatabase", "Port", &port);
	result = !result ? result : Config.GlobalConfig.GetInt("GameDatabase", "Type", &type);
	charset = Config.GlobalConfig.GetStringDefault("GameDatabase", "CharSet", "UTF8");

	if(result == false)
	{
		sLog.outError("sql: mainconfig.conf�����ļ���ȱ��GameDatabase����.");
		return false;
	}

	Database_Data = new MySQLDatabase(sThreadMgr.GenerateThreadId());

	if(Database_Data->Initialize(hostname.c_str(), (unsigned int)port, username.c_str(),password.c_str(), database.c_str(), 1, 1000, charset.c_str()))
	{
		cLog.Success("Database", "Connections established.");
	}
	else
	{
		cLog.Error("Database", "One or more errors occured while connecting to databases.");
		DestroyDatabaseInterface(Database_Data);//ʵ������
		Database_Data = NULL;
		return false;
	}*/

	//hohogamelog���ݿ�
	result = Config.GlobalConfig.GetString("LogDataBase", "Username", &username);
	Config.GlobalConfig.GetString("LogDataBase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("LogDataBase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("LogDataBase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("LogDataBase", "Port", &port);
	result = !result ? result : Config.GlobalConfig.GetInt("LogDataBase", "Type", &type);
	charset = Config.GlobalConfig.GetStringDefault("LogDataBase", "CharSet", "UTF8");
	if(result == false)
	{
		sLog.outError("sql: loginserver.conf�����ļ���ȱ��LogDatabase����.");
		SAFE_DELETE(MongoDB_Data);
		mongo::client::shutdown();
		return false;
	}

	Database_Log = new MySQLDatabase(sThreadMgr.GenerateThreadId());

	if(Database_Log->Initialize(hostname.c_str(), (unsigned int)port, username.c_str(), password.c_str(), database.c_str(), 1, 1000, charset.c_str()))
	{
		cLog.Success("LogDatabase", "Connections established.");
	}
	else
	{
		cLog.Error("LogDatabase", "One or more errors occured while connecting to databases.");
		/*DestroyDatabaseInterface(Database_Data); //ʵ������
		Database_Data = NULL;*/
		DestroyDatabaseInterface(Database_Log);  //ʵ������
		Database_Log  = NULL;
		SAFE_DELETE(MongoDB_Data);
		mongo::client::shutdown();
		return false;
	}

	return true;
}

//ListenSocket<PhpLogonSocket> * g_s1;
ListenSocket<LogonCommServerSocket> * g_s2;
ListenSocket<DirectSocket> * g_s3;

bool _StartListen()
{
	uint32 wport =sInfoCore.GetStartParmInt("webport");
	uint32 sport = sInfoCore.GetStartParmInt("serverport");
	string webhost = Config.GlobalConfig.GetStringDefault("LoginServer", "PHPHost", "127.0.0.1");
	string shost = Config.GlobalConfig.GetStringDefault("LoginServer", "GSHost", "127.0.0.1");
	uint32 authport =sInfoCore.GetStartParmInt("clientport");
	string authhost = Config.GlobalConfig.GetStringDefault("LoginServer", "AuthHost", "0.0.0.0");

	bool listenfailed = false;
	/*g_s1 = new ListenSocket<PhpLogonSocket>();
	if(g_s1->Open(webhost.c_str(),wport))
	sLog.outString("����PHP���ӣ�IP:%s Port:%d",webhost.c_str(),wport);
	else
	listenfailed = true;*/

	g_s2 = new ListenSocket<LogonCommServerSocket>();
	if(g_s2->Open(shost.c_str(),sport))
		sLog.outString("����GateServer���ӣ�IP:%s Port:%d",shost.c_str(),sport);
	else
		listenfailed = true;

	g_s3 = new ListenSocket<DirectSocket>();
	if(g_s3->Open(authhost.c_str(), authport))
		sLog.outString("�����ͻ�����֤���ӣ�IP:%s Port:%d",authhost.c_str(),authport);
	else
		listenfailed = true;

	return !listenfailed;
}

void _StopListen()
{
	//g_s1->Disconnect();
	g_s2->Disconnect();
	g_s3->Disconnect();
}

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

	sInfoCore.SetStartParm("webport", Config.GlobalConfig.GetStringDefault("LoginServer","WebListPort","2445"));
	sInfoCore.SetStartParm("serverport", Config.GlobalConfig.GetStringDefault("LoginServer","ServerPort","8293"));
	sInfoCore.SetStartParm("clientport", Config.GlobalConfig.GetStringDefault("LoginServer","AuthPort","8294"));
	sInfoCore.SetStartParm("consolename", Config.GlobalConfig.GetStringDefault("LoginServer","Name","��¼��"));

	return true;
}

int RunLS(int argc, char* argv[])
{
#ifndef _WIN64
	SetEXEPathAsWorkDir();
#endif

	UNIXTIME = time(NULL);
	UNIXMSTIME = getMSTime64();
	launch_thread(new TextLoggerThread);//�ı���¼���߳�����

#ifdef _WIN64
	Crash_Log = new TextLogger("logs", "logonCrashLog", false);
#endif

	//ȡ�����ļ�
	if(!_LoadConfig())
		return 0;

	if(!sInfoCore.LoadStartParam(argc,argv))
		return 0;

	sLog.Init(Config.GlobalConfig.GetIntDefault("LoginLogLevel", "File", -1),Config.GlobalConfig.GetIntDefault("LoginLogLevel", "Screen", 1), "LoginServerLog");

	string logon_pass = Config.GlobalConfig.GetStringDefault("RemotePassword", "LoginServer", "change_me_logon");
	sInfoCore.PHP_SECURITY = Config.GlobalConfig.GetStringDefault("Global", "PHPKey", "��������Կ");
	char szLogonPass[20] = { 0 };
	snprintf(szLogonPass, 20, "%s", logon_pass.c_str());
	SHA_1 sha1;
	sha1.SHA1Reset(&(sInfoCore.m_gatewayAuthSha1Data));
	sha1.SHA1Input(&(sInfoCore.m_gatewayAuthSha1Data), (uint8*)szLogonPass, 20);

#ifdef _WIN64
	::SetConsoleTitle(sInfoCore.GetStartParm("consolename"));
#endif

#ifndef EPOLL_TEST
	cLog.Line();
	cLog.Notice("Database", "Connecting to database...");

	//�������ݿ�
	if(!_StartDB())
		return 0;
#endif

	// ��ȡsp������
	sInfoCore.LoadSpConfig();
	
	// ��ȡserver_setting������Ϣ
	sDBConfLoader.LoadDBConfig();
	
#ifndef EPOLL_TEST
	// ��ʼ��AccountMgr
	sAccountMgr.InitConnectorData(MongoDB_Data);

	// ��ȡ����
	sLog.outColor(TNORMAL, " >> precaching accounts...\n");
	sAccountMgr.ReloadAccounts(true);
	sLog.outColor(TNORMAL, " >> finished...\n");
#endif

	// ��ʼ�����ݰ���sqlִ�л���ĳ�
	g_worldPacketBufferPool.Init();
	g_sqlExecuteBufferPool.Init();

	// ��ʼ��pbc�Ľ�����
	sPbcRegister.Init();
	RegistProtobufConfig(&(sPbcRegister));

	// ��ʼ�������������
	LogonCommServerSocket::InitLoginServerPacketHandler();

	//network
	CreateSocketEngine();
	SocketEngine::getSingleton().SpawnThreads();
	WorldRun_Thread = new WorldRunnable(sThreadMgr.GenerateThreadId());
	launch_thread(WorldRun_Thread);

	bool bListSucess = _StartListen();
	if(!bListSucess)
		return 0;

	cLog.Line();
	//��Ѱ����
	sLog.outColor(TNORMAL, " >> Netword Started...\n");

#ifdef _WIN64
	_HookSignals();
	while(!m_stopEvent)
	{
		int input = getchar();
		switch(input)
		{
		case 's':
			printf(sThreadMgr.ShowStatus().c_str());
			break;
		case 'r':
			sAccountMgr.ReloadAccounts(true);
			break;
		}
		Sleep(1000);
	}
	_UnhookSignals();

#else
	_HookSignals();
	while(!m_stopEvent)//�˳�
	{
		Sleep(1000);
	}
	_UnhookSignals();
#endif

	_StopListen();

#ifndef EPOLL_TEST

	// �ر�mongo���ݿ�
	SAFE_DELETE(MongoDB_Data);
	mongo::client::shutdown();

	//�ر�data���ݿ�
	/*((MySQLDatabase*)Database_Data)->SetThreadState(THREADSTATE_TERMINATE);
	// send a query to wake up condition//��Ҫ�����̲߳��ܹر�
	((MySQLDatabase*)Database_Data)->Execute("SELECT count(account_name) from accounts");
	cLog.Notice("Database","Waiting for database to close..");
	while(((MySQLDatabase*)Database_Data)->ThreadRunning)
		Sleep(100);
	cLog.Success("Database","database has been shutdown");
	DestroyDatabaseInterface(Database_Data);//ʵ������
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Data));*/

	//�ر�log���ݿ�
	((MySQLDatabase*)Database_Log)->SetThreadState(THREADSTATE_TERMINATE);
	Database_Log->Execute("Update game_forbid_ip Set forbid_state = 0 Where forbid_state = 1 And end_time<NOW() or forbid_state = 2");
	cLog.Notice("LogDatabase","Waiting for database to close..");
	while(((MySQLDatabase*)Database_Log)->ThreadRunning)
		Sleep(100);

	cLog.Success("LogDatabase","database has been shutdown");
	DestroyDatabaseInterface(Database_Log);
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Log));
#endif

	WorldRun_Thread->SetThreadState(THREADSTATE_TERMINATE);
	Sleep(100);

	sThreadMgr.RemoveThread(WorldRun_Thread);
	sThreadMgr.Shutdown();
	Sleep(500);

	printf("Network Engine Shutting down..\n");
	SocketEngine::getSingleton().Shutdown();

	printf("delete AccountMgr ..\n");
	delete AccountMgr::GetSingletonPtr();

	printf("delete ThreadMgr ..\n");
	delete ThreadMgr::GetSingletonPtr();

	printf("delete pbcRegister ..\n");
	delete PbcDataRegister::GetSingletonPtr();

	printf("delete DBConfigLoader ..\n");
	delete DBConfigLoader::GetSingletonPtr();

	delete InformationCore::GetSingletonPtr();

	printf("delete CLog\n");
	delete CLog::GetSingletonPtr();

	printf("TextLogger::Thread->Die()\n");
	TextLogger::Thread->Die();
	Sleep(100);

	printf("delete oLog\n");
	delete oLog::GetSingletonPtr();
	
#ifdef _WIN64
	WSACleanup();
#endif

	printf("g_worldPacketBufferPool.Stats\n");
	g_worldPacketBufferPool.Stats();
	g_worldPacketBufferPool.Destroy();

	printf("g_sqlExecuteBufferPool.Stats\n");
	g_sqlExecuteBufferPool.Stats();
	g_sqlExecuteBufferPool.Destroy();

	printf("����ȫ�˳������������������\n");
	return 0;
}

int main(int argc, char** argv)
{
	THREAD_TRY_EXECUTION
	{
		RunLS(argc, argv);
	}
	THREAD_HANDLE_CRASH
}
