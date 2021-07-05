#include "ScriptConnectMgr.h"
#include "ScriptConnector.h"
#include "zByteArray.h"
void InitNetworkConnect()
{
	if (!initSocket())
	{
		//PrintDebug("debug", "网络初始化失败");
	}
	CScriptConnectMgr::GetInstance()->OnInit();
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("SetListenPort", SetListenPort2Script);
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("NewConnector", NewConnector2Script);
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("GetConnector", GetConnector2Script);
}

int SetListenPort2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	int nPort = pState->GetIntVarFormStack(0);
	std::string strType = pState->GetStringVarFormStack(1);
	std::string strPassword = pState->GetStringVarFormStack(2);
	std::string strScript = pState->GetStringVarFormStack(3);
	std::string strDisconnectScript = pState->GetStringVarFormStack(4);
	CScriptConnectMgr::GetInstance()->SetListen(nPort, CScriptConnectMgr::CreateNew);
	CScriptConnectMgr::SetInitConnectInfo(nPort, strType, strPassword, strScript, strDisconnectScript);

	return ECALLBACK_FINISH;
}

int SetListenWebSocket2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptCallState* pState)
{
	return 0;
}

int NewConnector2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strIP = pState->GetStringVarFormStack(0);
	int nPort = pState->GetIntVarFormStack(1);
	std::string strType = pState->GetStringVarFormStack(2);
	std::string strPassword = pState->GetStringVarFormStack(3);

	auto pClient = CScriptConnectMgr::GetInstance()->NewConnector(strIP.c_str(), nPort, strType, strPassword);

	pState->SetClassPointResult(pClient);
	return ECALLBACK_FINISH;
}

int RemoveConnector2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	PointVarInfo point = pState->GetClassPointFormStack(0);
	if (point.pPoint)
	{
		auto pConnect = point.pPoint->GetPoint();
		if (pConnect)
		{
			if (pConnect->IsInitScriptPointIndex())
			{
				CScriptSuperPointerMgr::GetInstance()->RemoveClassPoint(pConnect->GetScriptPointIndex());
				pConnect->ClearScriptPointIndex();
			}
			delete pConnect;
		}
	}

	return ECALLBACK_FINISH;
}

