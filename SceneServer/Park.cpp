/****************************************************************************
 *
 * Main Park System
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

#include "stdafx.h"
#include "ObjectPoolMgr.h"
#include "GateCommHandler.h"
#include "SceneLuaTimer.h"
#include "SkillHelp.h"

#define SCENE_MAP_PLAYER_COUNTER_SYNC_TIMER		30000		// ÿ30���������ͬ��һ�µ�ǰ�������ĵ�ͼ�������

enum UPDATE_CLASS_NAME
{
	UPDATETIME_GAMESESSION = 0,
	UPDATETIME_INSTANCEMGR,
	UPDATETIME_EVENTHOLD,
	UPDATETIME_TEAM,
	UPDATETIME_AUCTION,
	UPDATETIME_STATISTICS,
	UPDATETIME_BARRIERMGR,
	UPDATETIME_CREDIT,
	UPDATETIME_MANAGER,
	UPDATETIME_MAXCOUNT,
};

const char* g_updateclassname[]={
	"UPDATETIME_GAMESESSION ",
	"UPDATETIME_INSTANCEMGR",
	"UPDATETIME_EVENTHOLD",
	"UPDATETIME_TEAM",
	"UPDATETIME_AUCTION",
	"UPDATETIME_STATISTICS",
	"UPDATETIME_BARRIERMGR",
	"UPDATETIME_CREDIT",
	"UPDATETIME_MANAGER"
};

Park::Park()
{
	m_playerLimit = 0;
	eventholder = new EventableObjectHolder(-1);
    m_PlatformID = 0;
    m_ServerGlobalID = 0;

	memset(m_ClassUpdateTime,0,15*sizeof(uint32));
	memset(m_ClassUpdateMaxTime,0,15*sizeof(uint32));
	m_TotalUpdate=0;
    m_pass_min_today = 0;
	m_mapPlayerCountSyncTimer = SCENE_MAP_PLAYER_COUNTER_SYNC_TIMER;
}

Park::~Park()
{
	// sLog.outString("  Deleting world packet logger...");

	delete eventholder;
	eventholder = NULL;
}


GameSession* Park::FindSession(uint64 plrGuid)
{
	GameSession * ret = NULL;
	m_sessionlock.AcquireReadLock();
	
	SessionMap::iterator it = m_sessions.find(plrGuid);
	if (it != m_sessions.end())
		ret = it->second;

	m_sessionlock.ReleaseReadLock();
	return ret;
}


void Park::DeleteSession(uint64 plrGuid, GameSession *session, bool gcToPool /* = true */)
{
	m_sessionlock.AcquireWriteLock();

	if (m_sessions.find(plrGuid) != m_sessions.end())
		m_sessions.erase(plrGuid);
	
	m_sessionlock.ReleaseWriteLock();
	
	// delete session to buffer
	if (gcToPool)
		objPoolMgr.deleteGameSession(session);
}

void Park::AddSession(uint64 plrGuid, GameSession* s)
{
	m_sessionlock.AcquireWriteLock();
	ASSERT(s);
	m_sessions.insert(make_pair(plrGuid, s));
	m_sessionlock.ReleaseWriteLock();

}
//globalsessionָ��Ҳ��ھ���ĳ�ŵ�ͼ�е�״̬
void Park::AddGlobalSession(GameSession *session)
{
	m_globalSessionMutex.Acquire();
	m_globalSessions.insert(session);
	m_globalSessionMutex.Release();
}

void Park::RemoveGlobalSession(GameSession *session)
{
	m_globalSessionMutex.Acquire();
	m_globalSessions.erase(session);
	m_globalSessionMutex.Release();
}

