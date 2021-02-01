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

#include "winsock2i.h"
#include"SocketConnectorMgr.h"
namespace zlnetwork
{
	CSocketConnectorMgr::CSocketConnectorMgr()
	{
		m_bCanConnect = true;
		memset(m_strPassword, 0, sizeof(m_strPassword));

	}
	CSocketConnectorMgr::~CSocketConnectorMgr()
	{
		m_ConnecterLock.lock();
		auto it = m_mapConnector.begin();
		for (; it != m_mapConnector.end(); it++)
		{
			CBaseConnector* pConn = it->second.pConnector;
			if (pConn != NULL)
			{
				pConn->OnProcess();
				if (!pConn->IsSocketClosed())
				{

					pConn->Close();
				}
				//delete pConn;

				pConn->clear();
				delete pConn;
			}
		}
		m_ConnecterLock.unlock();
	}

	void CSocketConnectorMgr::OnInit()
	{
		initSocket();

		m_Connect.OnInit();
	}
	void CSocketConnectorMgr::OnProcess()
	{
		//AUTO_FUN_PERF();
		unsigned int nMsgRecvAmount = 0;
		unsigned int nMsgSendAmount = 0;
		m_Connect.OnProcess();

		m_ConnecterLock.lock();
		std::map<__int64, tagConnecter>::iterator it = m_mapConnector.begin();
		for (; it != m_mapConnector.end();)
		{
			tagConnecter& tagInfo = it->second;
			if (tagInfo.nType >= 1 && tagInfo.pConnector == nullptr && tagInfo.nPort == 0)
			{
				it++;
				continue;
			}
			if (tagInfo.pConnector == nullptr)
			{
				tagInfo.pConnector = Create();
				tagInfo.pConnector->SetIP(tagInfo.strIP);
				tagInfo.pConnector->SetPort(tagInfo.nPort);
			}
			if (tagInfo.pConnector != nullptr)
			{
				if (tagInfo.pConnector->IsSocketClosed())
				{
					if (tagInfo.nType == 1)
					{
						tagInfo.pConnector->Open();
						tagInfo.pConnector->OnInit();
					}
					else if (tagInfo.pConnector->CanDelete())
					{
						tagInfo.pConnector->OnDestroy();
						delete tagInfo.pConnector;
						m_mapConnector.erase(it++);
						continue;
					}
				}
				tagInfo.pConnector->OnProcess();
			}
			it++;
		}
		//PERF("消息接收队列平均长度",nMsgRecvAmount);
		//PERF("消息发送队列平均长度",nMsgSendAmount);
		//PERF("客户端当前链接数",m_mapConnector.size());
		m_ConnecterLock.unlock();
		return;
	}
	void CSocketConnectorMgr::OnDestroy()
	{
		m_Connect.OnDestroy();
		FinalSocket();
	}

	bool CSocketConnectorMgr::SetListen(int nPort, CreateConnectorFun Fun)
	{
		m_Connect.SetListen(nPort, Fun);
		return true;
	}

	bool CSocketConnectorMgr::SendData(__int64 nConnectorID, char* pData, int len)
	{
		m_ConnecterLock.lock();
		std::map<__int64, tagConnecter>::iterator it = m_mapConnector.find(nConnectorID);
		if (it != m_mapConnector.end())
		{
			CBaseConnector* pConn = it->second.pConnector;
			if (pConn != NULL)
			{
				pConn->SendData(pData, len);
				m_ConnecterLock.unlock();
				return true;
			}
		}
		m_ConnecterLock.unlock();
		return false;
	}
	bool CSocketConnectorMgr::SendData2All(char* pData, int len, __int64 except)
	{
		m_ConnecterLock.lock();
		std::map<__int64, tagConnecter>::iterator it = m_mapConnector.begin();
		for (; it != m_mapConnector.end(); it++)
		{
			CBaseConnector* pConn = it->second.pConnector;
			if (pConn != NULL && it->first != except)
			{
				pConn->SendData(pData, len);
			}
		}
		m_ConnecterLock.unlock();
		return true;
		//return false;
	}
	bool CSocketConnectorMgr::AddClient(const char* pIP, int Port, __int64 nConnectID)
	{
		if (pIP == NULL)
		{
			return false;
		}

		m_ConnecterLock.lock();
		tagConnecter* pConnectInfo = NULL;
		std::map<__int64, tagConnecter>::iterator it = m_mapConnector.find(nConnectID);
		if (it == m_mapConnector.end())
		{
			tagConnecter& tagInfo = m_mapConnector[nConnectID];
			strncpy(tagInfo.strIP, pIP,sizeof(tagInfo.strIP));
			tagInfo.nPort = Port;
			tagInfo.nServerID = nConnectID;
			tagInfo.nType = 1;
			m_ConnecterLock.unlock();
			return true;
		}
		m_ConnecterLock.unlock();
		return false;
	}
	bool CSocketConnectorMgr::AddClient(CBaseConnector* pConn)
	{
		if (pConn == NULL)
		{
			return false;
		}
		std::lock_guard<std::mutex> Lock(m_ConnecterLock);

		tagConnecter& tagInfo = m_mapConnector[pConn->GetID()];
		tagInfo.pConnector = pConn;

		return true;
	}


	bool CSocketConnectorMgr::waitMsgAndClose()
	{
		bool bEnd = false;
		m_ConnecterLock.lock();
		std::map<__int64, tagConnecter>::iterator it;
		while (!bEnd)
		{
			bEnd = true;
			it = m_mapConnector.begin();
			for (; it != m_mapConnector.end();)
			{
				tagConnecter& tagConn = it->second;
				if (tagConn.pConnector)
				{
					if (tagConn.pConnector->IsSocketClosed())
					{
						if (tagConn.pConnector->CheckOverlappedIOCompleted())
						{
							tagConn.pConnector->clear();
							delete tagConn.pConnector;
							m_mapConnector.erase(it++);
							continue;
						}
						bEnd = false;
					}
					else
					{
						tagConn.pConnector->OnProcess();
						
						{
							tagConn.pConnector->Close();
							it++;
							continue;
						}
						bEnd = false;
					}

				}
				it++;
			}
		}
		m_ConnecterLock.unlock();
		return true;
	}
	bool CSocketConnectorMgr::kickAll()
	{
		//bool bEnd = false;
		m_ConnecterLock.lock();
		std::map<__int64, tagConnecter>::iterator it;
		//while (!bEnd)
		{
			//bEnd = true;
			it = m_mapConnector.begin();
			for (; it != m_mapConnector.end();)
			{
				tagConnecter& tagConn = it->second;
				if (tagConn.pConnector)
				{
					if (tagConn.pConnector->IsSocketClosed())
					{
					}
					else
					{
						tagConn.pConnector->Close();
					}
				}
				it++;
			}
		}
		m_ConnecterLock.unlock();
		return true;
	}
	unsigned int CSocketConnectorMgr::GetConnectSize()
	{
		std::lock_guard<std::mutex> Lock(m_ConnecterLock);

		return m_mapConnector.size();
	}
}