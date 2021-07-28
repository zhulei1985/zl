#include "ScriptConnector.h"
#include "MsgReceive.h"
#include "zByteArray.h"
#include "TempScriptRunState.h"
#include "RouteEvent.h"
#include "ScriptConnectMgr.h"

CBaseMsgReceiveState::~CBaseMsgReceiveState()
{
	Clear();
}
bool CBaseMsgReceiveState::Send(CBaseScriptConnector* pClient)
{
	std::vector<char> m_vBuff;
	AddAllData2Bytes(pClient, m_vBuff);

	return pClient->SendMsg(&m_vBuff[0], m_vBuff.size());
	//return true;
}
bool CBaseMsgReceiveState::AddVar2Bytes(CBaseScriptConnector* pClient,std::vector<char>& vBuff, StackVarInfo* pVal)
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
	case EScriptVal_Binary:
	{
		AddChar2Bytes(vBuff, EScriptVal_Binary);
		unsigned int size = 0;
		const char* pStr = StackVarInfo::s_binPool.GetBinary(pVal->Int64, size);
		::AddData2Bytes(vBuff, pStr, size);
	}
	case EScriptVal_ClassPoint:
	{
		pClient->AddVar2Bytes(vBuff, pVal->pPoint);
	}
	break;
	default:
		AddChar2Bytes(vBuff, EScriptVal_None);
		break;
	}
	return true;
}
bool CBaseMsgReceiveState::DecodeVar4Bytes(CBaseScriptConnector* pClient, StackVarInfo& val)
{
	int nCurParmType = 0;
	std::vector<char> vOut;
	int nPos = 0;
	if (m_GetData(vOut, 1))
	{
		nCurParmType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	switch (nCurParmType)
	{
	case EScriptVal_None:
		{
			val.Clear();
		}
		break;
	case EScriptVal_Int:
		{
			nPos = 0;
			if (m_GetData(vOut, 8))
			{
				val = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
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
			if (m_GetData(vOut, 8))
			{
				val = DecodeBytes2Double(&vOut[0], nPos, vOut.size());
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
			int nStringLen = 0;

			if (m_GetData(vOut, 4))
			{
				nStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
			}
			else
			{
				return false;
			}

			if (nStringLen > 0)
			{
				if (m_GetData(vOut, nStringLen))
				{
					vOut.push_back('\0');
					val = (const char*)&vOut[0];
				}
				else
				{
					return false;
				}
			}
		}
		break;
	case EScriptVal_Binary:
		{
			nPos = 0;
			int nStringLen = 0;

			if (m_GetData(vOut, 4))
			{
				nStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
			}
			else
			{
				return false;
			}

			if (nStringLen > 0)
			{
				if (m_GetData(vOut, nStringLen))
				{
					val.Clear();
					val.cType = EScriptVal_Binary;
					val.Int64 = StackVarInfo::s_binPool.NewBinary((const char*)&vOut[0], nStringLen);
				}
				else
				{
					return false;
				}
			}
		}
		break;
	case EScriptVal_ClassPoint:
		{
			nPos = 0;
			unsigned int nIndex = 0;

			if (m_GetData(vOut, 9))
			{
				nPos = 0;
				char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());

				PointVarInfo pointVar;
				if (cType == 0)
				{
					//本地类实例
					pointVar = nClassID;
				}
				else if (cType == 1)
				{
					//镜像类实例
					//获取在本地的类实例索引
					__int64 nIndex = pClient->GetIndex4Image(nClassID);

					pointVar = nIndex;

				}
				else if (cType == 2)
				{
				}
			}
		}
		break;
	}
	return false;
}
void CBaseMsgReceiveState::SetGetDataFun(std::function<bool(std::vector<char>&, unsigned int)> fun)
{
	m_GetData = std::move(fun);
}
bool CScriptMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;

	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nReturnID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nScriptFunNameLen = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nScriptFunNameLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nScriptFunNameLen > 0)
	{
		if (m_GetData(vOut, nScriptFunNameLen))
		{
			vOut.push_back('\0');
			strScriptFunName = (const char*)&vOut[0];
		}
		else
		{
			return false;
		}
	}

	int nScriptParmNum = 0;

	if (m_GetData(vOut, 1))
	{
		nPos = 0;
		nScriptParmNum = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nClassPointSize = 0;
	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nClassPointSize = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}
	for (int i = 0; i < nClassPointSize; i++)
	{
		
	}

	while (m_scriptParm.size() < nScriptParmNum)
	{
		StackVarInfo var;
		if (DecodeVar4Bytes(pClient,var) == false)
		{
			return false;
		}
		m_scriptParm.push(var);
	}

	return true;
}

