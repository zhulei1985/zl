#include "ScriptConnector.h"
#include "zByteArray.h"
#include "ScriptExecCodeMgr.h"
#include "ScriptConnectMgr.h"
#include "TempScriptRunState.h"
#include <functional>
#include "RouteEvent.h"

CBaseScriptConnector::CBaseScriptConnector()
{
	RegisterClassFun(GetID, this, &CBaseScriptConnector::GetID2Script);
	RegisterClassFun(GetPort, this, &CBaseScriptConnector::GetPort2Script);

	RegisterClassFun(IsConnect, this, &CBaseScriptConnector::IsConnect2Script);
	RegisterClassFun(RunScript, this, &CBaseScriptConnector::RunScript2Script);
	RegisterClassFun(SetAccount, this, &CBaseScriptConnector::SetAccount2Script);
	RegisterClassFun(GetAccount, this, &CBaseScriptConnector::GetAccount2Script);

	RegisterClassFun(SetScriptLimit, this, &CBaseScriptConnector::SetScriptLimit2Script);
	RegisterClassFun(CheckScriptLimit, this, &CBaseScriptConnector::CheckScriptLimit2Script);

	RegisterClassFun(SetRemoteFunction, this, &CBaseScriptConnector::SetRemoteFunction2Script);

	RegisterClassFun(SetRoute, this, &CBaseScriptConnector::SetRoute2Script);
	RegisterClassFun(SetRouteInitScript, this, &CBaseScriptConnector::SetRouteInitScript2Script);
}

CBaseScriptConnector::~CBaseScriptConnector()
{
}

void CBaseScriptConnector::Init2Script()
{
	RegisterClassType("Connector", CBaseScriptConnector);

	RegisterClassFun1("GetID", CBaseScriptConnector);
	RegisterClassFun1("GetPort", CBaseScriptConnector);

	RegisterClassFun1("IsConnect", CBaseScriptConnector);
	RegisterClassFun1("RunScript", CBaseScriptConnector);
	RegisterClassFun1("SetAccount", CBaseScriptConnector);
	RegisterClassFun1("GetAccount", CBaseScriptConnector);

	RegisterClassFun1("SetScriptLimit", CBaseScriptConnector);
	RegisterClassFun1("CheckScriptLimit", CBaseScriptConnector);

	RegisterClassFun1("SetRemoteFunction", CBaseScriptConnector);

	RegisterClassFun1("SetRoute", CBaseScriptConnector);
	RegisterClassFun1("SetRouteInitScript", CBaseScriptConnector);
}

int CBaseScriptConnector::GetID2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(GetEventIndex());
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::GetPort2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(GetSocketPort());
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::IsConnect2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(IsSocketClosed() ? 0 : 1);
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::RunScript2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	int nParmNum = pState->GetParamNum() - 2;
	CScriptStack parm;
	__int64 nEventIndex = 0;
	__int64 nReturnID = 0;
	if (pState->m_pMachine)
		nEventIndex = pState->m_pMachine->GetEventIndex();
	int nIsWaiting = pState->PopIntVarFormStack();//�Ƿ�ȴ����ú������
	if (nIsWaiting > 0)
		nReturnID = (__int64)pState->GetId();
	std::string strScriptFunName = pState->PopCharVarFormStack();
	pState->CopyToStack(&parm, nParmNum);
	RunFrom(strScriptFunName, parm, nReturnID, nEventIndex);


	pState->ClearFunParam();
	if (nIsWaiting > 0)
	{
		return ECALLBACK_WAITING;
	}
	else
		return ECALLBACK_FINISH;
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::SetAccount2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strAccountName = pState->PopCharVarFormStack();
	pState->ClearFunParam();

	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::GetAccount2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(strAccountName.c_str());
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::SetScriptLimit2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strName = pState->PopCharVarFormStack();
	int nVal = pState->PopIntVarFormStack();
	if (nVal > 0)
	{
		SetScriptLimit(strName);
	}
	else
	{
		RemoveScriptLimit(strName);
	}
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::CheckScriptLimit2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strName = pState->PopCharVarFormStack();
	bool bCheck = CheckScriptLimit(strName);
	pState->ClearFunParam();
	pState->PushVarToStack(bCheck ? 1 : 0);
	return ECALLBACK_FINISH;
}
int CBaseScriptConnector::SetRemoteFunction2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strName = pState->PopCharVarFormStack();
	CScriptExecCodeMgr::GetInstance()->SetRemoteFunction(strName, GetEventIndex());
	m_setRemoteFunName.insert(strName);
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::SetRoute2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	__int64 nClassIndex = pState->PopClassPointFormStack();
	CScriptBasePointer* pPoint = CScriptSuperPointerMgr::GetInstance()->PickupPointer(nClassIndex);
	if (pPoint)
	{
		pPoint->Lock();
		auto pConnector = dynamic_cast<CBaseScriptConnector*>(pPoint->GetPoint());
		if (pConnector)
		{
			nRouteMode_ConnectID = pConnector->GetEventIndex();
		}
		pPoint->Unlock();
		CScriptSuperPointerMgr::GetInstance()->ReturnPointer(pPoint);
	}
	else
	{
		nRouteMode_ConnectID = 0;
	}
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

