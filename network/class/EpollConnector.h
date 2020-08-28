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

#ifndef _WIN32
#include <map>
#include <list>
#include <vector>
#include <mutex>
#include <functional>
#include "nowin.h"
#include "BaseConnector.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef int SOCKET;
inline bool initSocket()
{

	return true;
}
inline bool FinalSocket()
{

	return true;
}
namespace zlnetwork
{
	class CSocketConnector;
	typedef std::function<CBaseConnector * (SOCKET, const char*, int)> CreateConnectorFun;
#define INVALID_SOCKET -1
#define	SOCKET_ERROR -1
	class CEpollConnector
	{
	public:
		CEpollConnector();
		~CEpollConnector();
	public:

		virtual	void OnInit();
		virtual bool OnProcess();
		virtual void OnDestroy();
	public:
		void SetListen(int nPort, CreateConnectorFun Fun);

	public:
		//监听端口们
		struct tagListen
		{
			tagListen()
			{
				m_nPort = 0;
				m_sListen = INVALID_SOCKET;
			}
			int m_nPort;//端口
			int m_sListen;//监听socket
			CreateConnectorFun m_Fun;
		};
		std::map<int, tagListen> m_mapListen;
		std::mutex	m_xListenCtrl;

	public:
		enum E_EPOLL_EVENT
		{
			E_EPOLL_EVENT_LISTEN,
			E_EPOLL_EVENT_CONNECT,
		};
		struct tagEpollEventData
		{
			int nType;//0,监听,1 连接
			int nListenPort;
			CSocketConnector* pSocketConnector;
		};

		static CRingBuffer<tagEpollEventData> m_EpollEventCache;

	public:
		static int GetEpollFD()
		{
			return m_epollfd;
		}
	protected:
		int m_nMaxEventSize;
		static int m_epollfd;

		struct epoll_event eventList[100];
	};

	//和客户端的连接
	class CSocketConnector : public CBaseConnector
	{
	public:
		CSocketConnector();
		virtual ~CSocketConnector(void);

	protected:
		char strIP[16];
		int m_nPort;

		SOCKET m_Socket;

		CEpollConnector* m_pConnector;
	public:

		virtual int GetPort()
		{
			return m_nPort;
		}
		virtual void SetPort(int val)
		{
			m_nPort = val;
		}
		virtual void SetIP(const char*);
		virtual char* GetIP();
		SOCKET GetSocket() const { return m_Socket; }
		void SetSocket(SOCKET val) { m_Socket = val; }
		bool IsSocketClosed();

		
		virtual void	OnInit();
		virtual bool	OnProcess();
		virtual void	OnDestroy();

		virtual bool	CheckOverlappedIOCompleted() {
			return true;
		}
	public:
		bool Recv();
		bool Send();
	public:
		bool Open();
		virtual void Close();
		virtual void clear();


	protected:
#define BUFFER_SIZE 1024
		char recv_buf[BUFFER_SIZE];
		std::vector<char> vSend_Buf;

		std::mutex m_xSocketCtrl;

		//以下是操作重叠端口的部分
	private://这是提供给IOCP的数据收发
		void SetSendMode();
		void SetRecvMode();
	public:
		void SendData(const char* pData, unsigned int nSize);


	private:
		std::vector<char> m_vSendDataBuff;

	protected:


	private:
		enum
		{
			E_OPER_NONE,
			E_OPER_SEND,
			E_OPER_RECV,
		};
		//保证同一时刻只有一个WSASend
		std::atomic_int m_bIsSendOrRecv;


		////确保安全机制
		std::mutex m_lockSend;
		std::mutex m_lockRecv;
	private:

		//连接器实例重用的情况下
		//连接器特殊码，和指针一起标示是否是
		//被连接的完成消息
		unsigned int m_dwConnecttorSpecialKey;
	protected:
		CEpollConnector::tagEpollEventData m_eventData;
	public:
		friend class CEpollConnector;
	};
}
#endif