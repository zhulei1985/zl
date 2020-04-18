#pragma once
#include "SocketConnectorMgr.h"
#include <chrono>
#include "ZLScript.h"


void InitNetworkConnect();

int SetListenPort2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);

using namespace zlnetwork;
class CClientConnectMgr : public CSocketConnectorMgr
{
public:
	CClientConnectMgr();
	~CClientConnectMgr();

	CSocketConnector* Create()
	{
		return nullptr;
	}

	static CSocketConnector* CreateNew(SOCKET sRemote,  const char* pIP);

	virtual void OnProcess();

	std::chrono::time_point<std::chrono::steady_clock> m_LastSendMsgTime;
public:
	static CClientConnectMgr* GetInstance()
	{
		return &s_Instance;
	}
private:
	static CClientConnectMgr s_Instance;
};

