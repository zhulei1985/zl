#pragma once

#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
using namespace zlscript;

class CAccount : public CSyncScriptPointInterface
{
public:
	CAccount();
	~CAccount();
public:
	static void Init2Script();

	int SetAccountName2Script(CScriptRunState* pState);
	int GetAccountName2Script(CScriptRunState* pState);
	int SetPassword2Script(CScriptRunState* pState);
	int GetPassword2Script(CScriptRunState* pState);

	int SetNickName2Script(CScriptRunState* pState);
	int GetNickName2Script(CScriptRunState* pState);

	int SetConnect2Script(CScriptRunState* pState);
	int GetConnect2Script(CScriptRunState* pState);
public:
	virtual bool AddAllData2Bytes(std::vector<char>& vBuff);
	virtual bool DecodeData4Bytes(char* pBuff, int& pos, int len);

private:
	std::string strNickName;

	std::string strAccountName;
	std::string strPassword;

	__int64 nConnectPointIndex;
};