int GetConnector2Script(zlscript::CScriptVirtualMachine* pMachine, zlscript::CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	__int64 nID = pState->GetIntVarFormStack(0);
	auto pClient = CScriptConnectMgr::GetInstance()->GetConnector(nID);

	pState->SetClassPointResult(pClient);
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


CScriptConnectMgr CScriptConnectMgr::s_Instance;
std::map<int, CScriptConnectMgr::tagConnectInit> CScriptConnectMgr::m_mapInitConnectInfo;

CScriptConnectMgr::CScriptConnectMgr()
{
}


CScriptConnectMgr::~CScriptConnectMgr()
{
}

CSocketConnector* CScriptConnectMgr::CreateNew(SOCKET sRemote, const char* pIP, int nPort)
{

	auto itScript = m_mapInitConnectInfo.find(nPort);
	if (itScript != m_mapInitConnectInfo.end())
	{
		CNetConnector* pConnector = 
			new CNetConnector(itScript->second.strType,"server",itScript->second.strPassword);
		if (pConnector == NULL)
		{
			return NULL;
		}

		pConnector->SetSocket(sRemote);
		pConnector->SetIP(pIP);
		pConnector->OnInit();
		CScriptConnectMgr::GetInstance()->AddClient(pConnector);
		printf("new connect: %s; all connect count: %d \n", pIP, CScriptConnectMgr::GetInstance()->GetConnectSize());
		pConnector->SetInitScrript(itScript->second.strInitScript);
		pConnector->SetDisconnectScrript(itScript->second.strDisconnectScript);
		//CScriptStack m_scriptParm;
		//ScriptVector_PushVar(m_scriptParm, pConnector);
		//if (zlscript::CScriptVirtualMachine::GetInstance())
		//{
		//	zlscript::CScriptVirtualMachine::GetInstance()->RunFunImmediately(itScript->second.strInitScript, m_scriptParm);
		//}

		//pConnector->SetDisconnectScript(itScript->second.strDisconnectScript);

		return pConnector;
	}

	return nullptr;
}

CScriptConnector* CScriptConnectMgr::GetConnector(__int64 nID)
{
	auto it = m_mapConnector.find(nID);
	if (it != m_mapConnector.end())
	{
		return dynamic_cast<CScriptConnector*>(it->second.pConnector);
	}
	return nullptr;
}

CScriptConnector* CScriptConnectMgr::NewConnector(const char* pIP, int Port, std::string strType, std::string strPassword)
{
	if (pIP == nullptr)
	{
		return nullptr;
	}
	tagConnecter tagInfo;

	CNetConnector* pClient = new CNetConnector(strType,"client", strPassword);
	if (pClient)
	{
		pClient->SetIP(pIP);
		pClient->SetPort(Port);
		pClient->Open();
		pClient->OnInit();

		m_ConnecterLock.lock();
		tagConnecter& tagInfo = m_mapConnector[pClient->CBaseConnector::GetID()];
		tagInfo.nType = 1;
		tagInfo.pConnector = pClient;
		m_ConnecterLock.unlock();

		CScriptConnector* pScriptConnector = new CScriptConnector();
		if (pScriptConnector)
		{
			pScriptConnector->SetNetConnector(pClient);
			pClient->SetMaster(pScriptConnector);
		}
		return pScriptConnector;
	}

	return nullptr;;
}

void CScriptConnectMgr::SetInitConnectInfo(int nPort, std::string strType, std::string strPassword,
											std::string strScript, std::string strDisconnectScript)
{
	tagConnectInit script;
	script.strType = strType;
	script.strPassword = strPassword;
	script.strInitScript = strScript;
	script.strDisconnectScript = strDisconnectScript;
	m_mapInitConnectInfo[nPort] = script;
}

void CScriptConnectMgr::SetSockectConnector(CScriptConnector* pConnector)
{
	//std::lock_guard<std::mutex> Lock(m_lockScriptConnector);
	if (pConnector)
	{
		m_mapScriptConnector[pConnector->GetScriptPointIndex()] = pConnector->GetScriptPointIndex();
	}
}

CScriptConnector* CScriptConnectMgr::GetSocketConnector(__int64 id)
{
	//std::lock_guard<std::mutex> Lock(m_lockScriptConnector);
	auto it = m_mapScriptConnector.find(id);
	if (it != m_mapScriptConnector.end())
	{
		if (it->second.pPoint)
		{
			return dynamic_cast<CScriptConnector*>(it->second.pPoint->GetPoint());
		}
	}
	return nullptr;
}

void CScriptConnectMgr::RemoveSocketConnect(__int64 id)
{
	m_listRemoveScriptConnect.push_back(id);
}

__int64 CScriptConnectMgr::GetSyncIndex(int serverID, __int64 id)
{
	std::lock_guard<std::mutex> Lock(m_lockSyncInfo);
	std::map<struct stSyncInfo, __int64>::iterator it = m_mapSyncPointInfo.find(stSyncInfo(serverID, id));
	if (it != m_mapSyncPointInfo.end())
	{
		return it->second;
	}
	return 0;
}

void CScriptConnectMgr::SetSyncIndex(int serverID, __int64 id, __int64 imageId)
{
	std::lock_guard<std::mutex> Lock(m_lockSyncInfo);
	m_mapSyncPointInfo.insert(std::make_pair(stSyncInfo(serverID, id), imageId));
}

void CScriptConnectMgr::RemoveSyncIndex(int serverID, __int64 id)
{
	std::lock_guard<std::mutex> Lock(m_lockSyncInfo);
	std::map<struct stSyncInfo, __int64>::iterator it = m_mapSyncPointInfo.find(stSyncInfo(serverID, id));
	if (it != m_mapSyncPointInfo.end())
	{
		m_mapSyncPointInfo.erase(it);
	}
}

__int64 CScriptConnectMgr::AssginSyncProcessID(__int64 connectid)
{
	std::lock_guard<std::mutex> Lock(m_lockSyncProcess);
	m_nSyncProcessIDCount++;
	m_mapSyncProcessID[m_nSyncProcessIDCount] = connectid;
	return m_nSyncProcessIDCount;
}

void CScriptConnectMgr::SetConnectID2ProcessID(__int64 connectid, __int64 processid)
{
	std::lock_guard<std::mutex> Lock(m_lockSyncProcess);
	m_mapSyncProcessID[processid] = connectid;
}

__int64 CScriptConnectMgr::GetConnectIDFromProcessID(__int64 processid)
{
	std::lock_guard<std::mutex> Lock(m_lockSyncProcess);
	auto it = m_mapSyncProcessID.find(processid);
	if (it != m_mapSyncProcessID.end())
	{
		return it->second;
	}
	return 0;
}

void CScriptConnectMgr::removeSyncProcessID(__int64 processid)
{
	std::lock_guard<std::mutex> Lock(m_lockSyncProcess);
	auto it = m_mapSyncProcessID.find(processid);
	if (it != m_mapSyncProcessID.end())
	{
		m_mapSyncProcessID.erase(it);
	}
}

void CScriptConnectMgr::OnProcess()
{
	CSocketConnectorMgr::OnProcess();

	auto itRemove = m_listRemoveScriptConnect.begin();
	while (itRemove != m_listRemoveScriptConnect.end())
	{
		m_mapScriptConnector.erase(*itRemove);
		itRemove = m_listRemoveScriptConnect.erase(itRemove);
	}

	auto curTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - m_LastSendMsgTime);
	if (duration.count() >= 1000)
	{
		m_LastSendMsgTime = curTime;


	}
}