bool CScriptMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	//对连接可执行的脚本做限制
	printf("远程调用函数:%s\n", strScriptFunName.c_str());
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

	vClassPoint.clear();
	tagByteArray vDataBuff;

	for (int i = 0; i < m_scriptParm.size(); i++)
	{
		AddVar2Bytes(pClient, vDataBuff, m_scriptParm.GetVal(i));
	}
	AddInt2Bytes(vBuff, vClassPoint.size());
	for (unsigned int i = 0; i < vClassPoint.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, vClassPoint[i].pPoint);
	}
	AddData2Bytes(vBuff, vDataBuff);
	return true;
}

bool CReturnMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;

	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nReturnID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		//ScriptVector_PushVar(m_scriptParm, nReturnID);
	}
	else
	{
		return false;
	}

	int nScriptParmNum = 0;

	if (m_GetData(vOut, 1))
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


	while (m_scriptParm.size() < nScriptParmNum)
	{
		StackVarInfo var;
		if (DecodeVar4Bytes(pClient, var) == false)
		{
			return false;
		}
		m_scriptParm.push(var);
	}

	return true;
}

bool CReturnMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	//读取完成，执行结果
	if (pClient)
	{
		pClient->ResultTo(m_scriptParm, nReturnID, 0);
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
		AddVar2Bytes(pClient, vBuff, m_scriptParm.GetVal(i));
	}

	return true;
}


bool CSyncClassInfoMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	__int64 nClassID = 0;
	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nClassNameStringLen = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nClassNameStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nClassNameStringLen > 0)
	{
		if (m_GetData(vOut, nClassNameStringLen))
		{
			vOut.push_back('\0');
			strClassName = (const char*)&vOut[0];
		}
		else
		{
			return false;
		}
	}

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nRootServerID = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nRootClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nTier = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nClassPointNum = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nClassPointNum = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}
	vClassPoint.clear();
	while (vClassPoint.size() < nClassPointNum)
	{
		int nCurParmType = 0;

		nPos = 0;
		if (m_GetData(vOut, 1))
		{
			nCurParmType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}

		switch (nCurParmType)
		{
		case EScriptVal_ClassPoint:
		{
			if (m_GetData(vOut, 9))
			{
				nPos = 0;
				char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
				PointVarInfo pointVar;
				if (cType == 0)
				{
					//本地类实例
					pointVar = nClassID;
				}
				else if (cType == 1)
				{
					//镜像类实例
					//获取在本地的类实例索引
					__int64 nIndex = pClient->GetIndex4Image(nClassID);
					pointVar = nIndex;
				}
				else if (cType == 2)
				{
					pointVar = pClient->GetNoSyncImage4Index(nClassID);
				}
				vClassPoint.push_back(pointVar);
			}
			else
				return false;

		}
		break;
		default:
			return false;
		}
	}

	int nDataLen = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nDataLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nDataLen > 0)
	{
		if (m_GetData(vOut, nDataLen))
		{
			if (m_pPoint == nullptr)
			{
				int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
				CBaseScriptClassMgr* pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
				if (pMgr)
				{
					CScriptPointInterface* pPoint = nullptr;
					__int64 nIndex = pClient->GetIndex4Image(nClassID);//先检查是不是本链接的镜像类
					if (nIndex != 0)
					{
						pPoint = pMgr->Get(nIndex);
					}
					if (pPoint == nullptr)
					{
						//再检查其他连接上有没有这个镜像类
						nIndex = CScriptConnectMgr::GetInstance()->GetSyncIndex(nRootServerID, nRootClassID);
						if (nIndex != 0)
						{
							pPoint = pMgr->Get(nIndex);
						}

						//创建新的镜像类
						if (pPoint == nullptr)
						{
							pPoint = pMgr->New(SCRIPT_NO_DOWN_SYNC_AUTO_RELEASE | SCRIPT_NO_USED_AUTO_RELEASE);
							m_pPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint);
							if (m_pPoint)
							{
								m_pPoint->AddUpSyncProcess(pClient->GetEventIndex(), nTier);
								m_pPoint->SetRootServerID(nRootServerID);
								m_pPoint->SetRootClassID(nRootClassID);
								m_pPoint->SetImageTier(nTier);
							}
							else
							{
								//失败，不是同步类
								pMgr->Release(pPoint);
								return true;
							}

							CScriptConnectMgr::GetInstance()->SetSyncIndex(nRootServerID, nRootClassID, pPoint->GetScriptPointIndex());
							pClient->SetImageAndIndex(nClassID, pPoint->GetScriptPointIndex());
						}
						else
						{
							//通过其他连接已经获得了这个镜像类
							m_pPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint);
							if (m_pPoint)
							{
								m_pPoint->AddUpSyncProcess(pClient->GetEventIndex(), nTier);
							}
							else
							{
								//失败，不是同步类
								pMgr->Release(pPoint);
								return true;
							}
						}
					}
					else
					{
						m_pPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint);
						if (m_pPoint == nullptr)
						{
							//失败，不是同步类
							pMgr->Release(pPoint);
							return true;
						}
					}
				}
			}
			nPos = 0;
			m_pPoint->DecodeData4Bytes(&vOut[0], nPos, vOut.size(), vClassPoint);
			//m_pPoint->SyncDownClassData(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CSyncClassInfoMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	return true;
}

