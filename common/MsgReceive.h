#pragma once
enum E_MSG_TYPE
{
	E_RUN_SCRIPT = 64,
	E_RUN_SCRIPT_RETURN,
	//TODO 建立同步和上下行同步还需要思考
	//同步类数据要分状态，同步所有数据还是只同步改变数据
	E_SYNC_CLASS_INFO,//同步类状态
	E_SYNC_CLASS_DATA,//同步一个类的数据

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
		nScriptFunNameLen = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_RUN_SCRIPT;
	}
	virtual void Clear()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
		nScriptFunNameLen = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;

		strScriptFunName.clear();
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	//__int64 nEventListIndex;//脚本执行器的ID
	__int64 nReturnID;//脚本执行返回用的ID

	std::string strScriptFunName;//脚本名
	CScriptStack m_scriptParm;
protected:
	//以下是读取时的临时变量
	int nScriptFunNameLen;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};

class CReturnMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CReturnMsgReceiveState()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_RUN_SCRIPT_RETURN;
	}
	virtual void Clear()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	//int nEventListIndex;//脚本执行器的ID
	__int64 nReturnID;//脚本执行状态的ID
	CScriptStack m_scriptParm;
protected:
	//以下是读取时的临时变量

	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};

class CSyncClassInfoMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncClassInfoMsgReceiveState()
	{
		nClassID = -1;

		nRootServerID = -1;
		nRootClassID = -1;
		nTier = -1;

		nClassNameStringLen = -1;
		nDataLen = -1;
		m_pPoint = nullptr;

		nCurParmType = -1;
		nClassPointNum = -1;
	}
	int GetType()
	{
		return E_SYNC_CLASS_INFO;
	}
	virtual void Clear()
	{
		nClassID = -1;

		nRootServerID = -1;
		nRootClassID = -1;
		nTier = -1;

		nClassNameStringLen = -1;
		nDataLen = -1;
		m_pPoint = nullptr;

		nCurParmType = -1;
		nClassPointNum = -1;

		strClassName.clear();
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
private:
	//以下是读取时的临时变量
	int nClassNameStringLen;
	int nCurParmType;
	int nClassPointNum;
	__int64 nClassID;//涉及到的类ID
	int nDataLen;
};
class CSyncClassDataReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncClassDataReceiveState()
	{
		nClassID = -1;
		nCurParmType = -1;
		nClassNameStringLen = -1;
		nClassPointNum = -1;
		nDataLen = -1;

		strClassName.clear();
	}
	int GetType()
	{
		return E_SYNC_CLASS_DATA;
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	std::string strClassName;
	__int64 nClassID;//类ID
	std::vector<char> vData;//同步数据
	std::vector<PointVarInfo> vClassPoint;
private:
	//以下是读取时的临时变量
	int nClassNameStringLen;
	int nCurParmType;
	int nClassPointNum;
	int nDataLen;
};
//需要记录到根节点的路径，以便根节点执行完了函数后将返回值传给发起调用的子节点
class CSyncUpMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncUpMsgReceiveState()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nRouteNum = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_SYNC_UP_PASSAGE;
	}
	virtual void Clear()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nRouteNum = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
		strFunName.clear();
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//涉及到的类ID

	std::string strFunName;
	CScriptStack m_scriptParm;

	std::list<__int64> m_listRoute;//路径
protected:
	//以下是读取时的临时变量
	int nRouteNum;
	int nFunNameStringLen;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};
class CSyncDownMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncDownMsgReceiveState()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_SYNC_DOWN_PASSAGE;
	}
	virtual void Clear()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
		strFunName.clear();
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//涉及到的类ID
	std::string strFunName;
	CScriptStack m_scriptParm;

protected:
	//以下是读取时的临时变量
	int nFunNameStringLen;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
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
		nRouteNum = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
		while (!m_scriptParm.empty())
		{
			m_scriptParm.pop();
		}
		m_listRoute.clear();
	}

	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	CScriptStack m_scriptParm;
	std::list<__int64> m_listRoute;//路径

protected:
	//以下是读取时的临时变量
	int nRouteNum;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
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