﻿#pragma once
enum E_MSG_TYPE
{
	E_RUN_SCRIPT = 64,
	E_RUN_SCRIPT_RETURN,
	//TODO 建立同步和上下行同步还需要思考
	//同步类数据要分状态，同步所有数据还是只同步改变数据
	E_SYNC_CLASS_INFO,//同步类状态
	E_SYNC_CLASS_DATA,//同步一个类的数据

	E_NO_SYNC_CLASS_INFO,

	E_SYNC_DOWN_PASSAGE,//下行同步通道
	E_SYNC_UP_PASSAGE,//上行同步通道

	E_SYNC_FUN_RETURN,//同步函数交由根节点执行后，需要向调用的子节点发送返回值

	E_SYNC_DOWN_REMOVE,//移除下行同步
	E_SYNC_UP_REMOVE,//移除上行同步

	E_ROUTE_FRONT,//用于客户端发给服务器
	E_ROUTE_BACK,//用于服务器发给客户端

	E_ROUTE_CHANGE,//更换路由前端
	E_ROUTE_REMOVE,//移除路由状态
};

class CScriptConnector;
class CBaseScriptConnector;
#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
#include <functional>
using namespace zlscript;

// 原来想的是可以边接受边解码消息，现在放弃这个想法，所有消息还是接受完成再解码
class CBaseMsgReceiveState
{
public:
	~CBaseMsgReceiveState();

	virtual int GetType() = 0;
public:
	virtual void Clear() {}
	virtual bool Recv(CScriptConnector*) = 0;
	virtual bool Send(CBaseScriptConnector*);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient,std::vector<char>& vBuff) = 0;
	//网络的连接和路由连接都有可能使用run
	virtual bool Run(CBaseScriptConnector* pClient) = 0;

	virtual bool AddVar2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff, StackVarInfo *val);
	virtual bool DecodeVar4Bytes(CBaseScriptConnector* pClient, StackVarInfo &val);
public:
	void SetGetDataFun(std::function<bool(std::vector<char>&, unsigned int)> fun);
protected:
	//bool CBaseConnector::GetData(std::vector<char>& vOut, unsigned int nSize)
	std::function<bool(std::vector<char>&, unsigned int)> m_GetData;
};
class CScriptMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CScriptMsgReceiveState()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
	}
	int GetType()
	{
		return E_RUN_SCRIPT;
	}
	virtual void Clear()
	{
		//nEventListIndex = -1;
		nReturnID = -1;

		strScriptFunName.clear();

		while (m_scriptParm.nIndex > 0)
			STACK_POP(m_scriptParm);
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	//__int64 nEventListIndex;//脚本执行器的ID
	__int64 nReturnID;//脚本执行返回用的ID

	std::string strScriptFunName;//脚本名
	tagScriptVarStack m_scriptParm;
};

class CReturnMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CReturnMsgReceiveState()
	{
		nReturnID = -1;
	}
	int GetType()
	{
		return E_RUN_SCRIPT_RETURN;
	}
	virtual void Clear()
	{
		nReturnID = -1;

		while (m_scriptParm.nIndex > 0)
			STACK_POP(m_scriptParm);
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nReturnID;//脚本执行状态的ID
	tagScriptVarStack m_scriptParm;

};

class CSyncClassInfoMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncClassInfoMsgReceiveState()
	{
		nRootServerID = -1;
		nRootClassID = -1;
		nTier = -1;

		m_pPoint = nullptr;
	}
	int GetType()
	{
		return E_SYNC_CLASS_INFO;
	}
	virtual void Clear()
	{
		nRootServerID = -1;
		nRootClassID = -1;
		nTier = -1;

		m_pPoint = nullptr;

		strClassName.clear();
		vClassPoint.clear();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	//2020.10.28 类同步消息需要更多的信息
	int nRootServerID;
	__int64 nRootClassID;
	int nTier;
	std::string strClassName;
	CSyncScriptPointInterface* m_pPoint;
	std::vector<PointVarInfo> vClassPoint;

};

class CNoSyncClassInfoMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CNoSyncClassInfoMsgReceiveState()
	{
		m_pPoint = nullptr;
	}
	int GetType()
	{
		return E_NO_SYNC_CLASS_INFO;
	}
	virtual void Clear()
	{
		m_pPoint = nullptr;

		strClassName.clear();
		vClassPoint.clear();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:

	std::string strClassName;
	CScriptPointInterface* m_pPoint;
	std::vector<PointVarInfo> vClassPoint;

};

class CSyncClassDataReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncClassDataReceiveState()
	{
		nClassID = -1;

		strClassName.clear();
	}
	int GetType()
	{
		return E_SYNC_CLASS_DATA;
	}
	virtual void Clear()
	{
		nClassID = 0;

		strClassName.clear();
		vClassPoint.clear();
		vData.clear();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	std::string strClassName;
	__int64 nClassID;//类ID
	std::vector<char> vData;//同步数据
	std::vector<PointVarInfo> vClassPoint;
};
//需要记录到根节点的路径，以便根节点执行完了函数后将返回值传给发起调用的子节点
class CSyncUpMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncUpMsgReceiveState()
	{
		nClassID = -1;
	}
	int GetType()
	{
		return E_SYNC_UP_PASSAGE;
	}
	virtual void Clear()
	{
		nClassID = -1;
		strFunName.clear();
		while (m_scriptParm.nIndex > 0)
			STACK_POP(m_scriptParm);
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//涉及到的类ID

	std::string strFunName;
	tagScriptVarStack m_scriptParm;

	std::list<__int64> m_listRoute;//路径
};
class CSyncDownMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncDownMsgReceiveState()
	{
		nClassID = -1;

	}
	int GetType()
	{
		return E_SYNC_DOWN_PASSAGE;
	}
	virtual void Clear()
	{
		nClassID = -1;

		strFunName.clear();
		while (m_scriptParm.nIndex > 0)
			STACK_POP(m_scriptParm);
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//涉及到的类ID
	std::string strFunName;
	tagScriptVarStack m_scriptParm;

};
class CSyncFunReturnMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncFunReturnMsgReceiveState();
	~CSyncFunReturnMsgReceiveState();

	int GetType()
	{
		return E_SYNC_FUN_RETURN;
	}
	virtual void Clear()
	{
		//nRouteNum = -1;
		//nScriptParmNum = -1;
		//nCurParmType = -1;
		//nStringLen = -1;
		//strClassName.clear();
		while (m_scriptParm.nIndex > 0)
			STACK_POP(m_scriptParm);
		m_listRoute.clear();
	}

	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	tagScriptVarStack m_scriptParm;
	std::list<__int64> m_listRoute;//路径

//protected:
//	//以下是读取时的临时变量
//	int nRouteNum;
//	int nScriptParmNum;
//	int nCurParmType;
//	int nStringLen;
//	std::string strClassName;
};
//E_SYNC_DOWN_REMOVE,//移除下行同步
class CSyncDownRemoveMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncDownRemoveMsgReceiveState()
	{
		nClassID = -1;
	}
	int GetType()
	{
		return E_SYNC_DOWN_REMOVE;
	}
	virtual void Clear()
	{
		nClassID = -1;
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//涉及到的类ID
};
//E_SYNC_UP_REMOVE,//移除上行同步
class CSyncUpRemoveMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncUpRemoveMsgReceiveState()
	{
		nClassID = -1;
	}
	int GetType()
	{
		return E_SYNC_UP_REMOVE;
	}
	virtual void Clear()
	{
		nClassID = -1;
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//涉及到的类ID
};

class CRouteFrontMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CRouteFrontMsgReceiveState()
	{
		nConnectID = -1;
		nMsgType = 0;
		pState = nullptr;
	}
	int GetType()
	{
		return E_ROUTE_FRONT;
	}
	virtual void Clear();
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	__int64 nConnectID;
	CBaseMsgReceiveState* pState;

protected:
	//以下是读取时的临时变量
	int nMsgType;
};

class CRouteBackMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CRouteBackMsgReceiveState()
	{
		nConnectID = -1;
		nMsgType = 0;
		pState = nullptr;
	}
	int GetType()
	{
		return E_ROUTE_BACK;
	}
	virtual void Clear();
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	__int64 nConnectID;
	CBaseMsgReceiveState* pState;

protected:
	//以下是读取时的临时变量
	int nMsgType;
};
//E_ROUTE_CHANGE,//更换路由前端
class CRouteChangeMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CRouteChangeMsgReceiveState()
	{
		nOldConnectID = -1;
		nNewConnectID = -1;
	}
	int GetType()
	{
		return E_ROUTE_CHANGE;
	}
	virtual void Clear();
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	__int64 nOldConnectID;
	__int64 nNewConnectID;
protected:

};
//E_ROUTE_REMOVE,//移除路由状态
class CRouteRemoveMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CRouteRemoveMsgReceiveState()
	{
		nConnectID = -1;
	}
	int GetType()
	{
		return E_ROUTE_REMOVE;
	}
	virtual void Clear();
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	__int64 nConnectID;
protected:

};

class CMsgReceiveMgr
{
public:
	CBaseMsgReceiveState* CreateRceiveState(char cType);
	void RemoveRceiveState(CBaseMsgReceiveState* pState);
public:
	static CMsgReceiveMgr* GetInstance()
	{
		return &s_Instance;
	}
protected:
	static CMsgReceiveMgr s_Instance;
};