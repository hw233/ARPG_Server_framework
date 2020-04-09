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

//
// WorldRunnable.cpp
//
#include "stdafx.h"
#include "WorldRunnable.h"
#include "ObjectPoolMgr.h"
#include "SceneLuaTimer.h"

//40�Ļ�ÿ��25��update�����������ֵ���Ʒ���Ƶ�ʣ��ͻ�����Ӧ�ı���
#define WORLD_UPDATE_DELAY 50

bool g_sReloadStaticDataBase = false;
bool g_ManualDailyReset	= false;
uint32 g_ManualSetTime = 0;

WorldRunnable::WorldRunnable(const uint64 threadID) : UserThread(THREADTYPE_WORLDRUNNABLE, threadID)
{
	minparkdiff=100;
	maxparkdiff=0;
	totalparkdiff=0;
	totalupdate = 0;
	delete_after_use = false;
	//srand(time(NULL)); // ���߼�������������� ��Ч���߳���δ���� by eeeo@2011.8.27
}

void WorldRunnable::run()
{
	ThreadRunning = true;
#ifdef _WIN64
	//::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#endif
	SetThreadName("WorldRunnable (hohogame non inistance)");
	cLog.Success("WorldRunnable", "���߼������߳�����.");

	uint64 LastWorldUpdate = getMSTime64();
	uint64 LastSessionsUpdate = getMSTime64();
	uint64 SendPackTime = getMSTime64();
	uint64 DealPackTime = getMSTime64();

	currenttime = UNIXTIME;
	dupe_tm_pointer(localtime(&currenttime), &local_currenttime);
	
	load_settings();
	set_tm_pointers();

    srand(time(NULL));  // ��������� by eeeo@2011.8.27

	while(m_threadState != THREADSTATE_TERMINATE)
	{
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

		currenttime = UNIXTIME;
		dupe_tm_pointer(localtime(&currenttime), &local_currenttime);

		if(true == g_ManualDailyReset)
		{
			if(1 == g_ManualSetTime)
			{
				update_hour();
			}
			else if(24 == g_ManualSetTime)
			{
				update_daily();
			}

			g_ManualDailyReset = false;
			g_ManualSetTime = 0;
		}

		if(has_timeout_expired(&local_currenttime, &local_last_daily_reset_time, DAILY))
		{
			update_daily();
		}
		if (has_timeout_expired(&local_currenttime, &local_last_hour_reset_time, HOURLY))
		{
			update_hour();
		}
	//	if (has_timeout_expired(&local_currenttime, &local_last_halfdaily_reset_time, HALFDAILY))
	//	{
	//		update_halfdaily();
	//		update_settings();
	//	}
	//	if(has_timeout_expired(&local_currenttime, &local_last_2hour_reset_time, TWOHOURLY))
	//	{
	//		update_2hour();
	//		update_settings();
	//	}
	//	if(has_timeout_expired(&local_currenttime, &local_last_2hour_reset_time, WEEKLY))
	//	{
	//		update_weekly();
	//		update_settings();
	//	}

        sPark.SetPassMinuteToday(local_currenttime.tm_hour * 60 + local_currenttime.tm_min);
		
		// ���ñ��ȸ�
		if(g_sReloadStaticDataBase)
		{
			reload_database();
		}
		
		// calce time passed
		uint64 now, execution_start;
		now = getMSTime64();
		execution_start = now;
		
		uint32 diff = now - LastWorldUpdate;
		LastWorldUpdate = now;
		sPark.Update( diff );
		now = getMSTime64();

		//�������߼���updateʱ��ʹ�����ͳ��������ʱ��
		totalparkdiff += diff;
		totalupdate++;
		minparkdiff = diff < minparkdiff ? diff : minparkdiff;
		maxparkdiff = diff > maxparkdiff ? diff : maxparkdiff;

		sGateCommHandler.TimeoutSockets();
		
		// ���ݰ������update
		diff = now - LastSessionsUpdate;
		if(diff > 1000)
			sLog.outString("ERROR:����Update WorldRunnableֱ������%d���룬�쳣",diff);

		LastSessionsUpdate = now;
		sGateCommHandler.UpdateServerRecvPackets();

		uint64 serverRecvPacketEndTime = getMSTime64();
		if(serverRecvPacketEndTime - now > 1000)
			sLog.outString("UpdateServerRecvPackets ʱ�䳬��1��");

		SendPackTime = serverRecvPacketEndTime;
		sGateCommHandler.UpdateServerSendPackets();

		uint64 serverSendPacketEndTime = getMSTime64();
		if(serverSendPacketEndTime - SendPackTime > 1000)
			sLog.outString("UpdateServerSendPackets ʱ�䳬��1��");

		DealPackTime = serverSendPacketEndTime;
		sPark.UpdateSessions( diff );
		uint64 dealPacketEndTime = getMSTime64();
		if(dealPacketEndTime-DealPackTime>1000)
			sLog.outDebug("sPark.UpdateSessions ʱ�䳬��1��");

		now = dealPacketEndTime;
		// lua��ʱ��update
		sSceneLuaTimer.Update(now);
		uint64 timerEndTime = getMSTime64();
		if (timerEndTime - now > 200)
			sLog.outDebug("LuaTimer.update ʱ�䳬��200����");

		now = timerEndTime;

		//��������update��ʱ�䡣
		diff = now - execution_start;	// time used for updating 
		if(m_threadState == THREADSTATE_TERMINATE)
			break;

		m_threadState = THREADSTATE_SLEEPING;

		/*This is execution time compensating system
			if execution took more than default delay 
			no need to make this sleep*/
		if (diff > 50)
		{
			sLog.outDebug("����ѭ�� %u", diff);
			if (diff > 3000)
				sLog.outError("����ѭ�� %u,ʱ�����", diff);
		}

		if(diff < WORLD_UPDATE_DELAY)
			Sleep(WORLD_UPDATE_DELAY - diff);
	}
	ThreadRunning = false;
}

