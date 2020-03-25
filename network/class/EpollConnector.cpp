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

#include "EpollConnector.h"

#ifndef _WIN32

namespace zlnetwork
{
	int CEpollConnector::m_epollfd = 0;
	CRingBuffer<CEpollConnector::tagEpollEventData> CEpollConnector::m_EpollEventCache(10);
	CEpollConnector::CEpollConnector()
	{
		m_nMaxEventSize = 100;
	}
	CEpollConnector::~CEpollConnector()
	{

	}

	void CEpollConnector::OnInit()
	{
		m_epollfd = epoll_create(m_nMaxEventSize);

	}
	bool CEpollConnector::OnProcess()
	{
		int ret = epoll_wait(m_epollfd, eventList, m_nMaxEventSize, 100);
		if (ret < 0)
		{
			//错误
			if (errno)
			{

			}
			return false;
		}
		for (int i = 0; i < ret; i++)
		{
			tagEpollEventData* pData = (tagEpollEventData*)(eventList[i].data.ptr);
			if (pData)
			{
				if (pData->nType == E_EPOLL_EVENT_LISTEN)
				{
					tagListen& taglisten = m_mapListen[pData->nListenPort];

					if (eventList[i].events & EPOLLHUP)
					{
						close(taglisten.m_sListen);
						taglisten.m_sListen = INVALID_SOCKET;
						continue;
					}
					if (eventList[i].events & EPOLLERR)
					{
						close(taglisten.m_sListen);
						taglisten.m_sListen = INVALID_SOCKET;
						continue;
					}

					sockaddr_in saRemote;
					socklen_t nRemoteLen = sizeof(saRemote);
					SOCKET sRemote = ::accept(taglisten.m_sListen, (sockaddr*)&saRemote, &nRemoteLen);

					if (sRemote != INVALID_SOCKET)
					{
						char strbuff[16] = { 0 };
						strncpy(strbuff, inet_ntoa(saRemote.sin_addr), sizeof(strbuff));
						taglisten.m_Fun(sRemote, strbuff);
					}
				}
				else
				{
					CSocketConnector* pConnector = pData->pSocketConnector;
					if (pConnector)
					{
						if (eventList[i].events & EPOLLHUP)
						{
							pConnector->Close();
							continue;
						}
						if (eventList[i].events & EPOLLERR)
						{
							pConnector->Close();
							continue;
						}
						if (eventList[i].events & EPOLLIN)
						{
							if (pConnector->Recv())
							{
								pConnector->SetRecvMode();
							}
							else
							{
								pConnector->Close();
								continue;
							}
						}
						if (eventList[i].events & EPOLLOUT)
						{
							if (!pConnector->Send())
							{
								pConnector->Close();
							}
						}
					}
				}
			}
			else
			{

			}
		}
	}

	void CEpollConnector::OnDestroy()
	{
		close(m_epollfd);
	}
	void CEpollConnector::SetListen(int nPort, CreateConnectorFun Fun)
	{
		std::lock_guard<std::mutex> Lock(m_xListenCtrl);
		auto it = m_mapListen.find(nPort);
		if (it == m_mapListen.end())
		{
			tagListen& ln = m_mapListen[nPort];
			ln.m_nPort = nPort;
			ln.m_Fun = Fun;
		}

		tagListen& taglisten = m_mapListen[nPort];

		if (taglisten.m_sListen == INVALID_SOCKET)
		{
			taglisten.m_sListen = ::socket(AF_INET, SOCK_STREAM, 0);
			if (taglisten.m_sListen == SOCKET_ERROR)
			{
				return;
			}
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));//
			addr.sin_family = AF_INET;
			addr.sin_port = ::htons(taglisten.m_nPort);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			int nResult = ::bind(taglisten.m_sListen, (sockaddr*)&addr, sizeof(addr));
			if (nResult == SOCKET_ERROR)
			{
				taglisten.m_sListen = SOCKET_ERROR;
				return;
			}