bool CSyncClassInfoMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	if (m_pPoint == nullptr)
	{
		return false;
	}

	AddChar2Bytes(vBuff, E_SYNC_CLASS_INFO);
	AddInt642Bytes(vBuff, m_pPoint->GetScriptPointIndex());
	AddString2Bytes(vBuff, (char*)strClassName.c_str());
	AddInt2Bytes(vBuff, m_pPoint->GetRootServerID());
	if (m_pPoint->GetImageTier() == 0)
	{
		AddInt642Bytes(vBuff, m_pPoint->GetScriptPointIndex());
	}
	else
	{
		AddInt642Bytes(vBuff, m_pPoint->GetRootClassID());
	}
	AddInt2Bytes(vBuff, m_pPoint->GetImageTier() + 1);
	tagByteArray vDataBuff;
	m_pPoint->AddAllData2Bytes(vDataBuff, vClassPoint);
	AddInt2Bytes(vBuff, vClassPoint.size());
	for (unsigned int i = 0; i < vClassPoint.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, vClassPoint[i].pPoint);
	}
	AddData2Bytes(vBuff, vDataBuff);

	return true;
}
bool CNoSyncClassInfoMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	__int64 nClassID = 0;
	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nClassNameStringLen = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nClassNameStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nClassNameStringLen > 0)
	{
		if (m_GetData(vOut, nClassNameStringLen))
		{
			vOut.push_back('\0');
			strClassName = (const char*)&vOut[0];
		}
		else
		{
			return false;
		}
	}

	int nClassPointNum = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nClassPointNum = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}
	vClassPoint.clear();
	while (vClassPoint.size() < nClassPointNum)
	{
		int nCurParmType = 0;

		nPos = 0;
		if (m_GetData(vOut, 1))
		{
			nCurParmType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}

		switch (nCurParmType)
		{
		case EScriptVal_ClassPoint:
		{
			if (m_GetData(vOut, 9))
			{
				nPos = 0;
				char cType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
				__int64 nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
				PointVarInfo pointVar;
				if (cType == 0)
				{
					//本地类实例
					pointVar = nClassID;
				}
				else if (cType == 1)
				{
					//镜像类实例
					//获取在本地的类实例索引
					__int64 nIndex = pClient->GetIndex4Image(nClassID);
					pointVar = nIndex;
				}
				else if (cType == 2)
				{
					pointVar = pClient->GetNoSyncImage4Index(nClassID);
				}
				vClassPoint.push_back(pointVar);
			}
			else
				return false;

		}
		break;
		default:
			return false;
		}
	}

	int nDataLen = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nDataLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nDataLen > 0)
	{
		if (m_GetData(vOut, nDataLen))
		{
			PointVarInfo pointVar = pClient->MakeNoSyncImage(strClassName,nClassID);
			if (pointVar.pPoint && pointVar.pPoint->GetPoint())
			{
				pointVar.pPoint->Lock();
				nPos = 0;
				pointVar.pPoint->GetPoint()->DecodeData4Bytes(&vOut[0], nPos, nDataLen, vClassPoint);
				pointVar.pPoint->Unlock();

			}
		}
		else
		{
			return false;
		}
	}
	return true;
}
bool CNoSyncClassInfoMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	return false;
}
bool CNoSyncClassInfoMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	if (m_pPoint == nullptr)
	{
		return false;
	}

	AddChar2Bytes(vBuff, E_NO_SYNC_CLASS_INFO);
	AddInt642Bytes(vBuff, m_pPoint->GetScriptPointIndex());
	AddString2Bytes(vBuff, (char*)strClassName.c_str());

	tagByteArray vDataBuff;
	m_pPoint->AddAllData2Bytes(vDataBuff, vClassPoint);
	AddInt2Bytes(vBuff, vClassPoint.size());
	for (unsigned int i = 0; i < vClassPoint.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, vClassPoint[i].pPoint);
	}
	AddData2Bytes(vBuff, vDataBuff);

	return true;
}
bool CSyncClassDataReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;

	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nClassPointNum = 0;

	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nClassPointNum = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}
	vClassPoint.clear();
	while (vClassPoint.size() < nClassPointNum)
	{
		StackVarInfo var;
		if (DecodeVar4Bytes(pClient, var) == false)
		{
			return false;
		}
		if (var.cType == EScriptVal_ClassPoint)
			vClassPoint.push_back(var.pPoint);
		else
			return false;
	}

	int nDataLen = 0;
	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nDataLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nDataLen > 0)
	{
		if (m_GetData(vData, nDataLen))
		{
			return true;
		}
	}
	else if (nDataLen == 0)
	{
		return true;
	}


	return false;
}

