#pragma once

#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
using namespace zlscript;

class CTestClass : public CSyncScriptPointInterface
{
public:
	CTestClass();
	~CTestClass();
public:
	static void Init2Script();

	int Fun12Script(CScriptRunState* pState);

public:
	//virtual bool AddAllData2Bytes(std::vector<char> vBuff);
	//virtual bool DecodeData4Bytes(char* pBuff, int& pos, int len);
};