int CBaseScriptConnector::SetRouteInitScript2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strScriptName = pState->PopCharVarFormStack();
	SetRouteInitScript(strScriptName.c_str());
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

void CBaseScriptConnector::OnInit()
{
	CScriptExecFrame::OnInit();
	CRouteEventMgr::GetInstance()->Register(GetEventIndex());
	//nScriptEventIndex = CScriptEventMgr::GetInstance()->AssignID();
	m_nReturnCount = 0;
	m_mapReturnState.clear();
	AddClassObject(this->GetScriptPointIndex(), this);

	InitEvent(zlscript::E_SCRIPT_EVENT_RETURN, std::bind(&CBaseScriptConnector::EventReturnFun, this, std::placeholders::_1, std::placeholders::_2), false);
	InitEvent(zlscript::E_SCRIPT_EVENT_RUNSCRIPT, std::bind(&CBaseScriptConnector::EventRunFun, this, std::placeholders::_1, std::placeholders::_2), false);

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

	RemoveClassObject(this->GetScriptPointIndex());
	CRouteEventMgr::GetInstance()->Unregister(GetEventIndex());
	CScriptExecFrame::Clear();

}

bool CBaseScriptConnector::RunMsg(CBaseMsgReceiveState* pMsg)
{
	bool bResult = false;
	if (nRouteMode_ConnectID != 0 && pMsg->GetType() != E_RUN_SCRIPT)
	{
		//·��״̬������Ϣֱ��ת��
		//auto pRoute = CScriptConnectMgr::GetInstance()->GetConnector(nRouteMode_ConnectID);
		//if (pRoute)
		//{
		//	CRouteFrontMsgReceiveState msg;
		//	msg.nConnectID = GetEventIndex();
		//	msg.pState = pMsg;
		//	msg.Send(pRoute);
		//}
		//else
		//{
		//	nRouteMode_ConnectID = 0;
		//	bResult = pMsg->Run(this);
		//}
		if (CRouteEventMgr::GetInstance()->SendEvent(nRouteMode_ConnectID, true, pMsg, GetEventIndex()) == false)
		{
			//ת��ʧ�ܣ���Ҫת����Ŀ�겻���ڣ�ȡ��·��״̬
			nRouteMode_ConnectID = 0;
			bResult = pMsg->Run(this);
		}
	}
	else
	{
		bResult = pMsg->Run(this);
	}
	return bResult;
}


void CBaseScriptConnector::SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint)
{
	CSyncClassDataMsgReceiveState msg;
	msg.strClassName = strClassName;
	msg.m_pPoint = pPoint;
	SendMsg(&msg);
}

void CBaseScriptConnector::SyncUpClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack)
{
	CSyncUpMsgReceiveState msg;
	msg.strFunName = strFunName;
	msg.nClassID = pPoint->GetScriptPointIndex();
	//������ѹջ
	for (int i = stack.size() - 1; i >= 0; i--)
	{
		auto pVal = stack.GetVal(i);
		if (pVal)
			msg.m_scriptParm.push(*pVal);
	}

	SendMsg(&msg);
}

void CBaseScriptConnector::SyncDownClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack)
{

	CSyncDownMsgReceiveState msg;
	msg.strFunName = strFunName;
	msg.nClassID = pPoint->GetScriptPointIndex();
	//������ѹջ
	for (int i = stack.size() - 1; i >= 0; i--)
	{
		auto pVal = stack.GetVal(i);
		if (pVal)
			msg.m_scriptParm.push(*pVal);
	}
	SendMsg(&msg);
}

void CBaseScriptConnector::EventReturnFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nReturnID = ScriptStack_GetInt(ParmInfo);

	ResultFrom(ParmInfo, nReturnID);
}

void CBaseScriptConnector::EventRunFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	__int64 nReturnID = ScriptStack_GetInt(ParmInfo);
	std::string funName = ScriptStack_GetString(ParmInfo);

	RunFrom(funName, ParmInfo, nReturnID, nSendID);
}


void CBaseScriptConnector::RunFrom(std::string funName, CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex)
{
	//����nReturnID��nEventIndex����һ������ID

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

	//������ѹջ
	int nParmNum = (int)pram.size();
	for (int i = nParmNum-1; i >= 0; i--)
	{
		auto pVar = pram.GetVal(i);
		if (pVar)
			msg.m_scriptParm.push(*pVar);
	}

	SendMsg(&msg);
}