bool CSyncClassDataReceiveState::Run(CBaseScriptConnector* pClient)
{
	bool bResult = false;
	__int64 nIndex = pClient->GetIndex4Image(nClassID);

	PointVarInfo pointVar = nIndex;
	CScriptBasePointer* pPoint = pointVar.pPoint;
	if (pPoint)
	{
		pPoint->Lock();
		CSyncScriptPointInterface* pSyncPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
		if (pSyncPoint)
		{
			int pos = 0;

			pSyncPoint->SyncDownClassData(&vData[0], pos, vData.size(), vClassPoint);
			bResult = true;
		}
		pPoint->Unlock();
	}
	return bResult;
	return false;
}

bool CSyncClassDataReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_SYNC_CLASS_DATA);
	AddInt642Bytes(vBuff, nClassID);

	AddInt2Bytes(vBuff, vClassPoint.size());
	for (unsigned int i = 0; i < vClassPoint.size(); i++)
	{
		pClient->AddVar2Bytes(vBuff, vClassPoint[i].pPoint);
	}
	//AddString2Bytes(vBuff, (char*)strClassName.c_str());
	AddData2Bytes(vBuff, vData);

	return true;
}


bool CSyncUpMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;

	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nFunNameStringLen = 0;
	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nFunNameStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nFunNameStringLen > 0)
	{
		if (m_GetData(vOut, nFunNameStringLen))
		{
			vOut.push_back('\0');
			strFunName = (const char*)&vOut[0];
		}
		else
		{
			return false;
		}
	}

	int nRouteNum = 0;
	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nRouteNum = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}
	m_listRoute.clear();

	while (m_listRoute.size() < nRouteNum)
	{
		nPos = 0;
		if (m_GetData(vOut, 8))
		{
			__int64 nVal = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
			m_listRoute.push_back(nVal);
		}
		else
		{
			return false;
		}
	}

	int nScriptParmNum = 0;
	if (m_GetData(vOut, 1))
	{
		nPos = 0;
		nScriptParmNum = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	while (m_scriptParm.size() < nScriptParmNum)
	{
		StackVarInfo var;
		if (DecodeVar4Bytes(pClient, var) == false)
		{
			return false;
		}
		m_scriptParm.push(var);
	}

	return true;
}

