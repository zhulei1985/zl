#include "ScriptConnector.h"
#include "zByteArray.h"
#include "ScriptExecCodeMgr.h"

CScriptConnector::CScriptConnector()
{
	//m_nCurMsgLen = 0;

	pCurMsgReceive = nullptr;

	RegisterClassFun(GetID, this, &CScriptConnector::GetID2Script);
	RegisterClassFun(GetPort, this, &CScriptConnector::GetPort2Script);

	RegisterClassFun(IsConnect, this, &CScriptConnector::IsConnect2Script);
	RegisterClassFun(RunScript,this, &CScriptConnector::RunScript2Script);
	RegisterClassFun(SetAccount, this, &CScriptConnector::SetAccount2Script);
	RegisterClassFun(GetAccount, this, &CScriptConnector::GetAccount2Script);

	RegisterClassFun(SetScriptLimit, this, &CScriptConnector::SetScriptLimit2Script);
	RegisterClassFun(CheckScriptLimit, this, &CScriptConnector::CheckScriptLimit2Script);

	RegisterClassFun(SetRemoteFunction, this, &CScriptConnector::SetRemoteFunction2Script);
}


CScriptConnector::~CScriptConnector()
{
}
void CScriptConnector::Init2Script()
{
	RegisterClassType("Connector", CScriptConnector);

	RegisterClassFun1("GetID", CScriptConnector);
	RegisterClassFun1("GetPort", CScriptConnector);

	RegisterClassFun1("IsConnect", CScriptConnector);
	RegisterClassFun1("RunScript", CScriptConnector);
	RegisterClassFun1("SetAccount", CScriptConnector);
	RegisterClassFun1("GetAccount", CScriptConnector);

	RegisterClassFun1("SetScriptLimit", CScriptConnector);
	RegisterClassFun1("CheckScriptLimit", CScriptConnector);

	RegisterClassFun1("SetRemoteFunction", CScriptConnector);
}
int CScriptConnector::GetID2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(GetID());
	return ECALLBACK_FINISH;
}
int CScriptConnector::GetPort2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(GetPort());
	return ECALLBACK_FINISH;
}
int CScriptConnector::IsConnect2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(IsSocketClosed() ? 0 : 1);
	return ECALLBACK_FINISH;
}
int CScriptConnector::RunScript2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	CScriptMsgReceiveState msg;
	int nParmNum = pState->GetParamNum() - 2;
	if (pState->m_pMachine)
		msg.nEventListIndex = pState->m_pMachine->m_nEventListIndex;
	else
		msg.nEventListIndex = 0;
	int nIsWaiting = pState->PopIntVarFormStack();//是否等待调用函数完成
	if (nIsWaiting > 0)
		msg.nStateID = (__int64)pState->GetId();
	else
		msg.nStateID = 0;
	msg.strScriptFunName = pState->PopCharVarFormStack();

	pState->CopyToStack(&msg.m_scriptParm, nParmNum);
	msg.Send(this);

	pState->ClearFunParam();
	if (nIsWaiting > 0)
	{
		return ECALLBACK_WAITING;
	}
	else
		return ECALLBACK_FINISH;
	return ECALLBACK_FINISH;
}
int CScriptConnector::SetAccount2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strAccountName = pState->PopCharVarFormStack();
	pState->ClearFunParam();

	return ECALLBACK_FINISH;
}
int CScriptConnector::GetAccount2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(strAccountName.c_str());
	return ECALLBACK_FINISH;
}
int CScriptConnector::SetScriptLimit2Script(CScriptRunState* pState)
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
int CScriptConnector::CheckScriptLimit2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strName = pState->PopCharVarFormStack();
	bool bCheck = CheckScriptLimit(strName);
	pState->ClearFunParam();
	pState->PushVarToStack(bCheck?1:0);
	return ECALLBACK_FINISH;
}
int CScriptConnector::SetRemoteFunction2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strName = pState->PopCharVarFormStack();
	CScriptExecCodeMgr::GetInstance()->SetRemoteFunction(strName, GetScriptEventIndex());
	m_setRemoteFunName.insert(strName);
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}
void CScriptConnector::OnInit()
{
	CSocketConnector::OnInit();

	nScriptEventIndex = CScriptEventMgr::GetInstance()->AssignID();

	AddClassObject(this->GetScriptPointIndex(), this);

}
bool CScriptConnector::OnProcess()
{
	if (CSocketConnector::OnProcess() == false)
	{
		return false;
	}

	//从接收队列里取出消息处理
	std::vector<char> vOut;

	//TODO 要为新连接做验证

	if (pCurMsgReceive == nullptr)
	{
		if (GetData(vOut, 1))
		{
			int nPos = 0;
			char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());

			pCurMsgReceive = CMsgReceiveMgr::GetInstance()->CreateRceiveState(cType);
		}
	}
	if (pCurMsgReceive)
	{
		if (pCurMsgReceive->Recv(this) == true)
		{
			pCurMsgReceive->Run(this);
			CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pCurMsgReceive);
			pCurMsgReceive = nullptr;
		}
		else
		{
			//等待读完
			return true;
		}
	}


	std::vector<tagScriptEvent*> vEvent;
	CScriptEventMgr::GetInstance()->GetEventByChannel(GetScriptEventIndex(), vEvent);
	for (size_t i = 0; i < vEvent.size(); i++)
	{
		tagScriptEvent* pEvent = vEvent[i];
		if (pEvent)
		{
			if (pEvent->nEventType == E_SCRIPT_EVENT_NEWTWORK_RETURN)
			{
				EventReturnFun(pEvent->nSendID, pEvent->m_Parm);
			}
			else if (pEvent->nEventType == E_SCRIPT_EVENT_NETWORK_RUNSCRIPT)
			{
				EventRunFun(pEvent->nSendID, pEvent->m_Parm);
			}
		}
		CScriptEventMgr::GetInstance()->ReleaseEvent(pEvent);
	}
	return true;
}
void CScriptConnector::OnDestroy()
{
	for (auto it = m_setRemoteFunName.begin(); it != m_setRemoteFunName.end(); it++)
	{
		CScriptExecCodeMgr::GetInstance()->RemoveRemoteFunction(*it, GetScriptEventIndex());
	}
	CSocketConnector::OnDestroy();

	RemoveClassObject(this->GetScriptPointIndex());


}
bool CScriptConnector::SendMsg(char *pBuff, int len)
{
	CSocketConnector::SendData(pBuff, len);
	return true;
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
			const char *pStr = StackVarInfo::s_strPool.GetString(pVal->Int64);
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
						//如果这个类实例是本连接对应的镜像
						AddChar2Bytes(vBuff,0);
					}
					else
					{
						AddChar2Bytes(vBuff, 1);
						if (!pSyncPoint->CheckSyncProcess(GetID()))
						{
							//新同步
							pSyncPoint->AddSyncProcess(GetID());
							//发送消息
							SendSyncClassMsg(pPoint->GetClassName(), pSyncPoint);
						}
					}
					AddInt642Bytes(vBuff, pPoint->GetPoint()->GetScriptPointIndex());
				}
				pPoint->Unlock();
			}
			else
			{
				//错误，没有找到对象或是非同步性对象
			}
			CScriptSuperPointerMgr::GetInstance()->ReturnPointer(pPoint);
		}
		break;
	}
	return true;
}

