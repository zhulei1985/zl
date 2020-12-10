#pragma once
#include "ScriptPointInterface.h"
#include "SyncAttributes.h"
#include <unordered_map>
namespace zlscript
{
	enum E_SCRIPT_EVENT_TYPE_SYNC
	{
		E_SCRIPT_EVENT_UP_SYNC_DATA = 10,//
		E_SCRIPT_EVENT_DOWN_SYNC_DATA,

		E_SCRIPT_EVENT_UP_SYNC_FUN,
		E_SCRIPT_EVENT_DOWN_SYNC_FUN,
	};
#define SYNC_INT(val) CSyncIntAttribute val;
#define SYNC_STR(val) CSyncStringAttribute val;
#define INIT_SYNC_ATTRIBUTE(index,val) \
	m_mapSyncAttributes[index] = &val; \
	val.init(index,this);

//#define SYNC_ATTRIBUTE(index,type,name) \
//	type name; \ 
//	void set_##name##(type val) \
//	{ \
//		name = val; \
//		m_mapSyncAttribute[index] = ; \
//	} \
//	type get_##name##() \
//	{ \
//		return name; \
//	}

	class CSyncScriptPointInterface : public CScriptPointInterface
	{
	public:
		CSyncScriptPointInterface();
		~CSyncScriptPointInterface();
	public:
		virtual void SetSyncFun(int id, int type)
		{
			m_mapSyncFunFlag[id] = type;
		}
		virtual int RunFun(int id, CScriptRunState* pState);
		virtual int SyncUpRunFun(int nClassType, std::string strFun, CScriptRunState* pState);
		virtual int SyncDownRunFun(int nClassType, std::string strFun, CScriptRunState* pState);

		//ͬ�������ݣ�������ϲ�ڵ㣬���ϲ�ڵ㷢�ͣ�û���ϲ�ڵ㣬���²�ڵ㷢��
		//void SyncClassData();
		//����������ͬ�����ݣ������²�ڵ㷢��
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
		void RemoveUpSyncProcess(__int64 processId);

		void AddDownSyncProcess(__int64);
		bool CheckDownSyncProcess(__int64);

		virtual bool AddAllData2Bytes(std::vector<char>& vBuff, bool bAll = true);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);

		void ChangeSyncAttibute(CBaseSyncAttribute*);
	protected:
		int m_nRootServerID;//���ڵ����ڷ�����ID
		__int64 m_nRootClassID;//��ʵ���ڸ��ڵ��ϵ�ID
		unsigned int m_nImageTier;//����Ĳ���

		__int64 m_nProcessID;//������羵����������ID�������нڵ��Ӧ������ID��
		//std::set<stUpSyncInfo> m_listUpSyncProcess;
		std::map<__int64, unsigned int> m_mapUpSyncProcess;
		//��Щ���нڵ��֪ͨ��Ҫ����ͬ��
		std::map<__int64,bool> m_mapDownSyncProcess;//��Ҫͬ�����߳� �����нڵ��Ӧ������ID��

		std::unordered_map<int,int> m_mapSyncFunFlag;

		std::map<unsigned short, CBaseSyncAttribute*> m_mapSyncAttributes;
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