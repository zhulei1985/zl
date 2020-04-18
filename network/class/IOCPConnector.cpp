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
//#include <assert.h>
#include <thread>

#include "iocpconnector.h"

#pragma warning(disable : 4996) 

#define Safe_Delete(x) if(x){delete x;x=0;}
namespace zlnetwork
{
	HANDLE CIOCPConnector::hIoCompletionPort=nullptr;
	std::atomic_int CIOCPConnector::m_lActiveThreadNum = 0;
	std::atomic_ullong CIOCPConnector::m_AllSendDataCount = 0;
	std::atomic_ullong CIOCPConnector::m_AllRecvDataCount = 0;
	CIOCPConnector::CIOCPConnector()
	{
		hIoCompletionPort = 0;
		m_lActiveThreadNum = 0;

		m_bRun = false;

	};
	CIOCPConnector::~CIOCPConnector()
	{
	}

	bool CIOCPConnector::StartIoCompletionPort(int iThreadNum)
	{
		if (-1 == iThreadNum)
		{
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			DWORD dwNumProcessors = systemInfo.dwNumberOfProcessors;

			iThreadNum = dwNumProcessors * 2 + 1;
		}

		m_lActiveThreadNum = 0;

		hIoCompletionPort =
			CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, iThreadNum);
		if (NULL == hIoCompletionPort)
		{

			DWORD dwError = GetLastError();
			return false;
		}


		//创建IOCP工作线程
		for (int iLoop = 0; iLoop < iThreadNum; iLoop++)
		{
			std::thread tbg(ProcessOverlappedFunc, hIoCompletionPort);
			tbg.detach();


			m_lActiveThreadNum++;
		}

