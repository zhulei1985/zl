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

	int GetVal2Script(CScriptRunState* pState);
	int SetVal2Script(CScriptRunState* pState);

	int MakeConnectString2Script(CScriptRunState* pState);
public:
	////virtual bool AddAllData2Bytes(std::vector<char>& vBuff);
	////virtual bool DecodeData4Bytes(char* pBuff, int& pos, int len);

private:
	ATTR_INT64(Id);
	ATTR_STR(strNickName);
	ATTR_STR(strAccountName);
	//std::string strNickName;

	//std::string strAccountName;
	ATTR_STR(strPassword);

private:
	std::unordered_map<std::string, StackVarInfo> m_mapData;
};