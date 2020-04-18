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
#include <functional> 
#ifdef _WIN32
#include "IOCPConnector.h"
#pragma warning(disable : 4996) 
#else
#include<string.h>
#include "EpollConnector.h"
#endif
#include "nowin.h"
#include <map>
#include <mutex>
#include <atomic>

namespace zlnetwork
{
	//链接有2种模式，一种是使用ip和端口，链接到其他进程上去，称作client模式
	//一种是提供监听端口，等待其他进程链接过来，称作server模式
	class CSocketConnectorMgr// : public CSingle<CSocketConnectorMgr>
	{
	public:
		CSocketConnectorMgr();
		virtual ~CSocketConnectorMgr();

		virtual void OnInit();
		virtual void OnProcess();
		virtual void OnDestroy();

	public:
		bool SetListen(int nPort, CreateConnectorFun Fun);
		bool SendData(__int64 nConnectorID, char* pData, int len);
		bool SendData2All(char* pData, int len, __int64 except = -1);
	public:

		virtual bool AddClient(const char* pIP, int Port, __int64 nConnectID);
		virtual bool AddClient(CBaseConnector* pConn);

		virtual bool waitMsgAndClose();
		virtual bool kickAll();
		//这个Create是为client模式准备的
		virtual CBaseConnector* Create() = 0;
	public:
		//作为server模式，给IOCP提供的回调函数
		//static CSocketConnector* CreateNew(SOCKET sRemote,SOCKADDR_IN& saAdd);


		bool SetPassword(const char* pStr)
		{
			if (pStr == NULL)
			{
				return false;
			}
			strncpy(m_strPassword, pStr,sizeof(m_strPassword));
			return true;
		}
		bool GetCanConnect()
		{
			return m_bCanConnect;
		}
		void SetCanConnect(bool val)
		{
			m_bCanConnect = val;
		}
		unsigned int GetConnectSize();
	protected:
		char m_strPassword[32];

		bool m_bCanConnect;

	protected:
		struct tagConnecter
		{
			tagConnecter()
			{
				pConnector = nullptr;
				nType = 0;
				memset(strIP, 0, sizeof(strIP));
				nPort = 0;
				nServerID = 0;
			}
			int nType;//0,被动链接，server模式下的链接,1,主动链接，client模式下的链接
			char strIP[16];
			int nPort;
			__int64 nServerID;
			CBaseConnector* pConnector;
		};

		std::map<__int64, tagConnecter> m_mapConnector;
		//CRITICAL_SECTION	m_xContainerCtrl;
		std::mutex m_ConnecterLock;


#ifdef _WIN32
		CIOCPConnector m_Connect;
#else
		CEpollConnector m_Connect;
#endif
	};
}