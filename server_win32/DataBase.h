#pragma once

#include "ZLScript.h"
#include "ScriptPointInterface.h"
#include "winsock2i.h"
#include <mysql.h>
using namespace zlscript;

class CDataBase : public CScriptPointInterface
{
public:
	CDataBase();
	~CDataBase();
public:
	static void Init2Script();

	int InitDB2Script(CScriptRunState* pState);
	int InitTable2Script(CScriptRunState* pState);
	int Query2Script(CScriptRunState* pState);
	int Save2Script(CScriptRunState* pState);
	int Ping2Script(CScriptRunState* pState);
	int Close2Scipt(CScriptRunState* pState);
private:
	MYSQL mysql;
	std::mutex m_Lock;
};
