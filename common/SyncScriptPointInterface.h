#pragma once
#include "ScriptPointInterface.h"
#include "ScriptClassAttributes.h"
#include <unordered_map>
namespace zlscript
{
	enum E_SCRIPT_EVENT_TYPE_SYNC
	{
		E_SCRIPT_EVENT_UP_SYNC_DATA = 10,//
		E_SCRIPT_EVENT_DOWN_SYNC_DATA,

		E_SCRIPT_EVENT_UP_SYNC_FUN,
		E_SCRIPT_EVENT_DOWN_SYNC_FUN,

		E_SCRIPT_EVENT_REMOVE_UP_SYNC,
		E_SCRIPT_EVENT_REMOVE_DOWN_SYNC,
	};

#define INIT_SYNC_ATTRIBUTE(index,val) \
	m_mapSyncAttributes[index] = &val; \
	val.init(CBaseScriptClassAttribute::E_FLAG_SYNC,index,this);

#define INIT_SYNC_AND_DB_ATTRIBUTE(index,val) \
	m_mapSyncAttributes[index] = &val; \
	m_mapDBAttributes[#val] = &val; \
	val.init(CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index,this);

#define INIT_SYNC_AND_DB_ATTRIBUTE_PRIMARY(index,val) \
	m_mapSyncAttributes[index] = &val; \
	m_mapDBAttributes[#val] = &val; \
	val.init(CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB|CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY,index,this);


	class CSyncScriptPointInterface : public CScriptPointInterface
	{
	public:
		CSyncScriptPointInterface();
		virtual ~CSyncScriptPointInterface();
	public:
		virtual void SetSyncFun(int id, int type)
		{
			m_mapSyncFunFlag[id] = type;
		}
		virtual int RunFun(int id, CScriptRunState* pState);
		virtual int SyncUpRunFun(int nClassType, std::string strFun, CScriptRunState* pState);
		virtual int SyncDownRunFun(int nClassType, std::string strFun, CScriptRunState* pState);

		//同步类数据，如果有上层节点，向上层节点发送，没有上层节点，向下层节点发送
		//void SyncClassData();
		//解析传来的同步数据，并向下层节点发送
		void SyncDownClassData(const char* pBuff, int& pos, unsigned int len);

		CSyncScriptPointInterface(const CSyncScriptPointInterface& val);
		virtual CSyncScriptPointInterface& operator=(const CSyncScriptPointInterface& val);
	public:
		enum
		{
			E_TYPE_LOCAL,
			E_TYPE_IMAGE,
		};

		void SetRootServerID(int val)
		{
			m_nRootServerID = val;
		}
		int GetRootServerID()
		{
			return m_nRootServerID;
		}

		void SetRootClassID(__int64 val)
		{
			m_nRootClassID = val;
		}
		__int64 GetRootClassID()
		{
			return m_nRootClassID;
		}

		void SetImageTier(int val)
		{
			m_nImageTier = val;
		}
		int GetImageTier()
		{
			return m_nImageTier;
		}

		void SetProcessID(__int64 val)
		{
			m_nProcessID = val;
		}
		__int64 GetProcessID()
		{
			return m_nProcessID;
		}

		void AddUpSyncProcess(__int64 processId, int tier);
		bool CheckUpSyncProcess(__int64 processId);
		void RemoveUpSyncProcess(__int64 processId);

		void AddDownSyncProcess(__int64);
		bool CheckDownSyncProcess(__int64);
		void RemoveDownSyncProcess(__int64 processId);

		virtual bool AddAllData2Bytes(std::vector<char>& vBuff, bool bAll = true);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);

		void ChangeScriptAttribute(short flag, CBaseScriptClassAttribute* pAttr);
	protected:
		int m_nRootServerID;//根节点所在服务器ID
		__int64 m_nRootClassID;//本实例在根节点上的ID
		unsigned int m_nImageTier;//镜像的层数

		__int64 m_nProcessID;//如果网络镜像，所属进程ID，（上行节点对应的连接ID）
		//std::set<stUpSyncInfo> m_listUpSyncProcess;
		std::map<__int64, unsigned int> m_mapUpSyncProcess;
		//有些下行节点会通知不要给它同步
		std::map<__int64,bool> m_mapDownSyncProcess;//需要同步的线程 （下行节点对应的连接ID）

		std::unordered_map<int,int> m_mapSyncFunFlag;

		std::map<unsigned short, CBaseScriptClassAttribute*> m_mapSyncAttributes;
		
		std::set<unsigned short> m_setUpdateSyncAttibute;

		std::mutex m_SyncProcessLock;
	};


#define RegisterSyncClassFun(name, p, fun, type) \
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
		p->SetSyncFun(index,type); \
	}
}