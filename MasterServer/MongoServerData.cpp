#include "stdafx.h"
#include "MongoServerData.h"

MongoServerData::MongoServerData(void)
{

}

MongoServerData::~MongoServerData(void)
{
	SetConnection(NULL);
}

void MongoServerData::Initialize(MongoConnector *conn)
{
	SetConnection(conn);

	__LoadServerDatas();
}

void MongoServerData::SaveResetTimeValue()
{
	DBClientConnection *conn = GetClientConnection();
	string tableName = sDBConfLoader.GetMongoTableName("server_datas");

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_weekly_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_weekly_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_dailies_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_daily_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_halfdaily_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_halfdaily_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_2hour_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_2hour_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_hour_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_hour_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_halfhour_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_halfhour_reset_time)), true);
	
	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_minute_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_minute_reset_time)), true);
}

void MongoServerData::EnsureAllIndex()
{

}

void MongoServerData::__LoadServerDatas()
{
	BSONObj bsonObj;
	
	// ÿ�ܸ���
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_weekly_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("�߼��߳�", "��ʼ���ճ�����..\n");
		last_weekly_reset_time = 0;
	}
	else
	{
		last_weekly_reset_time = bsonObj["setting_value"].numberLong();
	}

	// ÿ�ո���
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_dailies_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("�߼��߳�", "��ʼ���ճ�����..\n");
		last_daily_reset_time = 0;
	}
	else
	{
		last_daily_reset_time = bsonObj["setting_value"].numberLong();
	}

	// ���ո���
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_halfdaily_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("�߼��߳�", "��ʼ��12Сʱ����..\n");
		last_halfdaily_reset_time = 0;
	}
	else
	{
		last_halfdaily_reset_time = bsonObj["setting_value"].numberLong();
	}

	// ÿ2Сʱ����
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_2hour_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("�߼��߳�", "��ʼ��2Сʱ����..\n");
		last_2hour_reset_time = 0;
	}
	else
	{
		last_2hour_reset_time = bsonObj["setting_value"].numberLong();
	}

	// ÿ1Сʱ����
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_hour_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("�߼��߳�", "��ʼ��1Сʱ����..\n");
		last_hour_reset_time = 0;
	}
	else
	{
		last_hour_reset_time = bsonObj["setting_value"].numberLong();
	}

	// ÿ��Сʱ����
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_halfhour_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("�߼��߳�", "��ʼ��0.5Сʱ����..\n");
		last_halfhour_reset_time = 0;
	}
	else
	{
		last_halfhour_reset_time = bsonObj["setting_value"].numberLong();
	}

	// ÿ���Ӹ���
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_minute_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("�߼��߳�", "��ʼ��0.5Сʱ����..\n");
		last_minute_reset_time = 0;
	}
	else
	{
		last_minute_reset_time = bsonObj["setting_value"].numberLong();
	}
}
