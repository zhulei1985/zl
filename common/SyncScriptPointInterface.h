#pragma once
#include "ScriptPointInterface.h"
#include "ScriptClassAttributes.h"
#include <unordered_map>

#define SCRIPT_NO_DOWN_SYNC_AUTO_RELEASE 2
namespace zlscript
{
	enum E_SCRIPT_EVENT_TYPE_SYNC
	{
		E_SCRIPT_EVENT_UP_SYNC_DATA = 10,//
		E_SCRIPT_EVENT_DOWN_SYNC_DATA,

		E_SCRIPT_EVENT_UP_SYNC_FUN,
		E_SCRIPT_EVENT_DOWN_SYNC_FUN,

		E_SCRIPT_EVENT_RETURN_SYNC_FUN,

		E_SCRIPT_EVENT_REMOVE_UP_SYNC,
		E_SCRIPT_EVENT_REMOVE_DOWN_SYNC,
	};

#define ATTR_SYNC_INT(val,index) ATTR_BASE_INT(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_INT64(val,index) ATTR_BASE_INT64(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_FLOAT(val,index) ATTR_BASE_FLOAT(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_DOUBLE(val,index) ATTR_BASE_DOUBLE(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_STR(val,index) ATTR_BASE_STR(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_INT64_ARRAY(val,index) ATTR_BASE_INT64_ARRAY(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_INT64_MAP(val,index) ATTR_BASE_INT64_MAP(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_CLASS_POINT(val,index) ATTR_BASE_CLASS_POINT(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_CLASS_POINT_ARRAY(val,index) ATTR_BASE_CLASS_POINT_ARRAY(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);
#define ATTR_SYNC_CLASS_POINT_MAP(val,index) ATTR_BASE_CLASS_POINT_MAP(val,CBaseScriptClassAttribute::E_FLAG_SYNC,index);

#define ATTR_SYNC_AND_DB_INT(val,index) ATTR_BASE_INT(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index);
#define ATTR_SYNC_AND_DB_INT64(val,index) ATTR_BASE_INT64(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index);
#define ATTR_SYNC_AND_DB_FLOAT(val,index) ATTR_BASE_FLOAT(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index);
#define ATTR_SYNC_AND_DB_DOUBLE(val,index) ATTR_BASE_DOUBLE(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index);
#define ATTR_SYNC_AND_DB_STR(val,index) ATTR_BASE_STR(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index);
#define ATTR_SYNC_AND_DB_INT64_ARRAY(val,index) ATTR_BASE_INT64_ARRAY(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index);
#define ATTR_SYNC_AND_DB_INT64_MAP(val,index) ATTR_BASE_INT64_MAP(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB,index);


#define ATTR_SYNC_AND_DB_PRIMARY_INT(val,index) ATTR_BASE_INT(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB|CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY,index);
#define ATTR_SYNC_AND_DB_PRIMARY_INT64(val,index) ATTR_BASE_INT64(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB|CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY,index);

#define ATTR_SYNC_AND_DB_UNIQUE_INT(val,index) ATTR_BASE_INT(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB|CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE,index);
#define ATTR_SYNC_AND_DB_UNIQUE_INT64(val,index) ATTR_BASE_INT64(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB|CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE,index);
#define ATTR_SYNC_AND_DB_UNIQUE_STR(val,index) ATTR_BASE_STR(val,CBaseScriptClassAttribute::E_FLAG_SYNC|CBaseScriptClassAttribute::E_FLAG_DB|CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE,index);

#define CLASS_SCRIPT_SYNC_FUN(classname,funname) \
	int funname##2Script(CScriptCallState* pState); \
	CBaseScriptClassFun funname##Fun{#funname,std::bind(&classname::funname##2Script,this,std::placeholders::_1),this,CBaseScriptClassFun::E_FLAG_SYNC };

#define CLASS_SCRIPT_SYNC_RELAY_FUN(classname,funname) \
	int funname##2Script(CScriptCallState* pState); \
	CBaseScriptClassFun funname##Fun{#funname,std::bind(&classname::funname##2Script,this,std::placeholders::_1),this,CBaseScriptClassFun::E_FLAG_SYNC|CBaseScriptClassFun::E_FLAG_SYNC_RELAY_FUN };


