#pragma once
#include "SocketConnectorMgr.h"
#include <chrono>
#include <map>
#include <string>
#include "ZLScript.h"


void InitNetworkConnect();

int SetListenPort2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
int NewConnector2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
int GetConnector2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);

using namespace zlnetwork;
class CScriptConnector;
class CScriptConnectMgr : public CSocketConnectorMgr
{
public:
	CScriptConnectMgr();
	~CScriptConnectMgr();

	CSocketConnector* Create()
	{
		return nullptr;
	}
	                                            
	static CSocketConnector* CreateNew(SOCKET sRemote,  const char* pIP, int nPort);

	virtual void OnProcess();

public:
	CScriptConnector* GetConnector(__int64 nID);
	CScriptConnector* NewConnector(const char* pIP, int Port);
public:
	std::chrono::time_point<std::chrono::steady_clock> m_LastSendMsgTime;
public:
	static CScriptConnectMgr* GetInstance()
	{
		return &s_Instance;
	}
private:
	static CScriptConnectMgr s_Instance;

	//通过监听端口建立的连接的初始化脚本
public:
	static void SetInitConnectScript(int nPort,std::string strScript);
private:
	static std::map<int, std::string> m_mapInitConnectScript;
};