void CBaseScriptConnector::ResultFrom(CScriptStack& pram, __int64 nReturnID)
{
	CReturnMsgReceiveState msg;
	msg.nReturnID = nReturnID;

	//������ѹջ
	for (int i = pram.size() - 1; i >= 0; i--)
	{
		auto pVal = pram.GetVal(i);
		if (pVal)
			msg.m_scriptParm.push(*pVal);
	}
	msg.m_scriptParm = pram;
	SendMsg(&msg);
}

void CBaseScriptConnector::SetScriptLimit(std::string strName)
{
	m_mapScriptLimit[strName] = 1;
}

bool CBaseScriptConnector::CheckScriptLimit(std::string strName)
{
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
	pCurMsgReceive = nullptr;
}


CScriptConnector::~CScriptConnector()
{
}

int CScriptConnector::GetSocketPort()
{
	return CSocketConnector::GetPort();
}
bool CScriptConnector::IsSocketClosed()
{
	return CSocketConnector::IsSocketClosed();
}

void CScriptConnector::OnInit()
{
	CSocketConnector::OnInit();
	CBaseScriptConnector::OnInit();

}
bool CScriptConnector::OnProcess()
{
	if (CSocketConnector::OnProcess() == false)
	{
		return false;
	}

	//�ӽ��ն�����ȡ����Ϣ����
	std::vector<char> vOut;

	//TODO ҪΪ����������֤

	if (pCurMsgReceive == nullptr)
	{
		if (GetData(vOut, 1))
		{
			int nPos = 0;
			char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());

			pCurMsgReceive = CMsgReceiveMgr::GetInstance()->CreateRceiveState(cType);
			if (pCurMsgReceive == nullptr)
			{
				//TODO �������ݣ��ر�����
			}
		}
	}
	if (pCurMsgReceive)
	{
		if (pCurMsgReceive->Recv(this) == true)
		{
			//����true����Ҫ�ͷ���Ϣ
			if (RunMsg(pCurMsgReceive))
			{
				CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pCurMsgReceive);
				pCurMsgReceive = nullptr;
			}

		}
		else
		{
			//�ȴ�����
			return true;
		}
	}
	CBaseScriptConnector::OnProcess();

	//����·����Ϣ
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
	for (; it != m_mapRouteConnect.end(); it++)
	{
		CScriptRouteConnector* pRoute = it->second;
		if (pRoute)
		{
			pRoute->OnProcess();
		}
	}
	return true;
}
void CScriptConnector::OnDestroy()
{
	CSocketConnector::OnDestroy();

	CBaseScriptConnector::OnDestroy();

}
bool CScriptConnector::SendMsg(char* pBuff, int len)
{
	CSocketConnector::SendData(pBuff, len);
	return true;
}

bool CScriptConnector::SendMsg(CBaseMsgReceiveState* pMsg)
{
	if (pMsg)
	{
		return pMsg->Send(this);
	}
	return false;
}

bool CScriptConnector::AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal)
{
	if (!pVal)
	{
		return false;
	}
	switch (pVal->cType)
	{
	case EScriptVal_Int:
	{
		AddChar2Bytes(vBuff, EScriptVal_Int);
		AddInt642Bytes(vBuff, pVal->Int64);
	}
	break;
	case EScriptVal_Double:
	{
		AddChar2Bytes(vBuff, EScriptVal_Double);
		AddDouble2Bytes(vBuff, pVal->Double);
	}
	break;
	case EScriptVal_String:
	{
		AddChar2Bytes(vBuff, EScriptVal_String);
		const char* pStr = StackVarInfo::s_strPool.GetString(pVal->Int64);
		AddString2Bytes(vBuff, (char*)pStr);
	}
	break;
	case EScriptVal_ClassPointIndex:
	{
		AddChar2Bytes(vBuff, EScriptVal_ClassPointIndex);
		auto pPoint = CScriptSuperPointerMgr::GetInstance()->PickupPointer(pVal->Int64);
		auto pSyncPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
		if (pPoint && pSyncPoint)
		{
			pPoint->Lock();
			AddString2Bytes(vBuff, (char*)pPoint->GetClassName());
			if (pPoint->GetPoint())
			{
				if (pSyncPoint->GetProcessID() == GetID())
				{
					//��������ʵ���Ǳ����Ӷ�Ӧ�ľ���
					AddChar2Bytes(vBuff, 0);
				}
				else
				{
					AddChar2Bytes(vBuff, 1);
					if (!pSyncPoint->CheckSyncProcess(GetID()))
					{
						//��ͬ��
						pSyncPoint->AddSyncProcess(GetID());
						//������Ϣ
						SendSyncClassMsg(pPoint->GetClassName(), pSyncPoint);
					}
				}
				AddInt642Bytes(vBuff, pPoint->GetPoint()->GetScriptPointIndex());
			}
			pPoint->Unlock();
		}
		else
		{
			//����û���ҵ�������Ƿ�ͬ���Զ���
		}
		CScriptSuperPointerMgr::GetInstance()->ReturnPointer(pPoint);
	}
	break;
	}
	return true;
}


