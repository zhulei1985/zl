#include "ScriptConnector.h"
#include "zByteArray.h"
#include "ScriptExecCodeMgr.h"
#include "ScriptConnectMgr.h"
#include "TempScriptRunState.h"
#include <functional>
#include "RouteEvent.h"

CNetConnector::CNetConnector(std::string type, std::string flag, std::string password)
{
	m_pMaster = nullptr;
	m_pHeadProtocol = nullptr;
	SetHeadProtocol(type, flag, password);
}
CNetConnector::~CNetConnector()
{
	m_LockProtocol.lock();
	if (m_pHeadProtocol)
	{
		CHeadProtocolMgr::GetInstance()->Remove(m_pHeadProtocol);
	}
	m_pHeadProtocol = nullptr;
	m_LockProtocol.unlock();
}

void CNetConnector::OnInit()
{
	CSocketConnector::OnInit();
}

bool CNetConnector::OnProcess()
{
	if (CSocketConnector::OnProcess() == false)
	{
		return false;
	}

	//从接收队列里取出消息处理
	std::vector<char> vOut;
	m_LockProtocol.lock();
	while (m_pHeadProtocol)
	{
		int nResult = m_pHeadProtocol->OnProcess();
		switch (nResult)
		{
		case CBaseHeadProtocol::E_RETURN_COMPLETE:
		{
			if (m_pHeadProtocol->IsBigMsg())
			{
				if (m_pHeadProtocol->GetBigData(vOut, 1))
				{
					int nPos = 0;
					char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());

					auto pCurMsgReceive = CMsgReceiveMgr::GetInstance()->CreateRceiveState(cType);
					if (pCurMsgReceive == nullptr)
					{
						//TODO 错误数据，关闭链接
						Close();
						break;
					}
					pCurMsgReceive->SetGetDataFun(std::bind(&CBaseHeadProtocol::GetBigData, m_pHeadProtocol, std::placeholders::_1, std::placeholders::_2));
					if (pCurMsgReceive->Recv(m_pMaster) == true)
					{
						m_LockMsgList.lock();
						m_listMsg.push_back(pCurMsgReceive);
						m_LockMsgList.unlock();
					}
					else
					{
						//TODO 错误数据，关闭链接
						Close();
						break;
					}
				}
				m_pHeadProtocol->ClearBigData();
			}
			else if (m_pHeadProtocol->GetDataLen() > 0)
			{
				if (m_pHeadProtocol->GetData(vOut, 1))
				{
					int nPos = 0;
					char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());

					auto pCurMsgReceive = CMsgReceiveMgr::GetInstance()->CreateRceiveState(cType);
					if (pCurMsgReceive == nullptr)
					{
						//TODO 错误数据，关闭链接
						Close();
						break;
					}
					pCurMsgReceive->SetGetDataFun(std::bind(&CBaseHeadProtocol::GetData, m_pHeadProtocol, std::placeholders::_1, std::placeholders::_2));
					if (pCurMsgReceive->Recv(m_pMaster) == true)
					{
						m_LockMsgList.lock();
						m_listMsg.push_back(pCurMsgReceive);
						m_LockMsgList.unlock();
					}
					else
					{
						//TODO 错误数据，关闭链接
						Close();
						break;
					}
				}
			}
		}
		break;
		case CBaseHeadProtocol::E_RETURN_CONTINUE:
			break;
		case CBaseHeadProtocol::E_RETURN_NEXT:
			break;
		case CBaseHeadProtocol::E_RETURN_SHAKE_HAND_COMPLETE:
			if (m_pHeadProtocol->IsServer() && m_pHeadProtocol->GetLastConnectID() != 0)
			{
				//合并连接信息
				auto pScriptConnector = CScriptConnectMgr::GetInstance()->GetSocketConnector(m_pHeadProtocol->GetLastConnectID());
				if (pScriptConnector)
				{
					pScriptConnector->SetNetConnector(this);
					this->SetMaster(pScriptConnector);
				}
			}
			else
			{
				if (m_pMaster == nullptr)
				{
					//创建scriptconnector
					CScriptConnector* pScriptConnector = new CScriptConnector();
					if (pScriptConnector)
					{
						pScriptConnector->SetNetConnector(this);
						this->SetMaster(pScriptConnector);
						CScriptConnectMgr::GetInstance()->SetSockectConnector(pScriptConnector);
					}
				}
			}
			m_bCanSend = true;
			break;
		case CBaseHeadProtocol::E_RETURN_ERROR:
		default:
			//错误，中断连接
			Close();
			break;
		}
		if (nResult != CBaseHeadProtocol::E_RETURN_CONTINUE)
		{
			break;
		}
	}
	m_LockProtocol.unlock();

	if (m_pMaster)
	{
		m_pMaster->OnProcess();
	}
	return true;
}

void CNetConnector::OnDestroy()
{
	CSocketConnector::OnDestroy();
	if (m_pMaster)
	{
		CScriptConnectMgr::GetInstance()->RemoveSocketConnect(m_pMaster->GetScriptPointIndex());
		m_pMaster->SetNetConnector(nullptr);
		m_pMaster = nullptr;
	}
}

bool CNetConnector::SendMsg(char* pBuff, int len)
{
	if (!CanSend())
	{
		return false;
	}
	if (m_pHeadProtocol)
	{
		m_pHeadProtocol->SendHead(len);
	}
	CSocketConnector::SendData(pBuff, len);
	return true;
}

void CNetConnector::SetMaster(CScriptConnector* pConnector)
{
	m_pMaster = pConnector;
}

bool CNetConnector::SetHeadProtocol(std::string& strName, std::string& flag, std::string& password)
{
	CBaseHeadProtocol* pProtocol = nullptr;
	if (strName == "websocket")
	{
		pProtocol = CHeadProtocolMgr::GetInstance()->Create(E_HEAD_PROTOCOL_WEBSOCKET);
	}
	else if (strName == "inner")
	{
		pProtocol = CHeadProtocolMgr::GetInstance()->Create(E_HEAD_PROTOCOL_INNER);
	}
	else
	{
		pProtocol = CHeadProtocolMgr::GetInstance()->Create(E_HEAD_PROTOCOL_NONE);
	}

	if (pProtocol)
	{
		if (flag == "server")
		{
			pProtocol->SetServer(true);
		}
		else
		{
			pProtocol->SetServer(false);
		}
	}

	if (password.size() > 0)
	{
		pProtocol->SetPassword(password.c_str());
	}
	pProtocol->SetConnector(this);
	m_LockProtocol.lock();
	if (m_pHeadProtocol)
	{
		CHeadProtocolMgr::GetInstance()->Remove(m_pHeadProtocol);
	}
	m_pHeadProtocol = pProtocol;
	m_LockProtocol.unlock();
	return true;
}

unsigned int CNetConnector::GetMsgSize()
{
	std::lock_guard<std::mutex> Lock(m_LockMsgList);
	return m_listMsg.size();
}

