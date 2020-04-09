#include "stdafx.h"
#include "SceneLuaTimer.h"

SceneLuaTimer::SceneLuaTimer(void)
{

}


SceneLuaTimer::~SceneLuaTimer(void)
{

}

void SceneLuaTimer::onTimeOut(uint32 *pEventIDs, const uint32 eventCount)
{
	// ����lua�Ķ�ʱ������
	if (eventCount == 0)
		return ;

	ScriptParamArray params;
	if (eventCount == 1)
	{
		params << pEventIDs[0];
	}
	else
	{
		params.AppendArrayData(pEventIDs, eventCount);

#ifdef _WIN64
		ASSERT(params[0].getArraySize() == eventCount);
#endif // _WIN64

	}
	LuaScript.Call("OnEventTimeOut", &params, NULL);
}