			nResult = ::listen(taglisten.m_sListen, 5);
			if (nResult == SOCKET_ERROR)
			{
				close(taglisten.m_sListen);
				taglisten.m_sListen = SOCKET_ERROR;
				return;
			}

			//建立监听端口的事件
			tagEpollEventData* pData = m_EpollEventCache.Get();
			if (pData)
			{
				pData->nType = E_EPOLL_EVENT_LISTEN;
				pData->nListenPort = nPort;
				struct epoll_event event;
				event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
				event.data.ptr = pData;
				if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, taglisten.m_sListen, &event) == -1)
				{
					//错误
					close(taglisten.m_sListen);
					taglisten.m_sListen = SOCKET_ERROR;
					return;
				}
			}
		}

	}

	CSocketConnector::CSocketConnector()
	{
		//PERF_ALLOCATO("CSocketConnector",1);
		memset(strIP, 0, sizeof(strIP));
		m_Socket = INVALID_SOCKET;

		m_dwConnecttorSpecialKey = 0;

	}
	CSocketConnector::~CSocketConnector(void)
	{
		//PERF_ALLOCATO("CSocketConnector",-1);

	}
	void CSocketConnector::SetIP(const char* pStr)
	{
		if (pStr)
		{
			strncpy(strIP, pStr, sizeof(strIP));
		}
		else
		{
			strncpy(strIP, "0.0.0.0", sizeof(strIP));
		}
	}
	char* CSocketConnector::GetIP()
	{
		return strIP;
	}

	bool CSocketConnector::IsSocketClosed()
	{
		bool bResult = false;
		m_xSocketCtrl.lock();
		bResult = (INVALID_SOCKET == m_Socket);
		m_xSocketCtrl.unlock();
		return bResult;
	}


	void CSocketConnector::OnInit()
	{
		m_eventData.nType = CEpollConnector::E_EPOLL_EVENT_CONNECT;
		m_eventData.pSocketConnector = this;

		// 设置为非阻塞方式
		int oldopt = fcntl(m_Socket, F_GETFL);
		int newopt = oldopt | O_NONBLOCK;
		fcntl(m_Socket, F_SETFL, newopt);

		struct epoll_event event;
		event.events = EPOLLIN | EPOLLET | EPOLLERR;
		event.data.ptr = &m_eventData;
		if (epoll_ctl(CEpollConnector::GetEpollFD(), EPOLL_CTL_ADD, m_Socket, &event) < 0) {
			//错误
			if (errno)
			{

			}
		}
	}
	bool CSocketConnector::OnProcess()
	{
	}
	void CSocketConnector::OnDestroy()
	{
	}

	bool CSocketConnector::Recv()
	{
		m_bIsSendOrRecv = E_OPER_NONE;
		while (true)
		{
			int ret = recv(m_Socket, recv_buf, BUFFER_SIZE, 0);
			if (ret < 0)
			{
				if (errno == EAGAIN)
					break;
				else
					return false;
			}
			else if (ret == 0)
			{
				return false;
			}
			else if (ret > 0)
			{
				m_Buffer_Recv.Push(recv_buf, ret);
				if (ret < BUFFER_SIZE)
				{
					break;
				}
			}

		}
		return true;
	}
	bool CSocketConnector::Send()
	{
		m_bIsSendOrRecv = E_OPER_NONE;
		int ret = 0;
		// 先发送上次没有发送完的数据
		while (vSend_Buf.size() > 0)
		{
			ret = send(m_Socket, &vSend_Buf[0], vSend_Buf.size(), 0);
			if (ret < 0)
			{

				if (errno == EINTR)
					continue;

				// 当socket是非阻塞时,如返回此错误,表示写缓冲队列已满,
				if (errno == EAGAIN)
				{
					SetSendMode();
					return true;
				}
				return false;
			}

			vSend_Buf.clear();
		}

		while (m_Buffer_Send.Get2(vSend_Buf, BUFFER_SIZE))
		{
			while (vSend_Buf.size() > 0)
			{
				ret = send(m_Socket, &vSend_Buf[0], vSend_Buf.size(), 0);
				if (ret < 0)
				{

					if (errno == EINTR)
						continue;

					// 当socket是非阻塞时,如返回此错误,表示写缓冲队列已满,
					if (errno == EAGAIN)
					{
						SetSendMode();
						return true;
					}
					return false;
				}
				vSend_Buf.clear();
			}
		}
		SetRecvMode();
		return true;
	}

	bool CSocketConnector::Open()
	{
		if (m_Socket == INVALID_SOCKET)
		{
			m_Socket = socket(PF_INET, SOCK_STREAM, 0);
			if (m_Socket == INVALID_SOCKET)
			{
				// Init socket() failed.
				return false;
			}

			// 设置为非阻塞方式
			int oldopt = fcntl(m_Socket, F_GETFL);
			int newopt = oldopt | O_NONBLOCK;
			fcntl(m_Socket, F_SETFL, newopt);
		}

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(m_nPort);
		inet_pton(PF_INET, "0.0.0.0", &addr.sin_addr);

		//PrintDebug("network","尝试连接服务器,IP:%s,PORT:%d",strIP,m_nPort);
		int ret = connect(m_Socket, (sockaddr*)&addr, sizeof(addr));
		if (ret == -1)
		{
			return false;
		}
		//m_lHaveSentThreadSafeReq = 0;
		return true;
	}
	void CSocketConnector::Close()
	{
		m_xSocketCtrl.lock();
		if (m_Socket != INVALID_SOCKET)
		{
			struct epoll_event ev;
			ev.data.ptr = &m_eventData;
			if (-1 == epoll_ctl(CEpollConnector::GetEpollFD(), EPOLL_CTL_DEL, m_Socket, &ev))
			{

			}
			::close(m_Socket);
		}
		m_Socket = INVALID_SOCKET;
		m_xSocketCtrl.unlock();
	}

	void CSocketConnector::clear()
	{

		// PrintDebug("iocp","m_xSendCtrl,退出临界区3");
		memset(strIP, 0, sizeof(strIP));
		m_nPort = 0;
		if (m_Socket != INVALID_SOCKET)
		{
			struct epoll_event ev;
			ev.data.ptr = &m_eventData;
			if (-1 == epoll_ctl(CEpollConnector::GetEpollFD(), EPOLL_CTL_DEL, m_Socket, &ev))
			{

			}
			::close(m_Socket);
		}
		m_Socket = INVALID_SOCKET;

		m_dwConnecttorSpecialKey = 0;


		m_bIsSendOrRecv = 0;
	}

	void CSocketConnector::SetSendMode()
	{
		if (m_bIsSendOrRecv != E_OPER_SEND)
		{
			struct epoll_event event;
			event.events = EPOLLOUT | EPOLLET | EPOLLERR;
			event.data.ptr = &m_eventData;
			if (epoll_ctl(CEpollConnector::GetEpollFD(), EPOLL_CTL_MOD, m_Socket, &event) < 0) {
					//错误
				if (errno)
				{

				}
			}
			
			m_bIsSendOrRecv = E_OPER_SEND;
		}

	}

	void CSocketConnector::SetRecvMode()
	{
		if (m_bIsSendOrRecv != E_OPER_RECV)
		{
			struct epoll_event event;
			event.events = EPOLLIN | EPOLLET | EPOLLERR;
			event.data.ptr = &m_eventData;
			if (epoll_ctl(CEpollConnector::GetEpollFD(), EPOLL_CTL_MOD, m_Socket, &event) < 0) {
					//错误
				if (errno)
				{

				}
			}
		}

		m_bIsSendOrRecv = E_OPER_RECV;
	}

	void CSocketConnector::SendData(const char* pData, unsigned int nSize)
	{
		CBaseConnector::SendData(pData, nSize);

		SetSendMode();
	}
}
#endif