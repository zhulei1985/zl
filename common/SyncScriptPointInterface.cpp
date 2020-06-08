#include "SyncScriptPointInterface.h"
#include "ScriptConnector.h"
#include "ScriptConnectMgr.h"
#include "zByteArray.h"
namespace zlscript
{
	CSyncScriptPointInterface::CSyncScriptPointInterface()
	{
		m_nProcessID = 0;
	}
	CSyncScriptPointInterface::~CSyncScriptPointInterface()
	{
	}
	int CSyncScriptPointInterface::RunFun(int id, CScriptRunState* pState)
	{
		if (m_setSyncFunFlag.find(id) != m_setSyncFunFlag.end())
		{
			
			if (GetProcessID() > 0)
			{
				std::string funName;
				CScriptStack tempStack;
				pState->CopyToStack(&tempStack, pState->GetParamNum());

				//��ȡ������
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
				CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(GetProcessID());
				if (pClient)
				{
					pClient->SyncUpClassFunRun(this, funName, tempStack);
				}
				pState->ClearFunParam();
				return ECALLBACK_FINISH;
			}
			else
			{
				std::string funName;
				CScriptStack tempStack;
				pState->CopyToStack(&tempStack, pState->GetParamNum());

				//��ִ�б�������
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
				

				//Ȼ����ͬ�����ӽڵ�
				auto Syncit = m_listSyncProcessID.begin();
				for (; Syncit != m_listSyncProcessID.end(); )
				{
					std::vector<char> vBuff;
					CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(*Syncit);
					if (pClient)
					{
						pClient->SyncDownClassFunRun(this, funName, tempStack);
						Syncit++;
					}
					else
					{
						Syncit = m_listSyncProcessID.erase(Syncit);
					}
					
				}

				return nResult;
			}

		}


		return CScriptPointInterface::RunFun(id, pState);
	}
	int CSyncScriptPointInterface::SyncUpRunFun(int nClassType, std::string strFun, CScriptRunState* pState)
	{
		if (GetProcessID() > 0)//���Ǹ��ڵ㣬�������Ϸ���
		{
			CScriptStack tempStack;
			pState->CopyToStack(&tempStack, pState->GetParamNum());

			CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(GetProcessID());
			if (pClient)
			{
				pClient->SyncUpClassFunRun(this, strFun, tempStack);
			}
			pState->ClearFunParam();

			return ECALLBACK_FINISH;
		}
		return SyncDownRunFun(nClassType,strFun,pState);
	}

	int CSyncScriptPointInterface::SyncDownRunFun(int nClassType,std::string strFun, CScriptRunState* pState)
	{
		std::string funName;
		CScriptStack tempStack;
		pState->CopyToStack(&tempStack, pState->GetParamNum());

		//��ִ�б�������
		int nResult = 0;
		//std::lock_guard<std::mutex> _lock{ *m_FunLock };
		int index = CScriptSuperPointerMgr::GetInstance()->GetClassFunIndex(nClassType, strFun);
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


		//Ȼ����ͬ�����ӽڵ�
		auto Syncit = m_listSyncProcessID.begin();
		for (; Syncit != m_listSyncProcessID.end(); )
		{
			std::vector<char> vBuff;
			CScriptConnector* pClient = CScriptConnectMgr::GetInstance()->GetConnector(*Syncit);
			if (pClient)
			{
				pClient->SyncDownClassFunRun(this, funName, tempStack);
				Syncit++;
			}
			else
			{
				Syncit = m_listSyncProcessID.erase(Syncit);
			}

		}
		return nResult;
	}
	CSyncScriptPointInterface::CSyncScriptPointInterface(const CSyncScriptPointInterface& val):CScriptPointInterface(val)
	{
	}
	CSyncScriptPointInterface& CSyncScriptPointInterface::operator=(const CSyncScriptPointInterface& val)
	{
		CScriptPointInterface::operator=(val);
		// TODO: �ڴ˴����� return ���
		return *this;
	}
	void CSyncScriptPointInterface::AddSyncProcess(__int64 nID)
	{
		m_listSyncProcessID.insert(nID);
	}

	bool CSyncScriptPointInterface::CheckSyncProcess(__int64 nID)
	{
		if (m_listSyncProcessID.find(nID) != m_listSyncProcessID.end())
		{
			return true;
		}
		return false;
	}
}