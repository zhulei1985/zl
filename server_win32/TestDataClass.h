#pragma once

#include "ZLScript.h"
#include "ScriptPointInterface.h"
using namespace zlscript;

class CTestDataClass : public CScriptPointInterface
{
public:
	CTestDataClass();
	~CTestDataClass();
public:
	static void Init2Script();

	//int Fun12Script(CScriptRunState* pState);
	CLASS_SCRIPT_FUN(CTestDataClass, Fun1);
public:
	ATTR_INT(varInt, 1);


	virtual bool AddAllData2Bytes(std::vector<char>& vBuff, std::vector<PointVarInfo>& vOutClassPoint);
	virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len, std::vector<PointVarInfo>& vOutClassPoint);
};