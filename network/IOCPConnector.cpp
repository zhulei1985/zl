#include "ByteArray.h"

#include <assert.h>
#include <process.h>
//#include "MsgBase.h"
#include "DataQueue.h"
#include "iocpconnector.h"
#include "MsgBase.h"
#include "ConsoleMgr.h"
#include "tagTime.h"
#include "PerformanceMonitor.h"

//#pragma comment(lib,"ws2_32.lib")

#define Safe_Delete(x) if(x){delete x;x=0;}



#define MSGCANCUTMAXSIZE 1024*1024

////�ṩ��GetMsg��SendMsgʹ�õ�buffer.....
//static CThreadSpecialBufForCon theSendTSB(MSGCANCUTMAXSIZE + 1024*10);
//static CThreadSpecialBufForCon theRecvTSB(MSGCANCUTMAXSIZE + 1024*10);
//
////Ϊѹ��׼������ʱbuf
//static CThreadSpecialBufForCon theTSBCompressSend(MSGCANCUTMAXSIZE*2);
//static CThreadSpecialBufForCon theTSBCompressRecv(MSGCANCUTMAXSIZE*2);

//�򵥵ļ����㷨key
///�򵥵ļ����㷨key
const char chKey = 0x86;
const char chKey1 = 0x47;   
const char chKey2 = 0x85;
const char chKey3 = 0x84;

const DWORD dwLogDelay = 1000*10;   //ÿ��log��¼��ʱ����

const unsigned short MAX_MSG_TYPE = 0xFFFF;
const unsigned int MAX_MSGSIZE = 1024*1024;     //��������֧�ַ���1M��С�����ݰ�

HANDLE CIOCPConnector::hIoCompletionPort = 0;
LONG CIOCPConnector::m_lActiveThreadNum;//��߳���
CAllocator<SOVERLAPPEDPLUS,8192> CIOCPConnector::s_OverlappedCache;
//CAllocator<tagOverlappedBuff,32768> CIOCPConnector::s_OverlappedDataBuffCache;
CIOCPConnector::CIOCPConnector()
{
	hIoCompletionPort = 0;
	m_lActiveThreadNum = 0;

	m_bRun = false;

};
CIOCPConnector::~CIOCPConnector()
{
}
bool CIOCPConnector::SocketStartUp()
{
	//WSADATA wsaData;
	//WORD wVersionRequested = MAKEWORD( 2, 2 );

	//int err = WSAStartup( wVersionRequested, &wsaData );
	//if ( err != 0 ) 
	//{
	//	return false;
	//}
	return true;
}
void CIOCPConnector::SocketCleanUp()
{
	//WSACleanup();
}

bool CIOCPConnector::StartIoCompletionPort(int iThreadNum)
{
	if( -1 == iThreadNum )
	{	
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		DWORD dwNumProcessors = systemInfo.dwNumberOfProcessors;

		iThreadNum = dwNumProcessors * 2 + 1;
	}

	m_lActiveThreadNum = 0;

	hIoCompletionPort = 
		CreateIoCompletionPort( INVALID_HANDLE_VALUE,NULL,0,iThreadNum );
	if( NULL == hIoCompletionPort )
	{

		DWORD dwError = GetLastError();
		return false;
	}

	//m_dwConnecttorSpecialKey = /*timeGetTime() * */m_sListen * rand();
	//if( NULL == ::CreateIoCompletionPort( ( HANDLE)m_sListen, hIoCompletionPort, (DWORD)m_dwConnecttorSpecialKey, 0 ))
	//{
	//	return false;
	//}

	//����IOCP�����߳�
	for (int iLoop = 0 ; iLoop < iThreadNum; iLoop ++)
	{
		if (_beginthread(SendRecvFunc, 0, NULL) == NULL)
		{
			return false;
		}

		m_lActiveThreadNum ++;
	}

	PrintDebug("iocp","iocp��ʼ����ɣ�������%d���߳�",m_lActiveThreadNum);
	return true;
}
void CIOCPConnector::StopIoCompletionPort()
{
	SOVERLAPPEDPLUS overlapped;
	memset( &overlapped,0,sizeof( overlapped ) );
	overlapped.iOpCode = OP_STOPTHREAD;		

	//ȷ���̹߳ر�
	while( m_lActiveThreadNum > 0 ) 
	{
		PostQueuedCompletionStatus( hIoCompletionPort,
			0,(ULONG_PTR)0,&overlapped.ol );
		Sleep( 1 );
	}
	//
	if( hIoCompletionPort )
	{
		CloseHandle(hIoCompletionPort );
	}
}

void CIOCPConnector::OnInit()
{
	SocketStartUp();

	StartIoCompletionPort();
}
bool CIOCPConnector::OnProcess()
{
	m_xListenCtrl.lock();
	std::map<int,tagListen>::iterator it = m_mapListen.begin();
	for (;it != m_mapListen.end(); it++)
	{
		tagListen &taglisten = it->second;
		if (taglisten.m_nPort == 0 || taglisten.m_pFun==NULL)
		{
			continue;
		}
		if (taglisten.m_sListen == INVALID_SOCKET)
		{
			taglisten.m_sListen = ::socket( AF_INET,SOCK_STREAM, 0 );
			if (taglisten.m_sListen == SOCKET_ERROR)
			{
				char strMsg[128];
				sprintf(strMsg,"socket���� Error (%d)",WSAGetLastError());
				//MessageBox(NULL,strMsg, "Error",MB_OK|MB_ICONERROR);
				PrintDebug("debug",strMsg);
				continue;
			}
			SOCKADDR_IN addr;
			memset(&addr,0,sizeof(addr));//
			addr.sin_family = AF_INET; 
			addr.sin_port = ::htons( taglisten.m_nPort ); 
			addr.sin_addr.S_un.S_addr = INADDR_ANY; 
			int nResult = ::bind( taglisten.m_sListen, (sockaddr *)&addr, sizeof( addr ) );
			if (nResult == SOCKET_ERROR)
			{
				char strMsg[128];
				sprintf_s(strMsg,"Binding Error (%d)",WSAGetLastError());
				closesocket(taglisten.m_sListen);
				taglisten.m_sListen = SOCKET_ERROR;
				//MessageBox(NULL,strMsg, "Error",MB_OK|MB_ICONERROR);
				PrintDebug("debug",strMsg);
				continue;
			}
			// ����Ϊ��������ʽ
			unsigned long	i = 1;
			if(ioctlsocket(taglisten.m_sListen, FIONBIO, &i))
			{
				// �������
				int err = WSAGetLastError();

				closesocket(taglisten.m_sListen);
				taglisten.m_sListen = SOCKET_ERROR;
				continue;
			}
			nResult = ::listen( taglisten.m_sListen, 5 );
			if (nResult == SOCKET_ERROR)
			{
				closesocket(taglisten.m_sListen);
				taglisten.m_sListen = SOCKET_ERROR;
				continue;
			}
		}


		if (taglisten.m_sListen != INVALID_SOCKET)
		{
			// �ȴ�����δ������������ 
			SOCKADDR_IN saRemote; 
			int nRemoteLen = sizeof( saRemote );
			SOCKET sRemote = ::accept( taglisten.m_sListen, (sockaddr*)&saRemote, &nRemoteLen );

			if (sRemote != INVALID_SOCKET)
			{
				(taglisten.m_pFun)(sRemote,saRemote);
			}
		}
	}
	m_xListenCtrl.unlock();

	Sleep(1);
	return true;
}