bool CSyncUpMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	//将参数放入临时状态中
	CTempScriptRunState TempState;
	CACHE_NEW(CScriptCallState, pCallState, &TempState);
	if (pCallState)
	{
		for (unsigned int i = 0; i < m_scriptParm.size(); i++)
		{
			pCallState->PushVarToStack(*m_scriptParm.GetVal(i));
		}

		//下传过来的数据，说明接收方只是镜像
		__int64 nIndex = nClassID;// pClient->GetIndex4Image(nClassID);

		PointVarInfo pointVar = nIndex;
		auto pPoint = pointVar.pPoint;
		if (pPoint)
		{
			pPoint->Lock();
			auto pMaster = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
			if (pMaster)
			{
				m_listRoute.push_back(pClient->GetEventIndex());
				pMaster->SyncUpRunFun(pPoint->GetType(), strFunName, pCallState, m_listRoute);
			}
			pPoint->Unlock();
		}
	}
	CACHE_DELETE(pCallState);
	return true;
}

bool CSyncUpMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_SYNC_UP_PASSAGE);
	AddInt642Bytes(vBuff, pClient->GetImage4Index(nClassID));
	AddString2Bytes(vBuff, (char*)strFunName.c_str());

	AddInt2Bytes(vBuff, m_listRoute.size());
	for (auto it = m_listRoute.begin(); it != m_listRoute.end(); it++)
	{
		AddInt642Bytes(vBuff, *it);
	}

	AddChar2Bytes(vBuff, (char)m_scriptParm.size());

	for (int i = 0; i < m_scriptParm.size(); i++)
	{
		AddVar2Bytes(pClient,vBuff, m_scriptParm.GetVal(i));
	}
	return true;
}

bool CSyncDownMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (m_GetData(vOut, 8))
	{
		nPos = 0;
		nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	int nFunNameStringLen = 0;
	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nFunNameStringLen = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}

	if (nFunNameStringLen > 0)
	{
		if (m_GetData(vOut, nFunNameStringLen))
		{
			vOut.push_back('\0');
			strFunName = (const char*)&vOut[0];
		}
		else
		{
			return false;
		}
	}

	int nScriptParmNum = 0;
	if (m_GetData(vOut, 1))
	{
		nPos = 0;
		nScriptParmNum = DecodeBytes2Char(&vOut[0], nPos, vOut.size());

	}
	else
	{
		return false;
	}

	while (m_scriptParm.size() < nScriptParmNum)
	{
		StackVarInfo var;
		if (DecodeVar4Bytes(pClient, var) == false)
		{
			return false;
		}
		m_scriptParm.push(var);
	}
	return true;
}

bool CSyncDownMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	//将参数放入临时状态中
	CTempScriptRunState TempState;
	CACHE_NEW(CScriptCallState, pCallState, &TempState);
	if (pCallState)
	{
		for (unsigned int i = 0; i < m_scriptParm.size(); i++)
		{
			pCallState->PushVarToStack(*m_scriptParm.GetVal(i));
		}
		//下传过来的数据，说明接收方只是镜像
		__int64 nIndex = pClient->GetIndex4Image(nClassID);

		PointVarInfo pointVar = nIndex;
		auto pPoint = pointVar.pPoint;
		if (pPoint)
		{
			pPoint->Lock();
			auto pMaster = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
			if (pMaster)
			{
				std::list<__int64> listRoute;
				pMaster->SyncDownRunFun(pPoint->GetType(), strFunName, pCallState, listRoute);
			}
			pPoint->Unlock();
		}
	}
	CACHE_DELETE(pCallState);
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
		AddVar2Bytes(pClient, vBuff, m_scriptParm.GetVal(i));
	}
	return true;
}

