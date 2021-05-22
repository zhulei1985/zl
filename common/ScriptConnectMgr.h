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
#include <unordered_map>
#include <string>
#include "ZLScript.h"


void InitNetworkConnect();

int SetListenPort2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
int SetListenWebSocket2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
int NewConnector2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
int RemoveConnector2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState);
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
	CScriptConnector* NewConnector(const char* pIP, int Port,std::string strType, std::string strPassword);
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
	static void SetInitConnectInfo(int nPort, std::string strType, std::string strPassword, 
									std::string strScript, std::string strDisconnectScript);

	void SetSockectConnector(CScriptConnector* pConnector);
	CScriptConnector* GetSocketConnector(__int64 id);
	void RemoveSocketConnect(__int64 id);
public:
	struct tagConnectInit
	{
		std::string strType;
		std::string strPassword;

		std::string strInitScript;
		std::string strDisconnectScript;
	};
private:
	static std::map<int, tagConnectInit> m_mapInitConnectInfo;
	std::map<__int64, zlscript::PointVarInfo> m_mapScriptConnector;
	std::list<__int64> m_listRemoveScriptConnect;
	//std::mutex m_lockScriptConnector;

	//同步类的判断，让不同连接创建的同一同步类的镜像不会重复建立
public:
	struct stSyncInfo
	{
		stSyncInfo(int serverid, __int64 classid)
		{
			nRootServerID = serverid;
			nRootClassID = classid;
		}
		bool operator<(const struct stSyncInfo& right)const   //重载<运算符
		{
			if (this->nRootServerID == right.nRootServerID && this->nRootClassID == right.nRootClassID)     //根据id去重
				return false;
			else
			{
				if (this->nRootServerID != right.nRootServerID)
				{
					return this->nRootServerID < right.nRootServerID;      //降序
				}
				else
				{
					return this->nRootClassID < right.nRootClassID;
				}
			}
		}
		int nRootServerID;
		__int64 nRootClassID;
	};
	__int64 GetSyncIndex(int serverID, __int64 id);
	void SetSyncIndex(int serverID, __int64 id, __int64 imageId);
	void RemoveSyncIndex(int serverID, __int64 id);

	__int64 AssginSyncProcessID(__int64 connectid);
	void SetConnectID2ProcessID(__int64 connectid, __int64 processid);
	__int64 GetConnectIDFromProcessID(__int64 processid);
	void removeSyncProcessID(__int64 processid);
private:
	//first 同步类根节点数据 second 同步类本节点ID
	std::map<struct stSyncInfo, __int64> m_mapSyncPointInfo;
	std::mutex m_lockSyncInfo;

	//同步通道ID分配计数
	__int64 m_nSyncProcessIDCount;
	//同步通道ID与连接ID的对应
	std::unordered_map<__int64, __int64> m_mapSyncProcessID;
	std::mutex m_lockSyncProcess;
};