void CIOCPConnector::OnDestroy()
{

}
void CIOCPConnector::SetListen(int nPort,C_CREATESOCKET pFun)
{
	tagListen taglisten;
	taglisten.m_nPort = nPort;
	taglisten.m_pFun = pFun;
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
__int64 CSocketConnector::s_nIDCount = 0;
//#ifdef _DEBUG
std::mutex CSocketConnector::s_xRecvDataAmountCtrl;
std::mutex CSocketConnector::s_xSendDataAmountCtrl;
unsigned long CSocketConnector::s_unRecvDataAmount = 0;
unsigned long CSocketConnector::s_unSendDataAmount = 0;

std::mutex CSocketConnector::s_xRecvMsgAmountCtrl;
std::mutex CSocketConnector::s_xSendMsgAmountCtrl;
unsigned long CSocketConnector::s_unRecvMsgAmount = 0;
unsigned long CSocketConnector::s_unSendMsgAmount = 0;

void CSocketConnector::AddRecvDataAmount(unsigned int nSize)
{
	s_xRecvDataAmountCtrl.lock();
	s_unRecvDataAmount += nSize;
  s_xRecvDataAmountCtrl.unlock();
}
void CSocketConnector::AddSendDataAmount(unsigned int nSize)
{
	s_xSendDataAmountCtrl.lock();
	s_unSendDataAmount += nSize;
  s_xSendDataAmountCtrl.unlock();
}
unsigned long CSocketConnector::GetRecvDataAmount()
{
  unsigned long nResult = 0;
	s_xRecvDataAmountCtrl.lock();
  nResult = s_unRecvDataAmount;
  s_xRecvDataAmountCtrl.unlock();
	return nResult;
}
unsigned long CSocketConnector::GetSendDataAmount()
{
  unsigned long nResult = 0;
	s_xSendDataAmountCtrl.lock();
  nResult = s_unSendDataAmount;
  s_xSendDataAmountCtrl.unlock();
	return nResult;
}
void CSocketConnector::ClearDataAmount()
{
	{
		s_xSendDataAmountCtrl.lock();
		s_unSendDataAmount = 0;
    s_xSendDataAmountCtrl.unlock();
	}
	{
		s_xRecvDataAmountCtrl.lock();
		s_unRecvDataAmount = 0;
    s_xRecvDataAmountCtrl.unlock();
	}
}
void CSocketConnector::AddRecvMsgAmount(unsigned int nSize)
{
	s_xRecvMsgAmountCtrl.lock();
	s_unRecvMsgAmount += nSize;
  s_xRecvMsgAmountCtrl.unlock();
}
void CSocketConnector::AddSendMsgAmount(unsigned int nSize)
{
	s_xSendMsgAmountCtrl.lock();
	s_unSendMsgAmount += nSize;
  s_xSendMsgAmountCtrl.unlock();
}
unsigned long CSocketConnector::GetRecvMsgAmount()
{
	s_xRecvMsgAmountCtrl.lock();
	return s_unRecvMsgAmount;
  s_xRecvMsgAmountCtrl.unlock();
}
unsigned long CSocketConnector::GetSendMsgAmount()
{
  unsigned long nResult = 0;
	s_xSendMsgAmountCtrl.lock();
  nResult = s_unSendMsgAmount;
  s_xSendMsgAmountCtrl.unlock();
	return nResult;
}
void CSocketConnector::ClearMsgAmount()
{
	{
		s_xSendMsgAmountCtrl.lock();
		s_unSendMsgAmount = 0;
    s_xSendMsgAmountCtrl.unlock();
	}
	{
		s_xRecvMsgAmountCtrl.lock();
		s_unRecvMsgAmount = 0;
    s_xRecvMsgAmountCtrl.unlock();
	}
}
//#endif
CSocketConnector::CSocketConnector(char *pPassword)
{
	//PERF_ALLOCATO("CSocketConnector",1);
	memset(strIP,0,sizeof(strIP));
	m_Socket = INVALID_SOCKET;
	//m_iRecvSize = 0;
	//m_iRecvRestSize = 0;
	//m_iSendRestStart = 0;
	m_iSendRestSize = 0;
	m_dwConnecttorSpecialKey = 0;
	m_lHaveSentSendReq = 0;
	m_lHaveSentThreadSafeReq = 0;
	m_lSendDataCount  = 0;
	m_lHasPutRecvDataCount = 0;
	m_lCurRecvDataCount = 0;
	//m_lHaveSentRecvReq = 0;
	//m_lHaveSentPolicyFileRequestReq = 0;
	m_nNeedRecvData = 0;
	m_nRecvDataSizeInQueue= 0;
	m_nSendDataSizeInQueue = 0;
	m_nRecvDataSize = 0;
	m_nRecvErrorDataSize = 0;
	//#ifdef _DEBUG
	//m_msgRecvQueue.SetMonitorFlag("��Ϣ�ӽ��յ������ƽ���ȴ�ʱ��");
	//m_msgSendQueue.SetMonitorFlag("��Ϣ�����󵽷��͵�ƽ���ȴ�ʱ��");
	//#endif

	if (pPassword)
	{
		m_Encrypt.set_key_rc5((const unsigned char*)pPassword,strlen(pPassword));
		m_bNeedEncrypt = true;
	}
	else
	{
		m_bNeedEncrypt = false;
	}

}
CSocketConnector::~CSocketConnector(void)
{
	//PERF_ALLOCATO("CSocketConnector",-1);

}
CSocketConnector* CSocketConnector::CreateNew(SOCKET sRemote,SOCKADDR_IN& saAdd)
{
	CSocketConnector* pConn = new CSocketConnector(NULL); 
	if( pConn == NULL ) 
	{ 
		return false;
	}
	char strbuff[16];
	strcpy_s(strbuff,sizeof(strbuff),inet_ntoa(saAdd.sin_addr));


	pConn->SetSocket(sRemote);
	pConn->SetIP(strbuff);
	pConn->OnInit();

	return pConn;
}
bool CSocketConnector::EnableIoCompletionPort()
{
	m_dwConnecttorSpecialKey = /*timeGetTime() * */m_Socket * rand();
	if( NULL == ::CreateIoCompletionPort( ( HANDLE)m_Socket, CIOCPConnector::GetHIoCompletionPort(), (DWORD)m_dwConnecttorSpecialKey, 0 ))
	{
		return false;
	}
	//m_overlappedrecv.iOpCode = OP_READ;
	//m_overlappedrecv.wsabuf.buf = m_szRecvBuf;
	//m_overlappedrecv.wsabuf.len = IOCP_BUFFER_SIZE;
	//m_overlappedrecv.pConnecttor = this;
	//memset( &m_overlappedrecv.ol,0,sizeof(m_overlappedrecv.ol) );

	//m_overlappedsend.wsabuf.buf = m_sSendRest;
	//m_overlappedsend.wsabuf.len = 0;//BUFFER_SIZE; 
	//m_overlappedsend.iOpCode = OP_WRITE; 
	//m_overlappedsend.pConnecttor = this; 
	//memset( &m_overlappedsend.ol,0,sizeof(m_overlappedsend.ol) );


	//DWORD dwRecv = 0; 
	//DWORD dwFlags = 0;
	//int iRet = ::WSARecv( m_Socket, &m_overlappedrecv.wsabuf, 1, &dwRecv, &dwFlags, &m_overlappedrecv.ol, NULL );

	//if( SOCKET_ERROR == iRet )
	//{
	//	DWORD dwRet = WSAGetLastError();
	//	switch( dwRet )
	//	{
	//	case WSA_IO_PENDING:		
	//		break;
	//	default:
	//		//DebugLogout("CConnectorWithMsgQueue::EnableIoCompletionPort��WSARecv���� %d ����Ͽ�����", dwRet);
	//		//CriticalError();
	//		Close();
	//		break;
	//	}
	//}

	// InterlockedIncrement(&m_lHaveSentThreadSafeReq);
	RecvByOverlapped();
	return true;

}
bool	CSocketConnector::CheckOverlappedIOCompleted()
{
	//::HasOverlappedIoCompleted()
	//if (m_overlappedrecv.ol.Internal != STATUS_PENDING
	//  &&m_overlappedsend.ol.Internal != STATUS_PENDING)
	//{
	//  return true;
	//}
	bool bResult = true;
	//std::list<SOVERLAPPEDPLUS*>::iterator it = m_listRecvOverlapped.begin();
	//for (;it != m_listRecvOverlapped.end();it++)
	//{
	//  SOVERLAPPEDPLUS *pPlus = *it;
	//  if (pPlus->ol.Internal == STATUS_PENDING)
	//  {
	//    bResult = false;
	//    break;
	//  }
	//}
	//if (bResult)
	//{
	//  it = m_listSendOverlapped.begin();
	//  for (;it != m_listSendOverlapped.end();it++)
	//  {
	//    SOVERLAPPEDPLUS *pPlus = *it;
	//    if (pPlus->ol.Internal == STATUS_PENDING)
	//    {
	//      bResult = false;
	//      break;
	//    }
	//  }
	//}
	//if (m_listSendOverlapped.size() > 0 || m_listRecvOverlapped.size() > 0)
	//{
	//  return false;
	//}

	return GetHaveSentThreadSafeReq() <= 0;
}
bool	CSocketConnector::OnProcess()
{ 
	if (true)
	{
		//m_nRecvDataSize = 0;
		m_nRecvErrorDataSize = 0;
	}

	if (IsSocketClosed())
	{
		return false;
	}

	//if (m_nLastRecvMsgTime - timeGetTime() > TIMEOUT_NO_RECVDATA)
	//{
	//  Close();
	//  return false;
	//}

	//{
	//  long nRecvSize = m_listRecvOverlapped.GetSize();
	//  for (long i = 0; i < nRecvSize; i++)
	//  {
	//    SOVERLAPPEDPLUS *pPlus = m_listRecvOverlapped.Get();
	//    if (pPlus)
	//    {
	//      if (m_lCurRecvDataCount+1 < pPlus->GetID())
	//      {
	//        if (m_listRecvOverlapped.GetSize() > 32)
	//        {
	//          Close();
	//        }
	//        break;
	//      }
	//      m_lCurRecvDataCount++;
	//      Recv(pPlus->wsabuf.buf,pPlus->wsabuf.len);
	//      CIOCPConnector::s_OverlappedCache.Release(pPlus);
	//    }
	//    m_listRecvOverlapped.Pop();
	//  }
	//  if (m_lHasPutRecvDataCount - m_lCurRecvDataCount < 2)
	//  {
	//    RecvByOverlapped();
	//  }
	//}
	//if (m_lHaveSentSendReq <= 0)
	{
		//��Ҫ���͵������ύ��IOCP
		//if (m_msgSendQueue.GetSize() > 0 || m_iSendRestSize > 0)
		//AUTO_FUN_PERF();
		char *pData = NULL;
		if (m_msgSendQueue.GetSize() == 0)
		{
			m_nSendDataSizeInQueue = 0;
		}
		//m_nSendDataSizeInQueue = 0;
		bool bNeedSend = m_iSendRestSize>0?true:false;
		while (m_lHaveSentSendReq < 16 && (m_msgSendQueue.GetSize() > 0 || m_iSendRestSize > 0))
		{

			//m_iSendRestSize = 0;

			//��֯buf	
			//����Ҫ���ٽ���
			while (m_msgSendQueue.GetSize() > 0)
			{
				int nLen = 0;
				pData = m_msgSendQueue.Get(nLen);

				//PrintDebug("datasend","����ƴװ��ʼ ����ǰ����ָ��%d,�ѷ��ͳ���%d���ܳ���%d����ǰ���ݿ�����%d",pData,m_nSendDataSizeInQueue,nLen,m_iSendRestSize);
				if (m_nSendDataSizeInQueue < 0 || m_nSendDataSizeInQueue >= nLen)
				{
					//PrintDebug("datasend","���ݷ�����ɣ���ǰ����ָ��%d,�ѷ��ͳ���%d���ܳ���%d",pData,m_nSendDataSizeInQueue,nLen);
					m_nSendDataSizeInQueue = 0;
					m_msgSendQueue.Pop();

					continue;
				}
				if (pData)
				{
					//memcpy((m_sSendRest + iWritePos),pData,nLen);
					//m_iSendRestSize += nLen;
					//iWritePos += nLen;
					//iSizeRest -= iSizeRest;
					if (nLen - m_nSendDataSizeInQueue + m_iSendRestSize > IOCP_BUFFER_SIZE)//����������
					{
						memcpy((m_sSendRest + m_iSendRestSize),pData+m_nSendDataSizeInQueue,IOCP_BUFFER_SIZE-m_iSendRestSize);
						m_nSendDataSizeInQueue = m_nSendDataSizeInQueue + IOCP_BUFFER_SIZE-m_iSendRestSize;
						m_iSendRestSize =  IOCP_BUFFER_SIZE;
						//PrintDebug("datasend","����ƴװ���� ����ǰ����ָ��%d,�ѷ��ͳ���%d���ܳ���%d����ǰ���ݿ�����%d",pData,m_nSendDataSizeInQueue,nLen,m_iSendRestSize);
						break;
					}
					else
					{
						memcpy((m_sSendRest + m_iSendRestSize),pData+m_nSendDataSizeInQueue,nLen-m_nSendDataSizeInQueue);
						m_iSendRestSize = m_iSendRestSize + nLen - m_nSendDataSizeInQueue;
						//iWritePos += nLen;

						m_nSendDataSizeInQueue = nLen;
						//PrintDebug("datasend","����ƴװ���� ����ǰ����ָ��%d,�ѷ��ͳ���%d���ܳ���%d����ǰ���ݿ�����%d",pData,m_nSendDataSizeInQueue,nLen,m_iSendRestSize);
					}
				}
				else
				{

				}
			}

			if (bNeedSend || m_iSendRestSize > IOCP_BUFFER_SIZE / 2)
			{
				if (SendByOverlapped(m_sSendRest,m_iSendRestSize) == false)
				{
					break;
				}
				bNeedSend = false;
				m_iSendRestSize = 0;
			}
			else
			{
				break;
			}
		}
	}



	return true;
}

void CSocketConnector::OnInit()
{

	m_nID = ++s_nIDCount;


	//memset(strIP,0,sizeof(strIP));
	//m_nPort = 0;
	//if (m_Socket != INVALID_SOCKET)
	//{
	//	::closesocket( m_Socket );
	//}
	//m_Socket = INVALID_SOCKET;
	//m_iRecvSize = 0;
	//m_iRecvRestSize = 0;
	//m_iSendRestStart = 0;
	//m_iSendRestSize = 0;
	//m_dwConnecttorSpecialKey = 0;
	//m_lHaveSentSendReq = 0;
	//m_lHaveSentPolicyFileRequestReq = 0;

	m_nRecvDataSize = 0;
	m_nRecvErrorDataSize = 0;
	m_nNeedRecvData = 0;
	m_nRecvDataSizeInQueue= 0;

	m_nSendDataSizeInQueue = 0;

	SetLastActionMsgTime(CMyTime::GetClockTime());
	//SetLastSendMsgTime(timeGetTime());
	//m_NeedEnd = false;
	EnableIoCompletionPort();
}
void CSocketConnector::OnDestroy()
{

}
void CSocketConnector::SetIP(char *pStr)
{
	if (pStr)
	{
		strcpy_s(strIP,sizeof(strIP),pStr);
	}
	else
	{
		strcpy_s(strIP,sizeof(strIP),"0.0.0.0");
	}
}
char *CSocketConnector::GetIP()
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
bool CSocketConnector::Recv(char *pBytes,int nLen)
{
	//AUTO_FUN_PERF();
	ADD_RECV_DATA_AMOUNT(nLen);
	PrintDebug("iocp"," %d:��������,����:%d",(int)GetID(),nLen);
	//return true;
	//return SendByOverlapped(pBytes,nLen);

	static DWORD dwLastLogTime = CMyTime::GetClockTime();
	//if (m_lHaveSentPolicyFileRequestReq > 0)
	//{
	//	return 0;
	//}
	if (nLen < 0)
	{
		assert(0);
	}

	//int iNeed;

	if( nLen <= 0 )
		return true;

	// m_iRecvRestSize : ��ǰ��Ϣ�����buff���Ѿ��������ֽ��� 
	// ����Ҫ�����ɵĺ����ķ�����ܹ���һ����Ϣ

	//m_nLastRecvMsgTime = timeGetTime();

	m_nRecvDataSize += nLen;

	m_xRecvMsgCtrl.lock();
	//if( m_iRecvRestSize + nLen < BUFFER_SIZE )
	//{
	//	memcpy( m_sRecvRest+m_iRecvRestSize,pBytes,nLen );
	//	m_iRecvRestSize += nLen;
	//}
	//�»��ƣ����һ����Ϣ��δ���꣬Ҳ����m_msgRecvQueue�ﴴ���¿ռ�
	//Ȼ��ÿ��һ�����Queue��дһ��
	int nPos = 0;
	while (nLen > nPos)
	{
		if (m_nNeedRecvData > 0)
		{
			int nBuffLen = 0;
			char *pBuff = m_msgRecvQueue.GetLast(nBuffLen);
			if (pBuff)
			{
				if (nLen >= nPos + m_nNeedRecvData)
				{
					memcpy(pBuff+m_nRecvDataSizeInQueue,pBytes+nPos,m_nNeedRecvData);
					if (m_bNeedEncrypt)
					{
						//��Ҫ����
						m_Encrypt.UnEncryptRC5(pBuff,nBuffLen);
					}
					m_nRecvDataSizeInQueue += m_nNeedRecvData;
					nPos += m_nNeedRecvData;
					//char* pBuffPoint = &m_sRecvRest[m_nNeedRecvData];
					//memcpy(m_sRecvRest, pBuffPoint, m_iRecvRestSize);
					m_nNeedRecvData = 0;
					//PrintDebug("datarecv","��ǰ��Ϣ������ɣ���ǰ����������%d,�ܽ�����%lld,��ǰ���%d,��Ϣ����,%d",nLen,m_nRecvDataSize,nPos,nBuffLen);
				}
				else
				{
					memcpy(pBuff+m_nRecvDataSizeInQueue,pBytes+nPos,nLen-nPos);
					m_nRecvDataSizeInQueue = m_nRecvDataSizeInQueue + (nLen - nPos);
					//char* pBuffPoint = &m_sRecvRest[m_nRecvDataSizeInQueue];
					//memcpy(m_sRecvRest, pBuffPoint, m_iRecvRestSize);
					m_nNeedRecvData = m_nNeedRecvData - (nLen-nPos);
					nPos = nLen;
					//PrintDebug("datarecv","��ǰ��Ϣ�����У���ǰ����������%d,�ܽ�����%lld,��ǰ���%d,��Ϣ����,%d��������ܳ���%d",nLen,m_nRecvDataSize,nPos,nBuffLen,m_nNeedRecvData);
					break;
				}
			}
			else
			{
				m_nNeedRecvData = 0;
			}
		}
		else
		{

			if ( m_vRecvHeadCache.size() + (nLen - nPos) >= 4 )
			{
				for (int i = m_vRecvHeadCache.size(); i < 4; i++)
				{
					m_vRecvHeadCache.push_back(*(pBytes+nPos));
					nPos++;
				}
				MsgHeader *pHeader = (MsgHeader *)&m_vRecvHeadCache[0];

				char cflag = 0xfd;
				//�յ�<policy-file-request/>�Ļ�
				if (pHeader->unflag1 == '<' && pHeader->unflag2 == 'p' && pHeader->unflag3 == 'o' && pHeader->unflag4 == 'l')
				{
					if (nLen>=22)
					{
						int nRecvlen = strlen("<policy-file-request/>");
						if (memcmp(pBytes,"<policy-file-request/>",nRecvlen) == 0)
						{
							static char strAnswerData[512];
							int nPos = 0;
							const char *pStrTemp = "<?xml version=\"1.0\"?><cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" />  </cross-domain-policy> ";
							int nStrLen = strlen(pStrTemp);

							//CCriticalLock cLock(&m_xSendCtrl,true);

							//m_iRecvRestSize -= nStrlen;

							//memcpy(m_sRecvRest, m_sRecvRest+nlen, m_iRecvRestSize);

							//memcpy(m_sSendRest+m_iSendRestSize ,pStrTemp,nStrLen+1);
							//m_iSendRestSize += nStrLen+1;

							SendByOverlapped(pStrTemp,m_iSendRestSize);

							break;
						}
						else
						{
							break;
							//char strRequst[32];
							//memcpy(strRequst,m_sRecvRest,30);
							//strRequst[31] = 0;
							//PrintDebug("iocp","���ư�ȫɳ�����������(%s),����������ݰ�",strRequst);

							//m_nRecvErrorDataSize += 4;
							//m_iRecvRestSize -= 4;
							//memcpy(m_sRecvRest, m_sRecvRest+4, m_iRecvRestSize);
						}
					}

					//m_NeedEnd = true;
				}

				//char cType = DecodeBytes2Char(m_sRecvRest,nPos);
				int nLoadLength = 0;
				unsigned int stLength = DecodeBytes2Int(&m_vRecvHeadCache[0],nLoadLength,4);

				if (stLength > MSG_DATASIZE_TOBIG)
				{
					PrintDebug("iocp","id:%lld ��Ϣ���ȹ���(%d),�����ǷǶ�Ӧ�ͻ��˷���",GetID(),stLength);
					return false;
				}
				m_vRecvHeadCache.clear();
				//PrintDebug("datarecv","��ʼһ������Ϣ����ǰ����������%d,�ܽ�����%lld,��ǰ���%d,��Ϣ����%d",nLen,m_nRecvDataSize,nPos,stLength);
				//�����

				//�������İ�,����������İ�
				//char* pBuffPoint = &m_sRecvRest[0];

				if ((int)stLength + nPos <= nLen)
				{
					char *pBuff = m_msgRecvQueue.New(stLength);
					memcpy(pBuff, pBytes+nPos, stLength);
					//m_msgRecvQueue.Push(pBytes+nPos,stLength);
					if (m_bNeedEncrypt)
					{
						//��Ҫ����
						m_Encrypt.UnEncryptRC5(pBuff,stLength);
					}

					nPos = nPos + stLength;
					//m_iRecvRestSize -= nPos;
					//char* pBuffPoint = &m_sRecvRest[nPos];

					//memcpy(m_sRecvRest, pBuffPoint, m_iRecvRestSize);
				}
				else
				{
					//û�������İ�
					char *pBuff = m_msgRecvQueue.New(stLength);

					memcpy(pBuff, pBytes+nPos, nLen-nPos);

					//m_iRecvRestSize -= nPos;
					m_nRecvDataSizeInQueue = nLen-nPos;
					m_nNeedRecvData = stLength - (nLen-nPos);
					nPos = nLen;
					break;
				}
			}
			else
			{
				for (int i = nPos; i < nLen; i++)
				{
					m_vRecvHeadCache.push_back(*(pBytes+nPos));
					nPos++;
				}
			}
		}
	}
  m_xRecvMsgCtrl.unlock();
	////�»��ƣ����һ����Ϣ��δ���꣬Ҳ����m_msgRecvQueue�ﴴ���¿ռ�
	////Ȼ��ÿ��һ�����Queue��дһ��
	//if (m_nNeedRecvData > 0)
	//{
	//	int nBuffLen = 0;
	//	char *pBuff = m_msgRecvQueue.GetLast(nBuffLen);
	//	if (pBuff)
	//	{
	//		if (nLen >= m_nNeedRecvData)
	//		{
	//			memcpy(pBuff+m_nRecvDataSizeInQueue,pBytes,m_nNeedRecvData);
	//			if (m_bNeedEncrypt)
	//			{
	//				//��Ҫ����
	//				m_Encrypt.UnEncryptRC5(pBuff,nBuffLen);
	//			}
	//			nLen -= m_nNeedRecvData;
	//			m_nRecvDataSizeInQueue += m_nNeedRecvData;
	//			char* pBuffPoint = &m_sRecvRest[m_nNeedRecvData];
	//			memcpy(m_sRecvRest, pBuffPoint, m_iRecvRestSize);
	//			m_nNeedRecvData = 0;
	//		}
	//		else
	//		{
	//			memcpy(pBuff+m_nRecvDataSizeInQueue,m_sRecvRest,m_iRecvRestSize);
	//			m_nRecvDataSizeInQueue += m_iRecvRestSize;
	//			//char* pBuffPoint = &m_sRecvRest[m_nRecvDataSizeInQueue];
	//			//memcpy(m_sRecvRest, pBuffPoint, m_iRecvRestSize);
	//			m_nNeedRecvData -= m_iRecvRestSize;
	//			m_iRecvRestSize = 0;
	//		}
	//	}
	//	else
	//	{
	//		m_nNeedRecvData = 0;
	//	}
	//}
	//else
	//if (m_nNeedRecvData <= 0)
	//{
	//	while ( m_iRecvRestSize >= 4 )
	//	{
	//		MsgHeader *pHeader = (MsgHeader *)m_sRecvRest;

	//		char cflag = 0xfd;
	//		//�յ�<policy-file-request/>�Ļ�
	//		if (pHeader->unflag1 == '<' && pHeader->unflag2 == 'p' && pHeader->unflag3 == 'o' && pHeader->unflag4 == 'l')
	//		{
	//			if (m_iRecvRestSize>=22)
	//			{
	//				int nlen = strlen("<policy-file-request/>");
	//				if (memcmp(m_sRecvRest,"<policy-file-request/>",nlen) == 0)
	//				{
	//					static char strAnswerData[512];
	//					int nPos = 0;
	//					const char *pStrTemp = "<?xml version=\"1.0\"?><cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" />  </cross-domain-policy> ";
	//					int nStrLen = strlen(pStrTemp);

	//					//CCriticalLock cLock(&m_xSendCtrl,true);

	//					m_iRecvRestSize -= nlen;

	//					memcpy(m_sRecvRest, m_sRecvRest+nlen, m_iRecvRestSize);

	//					memcpy(m_sSendRest+m_iSendRestSize ,pStrTemp,nStrLen+1);
	//					m_iSendRestSize += nStrLen+1;

	//					SendByOverlapped(m_sRecvRest,m_iSendRestSize);

	//					break;
	//				}
	//				else
	//				{
	//					char strRequst[32];
	//					memcpy(strRequst,m_sRecvRest,30);
	//					strRequst[31] = 0;
	//					PrintDebug("iocp","���ư�ȫɳ�����������(%s),����������ݰ�",strRequst);

	//					m_nRecvErrorDataSize += 4;
	//					m_iRecvRestSize -= 4;
	//					memcpy(m_sRecvRest, m_sRecvRest+4, m_iRecvRestSize);
	//				}
	//			}

	//			//m_NeedEnd = true;
	//		}
	//		int nPos = 0;

	//		
	//		//char cType = DecodeBytes2Char(m_sRecvRest,nPos);
	//		unsigned int stLength = DecodeBytes2Int(m_sRecvRest,nPos,m_iRecvRestSize);

	//     if (stLength > MSG_DATASIZE_TOBIG)
	//     {
	//       PrintDebug("iocp","id:%lld ��Ϣ���ȹ���(%d),�����ǷǶ�Ӧ�ͻ��˷���",GetID(),stLength);
	//       return false;
	//     }
	//		//�����

	//		//�������İ�,����������İ�
	//		//char* pBuffPoint = &m_sRecvRest[0];

	//		if ((int)stLength + nPos <= m_iRecvRestSize)
	//		{
	//			if (m_bNeedEncrypt)
	//			{
	//				//��Ҫ����
	//				m_Encrypt.UnEncryptRC5(&m_sRecvRest[nPos],stLength);
	//			}
	//			m_msgRecvQueue.Push(&m_sRecvRest[nPos],stLength);
	//			nPos = nPos + stLength;
	//			m_iRecvRestSize -= nPos;
	//			char* pBuffPoint = &m_sRecvRest[nPos];

	//			memcpy(m_sRecvRest, pBuffPoint, m_iRecvRestSize);
	//		}
	//		else
	//		{
	//			//û�������İ�
	//			char *pBuff = m_msgRecvQueue.New(stLength);

	//			m_iRecvRestSize -= nPos;
	//			memcpy(pBuff, &m_sRecvRest[nPos], m_iRecvRestSize);

	//			//m_iRecvRestSize -= nPos;
	//			m_nRecvDataSizeInQueue = m_iRecvRestSize;
	//			m_nNeedRecvData = stLength - m_iRecvRestSize;
	//			m_iRecvRestSize = 0;
	//			break;
	//		}
	//	}
	//}
	return true;
}
bool CSocketConnector::Recv(SOVERLAPPEDPLUS *pPlus)
{
	if (pPlus == NULL)
	{
		return false;
	}
	m_xRecvOverlappedCtrl.lock();
	//m_mapRecvOverlapped[pPlus->nDataIndex] = pPlus;
	m_listRecvOverlapped.Add(pPlus);
  m_xRecvOverlappedCtrl.unlock();
	return true;
}
//int CSocketConnector::UpdataSendData(unsigned int iSentBytes)
//{
//  AUTO_FUN_PERF();
//  ADD_SEND_DATA_AMOUNT(iSentBytes);
//	if (m_iSendRestSize == 0 && m_msgSendQueue.GetSize() == 0)
//	{
//		return 0;
//	}
//	//m_nLastSendMsgTime = timeGetTime();
//
//	//���Ƚ�δ��������ǰ��
//	//m_iSendRestStart += iSentBytes;
//	m_iSendRestSize -= iSentBytes;
//  PrintDebug("iocp"," IOCP:%lld:��������,����:%d��������ʣ�೤�� %d",GetID(),iSentBytes,m_iSendRestSize);
//	if(m_iSendRestSize > 0 )
//	{
//		memcpy( m_sSendRest,m_sSendRest+iSentBytes,m_iSendRestSize );
//	}
//	else
//	{
//		m_iSendRestSize = 0;
//	}
//	//Ȼ���ٴ���Ϣ��������ȡһЩ����
//	//CCriticalLock cLock(&m_xSendCtrl,true);	
//  //PERF_CRITICALLOCK_TIME(m_xSendCtrl);
//	if (m_msgSendQueue.GetSize() > 0)
//	{
//		char *pData = NULL;
//		//��֯buf	
//		//int iWritePos = m_iSendRestSize;
//		//iWritePos += m_iSendRestStart;
//
//		//int iSizeRest;
//		//iSizeRest = BUFFER_SIZE;
//		//iSizeRest -= iWritePos;		
//
//		while (m_msgSendQueue.GetSize() > 0/* && iSizeRest > 0*/)
//		{
//      PrintDebug("iocp"," IOCP:%lld:���������� %d, ��Ϣ���г��� %d",GetID(),m_iSendRestSize,m_msgSendQueue.GetSize());
//			int nLen = 0;
//			pData = m_msgSendQueue.Get(nLen);
//			if (m_nSendDataSizeInQueue < 0 || m_nSendDataSizeInQueue >= nLen)
//			{
//				m_nSendDataSizeInQueue = 0;
//				m_msgSendQueue.Pop();
//				continue;
//			}
//			if (pData)
//			{
//				if (nLen - m_nSendDataSizeInQueue + m_iSendRestSize > BUFFER_SIZE)//����������
//				{
//					memcpy((m_sSendRest + m_iSendRestSize),pData+m_nSendDataSizeInQueue,BUFFER_SIZE-m_iSendRestSize);
//					m_nSendDataSizeInQueue = m_nSendDataSizeInQueue + BUFFER_SIZE-m_iSendRestSize;
//					m_iSendRestSize =  BUFFER_SIZE;
//					break;
//				}
//				else
//				{
//					memcpy((m_sSendRest + m_iSendRestSize),pData+m_nSendDataSizeInQueue,nLen-m_nSendDataSizeInQueue);
//					m_iSendRestSize = m_iSendRestSize + (nLen-m_nSendDataSizeInQueue);
//
//					m_nSendDataSizeInQueue = nLen;
//				}
//
//        
//				//iSizeRest -= iSizeRest;
//			}
//			else
//			{
//				//��ӡ��Ϣ����m_msgSendQueue�������ȡ��Ϣʧ�ܣ�����
//        PrintDebug("error","m_msgSendQueue������Ϣ��ȡʧ��");
//        m_msgSendQueue.Pop();
//			}
//			//m_msgSendQueue.Pop();
//		}
//
//	}
//  PrintDebug("iocp"," IOCP:%lld:������ʣ�೤�� %d",GetID(),m_iSendRestSize);
//	return m_iSendRestSize;
//}

bool CSocketConnector::SendData(unsigned char *pBuff,int len)
{
	//����pbuff��ĸߵ�λ�Ѿ�ת������
	if (pBuff == NULL)
	{
		return false;
	}

	//PrintDebug("iocp","m_xSendCtrl,׼�������ٽ���2");
	//CCriticalLock cLock(&m_xSendCtrl,true);	
	//PERF_CRITICALLOCK_TIME(m_xSendCtrl);
	PrintDebug("iocp","IOCP׼����һ����Ϣ,ID:%lld,����:%d",GetID(),len);
	bool bResult = false;
	int nPos = 0;
	unsigned int datalen = len + 4;
	char *pMsgBuff = m_msgSendQueue.New(datalen);
	//char sMsgBuf[ CONNECTDATA_MAXSIZE ];
	unsigned char cflag = 0xfd;
	//AddChar2Bytes(pMsgBuff,cflag,nPos,datalen);
	//AddChar2Bytes(pMsgBuff,cflag,nPos,datalen);
	//AddChar2Bytes(pMsgBuff,cflag,nPos,datalen);
	//AddChar2Bytes(pMsgBuff,cflag,nPos,datalen);
	AddData2Bytes(pMsgBuff,(char*)pBuff,len,nPos,datalen);
	if (m_bNeedEncrypt)
	{
		//��Ҫ����
		m_Encrypt.EncryptRC5(pMsgBuff+8,len);
	}

	//{
	//	CCriticalLock cLock(&m_xSendCtrl,true);
	//	m_msgSendQueue.Push(sMsgBuf,nPos);
	//}
	//PrintDebug("iocp","IOCP׼��������Ϣ�Ѿ��������,ID:%lld,����:%d",GetID(),len);

	//PrintDebug("iocp","m_xSendCtrl,׼���˳��ٽ���2");
	return true;
}
char *CSocketConnector::GetData(int &len)
{
	m_xRecvMsgCtrl.lock();
	len = 0;
  char *pData = NULL;
	unsigned int nSize = m_msgRecvQueue.GetSize();
	//��������ݻ��ڽ���,���һ�����ݲ����ṩ
	if (m_nNeedRecvData > 0)
	{
		if (nSize > 1)
		{
			pData = m_msgRecvQueue.Get(len);
		}
	}
	else
	{
		if (nSize > 0)
		{
			pData = m_msgRecvQueue.Get(len);
		}
	}
  m_xRecvMsgCtrl.unlock();
	return pData;
}
bool CSocketConnector::PopData()
{
	m_xRecvMsgCtrl.lock();
	unsigned int nSize = m_msgRecvQueue.GetSize();

	//��������ݻ��ڽ���,���һ�����ݲ��ܵ���
	if (m_nNeedRecvData > 0)
	{
		if (nSize > 1)
		{
			m_msgRecvQueue.Pop();
		}
	}
	else
	{
		if (nSize > 0)
		{
			m_msgRecvQueue.Pop();
		}
	}
  m_xRecvMsgCtrl.unlock();
	return true;
}
unsigned int CSocketConnector::GetDataSize()
{
	m_xRecvMsgCtrl.lock();
	unsigned int nResult = m_msgRecvQueue.GetSize();

	//��������ݻ��ڽ���
	if (m_nNeedRecvData > 0)
	{
    m_xRecvMsgCtrl.unlock();
		return nResult - 1;
	}
  m_xRecvMsgCtrl.unlock();
	return nResult;
}
//bool CConnector::SendMsg(CBaseMsg *pMsg)
//{
//	if (pMsg == NULL)
//	{
//		return false;
//	}
//	SendData(pMsg,pMsg->stLength);
//	return true;
//}
//CBaseMsg* CConnector::GetMsg()
//{
//	CCriticalLock cLock(&m_xRecvCtrl);
//	CBaseMsg *pMsg = m_msgRecvQueue.GetMsg();
//	if (pMsg)
//	{
//		return pMsg;
//	}
//	return NULL;
//}
//bool CConnector::GetMsg(void *pbuff,int& len)
//{
//	CCriticalLock cLock(&m_xRecvCtrl);
//	CBaseMsg *pMsg = m_msgRecvQueue.GetMsg();
//	if (pMsg)
//	{
//		len = pMsg->stLength;
//		memcpy(pbuff,pMsg,pMsg->stLength);
//		m_msgRecvQueue.PopMsg();
//		return true;
//	}
//
//	m_msgRecvQueue.PopMsg();
//	return false;
//}
//bool CConnector::PopMsg(CDataQueue *pQueue)
//{
//	if (pQueue == NULL)
//	{
//		return false;
//	}
//	CCriticalLock cLock(&m_xRecvCtrl);
//	while (m_msgRecvQueue.GetSize() > 0)
//	{
//		CBaseMsg *pMsg = m_msgRecvQueue.GetMsg();
//		if (pMsg)
//		{
//			pQueue->PushMsg(pMsg);
//		}
//		m_msgRecvQueue.PopMsg();
//	}
//	return true;
//}
void CSocketConnector::SetEncryptKey(char *pPassword)
{
	if (pPassword)
	{
		m_Encrypt.set_key_rc5((const unsigned char*)pPassword,strlen(pPassword));
		m_bNeedEncrypt = true;
	}
	else
	{
		m_bNeedEncrypt = false;
	}
}
void CSocketConnector::CancelEncrypt()
{
	m_bNeedEncrypt = false;
}
void CSocketConnector::Close()
{	
	m_xSocketCtrl.lock();
	if (m_Socket != INVALID_SOCKET)
	{
		::closesocket( m_Socket );
	}
	m_Socket = INVALID_SOCKET;
  m_xSocketCtrl.unlock();
}
bool CSocketConnector::Open()
{

	//SafeCopy(m_szIP, szName, IPSTR_SIZE);


	if(m_Socket == INVALID_SOCKET)
	{
		m_Socket = socket(PF_INET, SOCK_STREAM, 0);
		if(m_Socket == INVALID_SOCKET)
		{
			// Init socket() failed.
			return false;
		}

		// ����Ϊ��������ʽ
		unsigned long i = 1;
		if(ioctlsocket(m_Socket, FIONBIO, &i))
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
	PrintDebug("network","�������ӷ�����,IP:%s,PORT:%d",strIP,m_nPort);
	int ret = connect(m_Socket, (sockaddr *)&addr, sizeof(addr));
	if(ret == -1)
	{
		int err = WSAGetLastError();
		if(err != WSAEWOULDBLOCK)
		{
			//Close();
			return false;
		}
		else
		{
			timeval timeout;
			timeout.tv_sec	= 30;
			timeout.tv_usec	= 0;
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
			else if(ret < 0)
			{
				int	err = WSAGetLastError();
				Close();
				return false;
			}
			else	// ret > 0
			{
				if(FD_ISSET(m_Socket, &exceptset))		// or (!FD_ISSET(m_sockClient, &writeset)
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
	{
		m_xRecvMsgCtrl.lock();
		while (m_msgRecvQueue.GetSize() > 0)
		{
			m_msgRecvQueue.Pop();
		}
    m_xRecvMsgCtrl.unlock();
	}
	// PrintDebug("iocp","m_xSendCtrl,׼�������ٽ���3");
	{
		//CCriticalLock cLock(&m_xSendCtrl);
		while (m_msgSendQueue.GetSize() > 0)
		{
			m_msgSendQueue.Pop();
		}
	}
	// PrintDebug("iocp","m_xSendCtrl,�˳��ٽ���3");
	memset(strIP,0,sizeof(strIP));
	m_nPort = 0;
	if (m_Socket != INVALID_SOCKET)
	{
		::closesocket( m_Socket );
	}
	m_Socket = INVALID_SOCKET;
	//m_iRecvSize = 0;
	//m_iRecvRestSize = 0;
	//	m_iSendRestStart = 0;
	m_iSendRestSize = 0;
	m_dwConnecttorSpecialKey = 0;
	m_lHaveSentSendReq = 0;
	//m_lHaveSentThreadSafeReq = 0;

	m_nRecvDataSize = 0;
	m_nRecvErrorDataSize = 0;

	m_nNeedRecvData = 0;
	m_nRecvDataSizeInQueue = 0;
	m_nSendDataSizeInQueue = 0;

	m_nLastActionTime = 0;

}
unsigned int CSocketConnector::GetSendMsgQueueSize()
{
	//CCriticalLock cLock(&m_xSendCtrl,true);
	//PERF_CRITICALLOCK_TIME(m_xSendCtrl);
	return m_msgSendQueue.GetSize();
}
unsigned int CSocketConnector::GetRecvMsgQueueSize()
{
  unsigned int nResult=0;
	m_xRecvMsgCtrl.lock();
  nResult = m_msgRecvQueue.GetSize();
  m_xRecvMsgCtrl.unlock();
	return nResult;
}
bool CSocketConnector::SendByOverlapped(const char *pBuff, unsigned int nLen)
{
	if (pBuff == NULL)
	{
		return false;
	}
	if (nLen > IOCP_BUFFER_SIZE)
	{
		return false;
	}
	SOVERLAPPEDPLUS *pPlus = CIOCPConnector::s_OverlappedCache.New();
	if (pPlus)
	{
		pPlus->wsabuf.len = nLen;
		memcpy(pPlus->data,pBuff,nLen);
		pPlus->iOpCode = OP_WRITE;
		pPlus->pConnecttor = this;
		pPlus->nDataIndex = InterlockedIncrement(&m_lSendDataCount);

		IncHaveSentThreadSafeReq();
		DWORD dwSend;
		int iRet = WSASend( 
			m_Socket,
			&pPlus->wsabuf,
			1,
			&dwSend,
			0,
			&pPlus->ol,
			0 );

		if( SOCKET_ERROR == iRet )
		{
			DWORD dwRet = WSAGetLastError();
			switch( dwRet )
			{
			case WSA_IO_PENDING:
				break;
			default:
				//DebugLogout("CConnectorWithMsgQueue::StartSendData��WSASend���� %d ����Ͽ�����", dwRet);
				DecHaveSentThreadSafeReq();
				Close();			
				return false;
				break;
			}
		}

		InterlockedIncrement(&m_lHaveSentSendReq);
		ADD_SEND_DATA_AMOUNT(nLen);
		//PrintDebug("SendOver","ָ�� %d",pPlus);
		//PrintDebug("datasend"," %d:��������,����:%d,ָ�� %d",(int)GetID(),pPlus->wsabuf.len,pPlus);
		return true;
	}
	return false;
}
bool CSocketConnector::EndSendByOverlapped(SOVERLAPPEDPLUS* pPlus)
{
	if (pPlus == NULL)
	{
		return false;
	}
	InterlockedDecrement(&m_lHaveSentSendReq);
	DecHaveSentThreadSafeReq();
	pPlus->pConnecttor = NULL;
	CIOCPConnector::s_OverlappedCache.Release(pPlus);
	PrintDebug("iocp"," %d:���շ��ͻ���,ָ�� %d",(int)GetID(),pPlus);
	return true;
}
bool CSocketConnector::RecvByOverlapped()
{
	SOVERLAPPEDPLUS *pPlus = CIOCPConnector::s_OverlappedCache.New();
	if (pPlus)
	{
		pPlus->wsabuf.len = IOCP_BUFFER_SIZE;
		pPlus->iOpCode = OP_READ;
		pPlus->pConnecttor = this;
		pPlus->nDataIndex = InterlockedIncrement(&m_lHasPutRecvDataCount);

		DWORD dwRecv = 0; 
		DWORD dwFlags = 0;
		IncHaveSentThreadSafeReq();
		int iRet = ::WSARecv( m_Socket, &pPlus->wsabuf, 1, &dwRecv, &dwFlags, &pPlus->ol, NULL );

		if( SOCKET_ERROR == iRet )
		{
			DWORD dwRet = WSAGetLastError();
			switch( dwRet )
			{
			case WSA_IO_PENDING:
				break;
			default:
				//DebugLogout("CConnectorWithMsgQueue::EnableIoCompletionPort��WSARecv���� %d ����Ͽ�����", dwRet);
				//CriticalError();
				DecHaveSentThreadSafeReq();
				Close();
				break;
			}
		}

		//CCriticalLock xLock(&m_xRecvOverlappedCtrl);

		//InterlockedIncrement(&m_lHaveSentThreadSafeReq);
		PrintDebug("iocp"," %d:�������ݽ��ܻ���,ָ�� %d",(int)GetID(),pPlus);
		return true;
	}
	return false;
}
bool CSocketConnector::RecvEndByOverlapped(SOVERLAPPEDPLUS* pPlus)
{
	if (pPlus == NULL)
	{
		return false;
	}
	//InterlockedDecrement(&m_lHaveSentThreadSafeReq);
	DecHaveSentThreadSafeReq();
	//pPlus->pConnecttor = NULL;
	//CIOCPConnector::s_OverlappedCache.Release(pPlus);
	//CCriticalLock xLock(&m_xRecvOverlappedCtrl);
	//xLock.Lock();
	//std::list<SOVERLAPPEDPLUS*>::iterator it = m_listRecvOverlapped.begin();
	//for (;it != m_listRecvOverlapped.end();it++)
	//{
	//  if (pPlus == *it)
	//  {
	//    m_listRecvOverlapped.erase(it);
	//    break;
	//  }
	//}
	//xLock.UnLock();
	//m_AllocatorOverlapped.Release(pPlus);
	//PrintDebug("iocp"," %d:�������ݽ��ܻ���,ָ�� %d",(int)GetID(),pPlus);
	return true;
}
void __cdecl SendRecvFunc(LPVOID lpParam)
{
	//::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	LPOVERLAPPED lpOverlapped;
	SOVERLAPPEDPLUS *pPlus;
	//PULONG_PTR lpCompletionKey;
	ULONG_PTR dwKey = 0;
	DWORD dwBytesXfered = 0;
	HANDLE hIoCompletionPort = CIOCPConnector::hIoCompletionPort;
	BOOL bRet = FALSE;
	int iRet = 0;
	DWORD dwRet;

	bool bFin = false;
	while( !bFin )
	{
		bRet = GetQueuedCompletionStatus( 
			hIoCompletionPort,	//��Ҫ���ӵ���ɶ˿ھ��
			&dwBytesXfered,		//ʵ�ʴ�����ֽ�
			&dwKey,				//���������
			&lpOverlapped,		//һ��ָ��α����ص�IO��ָ�룬��Сȡ������Ķ���
			1000				////�ȴ�������
			);
		if( 0 == bRet )
		{
			if (lpOverlapped)
			{
				//iocp����һ�����ӶϿ�
				pPlus = (SOVERLAPPEDPLUS*)lpOverlapped;
				CSocketConnector* pConnecttor = pPlus->pConnecttor;
				if (pConnecttor)
				{
					PrintRelease("iocp","SendRecvFunc�������ӶϿ� %d", pPlus);
					//InterlockedDecrement(&pConnecttor->m_lHaveSentThreadSafeReq);
					int nError = GetLastError();
					pConnecttor->Close();
					pConnecttor->DecHaveSentThreadSafeReq();
					PrintRelease("iocp","SendRecvFunc�����ӶϿ� %d,id:%lld", nError,pConnecttor->GetID());
				}
				//delete pPlus;
				CIOCPConnector::s_OverlappedCache.Release(pPlus);
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
			if( dwKey != pConnecttor->m_dwConnecttorSpecialKey )
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
		//InterlockedDecrement(&pConnecttor->m_lHaveSentThreadSafeReq);
		pConnecttor->SetLastActionMsgTime(CMyTime::GetClockTime());

		switch( pPlus->iOpCode )
		{
		case OP_READ:
			{
				if (pConnecttor->IsSocketClosed())
				{
					pConnecttor->RecvEndByOverlapped(pPlus);
					CIOCPConnector::s_OverlappedCache.Release(pPlus);
					break;
				}
				if (dwBytesXfered == 0)
				{
					//һ������£�dwBytesXferedΪ0��ʾ�ͻ��������Ͽ�����
					int nError = GetLastError();
					PrintRelease("iocp","SendRecvFunc�з��ֿͻ��������Ͽ� %d ����Ͽ�����,ID:%lld", nError,pConnecttor->GetID());
					pConnecttor->Close();
					pConnecttor->RecvEndByOverlapped(pPlus);
					CIOCPConnector::s_OverlappedCache.Release(pPlus);
					break;
				}

				PrintDebug("iocp","IOCP(ID:%lld)��������,����:%d",pConnecttor->GetID(),dwBytesXfered);
				pPlus->wsabuf.len = dwBytesXfered;
				if (pConnecttor->Recv((char*)pPlus->wsabuf.buf,dwBytesXfered) == false)
					//if (pConnecttor->Recv(pPlus) == false)
				{
					pConnecttor->Close();
					PrintRelease("iocp","IOCP(ID:%lld)���ݽ���ʧ�ܣ��Ͽ�������",pConnecttor->GetID());
					pConnecttor->RecvEndByOverlapped(pPlus);
					CIOCPConnector::s_OverlappedCache.Release(pPlus);
					break;
				}
				pConnecttor->RecvByOverlapped();
				pConnecttor->RecvEndByOverlapped(pPlus);
				CIOCPConnector::s_OverlappedCache.Release(pPlus);
			}
			break;
		case OP_WRITE:
			{
				if (pConnecttor->IsSocketClosed())
				{
					pConnecttor->EndSendByOverlapped(pPlus);
					//if (pConnecttor->m_lHaveSentPolicyFileRequestReq > 0)
					//{
					//  InterlockedDecrement(&pConnecttor->m_lHaveSentPolicyFileRequestReq);
					//  //PrintDebug("iocp","SendRecvFunc�жϿ�����,Ŀǰ����flash as3�İ�ȫ����,ID:%d", pConnecttor->GetID());
					//  //pConnecttor->Close();
					//}
					break;
				}
				//pPlus->wsabuf.len = pConnecttor->UpdataSendData(dwBytesXfered);

				if (pPlus->wsabuf.len != dwBytesXfered)
				{
					PrintRelease("error","iocp send��Ϣ���������⣬%d//%d",dwBytesXfered,pPlus->wsabuf.len);
				}
				pConnecttor->EndSendByOverlapped(pPlus);
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
	InterlockedDecrement( &CIOCPConnector::m_lActiveThreadNum);
	return;
}
