#include "stdafx.h"
#include "UpdateThread.h"
#include "threading/UserThreadMgr.h"
#include "WorldRunnable.h"

extern bool m_stopEvent;
extern bool m_ShutdownEvent;
extern bool g_sReloadStaticDataBase;

CUpdateThread::CUpdateThread(const int64 threadID) : UserThread(THREADTYPE_CONSOLEINTERFACE, threadID)
{
	this->delete_after_use =false;
}

CUpdateThread::~CUpdateThread(void)
{
}

void CUpdateThread:: run()
{	
	m_input=0;

	while(m_threadState != THREADSTATE_TERMINATE && m_stopEvent==false)
	{
		// Provision for pausing this thread.
		if(m_threadState == THREADSTATE_PAUSED)
		{
			while(m_threadState == THREADSTATE_PAUSED)
			{
				Sleep(200);
			}
		}
		if(m_threadState == THREADSTATE_TERMINATE)
			break;

		m_threadState = THREADSTATE_BUSY;
#ifdef _WIN64
		m_input=getchar();
#endif
		switch(m_input)
		{
		case 'q':
			m_ShutdownEvent=!m_ShutdownEvent;
			break;
		case 's':
			printf(sThreadMgr.ShowStatus().c_str());
			break;
		case 'p':
			{
				WorldRunnable* prun = (WorldRunnable*)sThreadMgr.GetThreadByType(THREADTYPE_WORLDRUNNABLE);
				if(prun->totalupdate==0)
					prun->totalupdate=1;
				printf("average %d; min %d ;max %d\n",prun->totalparkdiff/prun->totalupdate,prun->minparkdiff,prun->maxparkdiff);
				printf("���յ����� %d KB,�ܷ������� %d KB\n",sGateCommHandler.m_totalreceivedata>>10,sGateCommHandler.m_totalsenddata>>10);
				printf("���ݰ�������:%d�� ��ʱ������:%d��������������������:%d\n", sGateCommHandler.m_totalSendPckCount, sGateCommHandler.m_TimeOutSendCount, sGateCommHandler.m_SizeOutSendCount);
				sPark.PrintDebugLog();
				break;
			}
		case 'r':
			{
				WorldRunnable* prun = (WorldRunnable*)sThreadMgr.GetThreadByType(THREADTYPE_WORLDRUNNABLE);
				prun->totalupdate=prun->totalparkdiff=prun->maxparkdiff=0;
				prun->minparkdiff = 100;
			}
			break;
		case 'w':
			{
				WorldRunnable* prun = (WorldRunnable*)sThreadMgr.GetThreadByType(THREADTYPE_WORLDRUNNABLE);
				prun->PrintMemoryUseStatus();
			}
			break;
		case 'b':
				Config.GlobalConfig.SetSource("../arpg2016_config/mainconfig.conf");
			break;
		case 'l':
			{
				printf("���¼��ؾ�̬������Դ\n");
				g_sReloadStaticDataBase = true;
			}
			break;
		case 'm':
			g_worldPacketBufferPool.Stats();
			break;
		case 'o':
			g_worldPacketBufferPool.Optimize();
			break;
		}
		m_input=0;
		if(m_threadState == THREADSTATE_TERMINATE)
			break;

		if(m_ShutdownEvent)
			break;
		m_threadState = THREADSTATE_SLEEPING;
		Sleep(500);
	}
}
