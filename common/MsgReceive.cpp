#include "ScriptConnector.h"
#include "MsgReceive.h"
#include "zByteArray.h"
#include "TempScriptRunState.h"
#include "RouteEvent.h"

CBaseMsgReceiveState::~CBaseMsgReceiveState()
{
	Clear();
}
bool CBaseMsgReceiveState::Send(CBaseScriptConnector* pClient)
{
	std::vector<char> m_vBuff;
	AddAllData2Bytes(pClient,m_vBuff);

	pClient->SendMsg(&m_vBuff[0], m_vBuff.size());
	return true;
}
bool CScriptMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	//if (nEventListIndex == -1)
	//{
	//	if (pClient->GetData(vOut, 8))
	//	{
	//		nPos = 0;
	//		nEventListIndex = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	//		ScriptVector_PushVar(m_scriptParm, (__int64)nEventListIndex);
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	if (nReturnID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nReturnID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
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

	return true;
}

bool CScriptMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	//对连接可执行的脚本做限制
	pClient->RunTo(strScriptFunName, m_scriptParm, nReturnID, 0);

	return true;
}

bool CScriptMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_RUN_SCRIPT);
	//AddInt642Bytes(vBuff, nEventListIndex);
	AddInt642Bytes(vBuff, nReturnID);
	AddString2Bytes(vBuff, (char*)strScriptFunName.c_str());
	AddChar2Bytes(vBuff, (char)m_scriptParm.size());

	for (int i = 0; i < m_scriptParm.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, m_scriptParm.GetVal(i));
	}

	return true;
}

bool CReturnMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	//if (nEventListIndex == -1)
	//{
	//	if (pClient->GetData(vOut, 8))
	//	{
	//		nPos = 0;
	//		nEventListIndex = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	//		//ScriptVector_PushVar(m_scriptParm, (__int64)nEventListIndex);
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	if (nReturnID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nReturnID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
			//ScriptVector_PushVar(m_scriptParm, nReturnID);
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
			//nScriptParmNum += 1;
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

	return true;
}

bool CReturnMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	//读取完成，执行结果
	if (pClient)
	{
		pClient->ResultTo(m_scriptParm, nReturnID,0);
	}
	//CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RETURN, 0, m_scriptParm, nEventListIndex);
	return true;
}

bool CReturnMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_RUN_SCRIPT_RETURN);
	//AddInt642Bytes(vBuff, nEventListIndex);
	AddInt642Bytes(vBuff, nReturnID);
	AddChar2Bytes(vBuff, (char)m_scriptParm.size());

	for (int i = 0; i < m_scriptParm.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, m_scriptParm.GetVal(i));
	}

	return true;
}


bool CSyncClassDataMsgReceiveState::Recv(CScriptConnector* pClient)
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

bool CSyncClassDataMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	return true;
}

bool CSyncClassDataMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	if (m_pPoint == nullptr)
	{
		return false;
	}

	AddChar2Bytes(vBuff, E_SYNC_CLASS_DATA);
	AddInt642Bytes(vBuff, m_pPoint->GetScriptPointIndex());
	AddString2Bytes(vBuff, (char*)strClassName.c_str());

	tagByteArray vDataBuff;
	m_pPoint->AddAllData2Bytes(vDataBuff);
	AddData2Bytes(vBuff, vDataBuff);

	return true;
}

bool CSyncUpMsgReceiveState::Recv(CScriptConnector* pClient)
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

	return true;
}

bool CSyncUpMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
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

bool CSyncUpMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_SYNC_UP_PASSAGE);
	AddInt642Bytes(vBuff, nClassID);
	AddString2Bytes(vBuff, (char*)strFunName.c_str());

	AddChar2Bytes(vBuff, (char)m_scriptParm.size());

	for (int i = 0; i < m_scriptParm.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, m_scriptParm.GetVal(i));
	}
	return true;
}

bool CSyncDownMsgReceiveState::Recv(CScriptConnector* pClient)
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
	return true;
}

bool CSyncDownMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
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
				pMaster->SyncDownRunFun(pPoint->GetType(), strFunName, &TempState);
			}
			pPoint->Unlock();
		}
		CScriptSuperPointerMgr::GetInstance()->ReturnPointer(pPoint);
	}
	return true;
}

bool CSyncDownMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_SYNC_DOWN_PASSAGE);
	AddInt642Bytes(vBuff, nClassID);
	AddString2Bytes(vBuff, (char*)strFunName.c_str());

	AddChar2Bytes(vBuff, (char)m_scriptParm.size());
	for (int i = 0; i < m_scriptParm.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, m_scriptParm.GetVal(i));
	}
	return true;
}

void CRouteFrontMsgReceiveState::Clear()
{
	nConnectID = -1;
	nMsgType = 0;
	CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pState);
	pState = nullptr;
}

bool CRouteFrontMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nConnectID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nConnectID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	if (nMsgType == 0)
	{
		if (pClient->GetData(vOut, 1))
		{
			nPos = 0;
			nMsgType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
			pState = CMsgReceiveMgr::GetInstance()->CreateRceiveState(nMsgType);
		}
		else
		{
			return false;
		}
	}

	if (pState)
	{
		return pState->Recv(pClient);
	}
	return false;
}

bool CRouteFrontMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	if (pClient)
	{
		return pClient->RouteMsg(this);
	}
	return false;
}

bool CRouteFrontMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_ROUTE_FRONT);
	AddInt642Bytes(vBuff, nConnectID);
	if (pState)
	{
		pState->AddAllData2Bytes(pClient, vBuff);
	}
	else
	{
		AddChar2Bytes(vBuff, 0);
	}
	return true;
}

void CRouteBackMsgReceiveState::Clear()
{
	nConnectID = -1;
	CMsgReceiveMgr::GetInstance()->RemoveRceiveState(pState);
	pState = nullptr;
}

bool CRouteBackMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nConnectID == -1)
	{
		if (pClient->GetData(vOut, 8))
		{
			nPos = 0;
			nConnectID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	if (nMsgType == 0)
	{
		if (pClient->GetData(vOut, 1))
		{
			nPos = 0;
			nMsgType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
			pState = CMsgReceiveMgr::GetInstance()->CreateRceiveState(nMsgType);
		}
		else
		{
			return false;
		}
	}

	if (pState)
	{
		return pState->Recv(pClient);
	}
	return false;
}

bool CRouteBackMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	//直接给对应连接转发
	return CRouteEventMgr::GetInstance()->SendEvent(nConnectID, false, pState);
}

bool CRouteBackMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_ROUTE_BACK);
	AddInt642Bytes(vBuff, nConnectID);
	if (pState)
	{
		pState->AddAllData2Bytes(pClient, vBuff);
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
	case E_ROUTE_FRONT:
		return new CRouteFrontMsgReceiveState;
		break;
	case E_ROUTE_BACK:
		return new CRouteBackMsgReceiveState;
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