	class CSyncScriptPointInterface : public CScriptPointInterface
	{
	public:
		CSyncScriptPointInterface();
		virtual ~CSyncScriptPointInterface();

		virtual bool CanRelease();

	public:
		//virtual void SetSyncFun(int id, int type)
		//{
		//	m_mapSyncFunFlag[id] = type;
		//}
		int RunFun(int id, CScriptCallState* pState);
		virtual int SyncUpRunFun(int nClassType, std::string strFun, CScriptCallState* pState, std::list<__int64>&);
		virtual int SyncDownRunFun(int nClassType, std::string strFun, CScriptCallState* pState, std::list<__int64>&);

		//同步类数据，如果有上层节点，向上层节点发送，没有上层节点，向下层节点发送
		//void SyncClassData();
		//解析传来的同步数据，并向下层节点发送
		void SyncDownClassData(const char* pBuff, int& pos, unsigned int len, std::vector<PointVarInfo>& vClassPoint);

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

		void ClearSyncInfo();

		void AddUpSyncProcess(__int64 processId, int tier);
		bool CheckUpSyncProcess(__int64 processId);
		void RemoveUpSyncProcess(__int64 processId);

		void AddDownSyncProcess(__int64);
		bool CheckDownSyncProcess(__int64);
		void RemoveDownSyncProcess(__int64 processId);

		virtual bool AddAllData2Bytes(std::vector<char>& vBuff, std::vector<PointVarInfo>& vOutClassPoint);
		virtual bool AddUpdateData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);

		void ChangeScriptAttribute(CBaseScriptClassAttribute* pAttr, StackVarInfo& old);
		void RegisterScriptAttribute(CBaseScriptClassAttribute* pAttr);
		void RemoveScriptAttribute(CBaseScriptClassAttribute* pAttr);

		void RegisterScriptFun(CBaseScriptClassFun* pClassFun);

		virtual unsigned int GetSyncInfo_ClassPoint2Index(CScriptBasePointer* point);
		virtual PointVarInfo GetSyncInfo_Index2ClassPoint(unsigned int index);
	public:
		void UpdataSyncAttibute();
	protected:
		void ClearUpdateSyncAttibute();
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
		
		std::set<CBaseScriptClassAttribute*> m_setUpdateSyncAttibute;

		std::vector<PointVarInfo> m_vecSyncClassPoint;//需要同步的脚本类指针
		std::vector<PointVarInfo> m_vecDecodeSyncClassPoint;//需要同步的脚本类指针

		std::mutex m_SyncProcessLock;

		std::mutex m_UpdateSyncAttLock;
	};

////type 值为0表示不会要求下级节点执行此类函数
//#define RegisterSyncClassFun(name, p, fun, type) \
//	{ \
//		struct CScript_##name##_ClassFunInfo :public CScriptBaseClassFunInfo \
//		{ \
//			std::function< int (CScriptRunState *)> m_fun; \
//			int RunFun(CScriptRunState *pState) \
//			{ \
//				return m_fun(pState); \
//			} \
//			CScriptBaseClassFunInfo* Copy() \
//			{ \
//				CScript_##name##_ClassFunInfo* pInfo = new CScript_##name##_ClassFunInfo; \
//				if (pInfo) \
//				{ \
//					pInfo->vParmeterInfo = this->vParmeterInfo; \
//				} \
//				return pInfo; \
//			} \
//			const char* GetFunName() \
//			{ \
//				return #name; \
//			} \
//		}; \
//		CScript_##name##_ClassFunInfo *pInfo = new CScript_##name##_ClassFunInfo; \
//		pInfo->m_fun = std::bind(fun,p,std::placeholders::_1); \
//		int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(p); \
//		int index = CScriptSuperPointerMgr::GetInstance()->GetClassFunIndex(nClassType,#name); \
//		p->SetFun(index,pInfo); \
//		p->SetSyncFun(index,type); \
//	}
}