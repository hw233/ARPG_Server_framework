#pragma once

#include "script/BaseLuaScript.h"
#include "script/ScriptParamArray.h"

class SceneScript : public BaseLuaScript
{
public:
	SceneScript(void);
	virtual ~SceneScript(void);

public:
	static const char* ms_szSceneScriptInitializeFunc;
	static const char* ms_szSceneScriptTerminateFunc;

public:
	virtual bool registLocalLibs();
	virtual bool callInitialize();			// ���ýű���ʼ������
	virtual bool callTerminate();
};

