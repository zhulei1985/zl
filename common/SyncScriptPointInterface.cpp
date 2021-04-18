#include "SyncScriptPointInterface.h"
#include "ScriptConnector.h"
#include "ScriptConnectMgr.h"
#include "zByteArray.h"

namespace zlscript
{
	CSyncScriptPointInterface::CSyncScriptPointInterface()
	{
		m_nProcessID = 0;
		m_nRootServerID = 0;//根节点所在服务器ID
		m_nRootClassID = 0;//本实例在根节点上的ID
		m_nImageTier = 0;//镜像的层数
	}
	CSyncScriptPointInterface::~CSyncScriptPointInterface()
	{
		ClearSyncInfo();

		m_mapSyncAttributes.clear();
	}
	bool CSyncScriptPointInterface::CanRelease()
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		return m_mapDownSyncProcess.empty();
	}
	int CSyncScriptPointInterface::RunFun(int id, CScriptRunState* pState)
	{
		auto itSyncFlag = m_mapSyncFunFlag.find(id);
		if (itSyncFlag != m_mapSyncFunFlag.end())
		{

			if (GetProcessID() > 0)
			{
				std::string funName;
				CScriptStack tempStack;


				//获取函数名
				int nResult = 0;
				//std::lock_guard<std::mutex> _lock{ *m_FunLock };
				//m_FunLock.lock();
				{
					CScriptBaseClassFunInfo* pFunInfo = GetClassFunInfo(id);
					if (pFunInfo)
					{
						funName = pFunInfo->GetFunName();
					}
				}
				//m_FunLock.unlock();
				ScriptVector_PushVar(tempStack, this);
				ScriptVector_PushVar(tempStack, funName.c_str());
				ScriptVector_PushVar(tempStack, (__int64)1);
				ScriptVector_PushVar(tempStack, pState->GetId());
				pState->CopyToStack(&tempStack, pState->GetParamNum());
				//std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
				if (!CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_UP_SYNC_FUN, pState->m_pMachine->GetEventIndex(), tempStack, GetProcessID()))
				{
					pState->ClearFunParam();
					return ECALLBACK_ERROR;
				}
				//CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(GetProcessID());
				//if (pClient)
				//{
				//	pClient->SyncUpClassFunRun(this, funName, tempStack);
				//}
				pState->ClearFunParam();
				return ECALLBACK_WAITING;
			}
			else
			{
				std::string funName;
				CScriptStack parmStack;
				pState->CopyToStack(&parmStack, pState->GetParamNum());

				//先执行本地数据
				int nResult = 0;
				//m_FunLock.lock();
				//auto it = m_mapScriptClassFun.find(id);
				//if (it != m_mapScriptClassFun.cend())
				{
					CScriptBaseClassFunInfo* pFunInfo = GetClassFunInfo(id);
					if (pFunInfo)
					{
						nResult = pFunInfo->RunFun(pState);
						funName = pFunInfo->GetFunName();
					}
				}
				//m_FunLock.unlock();

				if (itSyncFlag->second >= 1)
				{
					CScriptStack tempStack;
					ScriptVector_PushVar(tempStack, this);
					ScriptVector_PushVar(tempStack, funName.c_str());
					for (unsigned int i = 0; i < parmStack.size(); i++)
					{
						tempStack.push(*parmStack.GetVal(i));
					}
					std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
					//然后再同步给子节点
					auto Syncit = m_mapDownSyncProcess.begin();
					for (; Syncit != m_mapDownSyncProcess.end(); )
					{
						if (Syncit->second == false)
						{
							Syncit++;
							continue;
						}
						//std::vector<char> vBuff;
						if (CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_DOWN_SYNC_FUN, pState->m_pMachine->GetEventIndex(), tempStack, Syncit->first))
						{
							Syncit++;
						}
						else
						{
							Syncit = m_mapDownSyncProcess.erase(Syncit);
						}

					}
				}

				else
				{
					m_UpdateSyncAttLock.lock();
					if (!m_setUpdateSyncAttibute.empty())
					{
						//同步属性有更新
						std::vector<char> vBuff;
						AddUpdateData2Bytes(vBuff);
						int pos = 0;
						SyncDownClassData(&vBuff[0], pos, vBuff.size(), m_vecSyncClassPoint);
						//m_setUpdateSyncAttibute.clear();
						ClearUpdateSyncAttibute();
					}
					m_UpdateSyncAttLock.unlock();
				}

				return nResult;
			}

		}


		return CScriptPointInterface::RunFun(id, pState);
	}

	int CSyncScriptPointInterface::SyncUpRunFun(int nClassType, std::string strFun, CScriptRunState* pState, std::list<__int64> &listRoute)
	{
		if (GetProcessID() > 0)//不是根节点，继续往上发送
		{
			CScriptStack tempStack;
			ScriptVector_PushVar(tempStack, this);
			ScriptVector_PushVar(tempStack, strFun.c_str());
			ScriptVector_PushVar(tempStack, (__int64)listRoute.size());
			for (auto it = listRoute.begin(); it != listRoute.end(); it++)
			{
				ScriptVector_PushVar(tempStack, *it);
			}
			pState->CopyToStack(&tempStack, pState->GetParamNum());

			//std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
			if (!CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_UP_SYNC_FUN, 0, tempStack, GetProcessID()))
			{

			}
			//CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(GetProcessID());
			//if (pClient)
			//{
			//	pClient->SyncUpClassFunRun(this->GetScriptPointIndex(), strFun, tempStack);
			//}
			pState->ClearFunParam();

			return ECALLBACK_FINISH;
		}
		return SyncDownRunFun(nClassType, strFun, pState, listRoute);
	}

	int CSyncScriptPointInterface::SyncDownRunFun(int nClassType, std::string strFun, CScriptRunState* pState, std::list<__int64> &listRoute)
	{
		std::string funName;
		CScriptStack parmStack;
		pState->CopyToStack(&parmStack, pState->GetParamNum());

		//先执行本地数据
		int nResult = 0;
		//std::lock_guard<std::mutex> _lock{ *m_FunLock };
		int index = CScriptSuperPointerMgr::GetInstance()->GetClassFunIndex(nClassType, strFun);
		bool bNeedSyncDown = false;
		//m_FunLock.lock();

		//auto it = m_mapScriptClassFun.find(index);
		//if (it != m_mapScriptClassFun.cend())
		{
			CScriptBaseClassFunInfo* pFunInfo = GetClassFunInfo(index);
			if (pFunInfo)
			{
				nResult = pFunInfo->RunFun(pState);
				funName = pFunInfo->GetFunName();
			}
		}

		if (listRoute.size() > 0)
		{
			//发送同步返回消息
			__int64 nEventIndex = listRoute.back();
			listRoute.pop_back();
			CScriptStack returnStack;

			//TODO 压入剩余路径
			ScriptVector_PushVar(returnStack, (__int64)listRoute.size());
			for (auto it = listRoute.begin(); it != listRoute.end(); it++)
			{
				ScriptVector_PushVar(returnStack, *it);
			}
			//TODO 压入返回值
			pState->CopyToStack(&returnStack, 1);
			if (CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_RETURN_SYNC_FUN, 0, returnStack, nEventIndex))
			{
			}
		}

		//m_FunLock.unlock();

		CScriptStack tempStack;
		ScriptVector_PushVar(tempStack, this);
		ScriptVector_PushVar(tempStack, funName.c_str());
		for (unsigned int i = 0; i < parmStack.size(); i++)
		{
			tempStack.push(*parmStack.GetVal(i));
		}
		auto itSyncFlag = m_mapSyncFunFlag.find(index);
		if (itSyncFlag != m_mapSyncFunFlag.end())
		{
			if (itSyncFlag->second >= 1)
			{
				std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
				//然后再同步给子节点
				auto Syncit = m_mapDownSyncProcess.begin();
				for (; Syncit != m_mapDownSyncProcess.end(); )
				{
					if (Syncit->second == false)
					{
						Syncit++;
						continue;
					}

					if (CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_DOWN_SYNC_FUN, 0, tempStack, Syncit->first))
					{
						Syncit++;
					}
					//std::vector<char> vBuff;
					//CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(Syncit->first);
					//if (pClient)
					//{
					//	pClient->SyncDownClassFunRun(this->GetScriptPointIndex(), funName, tempStack);
					//	Syncit++;
					//}
					else
					{
						Syncit = m_mapDownSyncProcess.erase(Syncit);
					}

				}
			}
			else
			{
				m_UpdateSyncAttLock.lock();
				if (m_setUpdateSyncAttibute.size() > 0)
				{
					//同步属性有更新
					std::vector<char> vBuff;
					AddUpdateData2Bytes(vBuff);
					int pos = 0;
					SyncDownClassData(&vBuff[0], pos, vBuff.size(), m_vecSyncClassPoint);
					//m_setUpdateSyncAttibute.clear();
					ClearUpdateSyncAttibute();
				}
				m_UpdateSyncAttLock.unlock();
			}

		}

		return nResult;
	}
	//void CSyncScriptPointInterface::SyncClassData()
	//{
	//	CScriptStack tempStack;
	//	ScriptVector_PushVar(tempStack, this);
	//	std::vector<char> vBuff;
	//	AddAllData2Bytes(vBuff);
	//	ScriptVector_PushVar(tempStack, &vBuff[0], vBuff.size());
	//	if (GetProcessID() > 0)
	//	{
	//		//向上层节点发送同步数据
	//		CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_UP_SYNC_DATA, 0, tempStack, GetProcessID());
	//	}
	//	else
	//	{
	//		//向下层节点发送同步数据
	//		auto Syncit = m_mapDownSyncProcess.begin();
	//		for (; Syncit != m_mapDownSyncProcess.end(); )
	//		{
	//			if (Syncit->second == false)
	//			{
	//				Syncit++;
	//				continue;
	//			}
	//			//std::vector<char> vBuff;
	//			if (CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_DOWN_SYNC_DATA, 0, tempStack, Syncit->first))
	//			{
	//				Syncit++;
	//			}
	//			else
	//			{
	//				Syncit = m_mapDownSyncProcess.erase(Syncit);
	//			}

	//		}
	//	}
	//}
	void CSyncScriptPointInterface::SyncDownClassData(const char* pBuff, int& pos, unsigned int len,std::vector<PointVarInfo>&vClassPoint)
	{
		int startPos = pos;
		//解码数据
		m_vecDecodeSyncClassPoint = vClassPoint;
		DecodeData4Bytes((char*)pBuff, pos, len);
		m_vecDecodeSyncClassPoint.clear();
		//发送同步消息给下层节点
		CScriptStack tempStack;
		ScriptVector_PushVar(tempStack, this);
		ScriptVector_PushVar(tempStack, pBuff + startPos, pos - startPos);
		ScriptVector_PushVar(tempStack, (__int64)vClassPoint.size());
		for (unsigned int i = 0; i < vClassPoint.size(); i++)
		{
			ScriptVector_PushVar(tempStack, vClassPoint[i].pPoint);
		}

		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		auto Syncit = m_mapDownSyncProcess.begin();
		for (; Syncit != m_mapDownSyncProcess.end(); )
		{
			if (Syncit->second == false)
			{
				Syncit++;
				continue;
			}
			//std::vector<char> vBuff;
			if (CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_DOWN_SYNC_DATA, 0, tempStack, Syncit->first))
			{
				Syncit++;
			}
			else
			{
				Syncit = m_mapDownSyncProcess.erase(Syncit);
			}

		}
	}
	CSyncScriptPointInterface::CSyncScriptPointInterface(const CSyncScriptPointInterface& val) :CScriptPointInterface(val)
	{
	}
	CSyncScriptPointInterface& CSyncScriptPointInterface::operator=(const CSyncScriptPointInterface& val)
	{
		CScriptPointInterface::operator=(val);
		// TODO: 在此处插入 return 语句
		return *this;
	}
	void CSyncScriptPointInterface::ClearSyncInfo()
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		std::map<__int64, unsigned int>::iterator itUp = m_mapUpSyncProcess.begin();
		CScriptStack tempStack;
		ScriptVector_PushVar(tempStack, this);
		for (; itUp != m_mapUpSyncProcess.end(); itUp++)
		{
			CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_REMOVE_UP_SYNC, itUp->first, tempStack, GetProcessID());
		}
		m_mapUpSyncProcess.clear();
		std::map<__int64, bool>::iterator itDown = m_mapDownSyncProcess.begin();
		for (; itDown != m_mapDownSyncProcess.end(); itDown++)
		{
			CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_REMOVE_DOWN_SYNC, itDown->first, tempStack, GetProcessID());
		}
		m_mapDownSyncProcess.clear();
	}
	void CSyncScriptPointInterface::AddUpSyncProcess(__int64 processId, int tier)
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		m_mapUpSyncProcess[processId] = tier;

		auto itOld = m_mapUpSyncProcess.find(m_nProcessID);
		if (itOld != m_mapUpSyncProcess.end())
		{
			//？需要根据层数动态调整上行同步索引吗？
			int curTier = itOld->second;
			if (tier < curTier)
			{
				m_nProcessID = processId;
				m_nImageTier = tier;
			}
		}
		else
		{
			m_nProcessID = processId;
			m_nImageTier = tier;
		}
	}
	bool CSyncScriptPointInterface::CheckUpSyncProcess(__int64 processId)
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		auto itOld = m_mapUpSyncProcess.find(processId);
		if (itOld != m_mapUpSyncProcess.end())
		{
			return true;
		}
		return false;
	}
	void CSyncScriptPointInterface::RemoveUpSyncProcess(__int64 processId)
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		auto itOld = m_mapUpSyncProcess.find(processId);
		if (itOld != m_mapUpSyncProcess.end())
		{
			m_mapUpSyncProcess.erase(itOld);
		}
		if (m_nProcessID == processId)
		{
			int nTier = 0x7fffffff;
			m_nProcessID = -1;
			m_nImageTier = 0;
			auto itOld = m_mapUpSyncProcess.begin();
			for (; itOld != m_mapUpSyncProcess.end(); itOld++)
			{
				if (itOld->second < nTier)
				{
					nTier = itOld->second;
					m_nProcessID = itOld->first;
					m_nImageTier = nTier;
				}
			}
		}
		if (m_mapUpSyncProcess.empty())
		{
			//是否应该删除了？
			//直接删除
			PointVarInfo pointVar = this->GetScriptPointIndex();
			auto pPoint = pointVar.pPoint;
			if (pPoint && (pPoint->GetAutoReleaseMode() & SCRIPT_NO_DOWN_SYNC_AUTO_RELEASE))
			{
				CScriptSuperPointerMgr::GetInstance()->AddPoint2Release(this->GetScriptPointIndex());
			}
		}
	}
	void CSyncScriptPointInterface::AddDownSyncProcess(__int64 nID)
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		m_mapDownSyncProcess[nID] = true;
	}

	bool CSyncScriptPointInterface::CheckDownSyncProcess(__int64 nID)
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		if (m_mapDownSyncProcess.find(nID) != m_mapDownSyncProcess.end())
		{
			return true;
		}
		return false;
	}
	void CSyncScriptPointInterface::RemoveDownSyncProcess(__int64 processId)
	{
		//std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		m_SyncProcessLock.lock();
		auto it = m_mapDownSyncProcess.find(processId);
		if (it != m_mapDownSyncProcess.end())
			m_mapDownSyncProcess.erase(it);
		if (!m_mapDownSyncProcess.empty())
		{
			m_SyncProcessLock.unlock();
			return;
		}
		m_SyncProcessLock.unlock();

		//直接放入释放队列，如果释放时检查还有地方引用，会停止释放的
		PointVarInfo pointVar = this->GetScriptPointIndex();
		auto pPoint = pointVar.pPoint;
		if (pPoint && (pPoint->GetAutoReleaseMode() & SCRIPT_NO_DOWN_SYNC_AUTO_RELEASE))
		{
			CScriptSuperPointerMgr::GetInstance()->AddPoint2Release(this->GetScriptPointIndex());
		}
	}
	bool CSyncScriptPointInterface::AddAllData2Bytes(std::vector<char>& vBuff, std::vector<PointVarInfo> &vOutClassPoint)
	{
		//std::lock_guard<std::mutex> Lock(m_FunLock);

		AddUShort2Bytes(vBuff, m_mapSyncAttributes.size());
		auto it = m_mapSyncAttributes.cbegin();
		for (; it != m_mapSyncAttributes.cend(); it++)
		{
			CBaseScriptClassAttribute* att = it->second;
			if (att)
			{
				AddUShort2Bytes(vBuff, it->first);
				//AddUChar2Bytes(vBuff, att->type);
				att->AddData2Bytes(vBuff);
			}
		}
		vOutClassPoint = m_vecSyncClassPoint;
		return true;
	}
	bool CSyncScriptPointInterface::AddUpdateData2Bytes(std::vector<char>& vBuff)
	{
		//在此函数之外加锁
		//std::lock_guard<std::mutex> Lock(m_UpdateSyncAttLock);
		AddUShort2Bytes(vBuff, m_setUpdateSyncAttibute.size());
		auto it = m_setUpdateSyncAttibute.begin();
		for (; it != m_setUpdateSyncAttibute.end(); it++)
		{
			CBaseScriptClassAttribute* att = *it;
			if (att)
			{
				AddUShort2Bytes(vBuff, att->m_index);
				//AddUChar2Bytes(vBuff, att->type);
				att->AddChangeData2Bytes(vBuff);
			}
		}
		return true;
	}
	bool CSyncScriptPointInterface::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len)
	{
		//std::lock_guard<std::mutex> Lock(m_FunLock);
		unsigned short size = DecodeBytes2Short(pBuff, pos, len);

		for (unsigned short i = 0; i < size; i++)
		{
			unsigned short index = DecodeBytes2Short(pBuff, pos, len);
			//char type = DecodeBytes2Char(pBuff, pos, len);
			auto it = m_mapSyncAttributes.find(index);
			if (it != m_mapSyncAttributes.cend())
			{
				CBaseScriptClassAttribute* att = it->second;
				if (att)
				{
					att->DecodeData4Bytes(pBuff, pos, len);
				}
			}
		}

		return true;
	}
	void CSyncScriptPointInterface::ChangeScriptAttribute(CBaseScriptClassAttribute* pAttr, StackVarInfo& old)
	{
		if (pAttr == nullptr)
		{
			return;
		}
		CScriptPointInterface::ChangeScriptAttribute(pAttr,old);
		std::lock_guard<std::mutex> Lock(m_UpdateSyncAttLock);
		if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_SYNC)
		{
			if (pAttr)
				m_setUpdateSyncAttibute.insert(pAttr);
		}
	}
	void CSyncScriptPointInterface::RegisterScriptAttribute(CBaseScriptClassAttribute* pAttr)
	{
		if (pAttr == nullptr)
		{
			return;
		}
		CScriptPointInterface::RegisterScriptAttribute(pAttr);
		if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_SYNC)
		{
			m_mapSyncAttributes[pAttr->m_index] = pAttr;
		}
	}
	void CSyncScriptPointInterface::RemoveScriptAttribute(CBaseScriptClassAttribute* pAttr)
	{
		CScriptPointInterface::RemoveScriptAttribute(pAttr);
	}
	unsigned int CSyncScriptPointInterface::GetSyncInfo_ClassPoint2Index(CScriptBasePointer* point)
	{
		for (unsigned int i = 0; i < m_vecSyncClassPoint.size(); i++)
		{
			if (m_vecSyncClassPoint[i].pPoint == point)
			{
				return i;
			}
		}
		m_vecSyncClassPoint.push_back(point);
		return m_vecSyncClassPoint.size()-1;
	}
	PointVarInfo CSyncScriptPointInterface::GetSyncInfo_Index2ClassPoint(unsigned int index)
	{
		if (m_vecDecodeSyncClassPoint.size() > index)
		{
			return m_vecDecodeSyncClassPoint[index];
		}
		return PointVarInfo();
	}
	void CSyncScriptPointInterface::ClearUpdateSyncAttibute()
	{
		//在此函数之外加锁
		//std::lock_guard<std::mutex> Lock(m_UpdateSyncAttLock);
		auto it = m_setUpdateSyncAttibute.begin();
		for (; it != m_setUpdateSyncAttibute.end(); it++)
		{
			CBaseScriptClassAttribute* att = *it;
			if (att)
			{
				att->ClearChangeFlag();
			}
		}
		m_setUpdateSyncAttibute.clear();
		m_vecSyncClassPoint.clear();
	}
}