CBaseMsgReceiveState* CNetConnector::PopMsg()
{
	std::lock_guard<std::mutex> Lock(m_LockMsgList);
	CBaseMsgReceiveState* pResult = m_listMsg.front();
	m_listMsg.pop_front();
	return pResult;
}

void CNetConnector::SetInitScrript(std::string str)
{
	strInitScript = str;
}

std::string& CNetConnector::GetInitScript()
{
	// TODO: 在此处插入 return 语句
	return strInitScript;
}

void CNetConnector::SetDisconnectScrript(std::string str)
{
	strDisconnectScript = str;
}

std::string& CNetConnector::GetDisconnectScript()
{
	// TODO: 在此处插入 return 语句
	return strDisconnectScript;
}

CBaseScriptConnector::CBaseScriptConnector()
{
}

CBaseScriptConnector::~CBaseScriptConnector()
{
}

void CBaseScriptConnector::Init2Script()
{
	RegisterClassType("Connector", CBaseScriptConnector);
}

int CBaseScriptConnector::GetID2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	//pState->ClearFunParam();
	pState->SetResult(GetEventIndex());
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::GetPort2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->SetResult((__int64)GetSocketPort());
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::Close2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	this->Close();

	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::IsConnect2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->SetResult((__int64)(IsSocketClosed() ? 0 : 1));
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::RunScript2Script(CScriptCallState* pState)
{
	if (pState == nullptr && pState->m_pMaster)
	{
		return ECALLBACK_ERROR;
	}
	int nParmNum = pState->GetParamNum();
	CScriptStack parm;
	__int64 nEventIndex = 0;
	__int64 nReturnID = 0;
	if (pState->m_pMaster->m_pMachine)
		nEventIndex = pState->m_pMaster->m_pMachine->GetEventIndex();
	int nIsWaiting = pState->GetIntVarFormStack(0);//是否等待调用函数完成
	if (nIsWaiting > 0)
		nReturnID = (__int64)pState->m_pMaster->GetId();
	std::string strScriptFunName = pState->GetStringVarFormStack(1);
	//pState->CopyToStack(&parm, nParmNum);
	for (int i = 2; i < nParmNum; i++)
	{
		auto var = pState->GetVarFormStack(i);
		parm.push(var);
	}
	RunFrom(strScriptFunName, parm, nReturnID, nEventIndex);


	if (nIsWaiting > 0)
	{
		return ECALLBACK_WAITING;
	}
	else
		return ECALLBACK_FINISH;
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::SetAllScriptLimit2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	int nVal = pState->GetIntVarFormStack(0);
	if (nVal > 0)
	{
		m_bIgnoreScriptLimit = true;
	}
	else
	{
		m_bIgnoreScriptLimit = false;
		m_mapScriptLimit.clear();
	}
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::SetScriptLimit2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strName = pState->GetStringVarFormStack(0);
	int nVal = pState->GetIntVarFormStack(1);
	if (nVal > 0)
	{
		SetScriptLimit(strName);
	}
	else
	{
		RemoveScriptLimit(strName);
	}

	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::CheckScriptLimit2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strName = pState->GetStringVarFormStack(0);
	bool bCheck = CheckScriptLimit(strName);

	pState->SetResult((__int64)(bCheck ? 1 : 0));
	return ECALLBACK_FINISH;
}
//int CBaseScriptConnector::SetRemoteFunction2Script(CScriptCallState* pState)
//{
//	if (pState == nullptr)
//	{
//		return ECALLBACK_ERROR;
//	}
//	std::string strName = pState->GetStringVarFormStack(0);
//	CScriptExecCodeMgr::GetInstance()->SetRemoteFunction(strName, GetEventIndex());
//	m_setRemoteFunName.insert(strName);
//
//	return ECALLBACK_FINISH;
//}

int CBaseScriptConnector::SetRoute2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	if (nRouteMode_ConnectID)
	{
		CScriptStack scriptParm;
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_TYPE_CONNECT_REMOVE, GetEventIndex(), scriptParm, nRouteMode_ConnectID);
	}
	nRouteMode_ConnectID = 0;
	auto pointVal = pState->GetClassPointFormStack(0);
	CScriptBasePointer* pPoint = pointVal.pPoint;
	if (pPoint)
	{
		pPoint->Lock();
		auto pConnector = dynamic_cast<CBaseScriptConnector*>(pPoint->GetPoint());
		if (pConnector)
		{
			nRouteMode_ConnectID = pConnector->GetEventIndex();
		}
		pPoint->Unlock();
	}
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::SetRouteInitScript2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strScriptName = pState->GetStringVarFormStack(0);
	SetRouteInitScript(strScriptName.c_str());
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::GetVal2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string str = pState->GetStringVarFormStack(0);

	auto it = m_mapData.find(str);
	if (it != m_mapData.end())
	{
		pState->SetResult(it->second);
		return ECALLBACK_FINISH;
	}
	pState->PushEmptyVarToStack();
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::SetVal2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string str = pState->GetStringVarFormStack(0);
	m_mapData[str] = pState->GetVarFormStack(1);

	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::Merge2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	CScriptConnector* pMain = dynamic_cast<CScriptConnector*>(this);
	CScriptConnector* pOld = nullptr;
	auto pointVal = pState->GetClassPointFormStack(0);
	CScriptBasePointer* pPoint = pointVal.pPoint;
	if (pPoint)
	{
		pPoint->Lock();
		pOld = dynamic_cast<CScriptConnector*>(pPoint->GetPoint());
		if (pMain && pOld)
		{
			pMain->Merge(pOld);
		}
		pPoint->Unlock();
	}
	//TODO 两者必须都是CScriptConnector,否则报错
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::SetDisconnectScript2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string val = pState->GetStringVarFormStack(0);
	SetDisconnectScript(val);

	return ECALLBACK_FINISH;
}