		return true;
	}
	void CIOCPConnector::StopIoCompletionPort()
	{
		SOVERLAPPEDPLUS overlapped;
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.iOpCode = OP_STOPTHREAD;

		//确认线程关闭
		while (m_lActiveThreadNum > 0)
		{
			PostQueuedCompletionStatus(hIoCompletionPort,
				0, (ULONG_PTR)0, &overlapped.ol);
			Sleep(1);
		}
		//
		if (hIoCompletionPort)
		{
			CloseHandle(hIoCompletionPort);
		}
	}

	void CIOCPConnector::OnInit()
	{
		StartIoCompletionPort();
	}
	bool CIOCPConnector::OnProcess()
	{
		std::lock_guard<std::mutex> Lock(m_xListenCtrl);

		std::map<int, tagListen>::iterator it = m_mapListen.begin();
		for (; it != m_mapListen.end(); it++)
		{
			tagListen& taglisten = it->second;
			if (taglisten.m_nPort == 0)
			{
				continue;
			}
			if (taglisten.m_sListen == INVALID_SOCKET)
			{
				taglisten.m_sListen = ::socket(AF_INET, SOCK_STREAM, 0);
				if (taglisten.m_sListen == SOCKET_ERROR)
				{
					char strMsg[128];
					sprintf(strMsg, "socket分配 Error (%d)", WSAGetLastError());

					//PrintDebug("debug",strMsg);
					continue;
				}
				SOCKADDR_IN addr;
				memset(&addr, 0, sizeof(addr));//
				addr.sin_family = AF_INET;
				addr.sin_port = ::htons(taglisten.m_nPort);
				addr.sin_addr.S_un.S_addr = INADDR_ANY;
				int nResult = ::bind(taglisten.m_sListen, (sockaddr*)&addr, sizeof(addr));
				if (nResult == SOCKET_ERROR)
				{
					char strMsg[128];
					sprintf_s(strMsg, "Binding Error (%d)", WSAGetLastError());
					closesocket(taglisten.m_sListen);
					taglisten.m_sListen = SOCKET_ERROR;
					//MessageBox(NULL,strMsg, "Error",MB_OK|MB_ICONERROR);
					//PrintDebug("debug",strMsg);
					continue;
				}
				// 设置为非阻塞方式
				unsigned long	i = 1;
				if (ioctlsocket(taglisten.m_sListen, FIONBIO, &i))
				{
					// 捕获错误
					int err = WSAGetLastError();

					closesocket(taglisten.m_sListen);
					taglisten.m_sListen = SOCKET_ERROR;
					continue;
				}
				nResult = ::listen(taglisten.m_sListen, 5);
				if (nResult == SOCKET_ERROR)
				{
					closesocket(taglisten.m_sListen);
					taglisten.m_sListen = SOCKET_ERROR;
					continue;
				}
			}


			if (taglisten.m_sListen != INVALID_SOCKET)
			{
				// 等待接受未决的连接请求 
				SOCKADDR_IN saRemote;
				int nRemoteLen = sizeof(saRemote);
				SOCKET sRemote = ::accept(taglisten.m_sListen, (sockaddr*)&saRemote, &nRemoteLen);

				if (sRemote != INVALID_SOCKET)
				{
					char strbuff[16] = { 0 };
					strcpy_s(strbuff, sizeof(strbuff), inet_ntoa(saRemote.sin_addr));
					taglisten.m_Fun(sRemote, strbuff);
				}
			}
		}

		return true;
	}

	void CIOCPConnector::OnDestroy()
	{

	}
	void CIOCPConnector::SetListen(int nPort, CreateConnectorFun Fun)
	{
		tagListen taglisten;
		taglisten.m_nPort = nPort;
		taglisten.m_Fun = Fun;
		{
			m_xListenCtrl.lock();
			m_mapListen[nPort] = taglisten;
			m_xListenCtrl.unlock();
		}

	}

	SOVERLAPPEDPLUS::SOVERLAPPEDPLUS()
	{
		//PERF_ALLOCATO("SOVERLAPPEDPLUS",1);
	}
	SOVERLAPPEDPLUS::~SOVERLAPPEDPLUS()
	{
		//PERF_ALLOCATO("SOVERLAPPEDPLUS",-1);
	}

	//#endif

	CRingBuffer<SOVERLAPPEDPLUS> CSocketConnector::s_OverlappedCache(8192);
	CSocketConnector::CSocketConnector()
	{
		//PERF_ALLOCATO("CSocketConnector",1);
		memset(strIP, 0, sizeof(strIP));
		m_Socket = INVALID_SOCKET;

		m_dwConnecttorSpecialKey = 0;


		m_bIsSendByOverlapped = false;
		m_bIsRecvByOverlapped = false;
	}
	CSocketConnector::~CSocketConnector(void)
	{
		//PERF_ALLOCATO("CSocketConnector",-1);

	}
	//CSocketConnector* CSocketConnector::CreateNew(SOCKET sRemote, sockaddr_in& saAdd)
	//{
	//	CSocketConnector* pConn = new CSocketConnector(); 
	//	if( pConn == NULL ) 
	//	{ 
	//		return false;
	//	}
	//	char strbuff[16];
	//	strcpy_s(strbuff,sizeof(strbuff),inet_ntoa(saAdd.sin_addr));
	//
	//
	//	pConn->SetSocket(sRemote);
	//	pConn->SetIP(strbuff);
	//	pConn->OnInit();
	//
	//	return pConn;
	//}
	bool CSocketConnector::EnableIoCompletionPort(HANDLE IocpPort)
	{
		m_dwConnecttorSpecialKey = /*timeGetTime() * */m_Socket * rand();
		if (NULL == ::CreateIoCompletionPort((HANDLE)m_Socket, IocpPort, (DWORD)m_dwConnecttorSpecialKey, 0))
		{
			return false;
		}

		RecvByOverlapped(nullptr);
		return true;

	}
	bool	CSocketConnector::CheckOverlappedIOCompleted()
	{
		bool bResult = true;


		return (!m_bIsSendByOverlapped) && (!m_bIsRecvByOverlapped);
	}
	bool	CSocketConnector::OnProcess()
	{
		if (IsSocketClosed())
		{
			return false;
		}

		{
			//将要发送的数据提交给IOCP
			//std::lock_guard<std::mutex> Lock(m_lockSend);
			if (m_bIsSendByOverlapped == false && m_Buffer_Send.GetSize() > 0)
			{
				SendByOverlapped(nullptr);

			}
		}


		return true;
	}

	void CSocketConnector::OnInit()
	{
		CBaseConnector::OnInit();
		EnableIoCompletionPort(CIOCPConnector::GetHIoCompletionPort());
	}
	void CSocketConnector::OnDestroy()
	{
		CBaseConnector::OnDestroy();
	}
	void CSocketConnector::SetIP(const char* pStr)
	{
		if (pStr)
		{
			strcpy_s(strIP, sizeof(strIP), pStr);
		}
		else
		{
			strcpy_s(strIP, sizeof(strIP), "0.0.0.0");
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
	bool CSocketConnector::Recv(char* pBytes, int nLen)
	{
		if (nLen <= 0)
			return true;

		m_Buffer_Recv.Push(pBytes, nLen);

		return true;
	}

	void CSocketConnector::Close()
	{
		m_xSocketCtrl.lock();
		if (m_Socket != INVALID_SOCKET)
		{
			::closesocket(m_Socket);
		}
		m_Socket = INVALID_SOCKET;
		m_xSocketCtrl.unlock();
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
			unsigned long i = 1;
			if (ioctlsocket(m_Socket, FIONBIO, &i))
			{
				//LOGERROR("Init ioctlsocket() failed.");
				Close();
				return false;
			}

		}

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(m_nPort);
		addr.sin_addr.S_un.S_addr = inet_addr(strIP);
		//PrintDebug("network","尝试连接服务器,IP:%s,PORT:%d",strIP,m_nPort);
		int ret = connect(m_Socket, (sockaddr*)&addr, sizeof(addr));
		if (ret == -1)
		{
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				//Close();
				return false;
			}
			else
			{
				timeval timeout;
				timeout.tv_sec = 30;
				timeout.tv_usec = 0;
				fd_set writeset, exceptset;
				FD_ZERO(&writeset);
				FD_ZERO(&exceptset);
				FD_SET(m_Socket, &writeset);
				FD_SET(m_Socket, &exceptset);

				int ret = select(FD_SETSIZE, NULL, &writeset, &exceptset, &timeout);
				if (ret == 0)
				{
					Close();
					return false;
				}
				else if (ret < 0)
				{
					int	err = WSAGetLastError();
					Close();
					return false;
				}
				else	// ret > 0
				{
					if (FD_ISSET(m_Socket, &exceptset))		// or (!FD_ISSET(m_sockClient, &writeset)
					{
						Close();
						return false;
					}
				}
			}
		}
		//m_lHaveSentThreadSafeReq = 0;
		return true;
	}
	void CSocketConnector::clear()
	{

		// PrintDebug("iocp","m_xSendCtrl,退出临界区3");
		memset(strIP, 0, sizeof(strIP));
		m_nPort = 0;
		if (m_Socket != INVALID_SOCKET)
		{
			::closesocket(m_Socket);
		}
		m_Socket = INVALID_SOCKET;

		m_dwConnecttorSpecialKey = 0;


		m_bIsSendByOverlapped = false;
		m_bIsRecvByOverlapped = false;
	}

	bool CSocketConnector::SendByOverlapped(SOVERLAPPEDPLUS* pPlus)
	{
		std::lock_guard<std::mutex> Lock(m_lockSend);
		if (pPlus == nullptr)
		{
			pPlus = s_OverlappedCache.Get();
		}
		if (m_Buffer_Send.Get2(m_vSendDataBuff, IOCP_BUFFER_SIZE))
		{
			memset(&pPlus->ol, 0, sizeof(pPlus->ol));
			pPlus->wsabuf.buf = pPlus->data;
			pPlus->wsabuf.len = m_vSendDataBuff.size();
			memcpy(pPlus->data, &m_vSendDataBuff[0], m_vSendDataBuff.size());
			pPlus->iOpCode = OP_WRITE;
			pPlus->pConnecttor = this;

			DWORD dwSend;
			int iRet = WSASend(
				m_Socket,
				&pPlus->wsabuf,
				1,
				&dwSend,
				0,
				&pPlus->ol,
				0);

			if (SOCKET_ERROR == iRet)
			{
				DWORD dwRet = WSAGetLastError();
				switch (dwRet)
				{
				case WSA_IO_PENDING:
					break;
				default:

					Close();
					return false;
					break;
				}
			}
			m_bIsSendByOverlapped = true;
			//printf("SendByOverlapped true \n");
			return true;
		}
		return false;
	}
	bool CSocketConnector::MoreSendByOverlapped(SOVERLAPPEDPLUS* pPlus)
	{
		if (pPlus == NULL)
		{
			m_bIsSendByOverlapped = false;
			return false;
		}
		//printf("MoreSendByOverlapped \n");
		//如果还有数据，继续发送
		if (!SendByOverlapped(pPlus))
		{
			return RemoveSendByOverlapped(pPlus);
			//printf("MoreSendByOverlapped SendByOverlapped false \n");
		}
		return true;
	}
	bool CSocketConnector::RemoveSendByOverlapped(SOVERLAPPEDPLUS* pPlus)
	{
		std::lock_guard<std::mutex> Lock(m_lockSend);
		if (pPlus == NULL)
		{
			return false;
		}

		m_bIsSendByOverlapped = false;
		pPlus->pConnecttor = NULL;
		s_OverlappedCache.Push(pPlus);	
		//printf("RemoveSendByOverlapped \n");
		return true;
	}
	bool CSocketConnector::RecvByOverlapped(SOVERLAPPEDPLUS* pPlus)
	{
		std::lock_guard<std::mutex> Lock(m_lockRecv);
		if (pPlus == nullptr)
		{
			pPlus = s_OverlappedCache.Get();
		}
		if (pPlus)
		{
			memset(&pPlus->ol, 0, sizeof(pPlus->ol));
			pPlus->wsabuf.buf = pPlus->data;
			pPlus->wsabuf.len = IOCP_BUFFER_SIZE;
			pPlus->iOpCode = OP_READ;
			pPlus->pConnecttor = this;

			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			int iRet = ::WSARecv(m_Socket, &pPlus->wsabuf, 1, &dwRecv, &dwFlags, &pPlus->ol, NULL);

			if (SOCKET_ERROR == iRet)
			{
				DWORD dwRet = WSAGetLastError();
				switch (dwRet)
				{
				case WSA_IO_PENDING:
					break;
				default:
					Close();
					break;
				}
			}

			return true;
		}
		return false;
	}
	bool CSocketConnector::MoreRecvByOverlapped(SOVERLAPPEDPLUS* pPlus)
	{
		if (pPlus == NULL)
		{
			return false;
		}

		if (!RecvByOverlapped(pPlus))
		{
			RemoveRecvByOverlapped(pPlus);
		}

		return true;
	}
	bool CSocketConnector::RemoveRecvByOverlapped(SOVERLAPPEDPLUS* pPlus)
	{
		std::lock_guard<std::mutex> Lock(m_lockRecv);
		if (pPlus == NULL)
		{
			return false;
		}

		m_bIsRecvByOverlapped = false;
		pPlus->pConnecttor = NULL;
		s_OverlappedCache.Push(pPlus);

		return true;
	}
	void ProcessOverlappedFunc(HANDLE hIoCompletionPort)
	{
		//::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		LPOVERLAPPED lpOverlapped;
		SOVERLAPPEDPLUS* pPlus;
		//PULONG_PTR lpCompletionKey;
		ULONG_PTR dwKey = 0;
		DWORD dwBytesXfered = 0;
		BOOL bRet = FALSE;
		int iRet = 0;
		DWORD dwRet;

		bool bFin = false;
		while (!bFin)
		{
			bRet = GetQueuedCompletionStatus(
				hIoCompletionPort,	//需要监视的完成端口句柄
				&dwBytesXfered,		//实际传输的字节
				&dwKey,				//单句柄数据
				&lpOverlapped,		//一个指向伪造的重叠IO的指针，大小取决于你的定义
				1000				////等待多少秒
			);
			if (0 == bRet)
			{
				if (lpOverlapped)
				{
					//iocp发现一个连接断开
					pPlus = (SOVERLAPPEDPLUS*)lpOverlapped;
					CSocketConnector* pConnecttor = pPlus->pConnecttor;
					if (pConnecttor)
					{
						//InterlockedDecrement(&pConnecttor->m_lHaveSentThreadSafeReq);
						int nError = GetLastError();
						pConnecttor->Close();
						switch (pPlus->iOpCode)
						{
						case OP_READ:
							{
								pConnecttor->RemoveRecvByOverlapped(pPlus);
							}
							break;
						case OP_WRITE:
							{
								pConnecttor->RemoveSendByOverlapped(pPlus);
							}
							break;
						}
					}
					else
					{
						//delete pPlus;
						CSocketConnector::s_OverlappedCache.Push(pPlus);
					}
				}
				continue;
			}

			pPlus = (SOVERLAPPEDPLUS*)lpOverlapped;
			CSocketConnector* pConnecttor = pPlus->pConnecttor;
			if (pConnecttor)
			{
				//if (pConnecttor->m_lHaveSentThreadSafeReq < 0)
				//{
				//  pConnecttor->m_lHaveSentThreadSafeReq = 0;
				//}
				if (dwKey != pConnecttor->m_dwConnecttorSpecialKey)
				{
					delete pPlus;
					Sleep(1);
					continue;
				}
			}
			else
			{
				delete pPlus;
				Sleep(1);
				continue;
			}

			switch (pPlus->iOpCode)
			{
			case OP_READ:
			{
				if (pConnecttor->IsSocketClosed())
				{
					pConnecttor->RemoveRecvByOverlapped(pPlus);
					break;
				}
				if (dwBytesXfered == 0)
				{
					//一般情况下，dwBytesXfered为0表示客户端主动断开连接
					int nError = GetLastError();
					//PrintRelease("iocp","SendRecvFunc中发现客户端主动断开 %d 错误断开连接,ID:%lld", nError,pConnecttor->GetID());
					pConnecttor->Close();
					pConnecttor->RemoveRecvByOverlapped(pPlus);

					break;
				}

				//PrintDebug("iocp","IOCP(ID:%lld)接收数据,长度:%d",pConnecttor->GetID(),dwBytesXfered);
				//pPlus->wsabuf.len = dwBytesXfered;
				CIOCPConnector::m_AllRecvDataCount += dwBytesXfered;
				if (pConnecttor->Recv((char*)pPlus->wsabuf.buf, dwBytesXfered) == false)
					//if (pConnecttor->Recv(pPlus) == false)
				{
					pConnecttor->Close();
					//PrintRelease("iocp","IOCP(ID:%lld)数据接收失败，断开此链接",pConnecttor->GetID());
					pConnecttor->RemoveRecvByOverlapped(pPlus);
					break;
				}
				pConnecttor->MoreRecvByOverlapped(pPlus);
			}
			break;
			case OP_WRITE:
			{
				if (pConnecttor->IsSocketClosed())
				{
					pConnecttor->RemoveSendByOverlapped(pPlus);
					//if (pConnecttor->m_lHaveSentPolicyFileRequestReq > 0)
					//{
					//  InterlockedDecrement(&pConnecttor->m_lHaveSentPolicyFileRequestReq);
					//  //PrintDebug("iocp","SendRecvFunc中断开连接,目前用于flash as3的安全策略,ID:%d", pConnecttor->GetID());
					//  //pConnecttor->Close();
					//}
					break;
				}
				//pPlus->wsabuf.len = pConnecttor->UpdataSendData(dwBytesXfered);
				CIOCPConnector::m_AllSendDataCount += dwBytesXfered;
				if (pPlus->wsabuf.len != dwBytesXfered)
				{
					//PrintRelease("error","iocp send消息发送有问题，%d//%d",dwBytesXfered,pPlus->wsabuf.len);
				}
				pConnecttor->MoreSendByOverlapped(pPlus);
			}
			break;
			case OP_STOPTHREAD:
			{
				bFin = true;
			}
			break;
			}
			//Sleep(1);
		}
		CIOCPConnector::m_lActiveThreadNum++;
		return;
	}
}
#endif