void Park::SetInitialWorldSettings()
{
    m_ServerGlobalID = Config.GlobalConfig.GetIntDefault( "Global", "ServerID", 0 );
    m_PlatformID = m_ServerGlobalID / 10000;

	m_lastTick = time(NULL);
	//sWorldLog ;
	// TODO: clean this
	time_t tiempo;
	char hour[3];
	char minute[3];
	char second[3];
	struct tm *tmPtr;
	tiempo = time(NULL);
	tmPtr = localtime(&tiempo);
	strftime( hour, 3, "%H", tmPtr );
	strftime( minute, 3, "%M", tmPtr );
	strftime( second, 3, "%S", tmPtr );
	m_gameTime = (3600*atoi(hour))+(atoi(minute)*60)+(atoi(second)); // server starts at noon
	
	sPbcRegister.Init();
	RegistProtobufConfig(&(sPbcRegister));

	//load���� �������ͷ���ǰ�棬�Է������õ�
	LoadLanguage();//������������
	LoadPlatformNameFromDB();

	// ��ʼ�����Կɼ����
	// Player::InitVisibleUpdateBits();
	// Monster::InitVisibleUpdateBits();
	// Partner::InitVisibleUpdateBits();

	// ��ʼ�����ܸ���
	SkillHelp::InitAttackableTargetCheckerTable();

	// ������Ϣ��ʼ����ǰ��
	sSceneMapMgr.Init();

	objmgr.LoadBaseData();
	sDBConfLoader.LoadDBConfig();

	sLog.outString("Create Player Buffer...");
	objPoolMgr.InitBuffer();

	sLog.outString("��ʼ��ͳ��ϵͳ...");
	sStatisticsSystem.Init();

	// ��������
	sInstanceMgr.Initialize();
}

void Park::Update(time_t diff)
{
    /* since time() is an expensive system call, we only update it once per server loop */
    // ��ʱ�������Ƶ��˴���ʹѭ���л�õ�ʱ��ֵ�������� by eeeo@2011.10.14

	m_TotalUpdate++;
	_CheckUpdateStart();
	eventholder->Update(diff);
	_CheckUpdateTime(UPDATETIME_EVENTHOLD);

	//DEBUG_LOG("Park", "InstanceMgr.Update()");
	sInstanceMgr.Update(diff);

	float ftime = diff/1000.0f;
	_CheckUpdateTime(UPDATETIME_INSTANCEMGR);

	sStatisticsSystem.Update(diff);
	_CheckUpdateTime(UPDATETIME_STATISTICS);

	_UpdateGameTime();

	// �������ͬ�����������ĳ�������������
	SyncPlayerCounterDataToMaster(diff);
}

void Park::SendGlobalMessage( WorldPacket *packet )
{
	// sGateCommHandler.SendServerPacket(packet, GAMESMSG_SENDGLOBALMSG);
}

// �����������һ��ȫ������
void Park::SendSystemBoardCastMessage(const string &strMessage, const uint32 location, const uint32 repeatCount, uint32 interval, bool castAtOnce)
{

}

void Park::SendChannelMessage(uint32 channel, uint32 guid, const string &msg, uint32 minLevel /* = 0 */, uint32 maxLevel /* = 0xFFFFFFFF */)
{
	ByteBuffer packet;
	packet << msg;
	SendPacketToChannel(channel, guid, packet, minLevel, maxLevel);
}

void Park::SendPacketToChannel(uint32 channel, uint32 guid, ByteBuffer &packet, uint32 minLevel /* = 0 */, uint32 maxLevel /* = 0xFFFFFFFF */)
{
	if(packet.rpos() == packet.wpos())
		return;

	/*WorldPacket transmitPacket(GAME_TO_CENTER_SERVER_MSG);
	transmitPacket << (uint32)0;
	transmitPacket << (uint16)STC_Server_msg_op_channel;
	transmitPacket << channel;
	transmitPacket << guid;
	transmitPacket << minLevel;
	transmitPacket << maxLevel;

	uint32 dataLen = packet.size() - packet.rpos();
	transmitPacket.append(packet.contents(packet.rpos()), dataLen);
	sGateCommHandler.TrySendPacket(&transmitPacket);*/
}