void CBaseScriptConnector::OnInit()
{
	CScriptExecFrame::OnInit();
	CRouteEventMgr::GetInstance()->Register(GetEventIndex());
	//nScriptEventIndex = CScriptEventMgr::GetInstance()->AssignID();
	m_nReturnCount = 0;
	m_bIgnoreScriptLimit = false;
	m_mapReturnState.clear();
	AddClassObject(this->GetScriptPointIndex(), this);

	InitEvent(zlscript::E_SCRIPT_EVENT_RETURN, std::bind(&CBaseScriptConnector::EventReturnFun, this, std::placeholders::_1, std::placeholders::_2), false);
	InitEvent(zlscript::E_SCRIPT_EVENT_RUNSCRIPT, std::bind(&CBaseScriptConnector::EventRunFun, this, std::placeholders::_1, std::placeholders::_2), false);

	InitEvent(zlscript::E_SCRIPT_EVENT_UP_SYNC_FUN, std::bind(&CBaseScriptConnector::EventUpSyncFun, this, std::placeholders::_1, std::placeholders::_2), false);
	InitEvent(zlscript::E_SCRIPT_EVENT_DOWN_SYNC_FUN, std::bind(&CBaseScriptConnector::EventDownSyncFun, this, std::placeholders::_1, std::placeholders::_2), false);

	InitEvent(zlscript::E_SCRIPT_EVENT_UP_SYNC_DATA, std::bind(&CBaseScriptConnector::EventUpSyncData, this, std::placeholders::_1, std::placeholders::_2), false);
	InitEvent(zlscript::E_SCRIPT_EVENT_DOWN_SYNC_DATA, std::bind(&CBaseScriptConnector::EventDownSyncData, this, std::placeholders::_1, std::placeholders::_2), false);

	InitEvent(zlscript::E_SCRIPT_EVENT_RETURN_SYNC_FUN, std::bind(&CBaseScriptConnector::EventReturnSyncFun, this, std::placeholders::_1, std::placeholders::_2), false);

	InitEvent(zlscript::E_SCRIPT_EVENT_REMOVE_UP_SYNC, std::bind(&CBaseScriptConnector::EventRemoveUpSync, this, std::placeholders::_1, std::placeholders::_2), false);
	InitEvent(zlscript::E_SCRIPT_EVENT_REMOVE_DOWN_SYNC, std::bind(&CBaseScriptConnector::EventRemoveDownSync, this, std::placeholders::_1, std::placeholders::_2), false);

	InitEvent(E_SCRIPT_EVENT_TYPE_CONNECT_REMOVE, std::bind(&CBaseScriptConnector::EventRemoveRoute, this, std::placeholders::_1, std::placeholders::_2), false);
	InitEvent(E_SCRIPT_EVENT_TYPE_CONNECT_CHANGE, std::bind(&CBaseScriptConnector::EventChangeRoute, this, std::placeholders::_1, std::placeholders::_2), false);

}

bool CBaseScriptConnector::OnProcess()
{
	CScriptExecFrame::OnUpdate();

	return true;
}

void CBaseScriptConnector::OnDestroy()
{
	for (auto it = m_setRemoteFunName.begin(); it != m_setRemoteFunName.end(); it++)
	{
		CScriptExecCodeMgr::GetInstance()->RemoveRemoteFunction(*it, GetEventIndex());
	}

	for (auto it = m_mapReturnState.begin(); it != m_mapReturnState.end(); it++)
	{
		CScriptStack scriptParm;
		ScriptVector_PushVar(scriptParm, it->second.nReturnID);
		ScriptVector_PushEmptyVar(scriptParm);
		//scriptParm.push();
		
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RETURN, GetEventIndex(), scriptParm, it->second.nEventIndex);

	}
	m_mapReturnState.clear();

	if (m_strDisconnectScript.size() > 0)
	{
		CScriptStack m_scriptParm;
		ScriptVector_PushVar(m_scriptParm, this);

		//zlscript::CScriptVirtualMachine machine;
		//machine.RunFunImmediately(m_strDisconnectScript, m_scriptParm);
		if (zlscript::CScriptVirtualMachine::GetInstance())
		{
			zlscript::CScriptVirtualMachine::GetInstance()->RunFunImmediately(m_strDisconnectScript, m_scriptParm);
		}
	}
	RemoveClassObject(this->GetScriptPointIndex());
	CRouteEventMgr::GetInstance()->Unregister(GetEventIndex());
	CScriptExecFrame::Clear();


}

bool CBaseScriptConnector::RunMsg(CBaseMsgReceiveState* pMsg)
{
	bool bResult = false;
	if (nRouteMode_ConnectID != 0)
	{
		switch (pMsg->GetType())
		{
		case E_RUN_SCRIPT_RETURN://看起来只有这一条消息需要无条件转发
			if (CRouteEventMgr::GetInstance()->SendEvent(nRouteMode_ConnectID, true, pMsg, GetEventIndex()) == false)
			{
				//转发失败，需要转发的目标不存在，取消路由状态
				nRouteMode_ConnectID = 0;
				bResult = true;// pMsg->Run(this);
			}
			break;
		default:
			bResult = pMsg->Run(this);
			break;
		}
	}
	else
	{
		bResult = pMsg->Run(this);
	}
	return bResult;
}


void CBaseScriptConnector::SendNoSyncClassMsg(std::string strClassName, CScriptPointInterface* pPoint)
{
	if (pPoint == nullptr)
	{
		return;
	}
	CNoSyncClassInfoMsgReceiveState msg;
	msg.strClassName = strClassName;
	msg.m_pPoint = pPoint;
	SendMsg(&msg);
}

void CBaseScriptConnector::SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint)
{
	if (pPoint == nullptr)
	{
		return;
	}
	CSyncClassInfoMsgReceiveState msg;
	msg.strClassName = strClassName;
	msg.m_pPoint = pPoint;
	SendMsg(&msg);
}

void CBaseScriptConnector::SyncUpClassFunRun(__int64 classID, std::string strFunName, CScriptStack& stack, std::list<__int64> listRoute)
{
	CSyncUpMsgReceiveState msg;
	msg.strFunName = strFunName;
	msg.nClassID = classID;
	msg.m_listRoute = listRoute;
	////倒过来压栈
	//for (int i = stack.size() - 1; i >= 0; i--)
	//{
	//	auto pVal = stack.GetVal(i);
	//	if (pVal)
	//		msg.m_scriptParm.push(*pVal);
	//}
	msg.m_scriptParm = stack;
	SendMsg(&msg);
}

void CBaseScriptConnector::SyncDownClassFunRun(__int64 classID, std::string strFunName, CScriptStack& stack)
{

	CSyncDownMsgReceiveState msg;
	msg.strFunName = strFunName;
	msg.nClassID = classID;
	////倒过来压栈
	//for (int i = stack.size() - 1; i >= 0; i--)
	//{
	//	auto pVal = stack.GetVal(i);
	//	if (pVal)
	//		msg.m_scriptParm.push(*pVal);
	//}
	msg.m_scriptParm = stack;
	SendMsg(&msg);
}

//void CBaseScriptConnector::SendSyncClassData(__int64 classID, std::vector<char>&data)
//{
//	CSyncClassDataReceiveState msg;
//	msg.nClassID = classID;
//
//	msg.vData = data;
//	
//	SendMsg(&msg);
//}

void CBaseScriptConnector::SendRemoveSyncUp(__int64 classID)
{
	CSyncUpRemoveMsgReceiveState msg;
	msg.nClassID = classID;

	SendMsg(&msg);
}

void CBaseScriptConnector::SendRemoveSyncDown(__int64 classID)
{
	CSyncDownRemoveMsgReceiveState msg;
	msg.nClassID = classID;

	SendMsg(&msg);
}

void CBaseScriptConnector::SendRemoveRoute(__int64 nConnectID)
{
	CRouteRemoveMsgReceiveState msg;
	msg.nConnectID = nConnectID;

	SendMsg(&msg);
}

void CBaseScriptConnector::SendChangeRoute(__int64 oldid, __int64 newid)
{
	CRouteChangeMsgReceiveState msg;
	msg.nOldConnectID = oldid;
	msg.nNewConnectID = newid;

	SendMsg(&msg);
}


