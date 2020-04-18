#include "ClientConnectMgr.h"
#include "Client.h"

void InitNetworkConnect()
{
	if (!initSocket())
	{
		//PrintDebug("debug", "ÍøÂç³õÊ¼»¯Ê§°Ü");
	}
	CClientConnectMgr::GetInstance()->OnInit();
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("SetListenPort", SetListenPort2Script);
}

int SetListenPort2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	int nPort = pState->PopIntVarFormStack();
	CClientConnectMgr::GetInstance()->SetListen(nPort, CClientConnectMgr::CreateNew);
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}


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
		

	}
}