CSyncFunReturnMsgReceiveState::CSyncFunReturnMsgReceiveState()
{
	//nRouteNum = -1;
	//nScriptParmNum = -1;
	//nCurParmType = -1;
	//nStringLen = -1;
}

CSyncFunReturnMsgReceiveState::~CSyncFunReturnMsgReceiveState()
{
}

bool CSyncFunReturnMsgReceiveState::Recv(CScriptConnector* pClient)
{
	std::vector<char> vOut;
	int nPos = 0;

	int nRouteNum = 0;
	if (m_GetData(vOut, 4))
	{
		nPos = 0;
		nRouteNum = DecodeBytes2Int(&vOut[0], nPos, vOut.size());
	}
	else
	{
		return false;
	}
	m_listRoute.clear();

	while (m_listRoute.size() < nRouteNum)
	{
		nPos = 0;
		if (m_GetData(vOut, 8))
		{
			__int64 nVal = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
			m_listRoute.push_back(nVal);
		}
	}
	int nScriptParmNum = 0;
	if (m_GetData(vOut, 1))
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

	while (m_scriptParm.size() < nScriptParmNum)
	{
		StackVarInfo var;
		if (DecodeVar4Bytes(pClient, var) == false)
		{
			return false;
		}
		m_scriptParm.push(var);
	}

	return true;
}

bool CSyncFunReturnMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	if (pClient)
	{
		if (m_listRoute.size() == 1)
		{
			__int64 nReturnID = m_listRoute.front();
			pClient->ResultTo(m_scriptParm, nReturnID, 0);
		}
		else if (m_listRoute.size() > 1)
		{
			__int64 nEventIndex = m_listRoute.back();
			//发送同步返回消息
			CScriptStack returnStack;

			//TODO 压入剩余路径
			m_listRoute.pop_back();
			ScriptVector_PushVar(returnStack, (__int64)m_listRoute.size());
			for (auto it = m_listRoute.begin(); it != m_listRoute.end(); it++)
			{
				ScriptVector_PushVar(returnStack, *it);
			}
			//TODO 压入返回值
			for (unsigned int i = 0; i < m_scriptParm.size(); i++)
				ScriptVector_PushVar(returnStack, m_scriptParm.GetVal(i));
			if (CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RETURN_SYNC_FUN, 0, returnStack, nEventIndex))
			{
			}
		}
	}
	return false;
}

bool CSyncFunReturnMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_SYNC_FUN_RETURN);
	//AddInt642Bytes(vBuff, nEventListIndex);
	AddInt2Bytes(vBuff, m_listRoute.size());
	for (auto it = m_listRoute.begin(); it != m_listRoute.end(); it++)
	{
		AddInt642Bytes(vBuff, *it);
	}
	AddChar2Bytes(vBuff, (char)m_scriptParm.size());
	for (int i = 0; i < m_scriptParm.size(); i++)
	{
		AddVar2Bytes(pClient, vBuff, m_scriptParm.GetVal(i));
	}

	return true;
}


bool CSyncDownRemoveMsgReceiveState::Recv(CScriptConnector*)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nClassID == -1)
	{
		if (m_GetData(vOut, 8))
		{
			nPos = 0;
			nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CSyncDownRemoveMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	if (pClient)
	{
		__int64 nIndex = pClient->GetIndex4Image(nClassID);
		PointVarInfo pointVar = nIndex;
		auto pPoint = pointVar.pPoint;
		if (pPoint)
		{
			CSyncScriptPointInterface* pSyncPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
			if (pSyncPoint)
			{
				//发送者的down对应接收者的up
				pSyncPoint->RemoveUpSyncProcess(pClient->GetEventIndex());
			}
		}
		return true;
	}
	return false;
}

bool CSyncDownRemoveMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_SYNC_DOWN_REMOVE);
	AddInt642Bytes(vBuff, nClassID);
	return true;
}

bool CSyncUpRemoveMsgReceiveState::Recv(CScriptConnector*)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nClassID == -1)
	{
		if (m_GetData(vOut, 8))
		{
			nPos = 0;
			nClassID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	return true;
}
bool CSyncUpRemoveMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	if (pClient)
	{
		PointVarInfo pointVar = nClassID;
		auto pPoint = pointVar.pPoint;
		if (pPoint)
		{
			CSyncScriptPointInterface* pSyncPoint = dynamic_cast<CSyncScriptPointInterface*>(pPoint->GetPoint());
			if (pSyncPoint)
			{
				//发送者的up对应接收者的down
				pSyncPoint->RemoveDownSyncProcess(pClient->GetEventIndex());
			}
		}
		return true;
	}
	return false;
}