void Park::UpdateSessions(uint32 diff)
{
	SessionSet::iterator itr, it2;
	GameSession *session = NULL;
	int result;

	for(itr = m_globalSessions.begin(); itr != m_globalSessions.end();)
	{
		session = (*itr);
		it2 = itr;
		++itr;
		if(!session || session->GetInstance() != 0)
		{
			m_globalSessions.erase(it2);
			continue;
		}

		if((result = session->Update(0)))
		{
			if(result == 1)
			{
				uint64 guid = session->GetUserGUID();
				// complete deletion
				sLog.outString("session (guid="I64FMTD") index %u be removed in update!", guid, session->m_SessionIndex);
				DeleteSession(guid, session);
			}
			m_globalSessions.erase(it2);
		}
	}

	if (_queueSessions.empty())
		return ;
	ByteBuffer *dataBuffer = NULL;
	_queueSessionMutex.Acquire();
	map<uint64, tagQueueSessionInfo>::iterator it = _queueSessions.begin(), it_2;
	for (; it != _queueSessions.end();)
	{
		it_2 = it++;
		session = it_2->second.session;
		dataBuffer = it_2->second.plrData;
		if (session == NULL || dataBuffer == NULL)
		{
			_queueSessions.erase(it_2);
			continue;
		}

		const uint64 userGuid = session->GetUserGUID();
		GameSession * oldsession = FindSession(userGuid);
		if (oldsession == NULL)		// �ɻỰ�Ѿ���ɾ��,���Դ����µ�
		{
			// ����Lua����
			uint32 dataSize = dataBuffer->size();
			ScriptParamArray params;
			params << session->GetPlatformID();
			params << userGuid;
			params << (const void*)session;
			params.AppendUserData((void*)dataBuffer->contents(), dataSize);
			params << dataSize;

			// ����LuaPlayer����ʧ��
			if (!LuaScript.Call("CreatePlayer", &params, &params) || !params[0])
			{
				sLog.outError("GameSession(guid="I64FMTD")�ӳٵ��볡����ʧ��", userGuid);
				// ��ȡʧ�ܵĻ�,���������Ƴ�Player����
				objPoolMgr.deleteGameSession(session);
				SAFE_DELETE(dataBuffer);
				continue; 
			}

			// �ɹ����뵽����
			AddSession(userGuid, session);
			AddGlobalSession(session);
			session->SendCreateSuccessPacket();
			sLog.outError("GameSession(guid="I64FMTD")�ӳٳɹ����볡���� %u", userGuid, sGateCommHandler.m_sceneServerID);
			// �Ƴ�������
			SAFE_DELETE(dataBuffer);
			_queueSessions.erase(it_2);
		}
		else
		{
			if (!oldsession->bDeleted)
			{
				sLog.outError("�Ƿ��Ի�(guid="I64FMTD")����(SessionIndex=%u) �»Ự�ӳټ���ʧ��", userGuid, session->m_SessionIndex);
				objPoolMgr.deleteGameSession(session);
				SAFE_DELETE(dataBuffer);
				_queueSessions.erase(it_2);
				session = NULL;
				continue ;
			}
			else
			{
				// ��δ��ɾ��,�ȴ�
				sLog.outError("GameSession(guid="I64FMTD")�ɻỰ��ɾ��,����δ�Ƴ�,", userGuid);
				continue;
			}
		}
	}
	_queueSessionMutex.Release();

	_CheckUpdateTime(UPDATETIME_GAMESESSION);
}

