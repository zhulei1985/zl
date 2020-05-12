#pragma once
#include "ScriptPointInterface.h"

namespace zlscript
{
	class CSyncScriptPointInterface : public CScriptPointInterface
	{
	public:
		CSyncScriptPointInterface();
		~CSyncScriptPointInterface();
	public:
		virtual void SetSyncFun(int id)
		{
			m_setSyncFunFlag.insert(id);
		}
		virtual int RunFun(int id, CScriptRunState* pState);
		virtual int SyncUpRunFun(int nClassType, std::string strFun, CScriptRunState* pState);
		virtual int SyncDownRunFun(int nClassType,std::string strFun, CScriptRunState* pState);

		CSyncScriptPointInterface(const CSyncScriptPointInterface& val);
		virtual CSyncScriptPointInterface& operator=(const CSyncScriptPointInterface& val);
	public:
		enum
		{
			E_TYPE_LOCAL,
			E_TYPE_IMAGE,
		};

		void SetProcessID(__int64 val)
		{
			m_nProcessID = val;
		}
		__int64 GetProcessID()
		{
			return m_nProcessID;
		}

		void AddSyncProcess(__int64);
		bool CheckSyncProcess(__int64);

		virtual bool AddAllData2Bytes(std::vector<char>& vBuff) {
			return true;
		}
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, int len) {
			return true;
		}

	protected:
		__int64 m_nProcessID;//如果网络镜像，所属进程ID，（同步父节点）

		std::set<__int64> m_listSyncProcessID;//需要同步的线程 （同步子节点）

		std::set<int> m_setSyncFunFlag;
	};


#define RegisterSyncClassFun(name, p, fun) \
	{ \
		struct CScript_##name##_ClassFunInfo :public CScriptBaseClassFunInfo \
		{ \
			std::function< int (CScriptRunState *)> m_fun; \
			int RunFun(CScriptRunState *pState) \
			{ \
				return m_fun(pState); \
			} \
			CScriptBaseClassFunInfo* Copy() \
			{ \
				CScript_##name##_ClassFunInfo* pInfo = new CScript_##name##_ClassFunInfo; \
				if (pInfo) \
				{ \
					pInfo->vParmeterInfo = this->vParmeterInfo; \
				} \
				return pInfo; \
			} \
			const char* GetFunName() \
			{ \
				return #name; \
			} \
		}; \
		CScript_##name##_ClassFunInfo *pInfo = new CScript_##name##_ClassFunInfo; \
		pInfo->m_fun = std::bind(fun,p,std::placeholders::_1); \
		int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(p); \
		int index = CScriptSuperPointerMgr::GetInstance()->GetClassFunIndex(nClassType,#name); \
		p->SetFun(index,pInfo); \
		p->SetSyncFun(index); \
	}
}