void CBaseScriptConnector::EventReturnFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nReturnID = GetInt_StackVar(ParmInfo.GetVal(0));
	ParmInfo.pop_front(1);
	ResultFrom(ParmInfo, nReturnID);
}

void CBaseScriptConnector::EventRunFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nReturnID = GetInt_StackVar(ParmInfo.GetVal(0));
	std::string funName = GetString_StackVar(ParmInfo.GetVal(0));
	ParmInfo.pop_front(2);
	RunFrom(funName, ParmInfo, nReturnID, nSendID);
}

void CBaseScriptConnector::EventUpSyncFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nClassID = GetPointIndex_StackVar(ParmInfo.GetVal(0));
	std::string funName = GetString_StackVar(ParmInfo.GetVal(1));
	__int64 nRouteNum = GetInt_StackVar(ParmInfo.GetVal(2));
	std::list<__int64> listRoute;
	if (nRouteNum == 1)
	{
		//路径第一个值是远程调用返回机制的返回值
		__int64 nStateID = GetInt_StackVar(ParmInfo.GetVal(3));
		listRoute.push_back(AddReturnState(nSendID,nStateID));
		ParmInfo.pop_front(4);
	}
	else
	{
		for (__int64 i = 0; i < nRouteNum; i++)
		{
			listRoute.push_back(GetInt_StackVar(ParmInfo.GetVal(3+i)));
		}
		ParmInfo.pop_front(3+nRouteNum);
	}
	SyncUpClassFunRun(nClassID, funName, ParmInfo, listRoute);
}

void CBaseScriptConnector::EventDownSyncFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nClassID = GetPointIndex_StackVar(ParmInfo.GetVal(0));
	std::string funName = GetString_StackVar(ParmInfo.GetVal(1));
	ParmInfo.pop_front(2);
	SyncDownClassFunRun(nClassID, funName, ParmInfo);
}

void CBaseScriptConnector::EventUpSyncData(__int64 nSendID, CScriptStack& ParmInfo)
{
}

void CBaseScriptConnector::EventDownSyncData(__int64 nSendID, CScriptStack& ParmInfo)
{
	CSyncClassDataReceiveState msg;
	msg.nClassID = GetPointIndex_StackVar(ParmInfo.GetVal(0));
	tagByteArray data;
	GetBinary_StackVar(ParmInfo.GetVal(1), msg.vData);
	__int64 nNum = ScriptStack_GetInt(ParmInfo);
	if (nNum > 0)
	{
		msg.vClassPoint.resize(nNum);
		for (__int64 i = 0; i < nNum; i++)
		{
			auto var = ParmInfo.GetVal(2+i);
			if (var && var->cType == EScriptVal_ClassPoint)
			{
				msg.vClassPoint[i] = var->pPoint;
			}
			else
			{
				msg.vClassPoint[i] = (__int64)0;
			}
			ParmInfo.pop();
		}
	}

	SendMsg(&msg);
}

void CBaseScriptConnector::EventReturnSyncFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	CSyncFunReturnMsgReceiveState msg;
	__int64 nRouteNum = GetInt_StackVar(ParmInfo.GetVal(0));
	std::list<__int64> listRoute;
	for (__int64 i = 0; i < nRouteNum; i++)
	{
		msg.m_listRoute.push_back(GetInt_StackVar(ParmInfo.GetVal(1+i)));
	}
	ParmInfo.pop_front(1 + nRouteNum);
	msg.m_scriptParm = ParmInfo;
	SendMsg(&msg);
}

void CBaseScriptConnector::EventRemoveUpSync(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nClassID = GetPointIndex_StackVar(ParmInfo.GetVal(0));

	SendRemoveSyncUp(nClassID);
}

void CBaseScriptConnector::EventRemoveDownSync(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nClassID = GetPointIndex_StackVar(ParmInfo.GetVal(0));

	SendRemoveSyncDown(nClassID);
}

void CBaseScriptConnector::EventRemoveRoute(__int64 nSendID, CScriptStack& ParmInfo)
{
	//__int64 nConnectID = ScriptStack_GetInt(ParmInfo);
	SendRemoveRoute(nSendID);
}

void CBaseScriptConnector::EventChangeRoute(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nOldConnectID = GetInt_StackVar(ParmInfo.GetVal(0));
	//__int64 nNewConnectID = ScriptStack_GetInt(ParmInfo);
	SendChangeRoute(nOldConnectID, nSendID);
}

void CBaseScriptConnector::SetDisconnectScript(std::string val)
{
	m_strDisconnectScript = val;
}




void CBaseScriptConnector::RunFrom(std::string funName, CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex)
{
	//根据nReturnID和nEventIndex生成一个返回ID

	CScriptMsgReceiveState msg;
	if (nReturnID > 0)
	{
		msg.nReturnID = AddReturnState(nEventIndex, nReturnID);
	}
	else
	{
		msg.nReturnID = 0;
	}
	msg.strScriptFunName = funName;

	////倒过来压栈
	//int nParmNum = (int)pram.size();
	//for (int i = nParmNum-1; i >= 0; i--)
	//{
	//	auto pVar = pram.GetVal(i);
	//	if (pVar)
	//		msg.m_scriptParm.push(*pVar);
	//}
	//不再倒过来压栈
	msg.m_scriptParm = pram;

	SendMsg(&msg);
}

void CBaseScriptConnector::ResultFrom(CScriptStack& pram, __int64 nReturnID)
{
	CReturnMsgReceiveState msg;
	msg.nReturnID = nReturnID;

	////倒过来压栈
	//for (int i = pram.size() - 1; i >= 0; i--)
	//{
	//	auto pVal = pram.GetVal(i);
	//	if (pVal)
	//		msg.m_scriptParm.push(*pVal);
	//}
	msg.m_scriptParm = pram;
	SendMsg(&msg);
}

void CBaseScriptConnector::SetScriptLimit(std::string strName)
{
	m_mapScriptLimit[strName] = 1;
}

bool CBaseScriptConnector::CheckScriptLimit(std::string strName)
{
	if (m_bIgnoreScriptLimit)
	{
		return true;
	}
	auto it = m_mapScriptLimit.find(strName);
	if (it != m_mapScriptLimit.end())
	{
		return true;
	}
	return false;
}

void CBaseScriptConnector::RemoveScriptLimit(std::string strName)
{
	auto it = m_mapScriptLimit.find(strName);
	if (it != m_mapScriptLimit.end())
	{
		m_mapScriptLimit.erase(it);
	}
}

__int64 CBaseScriptConnector::AddReturnState(__int64 nEventIndex, __int64 nReturnID)
{
	m_nReturnCount++;
	tagReturnState& state = m_mapReturnState[m_nReturnCount];
	state.nEventIndex = nEventIndex;
	state.nReturnID = nReturnID;
	return m_nReturnCount;
}

CBaseScriptConnector::tagReturnState* CBaseScriptConnector::GetReturnState(__int64 nID)
{
	auto it = m_mapReturnState.find(nID);
	if (it != m_mapReturnState.end())
	{
		return &it->second;
	}
	return nullptr;
}

