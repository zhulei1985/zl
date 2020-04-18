#include "Client.h"

#include "zByteArray.h"

bool CScriptMsgReceiveState::OnProcess(CClient *pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nEventListIndex == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nEventListIndex = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
			ScriptVector_PushVar(m_scriptParm, (__int64)nEventListIndex);
		}
		else
		{
			return false;
		}
	}

	if (nStateID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nStateID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
			ScriptVector_PushVar(m_scriptParm, nStateID);
		}
		else
		{
			return false;
		}
	}

	if (nScriptFunNameLen == -1)
	{
		if (pClient->GetData(vOut, 4))
		{
			nPos = 0;
			nScriptFunNameLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	if (nScriptFunNameLen > 0 && strScriptFunName.empty())
	{
		if (pClient->GetData(vOut, nScriptFunNameLen))
		{
			vOut.push_back('\0');
			strScriptFunName = (const char*)&vOut[0];
			ScriptVector_PushVar(m_scriptParm, strScriptFunName.c_str());
		}
		else
		{
			return false;
		}
	}

	if (nScriptParmNum == -1)
	{
		if (pClient->GetData(vOut, 1))
		{
			nPos = 0;
			nScriptParmNum = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}

	while (m_scriptParm.size() < nScriptParmNum)
	{
		if (nCurParmType == -1)
		{
			nPos = 0;
			if (pClient->GetData(vOut, 1))
			{
				nCurParmType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
			}
			else
			{
				return false;
			}
		}

		switch (nCurParmType)
		{
		case EScriptVal_Int:
			{
				nPos = 0;
				if (pClient->GetData(vOut, 8))
				{
					__int64 nVal = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
					ScriptVector_PushVar(m_scriptParm, nVal);
				}
				else
				{
					return false;
				}
			}
			break;
		case EScriptVal_Double:
			{
				nPos = 0;
				if (pClient->GetData(vOut, 8))
				{
					double fVal = DecodeBytes2Double(&vOut[0], nPos, vOut.size());
					ScriptVector_PushVar(m_scriptParm, fVal);
				}
				else
				{
					return false;
				}
			}
			break;
		case EScriptVal_String:
			{
				nPos = 0;
				if (nStringLen == -1)
				{
					if (pClient->GetData(vOut, 4))
					{
						nStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
					}
					else
					{
						return false;
					}
				}
				if (nStringLen > 0)
				{
					if (pClient->GetData(vOut, nStringLen))
					{
						vOut.push_back('\0');
						ScriptVector_PushVar(m_scriptParm, (const char*)&vOut[0]);
					}
					else
					{
						return false;
					}
				}
				nStringLen = -1;
			}
			break;
		case EScriptVal_ClassPointIndex:
			{
				nPos = 0;
				if (nStringLen == -1)
				{
					if (pClient->GetData(vOut, 4))
					{
						nStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
					}
					else
					{
						return false;
					}
				}
				if (nStringLen > 0 && strClassName.empty())
				{
					if (pClient->GetData(vOut, nStringLen))
					{
						vOut.push_back('\0');
						strClassName = &vOut[0];
					}
					else
					{
						return false;
					}
				}

				if (pClient->GetData(vOut, 8))
				{
					__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());

					int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
					CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
					if (pMgr)
					{
						auto pPoint = pMgr->New(nClassID);
						if (pPoint)
						{
							ScriptVector_PushVar(m_scriptParm, pPoint);
						}
					}
				}
				else
				{
					return false;
				}

				nStringLen = -1;
				strClassName.clear();
			}
			break;
		}
		nCurParmType = -1;
	}
	//TODO 日后做安全性验证，对连接可执行的脚本做限制
	//读取完成，执行结果
	CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_NETWORK_RUNSCRIPT, pClient->CBaseConnector::GetID() , m_scriptParm);

	return true;
}

