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

	CLASS_SCRIPT_FUN(CAccount, SetAccountName);
	CLASS_SCRIPT_FUN(CAccount, GetAccountName);
	CLASS_SCRIPT_FUN(CAccount, SetPassword);
	CLASS_SCRIPT_FUN(CAccount, GetPassword);

	CLASS_SCRIPT_SYNC_FUN(CAccount,SetNickName);
	CLASS_SCRIPT_FUN(CAccount, GetNickName);

	CLASS_SCRIPT_FUN(CAccount, GetVal);
	CLASS_SCRIPT_FUN(CAccount, SetVal);

	CLASS_SCRIPT_FUN(CAccount, MakeConnectString);
public:
	////virtual bool AddAllData2Bytes(std::vector<char>& vBuff);
	////virtual bool DecodeData4Bytes(char* pBuff, int& pos, int len);

private:
	ATTR_SYNC_AND_DB_PRIMARY_INT64(Id, 1);
	ATTR_SYNC_AND_DB_UNIQUE_STR(strAccountName, 2);
	ATTR_SYNC_AND_DB_STR(strNickName,3);
	ATTR_DB_STR(strPassword,4);

private:
	std::unordered_map<std::string, StackVarInfo> m_mapData;
};