void CBaseScriptConnector::RemoveReturnState(__int64 nID)
{
	m_mapReturnState.erase(nID);
}

CScriptConnector::CScriptConnector()
{
	//m_nCurMsgLen = 0;

	//pCurMsgReceive = nullptr;
	//m_pHeadProtocol = nullptr;
	OnInit();
}


CScriptConnector::~CScriptConnector()
{
	OnDestroy();
}

//int CScriptConnector::GetSocketPort()
//{
//	return CSocketConnector::GetPort();
//}
bool CScriptConnector::IsSocketClosed()
{
	if (m_pNetConnector)
	{
		if (!m_pNetConnector->IsSocketClosed())
		{
			if (!m_pNetConnector->CanSend())
			{
				return true;
			}
		}
		return false;
	}
	return true;
}
//
//void CScriptConnector::Close()
//{
//	CSocketConnector::Close();
//	tDisconnectTime = std::chrono::steady_clock::now();
//}
//
//bool CScriptConnector::CanDelete()
//{
//	if (CBaseConnector::CanDelete())
//	{
//		auto curTime = std::chrono::steady_clock::now();
//		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - tDisconnectTime);
//		if (duration.count() >= 1000)
//		{
//			return true;
//		}
//
//		if (m_nEventListIndex == 0)
//		{
//			return false;
//		}
//		return true;
//	}
//	return false;
//}

void CScriptConnector::OnInit()
{
	CBaseScriptConnector::OnInit();
}
bool CScriptConnector::OnProcess()
{
	//if (CSocketConnector::OnProcess() == false)
	//{
	//	return false;
	//}
	if (m_pNetConnector == nullptr)
	{
		return false;
	}

	if (CanSend() && m_WaitCanSendStateID > 0)
	{
		CScriptStack parm;
		ResultTo(parm, m_WaitCanSendStateID, 0);
		m_WaitCanSendStateID = 0;
	}

	unsigned int nMsgSize = m_pNetConnector->GetMsgSize();
	for (unsigned int i = 0; i < nMsgSize; i++)
	{
		auto pCurMsgReceive = m_pNetConnector->PopMsg();
		if (pCurMsgReceive)
		{
			//if (pCurMsgReceive->Recv(this) == true)
			{
				//返回true才需要释放消息
				if (RunMsg(pCurMsgReceive))
				{
					CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pCurMsgReceive);
					pCurMsgReceive = nullptr;
				}
				else
				{
					//错误,断开连接
					CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pCurMsgReceive);
					pCurMsgReceive = nullptr;
				}
			}
			//else
			//{
			//	//TODO 错误数据，关闭链接
			//	return false;
			//}
		}
	}
	CBaseScriptConnector::OnProcess();

	//处理路由消息
	std::vector<stRouteEvent*> vRouteEvent;
	CRouteEventMgr::GetInstance()->GetEvent(GetEventIndex(), vRouteEvent);
	for (auto it = vRouteEvent.begin(); it != vRouteEvent.end(); it++)
	{
		stRouteEvent* pEvent = *it;
		if (pEvent)
		{
			if (pEvent->bFront)
			{
				CRouteFrontMsgReceiveState msg;
				msg.nConnectID = pEvent->nIndex;
				msg.pState = pEvent->pMsg;
				SendMsg(&msg);

				//printf("Route_Front:%d\n", msg.nConnectID);
			}
			else
			{
				if (pEvent->pMsg)
				{
					SendMsg(pEvent->pMsg);
				}
					//pEvent->pMsg->Run(this);
			}
			CRouteEventMgr::GetInstance()->ReleaseEvent(pEvent);
		}
	}



	std::map<__int64, CScriptRouteConnector*>::iterator it = m_mapRouteConnect.begin();
	for (; it != m_mapRouteConnect.end(); )
	{
		CScriptRouteConnector* pRoute = it->second;
		if (pRoute)
		{
			if (!pRoute->OnProcess())
			{
				delete pRoute;
				it = m_mapRouteConnect.erase(it);
			}
			else
			{
				it++;
			}
		}
		else
		{
			it = m_mapRouteConnect.erase(it);
		}
	}
	return true;
}
void CScriptConnector::OnDestroy()
{
	//CSocketConnector::OnDestroy();

	CBaseScriptConnector::OnDestroy();

}
bool CScriptConnector::SendMsg(char* pBuff, int len)
{
	if (m_pNetConnector == nullptr)
	{
		return false;
	}
	//if (m_pHeadProtocol)
	//{
	//	m_pHeadProtocol->SendHead(len);
	//}
	return m_pNetConnector->SendMsg(pBuff, len);
}

bool CScriptConnector::SendMsg(CBaseMsgReceiveState* pMsg)
{
	if (pMsg)
	{
		return pMsg->Send(this);
	}
	return false;
}

//bool CScriptConnector::AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal)
//{
//	if (!pVal)
//	{
//		return false;
//	}
//	switch (pVal->cType)
//	{
//	case EScriptVal_Int:
//	{
//		AddChar2Bytes(vBuff, EScriptVal_Int);
//		AddInt642Bytes(vBuff, pVal->Int64);
//	}
//	break;
//	case EScriptVal_Double:
//	{
//		AddChar2Bytes(vBuff, EScriptVal_Double);
//		AddDouble2Bytes(vBuff, pVal->Double);
//	}
//	break;
//	case EScriptVal_String:
//	{
//		AddChar2Bytes(vBuff, EScriptVal_String);
//		const char* pStr = StackVarInfo::s_strPool.GetString(pVal->Int64);
//		AddString2Bytes(vBuff, (char*)pStr);
//	}
//	break;
//	case EScriptVal_Binary:
//	{
//		AddChar2Bytes(vBuff, EScriptVal_Binary);
//		unsigned int size = 0;
//		const char* pStr = StackVarInfo::s_binPool.GetBinary(pVal->Int64, size);
//		::AddData2Bytes(vBuff, pStr, size);
//	}
//	case EScriptVal_ClassPoint:
//	{
//		auto pPoint = pVal->pPoint;
//		if (pPoint)
//		{
//			auto pSyncPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
//			if (pSyncPoint)
//			{
//				AddChar2Bytes(vBuff, EScriptVal_ClassPoint);
//				pPoint->Lock();
//				//AddString2Bytes(vBuff, (char*)pPoint->ClassName());
//				if (pPoint->GetPoint())
//				{
//					//if (pSyncPoint->GetProcessID() == GetEventIndex())
//					if (pSyncPoint->CheckUpSyncProcess(GetEventIndex()))
//					{
//						//如果这个类实例是本连接对应的镜像
//						AddChar2Bytes(vBuff, 0);
//						__int64 nImageIndex = GetImage4Index(pPoint->GetPoint()->GetScriptPointIndex());
//						AddInt642Bytes(vBuff, nImageIndex);
//					}
//					else
//					{
//						AddChar2Bytes(vBuff, 1);
//						if (!pSyncPoint->CheckDownSyncProcess(this->GetEventIndex()))
//						{
//							//新同步
//							pSyncPoint->AddDownSyncProcess(this->GetEventIndex());
//							//发送消息
//							SendSyncClassMsg(pPoint->ClassName(), pSyncPoint);
//						}
//						AddInt642Bytes(vBuff, pPoint->GetPoint()->GetScriptPointIndex());
//					}
//					//AddInt642Bytes(vBuff, pPoint->GetPoint()->GetScriptPointIndex());
//				}
//				pPoint->Unlock();
//			}
//			else
//			{
//				//错误，没有找到对象或是非同步性对象
//				auto pNoSyncPoint = pPoint->GetPoint();
//				if (pNoSyncPoint)
//				{
//					AddChar2Bytes(vBuff, EScriptVal_ClassData);
//					tagByteArray vClassData;
//					pPoint->Lock();
//					AddString2Bytes(vBuff, (char*)pPoint->ClassName());
//					pNoSyncPoint->AddAllData2Bytes(vClassData);
//					pPoint->Unlock();
//					::AddData2Bytes(vBuff, vClassData);
//				}
//				else
//				{
//					//TODO 错误
//					AddChar2Bytes(vBuff, EScriptVal_None);
//				}
//			}
//		}
//		else
//		{
//			//TODO 错误
//			AddChar2Bytes(vBuff, EScriptVal_None);
//		}
//	}
//	break;
//	default:
//		AddChar2Bytes(vBuff, EScriptVal_None);
//		break;
//	}
//	return true;
//}

