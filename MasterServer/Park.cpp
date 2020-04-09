#include "stdafx.h"
#include "Park.h"
#include "ObjectPoolMgr.h"
#include "convert/strconv.h"
#include "MongoServerData.h"

Park::Park(void)
{
    m_pass_min_today = 0;
}

Park::~Park(void)
{

}

void Park::Rehash(bool load /* = false */)
{
	SetPlayerLimit(sDBConfLoader.GetStartParmInt("playerlimit"));
	printf("current playerlimit %d\n", GetPlayerLimit());

#ifdef _WIN64
	DWORD current_priority_class = GetPriorityClass(GetCurrentProcess());
	bool high = sDBConfLoader.GetStartParmInt("adjustpriority")>0;

	if(current_priority_class == HIGH_PRIORITY_CLASS && !high)
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	else if(current_priority_class != HIGH_PRIORITY_CLASS && high)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
	// ����ˢ�¿���
	flood_lines = sDBConfLoader.GetStartParmInt("floodlines");
	flood_seconds = sDBConfLoader.GetStartParmInt("floodseconds");
	flood_message = sDBConfLoader.GetStartParmInt("sendmessage")>0;

	if(!flood_lines || !flood_seconds)
		flood_lines = flood_seconds = 0;
}

void Park::Initialize()
{
    m_ServerGlobalID = Config.GlobalConfig.GetIntDefault( "Global", "ServerID", ILLEGAL_DATA_32 );
    if ( m_ServerGlobalID != ILLEGAL_DATA_32 )
    {
		m_ServerIndex = m_ServerGlobalID % 10000;
        m_PlatformID = m_ServerGlobalID / 10000;
    }

	// ��ȡserver_setting������Ϣ�����ݱ���������Ϣ
	sDBConfLoader.LoadDBConfig();

	// ������������߱�����ó�0

	// GameDatabase.Execute("UPDATE character_dynamic_info SET online_flag = 0");
	MongoGameDatabase.update(sDBConfLoader.GetMongoTableName("character_datas"), mongo::Query(), BSON("$set" << BSON("online_flag" << 0)), false, true);

	// ɾ������־
	_DeleteOldDatabaseLogs();

	// ע��pb����
	sPbcRegister.Init();
	RegistProtobufConfig(&(sPbcRegister));

	// ���ø������ݱ��guid
	// sPlayerMgr.LoadHighestGuid();
	objmgr.SetHighestGuids();

	// ���������
	objPoolMgr.InitBuffer();
	
	// �ྲ̬������ʼ��
	// PlayerEntity::InitPlayerEntityVisibleUpdateMask();

	// ��ȡ��������
    LoadLanguage();

	// ��ȡƽ̨��Ϣ����
	LoadPlatformNameFromDB();

	// ������Ϣ��ʼ��
	sSceneMapMgr.Init();

	// ��ȡ������������
	LoadSceneMapControlConfig();

	// ��ȡ��̬���ö���
	objmgr.LoadBaseData();
	
	// ��ҹ���ģ���ʼ��
	sPlayerMgr.Initialize();
}

void Park::Update(uint32 diff)
{
}

void Park::SaveAllPlayers()
{
	sPlayerMgr.SaveAllOnlineServants();
}

void Park::ShutdownClasses()
{
	sThreadMgr.Shutdown();
	sLog.outString("\nDeleting SceneMapMgr...");
	delete SceneMapMgr::GetSingletonPtr();

	sLog.outString("delete PlayerMgr..");
	delete CPlayerMgr::GetSingletonPtr();

	sLog.outString("delete MongoServerDataMgr..");
	delete MongoServerData::GetSingletonPtr();

	delete LanguageMgr::GetSingletonPtr();
	delete PbcDataRegister::GetSingletonPtr();
	delete DBConfigLoader::GetSingletonPtr();
	
	sLog.outString("Deleting MasterLuaTimer.. ");
	delete MasterLuaTimer::GetSingletonPtr();

	// ���ж����һЩ���ݵ����ɾ��
	delete ObjectMgr::GetSingletonPtr();
	delete ObjectPoolMgr::GetSingletonPtr();
}

void Park::LoadLanguage()
{
    /*QueryResult * reslut = WorldDatabase.Query( "Select * from language" );
    if ( reslut != NULL )
    {
        cLog.Success( "Language Setting", "%u pairs loaded.", reslut->GetRowCount() );
        Field * fields = reslut->Fetch();
        std::string key;
        std::string words;
        std::string::size_type key_pos = 0;
        std::string::size_type key_len = 0;
        std::string::size_type word_pos = 0;
        std::string::size_type word_len = 0;
        uint32 id = 0;
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
        while (reslut->NextRow());

        delete reslut;
    }*/
}