void Park::SyncPlayerCounterDataToMaster(uint32 diff)
{
	if (m_mapPlayerCountSyncTimer > diff)
	{
		m_mapPlayerCountSyncTimer -= diff;
		return ;
	}
	m_mapPlayerCountSyncTimer = SCENE_MAP_PLAYER_COUNTER_SYNC_TIMER;
	
	//WorldPacket centrePacket(SCENE_2_MASTER_SERVER_MSG_EVENT);
	//centrePacket << uint32(0) << uint16(STC_Server_msg_op_plr_counter_sync);
	//centrePacket << sGateCommHandler.GetSelfSceneServerID();
	//uint32 counter = 0, curPos = centrePacket.size();
	//centrePacket << counter;
	//// �������ͬ������
	//for (map<uint32, int32>::iterator it = m_mapPlayerCountModifier.begin(); it != m_mapPlayerCountModifier.end(); ++it)
	//{
	//	if (it->second != 0)
	//	{
	//		centrePacket << it->first;
	//		centrePacket << it->second;
	//		it->second = 0;
	//		++counter;
	//	}
	//}

	//if (counter > 0)
	//{
	//	centrePacket.put(curPos, (uint8*)&counter, sizeof(uint32));
	//	sGateCommHandler.TrySendPacket(&centrePacket);
	//}
}

void Park::SceneServerChangeProc(GameSession *session, uint32 targetSceneID, const FloatPoint &targetPos, uint32 ret, uint32 destSceneServerID /* = 0xffffffff */)
{
	if (destSceneServerID == ILLEGAL_DATA_32)
		destSceneServerID = sInstanceMgr.GetHandledServerIndexBySceneID(targetSceneID);
	else
	{
		if (destSceneServerID != sGateCommHandler.GetSelfSceneServerID())
			ret = TRANSFER_ERROR_NONE_BUT_SWITCH;
	}

	if (destSceneServerID == ILLEGAL_DATA_32)
		ret = TRANSFER_ERROR_MAP;			// ����Ҳ���Ŀ�곡�����Ļ�����ʾ�����ͼ

	// ֪ͨ��Ӧ�����أ������׼���л�������
	WorldPacket packet(SMSG_SCENE_2_GATE_CHANGE_SCENE_NOTICE, 128);
	packet << session->GetUserGUID();
	packet << ret;

	if (ret == TRANSFER_ERROR_NONE || ret == TRANSFER_ERROR_NONE_BUT_SWITCH)
	{
		packet << targetSceneID << targetPos.m_fX << targetPos.m_fY << sSceneMapMgr.GetPositionHeight(targetSceneID, targetPos.m_fX, targetPos.m_fY);

		if (ret == TRANSFER_ERROR_NONE_BUT_SWITCH)
		{
			Player *plr = session->GetPlayer();
			// �鿴Ŀ���ͼ�ǲ��Ƿ���ͨ����
			MapInfo *targetMapInfo = sSceneMapMgr.GetMapInfo(targetSceneID);
			if (targetMapInfo != NULL)
			{
				// ����ǵ���ͨ����֮��Ĵ��ͣ���û��Ҫ����lastdata�ˡ�
				if (targetMapInfo->isNormalScene())
				{
					plr->GetLastMapData()->ClearData();
				}
				else
				{
					if(plr->isInNormalScene())
						plr->GetLastMapData()->SetData(plr->GetMapId(), plr->GetPositionX(), plr->GetPositionY());
					else
						plr->GetLastMapData()->SetData(sInstanceMgr.GetDefaultBornMap(), sInstanceMgr.GetDefaultBornPosX(), sInstanceMgr.GetDefaultBornPosY());
				}
			}

			if (plr->IsInWorld())
			{
				//plr->GetAuraWrap()->RemoveAuraByFlag(A_REMOVEFLAG_AS_OFFLINE, false);
				plr->RemoveFromWorld();
			}

			// ����session�ǳ�ʱ��
			session->SetLogoutTimer(200);
			session->SetNextSceneServerID(destSceneServerID);

			// ���������������(Ϊ�˷�����һ�����������з���)
			plr->SetMapId(targetSceneID);
			plr->SetPosition(targetPos.m_fX, targetPos.m_fY);
		}
	}
	session->SendServerPacket(&packet);
}

void Park::SaveAllPlayers()
{
	m_sessionlock.AcquireWriteLock();
	sLog.outString(" Saving %u players to DB at SaveAllPlayers()...", GetSessionCount());
	SessionMap::iterator it = m_sessions.begin();
	for (; it != m_sessions.end(); ++it)
	{
		it->second->LogoutPlayer(true);
	}
	m_sessionlock.ReleaseWriteLock();
}