bool CScriptConnector::AddVar2Bytes(std::vector<char>& vBuff, CScriptBasePointer* pPoint)
{

	if (pPoint)
	{
		auto pSyncPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
		if (pSyncPoint)
		{
			AddChar2Bytes(vBuff, EScriptVal_ClassPoint);
			pPoint->Lock();
				if (pSyncPoint->CheckUpSyncProcess(GetEventIndex()))
				{
					//如果这个类实例是本连接对应的镜像
					AddChar2Bytes(vBuff, 0);
					__int64 nImageIndex = GetImage4Index(pPoint->GetPoint()->GetScriptPointIndex());
					AddInt642Bytes(vBuff, nImageIndex);
				}
				else
				{
					AddChar2Bytes(vBuff, 1);
					if (!pSyncPoint->CheckDownSyncProcess(this->GetEventIndex()))
					{
						//新同步
						pSyncPoint->AddDownSyncProcess(this->GetEventIndex());
						//发送消息
						SendSyncClassMsg(pPoint->ClassName(), pSyncPoint);
					}
					AddInt642Bytes(vBuff, pSyncPoint->GetScriptPointIndex());
				}
				pPoint->Unlock();

		}
		else
		{
			//错误，没有找到对象或是非同步性对象
			auto pNoSyncPoint = pPoint->GetPoint();
			if (pNoSyncPoint)
			{
				AddChar2Bytes(vBuff, EScriptVal_ClassPoint);
				AddChar2Bytes(vBuff, 2);
				pPoint->Lock();
				//发送消息
				SendNoSyncClassMsg(pPoint->ClassName(), pNoSyncPoint);
				AddInt642Bytes(vBuff, pNoSyncPoint->GetScriptPointIndex());
				pPoint->Unlock();
			}
			else
			{
				//TODO 错误
				AddChar2Bytes(vBuff, EScriptVal_None);
				return false;
			}
		}

		pPoint->Unlock();
	}
	else
	{
		//TODO 错误
		AddChar2Bytes(vBuff, EScriptVal_None);
		return false;
	}
	return true;
}

void CScriptConnector::SetNetConnector(CNetConnector* pConnector)
{
	if (m_pNetConnector == nullptr)
	{
		m_pNetConnector = pConnector;
		//说明要初始化
		CScriptStack m_scriptParm;
		ScriptVector_PushVar(m_scriptParm, this);
		if (zlscript::CScriptVirtualMachine::GetInstance())
		{
			zlscript::CScriptVirtualMachine::GetInstance()->RunFunImmediately(pConnector->GetInitScript(), m_scriptParm);
		}

		this->SetDisconnectScript(pConnector->GetDisconnectScript());
	}
	else
	{
		m_pNetConnector = pConnector;
	}

}

//void CScriptConnector::SetHeadProtocol(CBaseHeadProtocol* pProtocol)
//{
//	m_bCanSend = false;
//	pProtocol->SetConnector(this);
//	m_LockProtocol.lock();
//	if (m_pHeadProtocol)
//	{
//		CHeadProtocolMgr::GetInstance()->Remove(m_pHeadProtocol);
//		m_pHeadProtocol = nullptr;
//	}
//	m_pHeadProtocol = pProtocol;
//	m_LockProtocol.unlock();
//}

void CScriptConnector::Merge(CScriptConnector* pOldConnect)
{
	if (pOldConnect == nullptr)
	{
		return;
	}
	m_vecEventIndexs.push_back(pOldConnect->GetEventIndex());
	for (unsigned int i = 0; i < pOldConnect->m_vecEventIndexs.size(); i++)
	{
		m_vecEventIndexs.push_back(pOldConnect->m_vecEventIndexs[i]);
	}

	m_mapReturnState = pOldConnect->m_mapReturnState;
	m_nReturnCount = pOldConnect->m_nReturnCount;

	m_mapClassImage2Index = pOldConnect->m_mapClassImage2Index;
	m_mapClassIndex2Image = pOldConnect->m_mapClassIndex2Image;

	if (pOldConnect->nRouteMode_ConnectID)
	{
		CScriptStack scriptParm;
		ScriptVector_PushVar(scriptParm, pOldConnect->GetEventIndex());
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_TYPE_CONNECT_CHANGE, 
			GetEventIndex(), scriptParm, nRouteMode_ConnectID);
	}
	nRouteMode_ConnectID = pOldConnect->nRouteMode_ConnectID;
	m_mapRouteConnect = pOldConnect->m_mapRouteConnect;

	pOldConnect->m_mapClassImage2Index.clear();
	pOldConnect->m_mapClassIndex2Image.clear();
	pOldConnect->m_vecEventIndexs.clear();
	pOldConnect->m_mapReturnState.clear();
	pOldConnect->m_nEventListIndex = 0;

	pOldConnect->m_mapRouteConnect.clear();
	pOldConnect->nRouteMode_ConnectID = 0;
}


__int64 CScriptConnector::GetImage4Index(__int64 nID)
{
	auto it = m_mapClassIndex2Image.find(nID);
	if (it != m_mapClassIndex2Image.end())
	{
		return it->second;
	}
	return 0;
}

__int64 CScriptConnector::GetIndex4Image(__int64 nImageID)
{
	auto it = m_mapClassImage2Index.find(nImageID);
	if (it != m_mapClassImage2Index.end())
	{
		return it->second;
	}
	return 0;
}

