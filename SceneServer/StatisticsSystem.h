#ifndef STATISTICSSYSTEM_HEAD_FILE
#define STATISTICSSYSTEM_HEAD_FILE

class Instance;
class Player;

class StatisticsSystem : public Singleton<StatisticsSystem>
{
public:
	StatisticsSystem();
	~StatisticsSystem();

private:
	tm m_tmLastOnlineUpdate;          //������������
	tm m_tmStatDataTime;              //��Ϸʱ��ͳ��
	bool m_bNewRecord;                //Insert����Update

public:
	void Init();                      //��ʼ��
	void DailyEvent();                //�ճ��¼�
	void WeeklyEvent();               //һ���¼�
	void SaveAll();
	void Update(time_t diff);
	
	void OnShutDownServer();//�ڹر�֮ǰͳ��һЩ����

	void OnInstanceCreated(Instance *in);
	void OnInstancePassed(Instance *in);
	void OnPlayerLeaveUndercity(Instance *in, Player *plr);
};

#define sStatisticsSystem StatisticsSystem::GetSingleton()

#endif