void WorldRunnable::dupe_tm_pointer(tm * returnvalue, tm * mypointer)
{
	memcpy(mypointer, returnvalue, sizeof(tm));
}

void WorldRunnable::set_tm_pointers()
{
	dupe_tm_pointer(localtime(&last_daily_reset_time), &local_last_daily_reset_time);
	dupe_tm_pointer(localtime(&last_hour_reset_time), &local_last_hour_reset_time);
	//dupe_tm_pointer(localtime(&last_2hour_reset_time), &local_last_2hour_reset_time);
	//dupe_tm_pointer(localtime(&last_halfdaily_reset_time), &local_last_halfdaily_reset_time);
	//dupe_tm_pointer(localtime(&last_weekly_reset_time),&local_last_weekly_reset_time);
	//dupe_tm_pointer(localtime(&last_eventid_time), &local_last_eventid_time);
}

void WorldRunnable::load_settings()
{
	//��ʵ���ﲻ��Ҫ��ȡ���ݿ��¼����Ϸ������ֻ����������ҵ��ճ��¼�
	cLog.Success("�߼��߳�", "��ʼ���ճ�����..\n");
	last_daily_reset_time = UNIXTIME;
	last_hour_reset_time = UNIXTIME;
}

bool WorldRunnable::has_timeout_expired(tm * now_time, tm * last_time, uint32 timeoutval)
{
	switch(timeoutval)
	{
	case WEEKLY:
		{
			return ( (now_time->tm_yday / 7) != (last_time->tm_yday / 7) );
		}
	case MONTHLY:
		return (now_time->tm_mon != last_time->tm_mon);
	case HOURLY:
		return ((now_time->tm_hour != last_time->tm_hour) || (now_time->tm_yday != last_time->tm_yday) );
	case DAILY:
		return (now_time->tm_yday != last_time->tm_yday);
	case HALFDAILY:
		return ((now_time->tm_hour % 12 == 0) && (now_time->tm_hour != last_time->tm_hour) || (now_time->tm_yday != last_time->tm_yday) );
	case MINUTELY:
		return ((now_time->tm_min != last_time->tm_min) || (now_time->tm_hour != last_time->tm_hour) || (now_time->tm_yday != last_time->tm_yday) );
	case TWOHOURLY:
		return ((now_time->tm_hour!=last_time->tm_hour)&&(now_time->tm_hour%2==0) || (now_time->tm_yday != last_time->tm_yday) );
	}
	return false;
}

void WorldRunnable::update_daily()
{
	cLog.Notice("�߼��߳�", "�����ճ���������...");

	sPark.DailyEvent();
	objmgr.ResetDailies();
    sStatisticsSystem.DailyEvent();

	last_daily_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&last_daily_reset_time), &local_last_daily_reset_time);
	cLog.Notice("�߼��߳�", "�ճ���Ϣ�������,����ʱ��%d��%d��%d��...",local_last_daily_reset_time.tm_year+1900,local_last_daily_reset_time.tm_mon+1,local_last_daily_reset_time.tm_mday);
}

void WorldRunnable::update_hour()
{
	cLog.Notice("�߼��߳�", "ÿСʱ��������...");

	last_hour_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&last_hour_reset_time), &local_last_hour_reset_time);
	cLog.Notice("�߼��߳�", "ÿСʱ��Ϣ�������,����ʱ��%d��%dʱ...", local_last_hour_reset_time.tm_mday, local_last_hour_reset_time.tm_hour);
}

void WorldRunnable::reload_database()
{
	g_sReloadStaticDataBase = false;

//#ifdef _DEBUG
//	for(HM_NAMESPACE::hash_set<std::string>::iterator itr = sLang.m_missingkeys.begin();itr!=sLang.m_missingkeys.end();++itr)
//	{
//		Database_World->Execute("INSERT INTO language VALUES(0,'%s','%s(δ����)','δ����');",(*itr).c_str(),(*itr).c_str());
//	};
//	sLang.m_missingkeys.clear();
//#endif

	sDBConfLoader.Reload();		// ֻ�ܸ���������
    sSceneMapMgr.Reload();
	objmgr.Reload();
    sPark.LoadLanguage();
}

void WorldRunnable::PrintMemoryUseStatus()
{
	objPoolMgr.PrintObjPoolsUseStatus();
}