void CScriptConnector::SetImageAndIndex(__int64 nImageID, __int64 nLoaclID)
{
	m_mapClassImage2Index[nImageID] = nLoaclID;
	m_mapClassIndex2Image[nLoaclID] = nImageID;
}


PointVarInfo CScriptConnector::MakeNoSyncImage(std::string strClassName, __int64 index)
{
	auto it = m_mapNoSyncImage.find(index);
	if (it != m_mapNoSyncImage.end())
	{
		it->second.nCount++;
		return it->second.Point;
	}
	int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
	CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
	if (pMgr)
	{
		auto pNewPoint = pMgr->New(SCRIPT_NO_USED_AUTO_RELEASE);
		auto &image = m_mapNoSyncImage[index];
		image.nCount++;
		image.Point = pNewPoint->GetScriptPointIndex();
		return image.Point;
	}
	return PointVarInfo();
}

PointVarInfo CScriptConnector::GetNoSyncImage4Index(__int64 index)
{
	PointVarInfo result;
	auto it = m_mapNoSyncImage.find(index);
	if (it != m_mapNoSyncImage.end())
	{
		it->second.nCount--;
		result = it->second.Point;
		if (it->second.nCount <= 0)
		{
			m_mapNoSyncImage.erase(it);
		}
	}
	return result;
}

void CScriptConnector::RunTo(std::string funName, CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex)
{
	if (CheckScriptLimit(funName))
	{

		CScriptStack scriptParm;
		ScriptVector_PushVar(scriptParm, nReturnID);
		ScriptVector_PushVar(scriptParm, funName.c_str());
		for (unsigned int i = 0; i < pram.size(); i++)
		{
			auto p = pram.GetVal(i);
			if (p)
				scriptParm.push(*p);
		}
		ScriptVector_PushVar(scriptParm, this);

		//读取完成，执行结果
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RUNSCRIPT, GetEventIndex(), scriptParm);
	}
	else if (nRouteMode_ConnectID > 0)
	{
		//路由模式，转发
		CScriptMsgReceiveState* pMsg = (CScriptMsgReceiveState*)CMsgReceiveMgr::GetInstance()->CreateRceiveState(E_RUN_SCRIPT);
		pMsg->nReturnID = nReturnID;
		pMsg->strScriptFunName = funName;
		pMsg->m_scriptParm = pram;
		//for (unsigned int i = 0; i < pram.size(); i++)
		//{
		//	auto pVar = pram.GetVal(i);
		//	if (pVar)
		//		pMsg->m_scriptParm.push(*pVar);
		//}
		if (CRouteEventMgr::GetInstance()->SendEvent(nRouteMode_ConnectID, true, pMsg, GetEventIndex()) == false)
		{
			//转发失败，需要转发的目标不存在，取消路由状态
			nRouteMode_ConnectID = 0;

			//初始化
			CScriptStack m_scriptParm;
			ScriptVector_PushVar(m_scriptParm, funName.c_str());
			//zlscript::CScriptVirtualMachine machine;
			//machine.RunFunImmediately("Error_CannotRunScript", m_scriptParm);
			if (zlscript::CScriptVirtualMachine::GetInstance())
			{
				zlscript::CScriptVirtualMachine::GetInstance()->RunFunImmediately("Error_CannotRunScript", m_scriptParm);
			}
			CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pMsg);
		}
		//auto pRoute = CScriptConnectMgr::GetInstance()->GetConnector(nRouteMode_ConnectID);
		//if (pRoute && pCurMsgReceive)
		//{
		//	if (pCurMsgReceive)
		//	{
		//		CRouteFrontMsgReceiveState msg;
		//		msg.nConnectID = nRouteMode_ConnectID;
		//		msg.pState = pCurMsgReceive;
		//		msg.Send(pRoute);
		//	}
		//}
		//else
		//{
		//	nRouteMode_ConnectID = 0;
		//	CTempScriptRunState tempState;
		//	tempState.PushVarToStack(funName.c_str());
		//	tempState.PushVarToStack("Error_CannotRunScript");
		//	tempState.PushVarToStack(0);
		//	RunScript2Script(&tempState);
		//}
	}
	else
	{
		CTempScriptRunState tempState;
		CACHE_NEW(CScriptCallState, pCallState, &tempState);
		if (pCallState)
		{
			pCallState->PushVarToStack(funName.c_str());
			pCallState->PushVarToStack("Error_CannotRunScript");
			pCallState->PushVarToStack(0);

			RunScript2Script(pCallState);
		}

		CACHE_DELETE(pCallState);
	}
}

void CScriptConnector::ResultTo(CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex)
{
	tagReturnState* pState = GetReturnState(nReturnID);
	if (pState)
	{
		CScriptStack scriptParm;
		ScriptVector_PushVar(scriptParm, pState->nReturnID);
		for (unsigned int i = 0; i < pram.size(); i++)
		{
			auto p = pram.GetVal(i);
			if (p)
				scriptParm.push(*p);
		}
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RETURN, GetEventIndex(), scriptParm, pState->nEventIndex);
		RemoveReturnState(nReturnID);
	}
}


void CScriptConnector::SetRouteInitScript(const char* pStr)
{
	strRouteInitScript = pStr;
}

void CScriptConnector::ChangeRoute(__int64 nOldId, __int64 nNewId)
{
	auto it = m_mapRouteConnect.find(nOldId);
	if (it != m_mapRouteConnect.end())
	{
		auto pRoute = it->second;
		if (pRoute)
		{
			pRoute->SetRouteID(nNewId);
			m_mapRouteConnect[nNewId] = pRoute;
		}

		m_mapRouteConnect.erase(it);
	}
}

void CScriptConnector::RemoveRoute(__int64 nId)
{
	auto it = m_mapRouteConnect.find(nId);
	if (it != m_mapRouteConnect.end())
	{
		auto pRoute = it->second;
		if (pRoute)
		{
			pRoute->Close();
		}
	}
}

bool CScriptConnector::RouteMsg(CRouteFrontMsgReceiveState* pState)
{
	//CRouteFrontMsgReceiveState* pState = dynamic_cast<CRouteFrontMsgReceiveState*>(pMsg);
	if (pState == nullptr)
	{
		return false;
	}
	auto pRoute = m_mapRouteConnect[pState->nConnectID];
	if (pRoute == nullptr)
	{
		//TODO 以后加缓存
		m_mapRouteConnect[pState->nConnectID] = pRoute = new CScriptRouteConnector;

		pRoute->SetMaster(this);
		pRoute->SetRouteID(pState->nConnectID);
		pRoute->OnInit();
		//初始化
		CScriptStack m_scriptParm;
		ScriptVector_PushVar(m_scriptParm, pRoute);

		//zlscript::CScriptVirtualMachine machine;
		//machine.RunFunImmediately(strRouteInitScript, m_scriptParm);
		if (zlscript::CScriptVirtualMachine::GetInstance())
		{
			zlscript::CScriptVirtualMachine::GetInstance()->RunFunImmediately(strRouteInitScript, m_scriptParm);
		}
	}

	if (pRoute)
	{
		return pRoute->RunMsg(pState->pState);
		//pMsg->Run(pRoute);
	}
	return false;
}

