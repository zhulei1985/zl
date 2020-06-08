#include "ScriptConnector.h"
#include "MsgReceive.h"
#include "zByteArray.h"

#include "TempScriptRunState.h"

bool CScriptMsgReceiveState::OnProcess(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nEventListIndex == -1)
	{
		if (pClient->GetData(vOut, 4))
		{
			nPos = 0;
			nEventListIndex = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
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
			//前面已经有3个值被压入堆栈
			nScriptParmNum += 3;
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

			if (nStringLen == -1)
			{
				nPos = 0;
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

			if (pClient->GetData(vOut, 9))
			{
				nPos = 0;
				char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());

				int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
				CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
				if (pMgr)
				{
					CScriptPointInterface* pPoint = nullptr;
					if (cType == 0)
					{
						//本地类实例
						pPoint = pMgr->Get(nClassID);
					}
					else
					{
						//镜像类实例
						//获取在本地的类实例索引
						__int64 nImageIndex = pClient->GetImageIndex(nClassID);

						pPoint = pMgr->Get(nImageIndex);

					}


					ScriptVector_PushVar(m_scriptParm, pPoint);
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
	//对连接可执行的脚本做限制
	if (pClient->CheckScriptLimit(strScriptFunName))
	{
		ScriptVector_PushVar(m_scriptParm, pClient);
		//读取完成，执行结果
		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_NETWORK_RUNSCRIPT, pClient->GetScriptEventIndex(), m_scriptParm);
	}
	else
	{
		CTempScriptRunState tempState;
		tempState.PushVarToStack(strScriptFunName.c_str());
		tempState.PushVarToStack("Error_CannotRunScript");
		tempState.PushVarToStack(0);
		pClient->RunScript2Script(&tempState);
	}

	return true;
}

bool CReturnMsgReceiveState::OnProcess(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nEventListIndex == -1)
	{
		if (pClient->GetData(vOut, 4))
		{
			nPos = 0;
			nEventListIndex = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
			//ScriptVector_PushVar(m_scriptParm, (__int64)nEventListIndex);
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
			//前面已经有1个值被压入堆栈
			nScriptParmNum += 1;
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
			if (nStringLen == -1)
			{
				if (pClient->GetData(vOut, 4))
				{
					nPos = 0;
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

			if (pClient->GetData(vOut, 9))
			{
				nPos = 0;
				char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());

				int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
				CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
				if (pMgr)
				{
					CScriptPointInterface* pPoint = nullptr;
					if (cType == 0)
					{
						//本地类实例
						pPoint = pMgr->Get(nClassID);
					}
					else
					{
						//镜像类实例
						//获取在本地的类实例索引
						__int64 nImageIndex = pClient->GetImageIndex(nClassID);
						pPoint = pMgr->Get(nImageIndex);
					}


					ScriptVector_PushVar(m_scriptParm, pPoint);
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


bool CSyncClassDataMsgReceiveState::OnProcess(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nClassID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}

	if (nClassNameStringLen == -1)
	{
		if (pClient->GetData(vOut, 4))
		{
			nPos = 0;
			nClassNameStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	if (nClassNameStringLen > 0 && strClassName.empty())
	{
		if (pClient->GetData(vOut, nClassNameStringLen))
		{
			vOut.push_back('\0');
			strClassName = (const char*)&vOut[0];
		}
		else
		{
			return false;
		}
	}
	if (nDataLen == -1)
	{
		if (pClient->GetData(vOut, 4))
		{
			nPos = 0;
			nDataLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}

	if (nDataLen > 0)
	{
		if (pClient->GetData(vOut, nDataLen))
		{
			if (m_pPoint == nullptr)
			{
				int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
				CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
				if (pMgr)
				{
					CScriptPointInterface* pPoint = nullptr;
					__int64 nImageIndex = pClient->GetImageIndex(nClassID);
					if (nImageIndex != 0)
					{
						pPoint = pMgr->Get(nImageIndex);
					}
					if (pPoint == nullptr)
					{
						pPoint = pMgr->New();

						pClient->SetImageIndex(nClassID, pPoint->GetScriptPointIndex());
					}

					m_pPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint);
					if (m_pPoint)
					{
						m_pPoint->SetProcessID(pClient->GetID());
					}
					else
					{
						//失败，不是同步类
						pMgr->Release(pPoint);
						return true;
					}
				}
			}
			nPos = 0;
			m_pPoint->DecodeData4Bytes(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CSyncUpMsgReceiveState::OnProcess(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nClassID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}

	if (nFunNameStringLen == -1)
	{
		if (pClient->GetData(vOut, 4))
		{
			nPos = 0;
			nFunNameStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	if (nFunNameStringLen > 0 && strFunName.empty())
	{
		if (pClient->GetData(vOut, nFunNameStringLen))
		{
			vOut.push_back('\0');
			strFunName = (const char*)&vOut[0];
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

			if (nStringLen == -1)
			{
				nPos = 0;
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

			if (pClient->GetData(vOut, 9))
			{
				nPos = 0;
				char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());

				int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
				CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
				if (pMgr)
				{
					CScriptPointInterface* pPoint = nullptr;
					if (cType == 0)
					{
						//本地类实例
						pPoint = pMgr->Get(nClassID);
					}
					else
					{
						//镜像类实例
						//获取在本地的类实例索引
						__int64 nImageIndex = pClient->GetImageIndex(nClassID);

						pPoint = pMgr->Get(nImageIndex);

					}


					ScriptVector_PushVar(m_scriptParm, pPoint);
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
	//将参数放入临时状态中
	CTempScriptRunState TempState;
	TempState.CopyFromStack(&m_scriptParm);
	//下传过来的数据，说明接收方只是镜像
	__int64 nImageIndex = pClient->GetImageIndex(nClassID);
	if (nImageIndex)
	{
		auto pPoint = CScriptSuperPointerMgr::GetInstance()->PickupPointer(nImageIndex);
		if (pPoint)
		{
			pPoint->Lock();
			auto pMaster = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
			if (pMaster)
			{
				pMaster->SyncUpRunFun(pPoint->GetType(), strFunName, &TempState);
			}
			pPoint->Unlock();
		}
		CScriptSuperPointerMgr::GetInstance()->ReturnPointer(pPoint);
	}
	return true;
}

bool CSyncDownMsgReceiveState::OnProcess(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nClassID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}

	if (nFunNameStringLen == -1)
	{
		if (pClient->GetData(vOut, 4))
		{
			nPos = 0;
			nFunNameStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	if (nFunNameStringLen > 0 && strFunName.empty())
	{
		if (pClient->GetData(vOut, nFunNameStringLen))
		{
			vOut.push_back('\0');
			strFunName = (const char*)&vOut[0];
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

			if (nStringLen == -1)
			{
				nPos = 0;
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

			if (pClient->GetData(vOut, 9))
			{
				nPos = 0;
				char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());

				int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
				CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
				if (pMgr)
				{
					CScriptPointInterface* pPoint = nullptr;
					if (cType == 0)
					{
						//本地类实例
						pPoint = pMgr->Get(nClassID);
					}
					else
					{
						//镜像类实例
						//获取在本地的类实例索引
						__int64 nImageIndex = pClient->GetImageIndex(nClassID);

						pPoint = pMgr->Get(nImageIndex);

					}


					ScriptVector_PushVar(m_scriptParm, pPoint);
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
	//将参数放入临时状态中
	CTempScriptRunState TempState;
	TempState.CopyFromStack(&m_scriptParm);
	//下传过来的数据，说明接收方只是镜像
	__int64 nImageIndex = pClient->GetImageIndex(nClassID);
	if (nImageIndex)
	{
		auto pPoint = CScriptSuperPointerMgr::GetInstance()->PickupPointer(nImageIndex);
		if (pPoint)
		{
			pPoint->Lock();
			auto pMaster = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
			if (pMaster)
			{
				pMaster->SyncDownRunFun(pPoint->GetType(),strFunName,&TempState);
			}
			pPoint->Unlock();
		}
		CScriptSuperPointerMgr::GetInstance()->ReturnPointer(pPoint);
	}
	return true;
}


CMsgReceiveMgr CMsgReceiveMgr::s_Instance;
CBaseMsgReceiveState* CMsgReceiveMgr::CreateRceiveState(char cType)
{
	switch (cType)
	{
	case E_RUN_SCRIPT:
		return new CScriptMsgReceiveState;
		break;
	case E_RUN_SCRIPT_RETURN:
		return new CReturnMsgReceiveState;
		break;
	case E_SYNC_CLASS_DATA:
		return new CSyncClassDataMsgReceiveState;
		break;
	case E_SYNC_DOWN_PASSAGE:
		return new CSyncUpMsgReceiveState;
		break;
	case E_SYNC_UP_PASSAGE:
		return new CSyncDownMsgReceiveState;
		break;
	}
	return nullptr;
}

void CMsgReceiveMgr::RemoveRceiveState(CBaseMsgReceiveState* pState)
{
	if (pState)
	{
		delete pState;
	}
}