void Park::PrintAllSessionStatusOnProgramExit()
{
	if (m_sessions.empty())
		return ;

	m_sessionlock.AcquireWriteLock();
	for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
	{
		// sLog.outError("Player(Platform %u AccId= %u) Still Online On ProgramExit", it->first, it2->first);
	}
	m_sessionlock.ReleaseWriteLock();
}

void Park::CreateSession(CSceneCommServer *pSock, WorldPacket &recvData)
{
	uint64 guid = 0;
	uint32 nSessionIndex = 0, gmflags = 0, enterTime, srcPlatformID = 0;
	string accountName, agentName;

	recvData >> guid >> nSessionIndex >> gmflags >> enterTime >> srcPlatformID >> agentName >> accountName;

	WorldPacket retPacket(SMSG_SCENE_2_GATE_ENTER_GAME_RET, 32);
	retPacket << guid;

	sLog.outString("[Park] ׼���������guid="I64FMTD",gmȨ��=%u,����:%s�˺���:%s sessionIndex=%u ��������:%u (��id=%u)", guid, gmflags, 
					agentName.c_str(), accountName.c_str(), nSessionIndex, pSock->associate_gateway_id, srcPlatformID);

	// �ȼ��ûỰ�Ƿ��Ѿ���sPark��ע����
	GameSession *pSession = FindSession(guid);
	bool bWaitAddSession = false;
	if (pSession != NULL)
	{
		//�˴���Ӧ�ó���
		sLog.outError("ERROR:��ͼ�����ظ�guid="I64FMTD" �ĻỰ SessionIndex=%u", guid, nSessionIndex);
		if(pSession->GetGateServerSocket() != pSock)
			pSession->Disconnect();				//���ı� 8.7����ӣ���������ص����
		InvalidSession(guid, 0, true);
		bWaitAddSession = true;
	}

	pSession = objPoolMgr.newSession();
	if (pSession == NULL)
	{
		sLog.outError("[Park] ����session����ʧ��,guid="I64FMTD"", guid);
		retPacket << uint32(P_ENTER_RET_CREATE_SESSION_FAILED);
		pSock->SendPacket(&retPacket);
		return;
	}

	// ����һ���Ự���󣬲�������ӵ�sPark��
	pSession->InitSession(accountName, srcPlatformID, agentName, pSock, nSessionIndex, guid, gmflags);
	size_t recvDataSize = recvData.size() - recvData.rpos();

	pSession->deleteMutex.Acquire();
	if (bWaitAddSession)
	{
		ByteBuffer *data = new ByteBuffer(256);
		data->append(recvData.contents(recvData.rpos()), recvDataSize);
		tagQueueSessionInfo info(pSession, data);

		// �Ự�ӳټ���
		_queueSessionMutex.Acquire();
		_queueSessions.insert(make_pair(guid, info));
		_queueSessionMutex.Release();
	}
	else
	{	
		ScriptParamArray params;
		params << srcPlatformID;
		params << guid;
		params << (const void*)pSession;
		params.AppendUserData((void*)recvData.contents(recvData.rpos()), recvDataSize);
		params << recvDataSize;

		// ����LuaPlayer����ʧ��
		if (!LuaScript.Call("CreatePlayer", &params, &params) || !params[0])
		{
			// ��ȡʧ�ܵĻ�,���������Ƴ�Player����
			objPoolMgr.deleteGameSession(pSession);
			retPacket << uint32(P_ENTER_RET_LOADDB_FAILED);
			pSock->SendPacket(&retPacket);
			return ;
		}

		Player *plr = pSession->GetPlayer();
		AddSession(guid, pSession);
		AddGlobalSession(pSession);
		pSession->SendCreateSuccessPacket();
		sLog.outString("GameSession(guid="I64FMTD",name:%s)�ڳ����� %u �������������", guid, plr->strGetGbkNameString().c_str(), sGateCommHandler.m_sceneServerID);
	}
	pSession->deleteMutex.Release();
}