CScriptRouteConnector::CScriptRouteConnector()
{
	bRun = true;
}

void CScriptRouteConnector::Close()
{
	bRun = false;
}

void CScriptRouteConnector::OnInit()
{
	CBaseScriptConnector::OnInit();
}

bool CScriptRouteConnector::OnProcess()
{
	if (!bRun)
	{
		return false;
	}
	CBaseScriptConnector::OnProcess();

	//处理路由消息
	std::vector<stRouteEvent*> vRouteEvent;
	CRouteEventMgr::GetInstance()->GetEvent(GetEventIndex(), vRouteEvent);
	for (auto it = vRouteEvent.begin(); it != vRouteEvent.end(); it++)
	{
		stRouteEvent* pEvent = *it;
		if (pEvent)
		{
			if (pEvent->bFront)
			{
				//理论上已经是路由虚拟连接了，不会收到ROUTE_FRONT消息
				//CRouteFrontMsgReceiveState msg;
				//msg.nConnectID = pEvent->nIndex;
				//msg.pState = pEvent->pMsg;
				//SendMsg(&msg);
			}
			else
			{
				//继续转发
				SendMsg(pEvent->pMsg);
			}
			CRouteEventMgr::GetInstance()->ReleaseEvent(pEvent);
		}
	}
	return true;
}

void CScriptRouteConnector::OnDestroy()
{
	CBaseScriptConnector::OnDestroy();
}

bool CScriptRouteConnector::SendMsg(char* pBuff, int len)
{
	if (m_pMaster)
	{
		return m_pMaster->SendMsg(pBuff, len);
	}
	return false;
}

bool CScriptRouteConnector::SendMsg(CBaseMsgReceiveState* pMsg)
{
	CRouteBackMsgReceiveState Msg;
	Msg.nConnectID = m_RouteID;
	Msg.pState = pMsg;
	bool bResult = Msg.Send(this);
	//printf("Route_Back:%d \n", m_RouteID);
	return bResult;
}

//bool CScriptRouteConnector::AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal)
//{
//	if (m_pMaster)
//	{
//		return m_pMaster->AddVar2Bytes(vBuff, pVal);
//	}
//	return false;
//}

bool CScriptRouteConnector::AddVar2Bytes(std::vector<char>& vBuff, CScriptBasePointer* pPoint)
{
	if (m_pMaster)
	{
		return m_pMaster->AddVar2Bytes(vBuff, pPoint);
	}
	return false;
}

__int64 CScriptRouteConnector::GetImage4Index(__int64 id)
{
	if (m_pMaster)
	{
		return m_pMaster->GetImage4Index(id);
	}
	return 0;
}

__int64 CScriptRouteConnector::GetIndex4Image(__int64 id)
{
	if (m_pMaster)
	{
		return m_pMaster->GetIndex4Image(id);
	}
	return 0;
}

void CScriptRouteConnector::SetImageAndIndex(__int64 nImageID, __int64 nLoaclID)
{
	if (m_pMaster)
	{
		m_pMaster->SetImageAndIndex(nImageID, nLoaclID);
	}
}


void CScriptRouteConnector::SetMaster(CScriptConnector* pConnect)
{
	m_pMaster = pConnect;
}

void CScriptRouteConnector::SetRouteID(__int64 nID)
{
	m_RouteID = nID;
}

void CScriptRouteConnector::RunTo(std::string funName, CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex)
{
	if (CheckScriptLimit(funName))
	{
		CScriptStack scriptParm;
		ScriptVector_PushVar(scriptParm, nReturnID);
		ScriptVector_PushVar(scriptParm, funName.c_str());
		for (unsigned int i = 0; i < pram.size(); i++)
		{
			auto p = pram.GetVal(i);
			if (p)
				scriptParm.push(*p);
		}
		ScriptVector_PushVar(scriptParm, this);
		//读取完成，执行结果
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RUNSCRIPT, GetEventIndex(), scriptParm);
	}
	else if (nRouteMode_ConnectID > 0)
	{
		//路由模式，转发
		CScriptMsgReceiveState* pMsg = (CScriptMsgReceiveState*)CMsgReceiveMgr::GetInstance()->CreateRceiveState(E_RUN_SCRIPT);
		pMsg->nReturnID = nReturnID;
		pMsg->strScriptFunName = funName;
		for (unsigned int i = 0; i < pram.size(); i++)
		{
			auto pVar = pram.GetVal(i);
			if (pVar)
				pMsg->m_scriptParm.push(*pVar);
		}
		if (CRouteEventMgr::GetInstance()->SendEvent(nRouteMode_ConnectID, true, pMsg, GetEventIndex()) == false)
		{
			//转发失败，需要转发的目标不存在，取消路由状态
			nRouteMode_ConnectID = 0;
			CTempScriptRunState tempState;
			CACHE_NEW(CScriptCallState, pCallState, &tempState);
			if (pCallState)
			{
				pCallState->PushVarToStack(funName.c_str());
				pCallState->PushVarToStack("Error_CannotRunScript");
				pCallState->PushVarToStack(0);

				RunScript2Script(pCallState);
			}

			CACHE_DELETE(pCallState);
			CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pMsg);
		}
		//auto pRoute = CScriptConnectMgr::GetInstance()->GetConnector(nRouteMode_ConnectID);
		//if (pRoute)
		//{
		//	//if (pCurMsgReceive)
		//	{
		//		CRouteFrontMsgReceiveState msg;
		//		msg.nConnectID = nRouteMode_ConnectID;
		//		msg.pState = pCurMsgReceive;
		//		msg.Send(pRoute);
		//	}
		//}
		//else
		//{
		//	nRouteMode_ConnectID = 0;
		//	CTempScriptRunState tempState;
		//	tempState.PushVarToStack(funName.c_str());
		//	tempState.PushVarToStack("Error_CannotRunScript");
		//	tempState.PushVarToStack(0);
		//	RunScript2Script(&tempState);
		//}
	}
	else
	{
		CTempScriptRunState tempState;
		CACHE_NEW(CScriptCallState, pCallState, &tempState);
		if (pCallState)
		{
			pCallState->PushVarToStack(funName.c_str());
			pCallState->PushVarToStack("Error_CannotRunScript");
			pCallState->PushVarToStack(0);

			RunScript2Script(pCallState);
		}

		CACHE_DELETE(pCallState);
	}
}

void CScriptRouteConnector::ResultTo(CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex)
{
	tagReturnState* pState = GetReturnState(nReturnID);
	if (pState)
	{
		CScriptStack scriptParm;
		ScriptVector_PushVar(scriptParm, pState->nReturnID);
		for (unsigned int i = 0; i < pram.size(); i++)
		{
			auto p = pram.GetVal(i);
			if (p)
				scriptParm.push(*p);
		}
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RETURN, GetEventIndex(), scriptParm, pState->nEventIndex);
		RemoveReturnState(nReturnID);
	}
}
