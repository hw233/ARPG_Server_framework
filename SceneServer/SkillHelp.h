#ifndef _SKILL_HELP_H_
#define _SKILL_HELP_H_

// ����Ϊlua�ṩ���ܸ����жϵ���
#include "script/Lunar.h"

// ��(20,20)ΪԲ��,�����뾶20��(640����)��Ŀ��
#define SKILL_HELP_MAX_CHECK_GRID	41

class SkillHelp
{
public:
	SkillHelp(void);
	~SkillHelp(void);

public:
	// ������lua��
	static int fillMonsterGuidsAroundObject(lua_State *l);
	static int fillObjectGuidsAroundObject(lua_State *l);
	static int fillPlayerGuidsAroundObject(lua_State *l);

	static int findObjectAssaultTargetPos(lua_State *l);
	static int checkSectorTargetsValidtiy(lua_State *l);		// �������Ŀ�����Ч��

public:
	static void Push64BitNumberArray(lua_State *l, void *arr, const uint32 arrSize);
	static float CalcuDistance(float startX, float startY, float endX, float endY);
	static float CalcuDirection(float srcX, float srcY, float dstX, float dstY);
	static const uint32 CalcuDireciton16(float fDir);
	static int32 CheckSectorTargetsValidtiy(MapMgr *mgr, float pixelPosX, float pixelPosY, float dir, float sectorAngle, float sectorRadius, 
											vector<uint64> &inGuids, vector<uint64> &outGuids);
	
	static void InitAttackableTargetCheckerTable();
	// ��ʼ������Ŀ�������ݱ�
	static void InitSectorTargetCheckerTable();
	// ��ʼ������Ŀ�������ݱ�
	static void InitRectangleTargetCheckerTable();


	LUA_EXPORT(SkillHelp)

private:
	// �������ι�����16�������,��[20,20]Ϊ���ĵ�,�����ĳ������Ϊ20�ĽǶȡ���������
	static float sectorRadianArray[GDirection16_Count][SKILL_HELP_MAX_CHECK_GRID][SKILL_HELP_MAX_CHECK_GRID];
	static float sectorDistanceArray[SKILL_HELP_MAX_CHECK_GRID][SKILL_HELP_MAX_CHECK_GRID];
	static const int snTargetCheckerCoordCentrePosX = 20;
	static const int snTargetCheckerCoordCentrePosY = 20;
};


#endif // !_SKILL_HELP_H_



