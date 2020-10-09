#pragma once
/****************************************************************************
	Copyright (c) 2020 ZhuLei
	Email:zhulei1985@foxmail.com

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
 ****************************************************************************/

#include "SocketConnectorMgr.h"
#include <chrono>
#include <map>
#include <string>
#include "ZLScript.h"


void InitNetworkConnect();

int SetListenPort2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
int SetListenWebSocket2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
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