bool CReturnMsgReceiveState::OnProcess(CClient* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nEventListIndex == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nEventListIndex = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}

	if (nStateID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nStateID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
			ScriptVector_PushVar(m_scriptParm, nStateID);
		}
		else
		{
			return false;
		}
	}

	if (nScriptParmNum == -1)
	{
		if (pClient->GetData(vOut, 1))
		{
			nPos = 0;
			nScriptParmNum = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}

	while (m_scriptParm.size() < nScriptParmNum)
	{
		if (nCurParmType == -1)
		{
			nPos = 0;
			if (pClient->GetData(vOut, 1))
			{
				nCurParmType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
			}
			else
			{
				return false;
			}
		}

		switch (nCurParmType)
		{
		case EScriptVal_Int:
		{
			nPos = 0;
			if (pClient->GetData(vOut, 8))
			{
				__int64 nVal = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
				ScriptVector_PushVar(m_scriptParm, nVal);
			}
			else
			{
				return false;
			}
		}
		break;
		case EScriptVal_Double:
		{
			nPos = 0;
			if (pClient->GetData(vOut, 8))
			{
				double fVal = DecodeBytes2Double(&vOut[0], nPos, vOut.size());
				ScriptVector_PushVar(m_scriptParm, fVal);
			}
			else
			{
				return false;
			}
		}
		break;
		case EScriptVal_String:
		{
			nPos = 0;
			if (nStringLen == -1)
			{
				if (pClient->GetData(vOut, 4))
				{
					nStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
				}
				else
				{
					return false;
				}
			}
			if (nStringLen > 0)
			{
				if (pClient->GetData(vOut, nStringLen))
				{
					vOut.push_back('\0');
					ScriptVector_PushVar(m_scriptParm, (const char*)&vOut[0]);
				}
				else
				{
					return false;
				}
			}
			nStringLen = -1;
		}
		break;
		case EScriptVal_ClassPointIndex:
		{
			nPos = 0;
			if (nStringLen == -1)
			{
				if (pClient->GetData(vOut, 4))
				{
					nStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
				}
				else
				{
					return false;
				}
			}
			if (nStringLen > 0 && strClassName.empty())
			{
				if (pClient->GetData(vOut, nStringLen))
				{
					vOut.push_back('\0');
					strClassName = &vOut[0];
				}
				else
				{
					return false;
				}
			}

			if (pClient->GetData(vOut, 8))
			{
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());

				int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
				CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
				if (pMgr)
				{
					auto pPoint = pMgr->New(nClassID);
					if (pPoint)
					{
						ScriptVector_PushVar(m_scriptParm, pPoint);
					}
				}
			}
			else
			{
				return false;
			}

			nStringLen = -1;
			strClassName.clear();
		}
		break;
		}
		nCurParmType = -1;
	}

	//读取完成，执行结果
	CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RETURN, 0, m_scriptParm, nEventListIndex);

	return true;
}

CClient::CClient()
{
	//m_nCurMsgLen = 0;

	pCurMsgReceive = nullptr;

	RegisterClassFun(RunScript,this, &CClient::RunScript2Script);
}


CClient::~CClient()
{
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
	AddInt2Bytes(m_vBuff, pState->m_pMachine->m_nEventListIndex);

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

			pCurMsgReceive = CreateRceiveState(cType);
		}
	}
	if (pCurMsgReceive)
	{
		if (pCurMsgReceive->OnProcess(this) == true)
		{
			RemoveRceiveState(pCurMsgReceive);
			pCurMsgReceive = nullptr;
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
			if (pPoint)
			{
				AddString2Bytes(vBuff, (char*)pPoint->GetClassName());
				if (pPoint->GetPoint())
				{
					AddInt642Bytes(vBuff, pPoint->GetPoint()->GetID());
				}

			}
		}
		break;
	}
	return true;
}

CBaseMsgReceiveState* CClient::CreateRceiveState(char cType)
{
	switch (cType)
	{
	case E_RUN_SCRIPT:
		return new CScriptMsgReceiveState;
		break;
	}
	return nullptr;
}

void CClient::RemoveRceiveState(CBaseMsgReceiveState* pState)
{
	if (pState)
	{
		delete pState;
	}
}

void CClient::EventReturnFun(int nSendID, CScriptStack& ParmInfo)
{

	m_vBuff.clear();

	AddChar2Bytes(m_vBuff, E_RUN_SCRIPT_RETURN);

	while (ParmInfo.size() > 0)
	{
		AddVar2Bytes(m_vBuff, &ParmInfo.top());
		ParmInfo.pop();
	}
	SendData(&m_vBuff[0], m_vBuff.size());
}
