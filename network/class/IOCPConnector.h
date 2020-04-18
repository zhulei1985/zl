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
#ifdef _WIN32
#include "winsock2i.h"
#include <windows.h>

#include <mswsock.h>
#include <map>
#include <list>
#include <vector>
#include <mutex>
#include <functional>

#include "BaseConnector.h"


#define IOCP_BUFFER_SIZE 8192
//无消息接收，超时时间
#define TIMEOUT_NO_RECVDATA 600000

namespace zlnetwork
{
	class CSocketConnector;
	typedef std::function<CBaseConnector * (SOCKET, const char*)> CreateConnectorFun;

	void ProcessOverlappedFunc(HANDLE hIoCompletionPort);

	enum E_OP_TYPE
	{
		OP_READ = 1,
		OP_WRITE = 2,
		OP_STOPTHREAD = 3,
	};

	struct SOVERLAPPEDPLUS
	{
		//ol必须是第一个定义
		OVERLAPPED	ol;
		WSABUF		wsabuf;
		CSocketConnector* pConnecttor;
		int iOpCode;
		char data[IOCP_BUFFER_SIZE];

	public:
		SOVERLAPPEDPLUS();
		~SOVERLAPPEDPLUS();
		void init()
		{
			wsabuf.buf = data;
			wsabuf.len = 0;
			pConnecttor = NULL;
			iOpCode = 0;
			memset(&ol, 0, sizeof(ol));
		}
		void clear()
		{
		}

	public:
	};

	class CIOCPConnector
	{
	public:
		CIOCPConnector();
		virtual ~CIOCPConnector();

	public:

		virtual	void	OnInit();
		virtual bool	OnProcess();
		virtual void	OnDestroy();

		bool StartIoCompletionPort(int iThreadNum = -1);
		void StopIoCompletionPort();

	public:
		static LONG GetActiveThreadNum() { return m_lActiveThreadNum; }
		static void SetActiveThreadNum(LONG val) { m_lActiveThreadNum = val; }

		static HANDLE GetHIoCompletionPort() { return hIoCompletionPort; }
		static void SetHIoCompletionPort(HANDLE val) { hIoCompletionPort = val; }

		void SetListen(int nPort, CreateConnectorFun Fun);
		//void SetPort(int nval)
		//{
		//	m_nPort = nval;
		//}
		//int GetConnectorCount() const { return m_nConnectorCount; }
		//void SetConnectorCount(int val) { m_nConnectorCount = val; }

		bool GetRun() const { return m_bRun; }
		void SetRun(bool val) { m_bRun = val; }
	protected:

		static HANDLE hIoCompletionPort;
		static std::atomic_int m_lActiveThreadNum;//活动线程数

		//监听端口们
		struct tagListen
		{
			tagListen()
			{
				m_nPort = 0;
				m_sListen = INVALID_SOCKET;
			}
			int m_nPort;//端口
			SOCKET m_sListen;//监听socket
			CreateConnectorFun m_Fun;
		};
		std::map<int, tagListen> m_mapListen;
		std::mutex	m_xListenCtrl;

		bool m_bRun;

	public:
		friend void ProcessOverlappedFunc(LPVOID lpParam);

		static std::atomic_ullong m_AllSendDataCount;
		static std::atomic_ullong m_AllRecvDataCount;
	};

	//SOVERLAPPEDPLUS m_overlappedsend;
	//typedef struct _PER_HANDLE_DATA
	//{
	//	SOCKET      s;      // 对应的套接字句柄 
	//	sockaddr_in addr;   // 对方的地址

	//}PER_HANDLE_DATA, * PPER_HANDLE_DATA;



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

		CIOCPConnector* m_pConnector;
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

		bool Open();
		virtual void	OnInit();
		virtual bool	OnProcess();
		virtual void	OnDestroy();

		bool	EnableIoCompletionPort(HANDLE IocpPort);
		virtual bool	CheckOverlappedIOCompleted();
	public:

	public:

		virtual void Close();
		virtual void clear();


	protected:
		std::mutex m_xSocketCtrl;

	  //以下是操作重叠端口的部分
	private://这是提供给IOCP的数据收发
		//int UpdataSendData(unsigned int iSentBytes);

		bool Recv(char* pBytes, int nLen);


	private:
		std::vector<char> m_vSendDataBuff;

	protected:

		bool SendByOverlapped(SOVERLAPPEDPLUS* pPlus);
		bool MoreSendByOverlapped(SOVERLAPPEDPLUS* pPlus);
		bool RemoveSendByOverlapped(SOVERLAPPEDPLUS* pPlus);
		bool RecvByOverlapped(SOVERLAPPEDPLUS* pPlus);
		bool MoreRecvByOverlapped(SOVERLAPPEDPLUS* pPlus);
		bool RemoveRecvByOverlapped(SOVERLAPPEDPLUS* pPlus);
	private:
		//保证同一时刻只有一个WSASend
		std::atomic_bool m_bIsSendByOverlapped;
		std::atomic_bool m_bIsRecvByOverlapped;


		////确保安全机制
		std::mutex m_lockSend;
		std::mutex m_lockRecv;
	public:
		static CRingBuffer<SOVERLAPPEDPLUS> s_OverlappedCache;
	private:

		//连接器实例重用的情况下
		//连接器特殊码，和指针一起标示是否是
		//被连接的完成消息
		DWORD		m_dwConnecttorSpecialKey;
	protected:

		friend void ProcessOverlappedFunc(LPVOID lpParam);
	};

}
#endif