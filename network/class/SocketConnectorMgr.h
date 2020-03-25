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
	//������2��ģʽ��һ����ʹ��ip�Ͷ˿ڣ����ӵ�����������ȥ������clientģʽ
	//һ�����ṩ�����˿ڣ��ȴ������������ӹ���������serverģʽ
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
		virtual bool ChangeClientID(int nOldID, int NewID);

		virtual bool waitMsgAndClose();
		virtual bool kickAll();
		//���Create��Ϊclientģʽ׼����
		virtual CBaseConnector* Create() = 0;
	public:
		//��Ϊserverģʽ����IOCP�ṩ�Ļص�����
		//static CSocketConnector* CreateNew(SOCKET sRemote,SOCKADDR_IN& saAdd);

		__int64 GetAllotConnectID() const
		{
			return m_nAllotConnectID;
		}
		void SetAllotConnectID(__int64 val)
		{
			m_nAllotConnectID = val;
		}
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
			int nType;//0,�������ӣ�serverģʽ�µ�����,1,�������ӣ�clientģʽ�µ�����
			char strIP[16];
			int nPort;
			__int64 nServerID;
			CBaseConnector* pConnector;
		};
		std::atomic_ullong m_nAllotConnectID;

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