void Park::SessioningProc(WorldPacket *recvData, uint64 plrGuid)
{
	m_sessionlock.AcquireWriteLock();
	GameSession *pSession = FindSession(plrGuid);
	if (pSession != NULL)
	{
		pSession->QueuePacket(recvData);
	}
	else
	{
		// sLog.outError("�յ������ڵ����(pID=%u,accID=%u)�ĻỰ���ݰ�(op=%u)", (uint32)recvData->GetOpcode());
		DeleteWorldPacket( recvData );
	}
	m_sessionlock.ReleaseWriteLock();
}

void Park::InvalidSession(uint64 plrGuid, uint32 sessionIndex, bool ignoreindex)
{
	m_sessionlock.AcquireWriteLock();
	GameSession *existSession = FindSession(plrGuid);
	if (existSession == NULL)
	{
		sLog.outError("��ҵĶԻ��Ѿ�������,�޷������Ƴ�,guid="I64FMTD", sessionIndex=%u", plrGuid, sessionIndex);
	}
	else
	{
		if (!ignoreindex && existSession->m_SessionIndex != sessionIndex)
		{
			sLog.outError("��ҵ�SessionIndex�Ѹı�(%u - %u),�޷������Ƴ�", existSession->m_SessionIndex, sessionIndex);
		}
		else
			existSession->Disconnect();
	}
	m_sessionlock.ReleaseWriteLock();
}

void Park::OnSocketClose(CSceneCommServer* sock)
{
	m_sessionlock.AcquireWriteLock();
	for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
	{
		if(it->second->GetGateServerSocket() == sock)
			it->second->SetGateServerSocket(NULL);
	}
	m_sessionlock.ReleaseWriteLock();
	sLog.outError("ERROR: ���ط����� %s �Ͽ�", sock->registeradderss.c_str());
}

uint32 Park::GetSessionCount()
{
	return m_sessions.size();
}

void Park::ShutdownClasses()
{
	cLog.Notice("InstanceMgr", "~InstanceMgr()");
	sInstanceMgr.Shutdown();

	sLog.outString("\nDeleting SceneMapMgr...");
	delete SceneMapMgr::GetSingletonPtr();

	sThreadMgr.Shutdown();
	delete DBConfigLoader::GetSingletonPtr();

	sLog.outString("\nDeleting StatisticsSystem...");
	delete StatisticsSystem::GetSingletonPtr();
	
	sLog.outString("Deleting LanguageMgr...");
	delete LanguageMgr::GetSingletonPtr();
	
	sLog.outString("Deleting InstanceMgr.. ");
	delete InstanceMgr::GetSingletonPtr();

	sLog.outString("Deleting SceneLuaTimer.. ");
	delete SceneLuaTimer::GetSingletonPtr();

	sLog.outString("Deleting PbcDataRegister.. ");
	delete PbcDataRegister::GetSingletonPtr();

	// ̫�ྲ̬�����ˣ���������
	delete ObjectMgr::GetSingletonPtr();
	delete ObjectPoolMgr::GetSingletonPtr();
}

void Park::Rehash(bool load)
{
	// load regeneration rates.

	SetPlayerLimit(sDBConfLoader.GetStartParmInt("playerlimit"));
	printf("current playerlimit %d\n",GetPlayerLimit());
	bool log_enabled = sDBConfLoader.GetStartParmInt("logcheaters")>0;

#ifdef _WIN64
	DWORD current_priority_class = GetPriorityClass(GetCurrentProcess());
	bool high = sDBConfLoader.GetStartParmInt("adjustpriority")>0;

	if(current_priority_class == HIGH_PRIORITY_CLASS && !high)
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	else if(current_priority_class != HIGH_PRIORITY_CLASS && high)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

	flood_lines = sDBConfLoader.GetStartParmInt("floodlines");
	flood_seconds =sDBConfLoader.GetStartParmInt("floodseconds");
	flood_message =sDBConfLoader.GetStartParmInt("sendmessage")>0;

	if(!flood_lines || !flood_seconds)
		flood_lines = flood_seconds = 0;

}

