#pragma once

#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
using namespace zlscript;

class CTestClass : public CSyncScriptPointInterface
{
public:
	int Fun1(CScriptStack& ParmInfo);
	int Fun12Script(CScriptRunState* pState);
};