__int64 CScriptConnector::GetImageIndex(__int64 nID)
{
	auto it = m_mapClassImageIndex.find(nID);
	if (it != m_mapClassImageIndex.end())
	{
		return it->second;
	}
	return 0;
}

void CScriptConnector::SetImageIndex(__int64 nImageID, __int64 nLoaclID)
{
	m_mapClassImageIndex[nImageID] = nLoaclID;
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
		//��ȡ��ɣ�ִ�н��
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RUNSCRIPT, GetEventIndex(), scriptParm);
	}
	else if (nRouteMode_ConnectID > 0)
	{
		//·��ģʽ��ת��
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
			//ת��ʧ�ܣ���Ҫת����Ŀ�겻���ڣ�ȡ��·��״̬
			nRouteMode_ConnectID = 0;

			//��ʼ��
			CScriptStack m_scriptParm;
			ScriptVector_PushVar(m_scriptParm, funName.c_str());
			zlscript::CScriptVirtualMachine machine;
			machine.RunFunImmediately("Error_CannotRunScript", m_scriptParm);

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
		tempState.PushVarToStack(funName.c_str());
		tempState.PushVarToStack("Error_CannotRunScript");
		tempState.PushVarToStack(0);
		RunScript2Script(&tempState);
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

bool CScriptConnector::RouteMsg(CBaseMsgReceiveState* pMsg)
{
	CRouteFrontMsgReceiveState* pState = dynamic_cast<CRouteFrontMsgReceiveState*>(pMsg);
	if (pState == nullptr)
	{
		return false;
	}
	auto pRoute = m_mapRouteConnect[pState->nConnectID];
	if (pRoute == nullptr)
	{
		//TODO �Ժ�ӻ���
		m_mapRouteConnect[pState->nConnectID] = pRoute = new CScriptRouteConnector;

		pRoute->SetMaster(this);
		pRoute->SetRouteID(pState->nConnectID);
		pRoute->OnInit();
		//��ʼ��
		CScriptStack m_scriptParm;
		ScriptVector_PushVar(m_scriptParm, pRoute);
		zlscript::CScriptVirtualMachine machine;
		machine.RunFunImmediately(strRouteInitScript, m_scriptParm);
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
}

void CScriptRouteConnector::OnInit()
{
	CBaseScriptConnector::OnInit();
}

bool CScriptRouteConnector::OnProcess()
{
	CBaseScriptConnector::OnProcess();

	//����·����Ϣ
	std::vector<stRouteEvent*> vRouteEvent;
	CRouteEventMgr::GetInstance()->GetEvent(GetEventIndex(), vRouteEvent);
	for (auto it = vRouteEvent.begin(); it != vRouteEvent.end(); it++)
	{
		stRouteEvent* pEvent = *it;
		if (pEvent)
		{
			if (pEvent->bFront)
			{
				//�������Ѿ���·�����������ˣ������յ�ROUTE_FRONT��Ϣ
				//CRouteFrontMsgReceiveState msg;
				//msg.nConnectID = pEvent->nIndex;
				//msg.pState = pEvent->pMsg;
				//SendMsg(&msg);
			}
			else
			{
				//����ת��
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

bool CScriptRouteConnector::AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal)
{
	if (m_pMaster)
	{
		return m_pMaster->AddVar2Bytes(vBuff, pVal);
	}
	return false;
}

__int64 CScriptRouteConnector::GetImageIndex(__int64 id)
{
	if (m_pMaster)
	{
		return m_pMaster->GetImageIndex(id);
	}
	return 0;
}

void CScriptRouteConnector::SetImageIndex(__int64 nImageID, __int64 nLoaclID)
{
	if (m_pMaster)
	{
		m_pMaster->SetImageIndex(nImageID, nLoaclID);
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
		//��ȡ��ɣ�ִ�н��
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RUNSCRIPT, GetEventIndex(), scriptParm);
	}
	else if (nRouteMode_ConnectID > 0)
	{
		//·��ģʽ��ת��
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
			//ת��ʧ�ܣ���Ҫת����Ŀ�겻���ڣ�ȡ��·��״̬
			nRouteMode_ConnectID = 0;
			CTempScriptRunState tempState;
			tempState.PushVarToStack(funName.c_str());
			tempState.PushVarToStack("Error_CannotRunScript");
			tempState.PushVarToStack(0);
			RunScript2Script(&tempState);
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
		tempState.PushVarToStack(funName.c_str());
		tempState.PushVarToStack("Error_CannotRunScript");
		tempState.PushVarToStack(0);
		RunScript2Script(&tempState);
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