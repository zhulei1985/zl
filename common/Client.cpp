#include "Client.h"
#include "zByteArray.h"

CClient::CClient()
{
	//m_nCurMsgLen = 0;

	pCurMsgReceive = nullptr;

	RegisterClassFun(GetID, this, &CClient::GetID2Script);

	RegisterClassFun(IsConnect, this, &CClient::IsConnect2Script);
	RegisterClassFun(RunScript,this, &CClient::RunScript2Script);
	RegisterClassFun(SetAccount, this, &CClient::SetAccount2Script);
	RegisterClassFun(GetAccount, this, &CClient::GetAccount2Script);

	RegisterClassFun(SetScriptLimit, this, &CClient::SetScriptLimit2Script);
	RegisterClassFun(CheckScriptLimit, this, &CClient::CheckScriptLimit2Script);
}


CClient::~CClient()
{
}
void CClient::Init2Script()
{
	RegisterClassType("Client", CClient);

	RegisterClassFun1("GetID", CClient);

	RegisterClassFun1("IsConnect", CClient);
	RegisterClassFun1("RunScript", CClient);
	RegisterClassFun1("SetAccount", CClient);
	RegisterClassFun1("GetAccount", CClient);

	RegisterClassFun1("SetScriptLimit", CClient);
	RegisterClassFun1("CheckScriptLimit", CClient);
}
int CClient::GetID2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(GetID());
	return ECALLBACK_FINISH;
}
int CClient::IsConnect2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(IsSocketClosed() ? 0 : 1);
	return ECALLBACK_FINISH;
}
int CClient::RunScript2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	m_vBuff.clear();
	int nParmNum = pState->GetParamNum() - 2;
	AddChar2Bytes(m_vBuff, E_RUN_SCRIPT);
	if (pState->m_pMachine)
		AddInt2Bytes(m_vBuff, pState->m_pMachine->m_nEventListIndex);
	else
		AddInt2Bytes(m_vBuff, 0);
	int nIsWaiting = pState->PopIntVarFormStack();//是否等待调用函数完成
	if (nIsWaiting > 0)
		AddInt642Bytes(m_vBuff, (__int64)pState->GetId());
	else
		AddInt642Bytes(m_vBuff, (__int64)0);
	std::string scriptName = pState->PopCharVarFormStack();
	AddString2Bytes(m_vBuff, (char*)scriptName.c_str());//脚本函数名

	AddChar2Bytes(m_vBuff, (char)nParmNum);

	for (int i = 0; i < nParmNum; i++)
	{
		AddVar2Bytes(m_vBuff, &pState->PopVarFormStack());
	}
	SendData(&m_vBuff[0], m_vBuff.size());


	pState->ClearFunParam();
	if (nIsWaiting > 0)
	{
		return ECALLBACK_WAITING;
	}
	else
		return ECALLBACK_FINISH;
	return ECALLBACK_FINISH;
}
int CClient::SetAccount2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strAccountName = pState->PopCharVarFormStack();
	pState->ClearFunParam();

	return ECALLBACK_FINISH;
}
int CClient::GetAccount2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(strAccountName.c_str());
	return ECALLBACK_FINISH;
}
int CClient::SetScriptLimit2Script(CScriptRunState* pState)
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
int CClient::CheckScriptLimit2Script(CScriptRunState* pState)
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
void CClient::OnInit()
{
	CSocketConnector::OnInit();

	AddClassObject(this->GetScriptPointIndex(), this);

	CScriptEventMgr::GetInstance()->RegisterEvent(E_SCRIPT_EVENT_NEWTWORK_RETURN, CBaseConnector::GetID());
}
bool CClient::OnProcess()
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
		if (pCurMsgReceive->OnProcess(this) == true)
		{
			CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pCurMsgReceive);
			pCurMsgReceive = nullptr;
		}
		else
		{
			//等待读完
			return true;
		}
	}

	CScriptEventMgr::GetInstance()->ProcessEvent(E_SCRIPT_EVENT_NEWTWORK_RETURN, CBaseConnector::GetID(),
		std::bind(&CClient::EventReturnFun, this, std::placeholders::_1, std::placeholders::_2));

	return true;
}
void CClient::OnDestroy()
{
	CScriptEventMgr::GetInstance()->RemoveEvent(E_SCRIPT_EVENT_NEWTWORK_RETURN, CBaseConnector::GetID());

	CSocketConnector::OnDestroy();

	RemoveClassObject(this->GetScriptPointIndex());


}
bool CClient::SendMsg(char *pBuff, int len)
{
	CSocketConnector::SendData(pBuff, len);
	return true;
}

