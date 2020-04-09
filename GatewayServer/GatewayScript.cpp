#include "stdafx.h"

#include "script/Lunar.h"
#include "GatewayScript.h"
#include "OpDispatcher.h"

const char* GatewayScript::ms_szGatewayScriptInitializeFunc = "GatewayServerInitialize";
const char* GatewayScript::ms_szGatewayScriptTerminateFunc = "GatewayServerTerminate";

GatewayScript::GatewayScript(void)
{

}

GatewayScript::~GatewayScript(void)
{

}

bool GatewayScript::registLocalLibs()
{
	// ע��һЩ�ű���Ҫ�õ��Ľӿ���
	Lunar<OpDispatcher>::Register(m_pLua);

	return true;
}

bool GatewayScript::callInitialize()
{
	bool bRet = true;
	if (m_bInitialize && isFunctionExists(GatewayScript::ms_szGatewayScriptInitializeFunc))
	{
		ScriptParamArray args;
		bRet = Call(GatewayScript::ms_szGatewayScriptInitializeFunc, &args, &args);
	}
	else
	{
		bRet = false;
	}
	return bRet;
}

bool GatewayScript::callTerminate()
{
	bool bRet = true;
	if (!m_bInitialize)
		return bRet;

	if (isFunctionExists(GatewayScript::ms_szGatewayScriptTerminateFunc))
	{
		ScriptParamArray args;
		bRet = Call(GatewayScript::ms_szGatewayScriptTerminateFunc, &args, &args);
	}
	else
	{
		bRet = false;
	}

	return bRet;
}
