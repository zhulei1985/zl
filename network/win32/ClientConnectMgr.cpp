#include "ClientConnectMgr.h"
#include "Client.h"

CClientConnectMgr CClientConnectMgr::s_Instance;

CClientConnectMgr::CClientConnectMgr()
{
}


CClientConnectMgr::~CClientConnectMgr()
{
}

CSocketConnector* CClientConnectMgr::CreateNew(SOCKET sRemote, const char* pIP)
{
	CSocketConnector* pConnector = new CClient;
	if (pConnector == NULL)
	{
		return NULL;
	}

	pConnector->SetSocket(sRemote);
	pConnector->SetIP(pIP);
	pConnector->OnInit();
	CClientConnectMgr::GetInstance()->AddClient(pConnector);
	printf("new connect: %s; all connect count: %d \n", pIP, CClientConnectMgr::GetInstance()->GetConnectSize());
	return pConnector;
}

void CClientConnectMgr::OnProcess()
{
	CSocketConnectorMgr::OnProcess();

	auto curTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - m_LastSendMsgTime);
	if (duration.count() >= 1000)
	{
		m_LastSendMsgTime = curTime;
		
		printf("all client recv data size : %d \n", (int)CClient::m_RecvSize);
		CClient::m_RecvSize = 0;

		printf("all client send data size : %d \n", (int)CClient::m_SendSize);
		CClient::m_SendSize = 0;

		printf("IOCP recv data size : %lld \n", (unsigned long long)CIOCPConnector::m_AllRecvDataCount);
		CIOCPConnector::m_AllRecvDataCount = 0;

		printf("IOCP send data size : %lld \n", (unsigned long long)CIOCPConnector::m_AllSendDataCount);
		CIOCPConnector::m_AllSendDataCount = 0;
	}
}
