#pragma once
enum E_MSG_TYPE
{
	E_RUN_SCRIPT = 64,
	E_RUN_SCRIPT_RETURN,
	E_SYNC_CLASS_DATA,//同步一个类的数据
	E_SYNC_DOWN_PASSAGE,//下行同步通道
	E_SYNC_UP_PASSAGE,//上行同步通道
};

class CScriptConnector;
#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
using namespace zlscript;

class CBaseMsgReceiveState
{
public:
	virtual bool Recv(CScriptConnector*) = 0;
	virtual bool Run(CScriptConnector* pClient) = 0;
	virtual bool Send(CScriptConnector*) = 0;
};
class CScriptMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CScriptMsgReceiveState()
	{
		nEventListIndex = -1;
		nStateID = -1;
		nScriptFunNameLen = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CScriptConnector* pClient);
	virtual bool Send(CScriptConnector*);
public:
	__int64 nEventListIndex;//脚本执行器的ID
	__int64 nStateID;//脚本执行状态的ID

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
		nEventListIndex = -1;
		nStateID = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}

	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CScriptConnector* pClient);
	virtual bool Send(CScriptConnector*);
public:
	int nEventListIndex;//脚本执行器的ID
	__int64 nStateID;//脚本执行状态的ID
	CScriptStack m_scriptParm;
protected:
	//以下是读取时的临时变量

	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};

class CSyncClassDataMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncClassDataMsgReceiveState()
	{
		nClassID = -1;

		nClassNameStringLen = -1;
		nDataLen = -1;
		m_pPoint = nullptr;
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CScriptConnector* pClient);
	virtual bool Send(CScriptConnector*);
public:
	std::string strClassName;
	CSyncScriptPointInterface* m_pPoint;
private:
	//以下是读取时的临时变量
	int nClassNameStringLen;
	__int64 nClassID;//涉及到的类ID
	int nDataLen;
};
class CSyncUpMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncUpMsgReceiveState()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CScriptConnector* pClient);
	virtual bool Send(CScriptConnector*);
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
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CScriptConnector* pClient);
	virtual bool Send(CScriptConnector*);
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