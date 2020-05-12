#include "ClientConnectMgr.h"
#include "Client.h"
#include "zByteArray.h"
void InitNetworkConnect()
{
	if (!initSocket())
	{
		//PrintDebug("debug", "网络初始化失败");
	}
	CClientConnectMgr::GetInstance()->OnInit();
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("SetListenPort", SetListenPort2Script);
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("NewClient", NewClient2Script);
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("GetClient", GetClient2Script);
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

int NewClient2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strIP = pState->PopCharVarFormStack();
	int nPort = pState->PopIntVarFormStack();

	auto pClient = CClientConnectMgr::GetInstance()->NewClient(strIP.c_str(), nPort);
	pState->ClearFunParam();
	pState->PushClassPointToStack(pClient);
	return ECALLBACK_FINISH;
}

int GetClient2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	__int64 nID = pState->PopIntVarFormStack();
	auto pClient = CClientConnectMgr::GetInstance()->GetClient(nID);
	pState->ClearFunParam();
	pState->PushClassPointToStack(pClient);
	return ECALLBACK_FINISH;
}

//int RunScriptToAllClient2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptRunState* pState)
//{
//	if (pState == nullptr)
//	{
//		return ECALLBACK_ERROR;
//	}
//	tagByteArray m_vBuff;
//	int nParmNum = pState->GetParamNum() - 2;
//	AddChar2Bytes(m_vBuff, E_RUN_SCRIPT);
//	AddInt2Bytes(m_vBuff, pState->m_pMachine->m_nEventListIndex);
//
//	AddInt642Bytes(m_vBuff, (__int64)0);
//
//	std::string scriptName = pState->PopCharVarFormStack();
//	AddString2Bytes(m_vBuff, (char*)scriptName.c_str());//脚本函数名
//
//	AddChar2Bytes(m_vBuff, (char)nParmNum);
//
//	for (int i = 0; i < nParmNum; i++)
//	{
//		AddVar2Bytes(m_vBuff, &pState->PopVarFormStack());
//	}
//	CClientConnectMgr::GetInstance()->SendData2All(&m_vBuff[0], m_vBuff.size());
//
//	pState->ClearFunParam();
//
//	return ECALLBACK_FINISH;
//}


CClientConnectMgr CClientConnectMgr::s_Instance;

CClientConnectMgr::CClientConnectMgr()
{
}


CClientConnectMgr::~CClientConnectMgr()
{
}

CSocketConnector* CClientConnectMgr::CreateNew(SOCKET sRemote, const char* pIP)
{
	CClient* pConnector = new CClient;
	if (pConnector == NULL)
	{
		return NULL;
	}

	pConnector->SetSocket(sRemote);
	pConnector->SetIP(pIP);
	pConnector->OnInit();
	CClientConnectMgr::GetInstance()->AddClient(pConnector);
	printf("new connect: %s; all connect count: %d \n", pIP, CClientConnectMgr::GetInstance()->GetConnectSize());

	//执行初始化脚本
	CScriptStack m_scriptParm;
	ScriptVector_PushVar(m_scriptParm, (__int64)0);
	ScriptVector_PushVar(m_scriptParm, "InitClient");
	ScriptVector_PushVar(m_scriptParm, pConnector);
	//读取完成，执行结果
	CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RUNSCRIPT, pConnector->CBaseConnector::GetID(), m_scriptParm);
	return pConnector;
}

CClient* CClientConnectMgr::GetClient(__int64 nID)
{
	auto it = m_mapConnector.find(nID);
	if (it != m_mapConnector.end())
	{
		return dynamic_cast<CClient*>(it->second.pConnector);
	}
	return nullptr;
}

CClient* CClientConnectMgr::NewClient(const char* pIP, int Port)
{
	if (pIP == nullptr)
	{
		return nullptr;
	}
	tagConnecter tagInfo;

	CClient *pClient = new CClient;
	if (pClient)
	{
		pClient->SetIP(pIP);
		pClient->SetPort(Port);
		pClient->Open();
		pClient->OnInit();

		m_ConnecterLock.lock();
		tagConnecter &tagInfo = m_mapConnector[pClient->GetID()];
		tagInfo.nType = 1;
		tagInfo.pConnector = pClient;
		m_ConnecterLock.unlock();


	}

	return pClient;
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