void CScriptConnector::SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint)
{
	CSyncClassDataMsgReceiveState msg;
	msg.strClassName = strClassName;
	msg.m_pPoint = pPoint;
	msg.Send(this);
}

void CScriptConnector::SyncUpClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack)
{
	CSyncUpMsgReceiveState msg;
	msg.strFunName = strFunName;
	msg.nClassID = pPoint->GetScriptPointIndex();
	msg.m_scriptParm = stack;
	msg.Send(this);
}

void CScriptConnector::SyncDownClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack)
{

	CSyncDownMsgReceiveState msg;
	msg.strFunName = strFunName;
	msg.nClassID = pPoint->GetScriptPointIndex();
	msg.m_scriptParm = stack;
	msg.Send(this);
}



void CScriptConnector::EventReturnFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	CReturnMsgReceiveState msg;
	msg.nEventListIndex = ScriptStack_GetInt(ParmInfo);
	msg.nStateID = ScriptStack_GetInt(ParmInfo);
	msg.m_scriptParm = ParmInfo;
	msg.Send(this);
}

void CScriptConnector::EventRunFun(__int64 nSendID, CScriptStack& ParmInfo)
{
	CScriptMsgReceiveState msg;
	msg.nEventListIndex = ScriptStack_GetInt(ParmInfo);
	msg.nStateID = ScriptStack_GetInt(ParmInfo);
	msg.strScriptFunName = ScriptStack_GetString(ParmInfo);
	msg.m_scriptParm = ParmInfo;

	msg.Send(this);
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

void CScriptConnector::SetScriptLimit(std::string strName)
{
	m_mapScriptLimit[strName] = 1;
}

bool CScriptConnector::CheckScriptLimit(std::string strName)
{
	auto it = m_mapScriptLimit.find(strName);
	if (it != m_mapScriptLimit.end())
	{
		return true;
	}
	return false;
}

void CScriptConnector::RemoveScriptLimit(std::string strName)
{
	auto it = m_mapScriptLimit.find(strName);
	if (it != m_mapScriptLimit.end())
	{
		m_mapScriptLimit.erase(it);
	}
}
