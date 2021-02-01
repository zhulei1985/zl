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
		std::map<__int64, unsigned int>::iterator itUp = m_mapUpSyncProcess.begin();
		CScriptStack tempStack;
		ScriptVector_PushVar(tempStack, this);
		for (; itUp != m_mapUpSyncProcess.end(); itUp++)
		{
			CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_REMOVE_UP_SYNC, itUp->first, tempStack, GetProcessID());
		}
		std::map<__int64, bool>::iterator itDown = m_mapDownSyncProcess.begin();
		for (; itDown != m_mapDownSyncProcess.end(); itDown++)
		{
			CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_REMOVE_DOWN_SYNC, itDown->first, tempStack, GetProcessID());
		}
		E_SCRIPT_EVENT_REMOVE_UP_SYNC,
			E_SCRIPT_EVENT_REMOVE_DOWN_SYNC,
		m_mapSyncAttributes.clear();
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
				m_FunLock.lock();
				std::map<int, CScriptBaseClassFunInfo*>::iterator it = m_mapScriptClassFun.find(id);
				if (it != m_mapScriptClassFun.end())
				{
					CScriptBaseClassFunInfo* pOld = it->second;
					if (pOld)
					{
						funName = pOld->GetFunName();
					}
				}
				m_FunLock.unlock();
				ScriptVector_PushVar(tempStack, this);
				ScriptVector_PushVar(tempStack, funName.c_str());
				pState->CopyToStack(&tempStack, pState->GetParamNum());
				std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
				if (!CScriptEventMgr::GetInstance()->SendEvent(E_SCRIPT_EVENT_UP_SYNC_FUN, pState->m_pMachine->GetEventIndex(), tempStack, GetProcessID()))
				{

				}
				//CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(GetProcessID());
				//if (pClient)
				//{
				//	pClient->SyncUpClassFunRun(this, funName, tempStack);
				//}
				pState->ClearFunParam();
				return ECALLBACK_FINISH;
			}
			else
			{
				std::string funName;
				CScriptStack parmStack;
				pState->CopyToStack(&parmStack, pState->GetParamNum());

				//先执行本地数据
				int nResult = 0;
				//std::lock_guard<std::mutex> _lock{ *m_FunLock };
				m_FunLock.lock();
				std::map<int, CScriptBaseClassFunInfo*>::iterator it = m_mapScriptClassFun.find(id);
				if (it != m_mapScriptClassFun.end())
				{
					CScriptBaseClassFunInfo* pOld = it->second;
					if (pOld)
					{
						nResult = pOld->RunFun(pState);
						funName = pOld->GetFunName();
					}
				}
				m_FunLock.unlock();

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

				else if (m_setUpdateSyncAttibute.size() > 0)
				{
					//同步属性有更新
					std::vector<char> vBuff;
					AddAllData2Bytes(vBuff, false);
					int pos = 0;
					SyncDownClassData(&vBuff[0], pos, vBuff.size());
					m_setUpdateSyncAttibute.clear();
				}

				return nResult;
			}

		}


		return CScriptPointInterface::RunFun(id, pState);
	}
	int CSyncScriptPointInterface::SyncUpRunFun(int nClassType, std::string strFun, CScriptRunState* pState)
	{
		if (GetProcessID() > 0)//不是根节点，继续往上发送
		{
			CScriptStack tempStack;
			ScriptVector_PushVar(tempStack, this);
			ScriptVector_PushVar(tempStack, strFun.c_str());
			pState->CopyToStack(&tempStack, pState->GetParamNum());

			std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
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
		return SyncDownRunFun(nClassType, strFun, pState);
	}

	int CSyncScriptPointInterface::SyncDownRunFun(int nClassType, std::string strFun, CScriptRunState* pState)
	{
		std::string funName;
		CScriptStack parmStack;
		pState->CopyToStack(&parmStack, pState->GetParamNum());

		//先执行本地数据
		int nResult = 0;
		//std::lock_guard<std::mutex> _lock{ *m_FunLock };
		int index = CScriptSuperPointerMgr::GetInstance()->GetClassFunIndex(nClassType, strFun);
		bool bNeedSyncDown = false;
		m_FunLock.lock();

		std::map<int, CScriptBaseClassFunInfo*>::iterator it = m_mapScriptClassFun.find(index);
		if (it != m_mapScriptClassFun.end())
		{
			CScriptBaseClassFunInfo* pOld = it->second;
			if (pOld)
			{
				nResult = pOld->RunFun(pState);
				funName = pOld->GetFunName();
			}
		}
		m_FunLock.unlock();

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
				if (m_setUpdateSyncAttibute.size() > 0)
				{
					//同步属性有更新
					std::vector<char> vBuff;
					AddAllData2Bytes(vBuff, false);
					int pos = 0;
					SyncDownClassData(&vBuff[0], pos, vBuff.size());
					m_setUpdateSyncAttibute.clear();
				}
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
	void CSyncScriptPointInterface::SyncDownClassData(const char* pBuff, int& pos, unsigned int len)
	{
		int startPos = pos;
		//解码数据
		DecodeData4Bytes((char*)pBuff, pos, len);
		//发送同步消息给下层节点
		CScriptStack tempStack;
		ScriptVector_PushVar(tempStack, this);
		ScriptVector_PushVar(tempStack, pBuff + startPos, pos - startPos);


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
		auto itOld = m_mapUpSyncProcess.find(m_nProcessID);
		if (itOld != m_mapUpSyncProcess.end())
		{
			return true;
		}
		return false;
	}
	void CSyncScriptPointInterface::RemoveUpSyncProcess(__int64 processId)
	{
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		auto itOld = m_mapUpSyncProcess.find(m_nProcessID);
		if (itOld != m_mapUpSyncProcess.end())
		{
			m_mapUpSyncProcess.erase(itOld);
		}
		if (m_nProcessID == processId)
		{
			int nTier = 0x7fffffff;
			m_nProcessID = 0;
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
		std::lock_guard<std::mutex> Lock(m_SyncProcessLock);
		auto it = m_mapDownSyncProcess.find(processId);
		if (it != m_mapDownSyncProcess.end())
			m_mapDownSyncProcess.erase(it);
	}
	bool CSyncScriptPointInterface::AddAllData2Bytes(std::vector<char>& vBuff, bool bAll)
	{
		std::lock_guard<std::mutex> Lock(m_FunLock);
		if (bAll)
		{
			AddUShort2Bytes(vBuff, m_mapSyncAttributes.size());
			auto it = m_mapSyncAttributes.begin();
			for (; it != m_mapSyncAttributes.end(); it++)
			{
				CBaseScriptClassAttribute* att = it->second;
				if (att)
				{
					AddUShort2Bytes(vBuff, it->first);
					//AddUChar2Bytes(vBuff, att->type);
					att->AddData2Bytes(vBuff);
				}
			}
		}
		else
		{
			AddUShort2Bytes(vBuff, m_setUpdateSyncAttibute.size());
			auto it = m_setUpdateSyncAttibute.begin();
			for (; it != m_setUpdateSyncAttibute.end(); it++)
			{
				auto itAtt = m_mapSyncAttributes.find(*it);
				if (itAtt != m_mapSyncAttributes.end())
				{
					CBaseScriptClassAttribute* att = itAtt->second;
					if (att)
					{
						AddUShort2Bytes(vBuff, itAtt->first);
						//AddUChar2Bytes(vBuff, att->type);
						att->AddData2Bytes(vBuff);
					}
				}
			}
		}
		return true;
	}
	bool CSyncScriptPointInterface::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len)
	{
		std::lock_guard<std::mutex> Lock(m_FunLock);
		unsigned short size = DecodeBytes2Short(pBuff, pos, len);

		for (unsigned short i = 0; i < size; i++)
		{
			unsigned short index = DecodeBytes2Short(pBuff, pos, len);
			//char type = DecodeBytes2Char(pBuff, pos, len);
			auto it = m_mapSyncAttributes.find(index);
			if (it != m_mapSyncAttributes.end())
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
	void CSyncScriptPointInterface::ChangeScriptAttribute(short flag, CBaseScriptClassAttribute* pAttr)
	{
		CScriptPointInterface::ChangeScriptAttribute(flag, pAttr);

		if (flag & CBaseScriptClassAttribute::E_FLAG_SYNC)
		{
			if (pAttr)
				m_setUpdateSyncAttibute.insert(pAttr->m_index);
		}
	}
}