void Park::PrintDebugLog()
{
	if(m_TotalUpdate==0)
		return;
	for(int i = UPDATETIME_GAMESESSION;i<UPDATETIME_MAXCOUNT;++i)
	{
		sLog.outString("%s MaxUpdateTime %d AverageTime %d",g_updateclassname[i],m_ClassUpdateMaxTime[i],m_ClassUpdateTime[i]/m_TotalUpdate);
	}
	memset(m_ClassUpdateTime,0,15*sizeof(uint32));
	memset(m_ClassUpdateMaxTime,0,15*sizeof(uint32));
	m_TotalUpdate=0;
}

void Park::_CheckUpdateStart()
{
	_CheckStartMsTime = getMSTime64();
	++m_TotalUpdate;
}

void Park::_CheckUpdateTime(uint32 classenum)
{
	uint32 diff = getMSTime64() - _CheckStartMsTime;
	m_ClassUpdateTime[classenum] += diff;
	m_ClassUpdateMaxTime[classenum] = diff > m_ClassUpdateMaxTime[classenum] ? diff : m_ClassUpdateMaxTime[classenum];
	_CheckStartMsTime = getMSTime64();
}

void Park::DailyEvent()
{

}

void Park::LoadLanguage()
{
	/*QueryResult * reslut = WorldDatabase.Query("Select * from language");
	if ( reslut != NULL )
	{
        cLog.Success( "Language Setting", "%u pairs loaded.", reslut->GetRowCount() );

        std::string key;
        std::string words;
        uint32 id = 0;
		Field * fields = reslut->Fetch();
		do
		{
			words = fields[2].GetString();
			if ( !words.empty() )
            {
                id = fields[0].GetUInt32();
                key = fields[1].GetString();
				sLang.SetTranslate( id, words.c_str() );

				if ( !key.empty() )
                {
                    if ( sLang.CompareStructure( key, words ) )
                    {
                        sLang.SetTranslate( key.c_str(), words.c_str() );
                    }
                    else
                    {
                        sLog.outError( "Language setting value ( %u, %s, %s ) doesn't match.", id, key.c_str(), words.c_str() );
                    }
                }
			}
		}
		while ( reslut->NextRow() );

		delete reslut;
	}*/
}

void Park::LoadPlatformNameFromDB()
{
	m_platformNames.clear();
	
	Json::Value j_root;
	Json::Reader j_reader;
	int32 ret = JsonFileReader::ParseJsonFile(strMakeJsonConfigFullPath("platform.json").c_str(), j_reader, j_root);

#ifdef _WIN64
	ASSERT(ret == 0 && "��ȡplatform��Ϣ����");
#else
	if (ret != 0)
	{
		sLog.outError("��ȡplatform�����ݳ���");
		return ;
	}
#endif // _WIN64

	for (Json::Value::iterator it = j_root.begin(); it != j_root.end(); ++it)
	{
		uint32 platformId = (*it)["platform_id"].asUInt();
		string platformName = (*it)["platform_name"].asString();

		m_platformNames.insert(make_pair(platformId, platformName));
	}

	cLog.Notice("ƽ̨����", "����ȡ%u��ƽ̨��", (uint32)m_platformNames.size());
}

std::string Park::GetPlatformNameById(uint32 platformId)
{
	if (platformId == 0)
		platformId = m_PlatformID;
	std::map< uint32, std::string >::iterator itr = m_platformNames.find( platformId );
	if ( itr != m_platformNames.end() )
	{
		return itr->second;
	}
	return "PlatformName";
}

void Park::OnPlayerEnterMap(uint32 mapID)
{
	++m_mapPlayerCountModifier[mapID];
}

void Park::OnPlayerLeaveMap(uint32 mapID)
{
	--m_mapPlayerCountModifier[mapID];
}