bool CSyncUpRemoveMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_SYNC_DOWN_REMOVE);
	AddInt642Bytes(vBuff, pClient->GetImage4Index(nClassID));
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
		if (m_GetData(vOut, 8))
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
		if (m_GetData(vOut, 1))
		{
			nPos = 0;
			nMsgType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
			pState = CMsgReceiveMgr::GetInstance()->CreateRceiveState(nMsgType);
			pState->SetGetDataFun(m_GetData);
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
		if (m_GetData(vOut, 8))
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
		if (m_GetData(vOut, 1))
		{
			nPos = 0;
			nMsgType = DecodeBytes2Char(&vOut[0], nPos, vOut.size());
			pState = CMsgReceiveMgr::GetInstance()->CreateRceiveState(nMsgType);
			pState->SetGetDataFun(m_GetData);
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

void CRouteChangeMsgReceiveState::Clear()
{
	nOldConnectID = -1;
	nNewConnectID = -1;
}

bool CRouteChangeMsgReceiveState::Recv(CScriptConnector*)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nOldConnectID == -1)
	{
		if (m_GetData(vOut, 8))
		{
			nPos = 0;
			nOldConnectID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	if (nNewConnectID == -1)
	{
		if (m_GetData(vOut, 8))
		{
			nPos = 0;
			nNewConnectID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CRouteChangeMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	pClient->ChangeRoute(nOldConnectID, nNewConnectID);
	return true;
}

bool CRouteChangeMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_ROUTE_CHANGE);
	AddInt642Bytes(vBuff, nOldConnectID);
	AddInt642Bytes(vBuff, nNewConnectID);
	return true;
}

void CRouteRemoveMsgReceiveState::Clear()
{
	nConnectID = -1;
}

bool CRouteRemoveMsgReceiveState::Recv(CScriptConnector*)
{
	std::vector<char> vOut;
	int nPos = 0;
	if (nConnectID == -1)
	{
		if (m_GetData(vOut, 8))
		{
			nPos = 0;
			nConnectID = DecodeBytes2Int64(&vOut[0], nPos, vOut.size());
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CRouteRemoveMsgReceiveState::Run(CBaseScriptConnector* pClient)
{
	pClient->RemoveRoute(nConnectID);
	return true;
}

bool CRouteRemoveMsgReceiveState::AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff)
{
	AddChar2Bytes(vBuff, E_ROUTE_REMOVE);
	AddInt642Bytes(vBuff, nConnectID);
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
	case E_SYNC_CLASS_INFO:
		return new CSyncClassInfoMsgReceiveState;
		break;
	case E_NO_SYNC_CLASS_INFO:
		return new CNoSyncClassInfoMsgReceiveState;
		break;
	case E_SYNC_CLASS_DATA:
		return new CSyncClassDataReceiveState;
		break;
	case E_SYNC_UP_PASSAGE:
		return new CSyncUpMsgReceiveState;
		break;
	case E_SYNC_DOWN_PASSAGE:
		return new CSyncDownMsgReceiveState;
		break;
	case E_ROUTE_FRONT:
		return new CRouteFrontMsgReceiveState;
		break;
	case E_SYNC_FUN_RETURN:
		return new CSyncFunReturnMsgReceiveState;
		break;
	case E_SYNC_DOWN_REMOVE:
		return new CSyncDownRemoveMsgReceiveState;
		break;
	case E_SYNC_UP_REMOVE:
		return new CSyncUpRemoveMsgReceiveState;
		break;
	case E_ROUTE_BACK:
		return new CRouteBackMsgReceiveState;
		break;
	case E_ROUTE_CHANGE:
		return new CRouteChangeMsgReceiveState;
		break;
	case E_ROUTE_REMOVE:
		return new CRouteRemoveMsgReceiveState;
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