bool CClient::AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal)
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

void CClient::SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint)
{
	tagByteArray vBuff;
	AddChar2Bytes(vBuff, E_SYNC_CLASS_DATA);
	AddInt642Bytes(vBuff, pPoint->GetScriptPointIndex());
	AddString2Bytes(vBuff, (char*)strClassName.c_str());

	tagByteArray vDataBuff;
	pPoint->AddAllData2Bytes(vDataBuff);
	AddData2Bytes(vBuff, vDataBuff);
	SendData(&vBuff[0], vBuff.size());
}

void CClient::SyncUpClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack)
{
	tagByteArray vBuff;
	AddChar2Bytes(vBuff, E_SYNC_UP_PASSAGE);
	AddInt642Bytes(vBuff, pPoint->GetScriptPointIndex());
	AddString2Bytes(vBuff, (char*)strFunName.c_str());

	AddChar2Bytes(m_vBuff, (char)stack.size());
	for (int i = stack.size() - 1; i >= 0; i--)
	{
		AddVar2Bytes(m_vBuff, stack.GetVal(i));
	}
	SendData(&vBuff[0], vBuff.size());
}

void CClient::SyncDownClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack)
{
	tagByteArray vBuff;
	AddChar2Bytes(vBuff, E_SYNC_DOWN_PASSAGE);
	AddInt642Bytes(vBuff, pPoint->GetScriptPointIndex());
	AddString2Bytes(vBuff, (char*)strFunName.c_str());

	AddChar2Bytes(m_vBuff, (char)stack.size());
	for (int i = stack.size()-1; i >=0; i--)
	{
		AddVar2Bytes(m_vBuff, stack.GetVal(i));
	}
	SendData(&vBuff[0], vBuff.size());
}



void CClient::EventReturnFun(int nSendID, CScriptStack& ParmInfo)
{

	m_vBuff.clear();

	AddChar2Bytes(m_vBuff, E_RUN_SCRIPT_RETURN);

	int nEventIndex = ScriptStack_GetInt(ParmInfo);
	AddInt2Bytes(m_vBuff, nEventIndex);
	__int64 nStateID = ScriptStack_GetInt(ParmInfo);
	AddInt642Bytes(m_vBuff, nStateID);

	AddChar2Bytes(m_vBuff, (char)ParmInfo.size());
	while (ParmInfo.size() > 0)
	{
		AddVar2Bytes(m_vBuff, &ParmInfo.top());
		ParmInfo.pop();
	}
	SendData(&m_vBuff[0], m_vBuff.size());
}

__int64 CClient::GetImageIndex(__int64 nID)
{
	auto it = m_mapClassImageIndex.find(nID);
	if (it != m_mapClassImageIndex.end())
	{
		return it->second;
	}
	return 0;
}

void CClient::SetImageIndex(__int64 nImageID, __int64 nLoaclID)
{
	m_mapClassImageIndex[nImageID] = nLoaclID;
}

void CClient::SetScriptLimit(std::string strName)
{
	m_mapScriptLimit[strName] = 1;
}

bool CClient::CheckScriptLimit(std::string strName)
{
	auto it = m_mapScriptLimit.find(strName);
	if (it != m_mapScriptLimit.end())
	{
		return true;
	}
	return false;
}

void CClient::RemoveScriptLimit(std::string strName)
{
	auto it = m_mapScriptLimit.find(strName);
	if (it != m_mapScriptLimit.end())
	{
		m_mapScriptLimit.erase(it);
	}
}