void Park::_DeleteOldDatabaseLogs()
{
	// ɾ��������������ˮ
	string oneMonthDateTime = ConvertTimeStampToMYSQL(UNIXTIME - TIME_MONTH);
	string twoMonthDateTime = ConvertTimeStampToMYSQL(UNIXTIME - TIME_MONTH * 2);
	string threeMonthDateTime = ConvertTimeStampToMYSQL(UNIXTIME - TIME_MONTH * 3);

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

bool Park::isValidablePlayerName(string& utf8_name)
{
	bool bRet = true;
	// ˼·���Ƚ�����ת��Ϊgbk�ĸ�ʽ�����������м���ַ��Ƿ�Ϊ���֡�Ӣ�ĵ�
	CUTF82C *utf8 = new CUTF82C(utf8_name.c_str());
	string strGBK = utf8->c_str();
	delete utf8;

	int len = strGBK.size();
	short high, low;
	unsigned int code;
	string s;
	for(int i = 0; i < len; i++)
	{
		if((strGBK[i] >=0) || (i==len-1))
			continue;
		else
		{
			//�������
			high = (short)(strGBK[i] + 256);
			low = (short)(strGBK[i + 1] + 256);
			code = high * 256 + low;

			//��ȡ�ַ�
			s = "";
			s += strGBK[i];
			s += strGBK[i + 1];
			i++;
			if( (code>=0xB0A1 && code<=0xF7FE) || 
				(code>=0x8140 && code<=0xA0FE) || 
				(code>=0xAA40 && code<=0xFEA0) )
			{
				continue;
			}
			else
			{
				bRet = false;
				break;
			}
		}
	}

	return bRet;
}

void Park::LoadSceneMapControlConfig(bool reloadFlag /*= false*/)
{
	m_sceneControlConfigs.clear();

	Json::Value j_root;
	Json::Reader j_reader;
	int32 ret = JsonFileReader::ParseJsonFile(strMakeJsonConfigFullPath("map_control_config.json").c_str(), j_reader, j_root);

#ifdef _WIN64
	ASSERT(ret == 0 && "��ȡserver_settings��Ϣ����");
#else
	if (ret != 0)
	{
		sLog.outError("��ȡserver_settings�����ݳ���");
		return ;
	}
#endif // _WIN64
	
	ASSERT(j_root.size() > 0 && "��ǰ�������κγ���������������");
	
	map<uint32, uint32> loadedSceneIDs;

	for (Json::Value::iterator it = j_root.begin(); it != j_root.end(); ++it)
	{
		uint32 sceneServerID = (*it)["scene_server_id"].asUInt();
		if (m_sceneControlConfigs.find(sceneServerID) == m_sceneControlConfigs.end())
			m_sceneControlConfigs.insert(make_pair(sceneServerID, set<uint32>()));

		ASSERT((*it)["map_ids"].isArray());
		Json::Value mapIDs = (*it)["map_ids"];

		for (int i = 0; i < mapIDs.size(); i++)
		{
			if (!mapIDs[i].isUInt())
			{
				sLog.outError("���õĳ���id(%s)����ȷ", mapIDs[i].asString().c_str());
				continue;
			}
			uint32 sceneID = mapIDs[i].asUInt();

			m_sceneControlConfigs[sceneServerID].insert(sceneID);

			// ������޶�����ϳ���������ͬһ������
			if (loadedSceneIDs.find(sceneID) != loadedSceneIDs.end())
			{
				sLog.outError("����%u �ѱ��� %u �ų����������޷��ٱ� %u �ų���������", sceneID, loadedSceneIDs[sceneID], sceneServerID);
				if (!reloadFlag)
					ASSERT(false && "ͬһ�������������������");
			}
			else
				loadedSceneIDs.insert(make_pair(sceneID, sceneServerID));
		}
	}
}

const uint32 Park::GetHandledServerIndexBySceneID(uint32 sceneID)
{
	uint32 uRet = ILLEGAL_DATA_32;
	for (map<uint32, set<uint32> >::iterator it = m_sceneControlConfigs.begin(); it != m_sceneControlConfigs.end(); ++it)
	{
		if (it->second.find(sceneID) != it->second.end())
		{
			uRet = it->first;
			break;
		}
	}
	return uRet;
}

uint32 Park::GetSceneIDOnPlayerLogin(uint32 targetMapID)
{
	uint32 uRet = 0;
	if (targetMapID == Scene_map_test1001 && !m_sceneServerMapPlayerCounter.empty())
	{
		// ���ֳ�����ÿ��100����䣬���ÿ�߾�����100�ˣ�����ѡ���ٵ���·����
		std::map<uint32, ScenePlayerCounterMap>::reverse_iterator rIter = m_sceneServerMapPlayerCounter.rbegin();
		int32 minPlayerCounter = 10000000;
		for (; rIter != m_sceneServerMapPlayerCounter.rend(); ++rIter)
		{
			ScenePlayerCounterMap::iterator it2 = rIter->second.find(targetMapID);
			if (it2 == rIter->second.end())
			{
				uRet = rIter->first;
				break;
			}
			else
			{
				int32 curPlayerCounter = it2->second;
				if (curPlayerCounter < 100)
				{
					uRet = rIter->first;
					break;
				}
				else
				{
					if (curPlayerCounter < minPlayerCounter)
					{
						minPlayerCounter = curPlayerCounter;
						uRet = rIter->first;
					}
				}
			}
		}
	}
	else
	{
		uRet = GetHandledServerIndexBySceneID(targetMapID);
	}

	return uRet;
}

uint32 Park::GetMinPlayerCounterServerIndex(uint32 defaultServerIndex)
{
	uint32 uRet = ILLEGAL_DATA_32;
	int32 minPlayerCount = 10000000;		// ����һ�������������������еĵ�һ��Ԫ��ֵһ������С
	for (map<uint32, int32>::iterator it = m_sceneServerPlayerCounter.begin(); it != m_sceneServerPlayerCounter.end(); ++it)
	{
		if (it->second < minPlayerCount)
		{
			minPlayerCount = it->second;
			uRet = it->first;
		}
	}
	
	return uRet == ILLEGAL_DATA_32 ? defaultServerIndex : uRet;
}

bool Park::IsValidationSceneServerIndex(uint32 serverIndex)
{
	// ͨ�����ó�������������������Ƿ�������ע���������Ƿ�Ϊһ����Ч�ĳ�����
	return m_sceneServerPlayerCounter.find(serverIndex) == m_sceneServerPlayerCounter.end() ? false : true;
}

void Park::UpdatePlayerMapCounterData(uint32 serverIndex, WorldPacket &recvData)
{
	if (m_sceneServerMapPlayerCounter.find(serverIndex) == m_sceneServerMapPlayerCounter.end())
		m_sceneServerMapPlayerCounter.insert(make_pair(serverIndex, ScenePlayerCounterMap()));

	uint32 counter, mapID;
	int32 modifyCounter, modifyCounterSummary = 0;
	ScenePlayerCounterMap::iterator it;

	recvData >> counter;
	for (int i = 0; i < counter; ++i)
	{
		recvData >> mapID >> modifyCounter;
		modifyCounterSummary += modifyCounter;
		it = m_sceneServerMapPlayerCounter[serverIndex].find(mapID);
		if (it != m_sceneServerMapPlayerCounter[serverIndex].end())
			it->second += modifyCounter;
		else
			m_sceneServerMapPlayerCounter[serverIndex].insert(make_pair(mapID, modifyCounter));
	}

	if (m_sceneServerPlayerCounter.find(serverIndex) == m_sceneServerPlayerCounter.end())
		m_sceneServerPlayerCounter.insert(make_pair(serverIndex, modifyCounterSummary));
	else
		m_sceneServerPlayerCounter[serverIndex] += modifyCounterSummary;
}

void Park::GetSceneServerConnectionStatus(WorldPacket &recvData)
{
	uint32 scene_counter, id;
	recvData >> scene_counter;
	set<uint32> valid_scenes;
	for (int i = 0; i < scene_counter; ++i)
	{
		recvData >> id;
		valid_scenes.insert(id);
	
		// ��������������������
		if (m_sceneServerMapPlayerCounter.find(id) == m_sceneServerMapPlayerCounter.end())
		{
			m_sceneServerMapPlayerCounter.insert(make_pair(id, ScenePlayerCounterMap()));
			sLog.outString("������ %u ������[����], �����[���]��Ӧ�ĵ�ͼ�������", id);
		}
		// ����������������
		if (m_sceneServerPlayerCounter.find(id) == m_sceneServerPlayerCounter.end())
			m_sceneServerPlayerCounter.insert(make_pair(id, 0));
	}

	map<uint32, ScenePlayerCounterMap>::iterator it = m_sceneServerMapPlayerCounter.begin(), it2;
	for (; it != m_sceneServerMapPlayerCounter.end();)
	{
		it2 = it++;
		if (valid_scenes.find(it2->first) == valid_scenes.end())
		{
			sLog.outError("������ %u ������[�Ͽ�], �����[�Ƴ�]��Ӧ�ĵ�ͼ�������", it2->first);
			it2->second.clear();

			if (m_sceneServerPlayerCounter.find(it2->first) != m_sceneServerPlayerCounter.end())
				m_sceneServerPlayerCounter.erase(it2->first);
			
			m_sceneServerMapPlayerCounter.erase(it2);
		}
	}
}

