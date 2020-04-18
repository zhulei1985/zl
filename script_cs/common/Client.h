#pragma once
#include "SocketConnectorMgr.h"
#include "ZLScript.h"
#include <atomic>
using namespace zlnetwork;
using namespace zlscript;

enum E_MSG_TYPE
{
	E_RUN_SCRIPT = 64,
	E_RUN_SCRIPT_RETURN,
};
class CClient;
class CBaseMsgReceiveState
{
public:
	virtual bool OnProcess(CClient*) = 0;
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
	virtual bool OnProcess(CClient*);

private:
	int nEventListIndex;//�ű�ִ������ID
	__int64 nStateID;//�ű�ִ��״̬��ID
	int nScriptFunNameLen;
	std::string strScriptFunName;//�ű���

	//�����Ƕ�ȡ�ű���������ʱ��Ҫ�ı���
	CScriptStack m_scriptParm;
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
	virtual bool OnProcess(CClient*);

private:
	int nEventListIndex;//�ű�ִ������ID
	__int64 nStateID;//�ű�ִ��״̬��ID

	//�����Ƕ�ȡ�ű���������ʱ��Ҫ�ı���
	CScriptStack m_scriptParm;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};

class CClient : public CSocketConnector, public CScriptPointInterface
{
public:
	CClient();
	~CClient();
	static void Init2Script();

	int IsConnect2Script(CScriptRunState* pState);
	int RunScript2Script(CScriptRunState* pState);
public:
	__int64 GetID() const
	{
		return CBaseConnector::GetID();
	}
	void SetID(__int64 val)
	{
		CBaseConnector::SetID(val);
	}

	virtual void OnInit();
	bool OnProcess();
	virtual void OnDestroy();
	virtual bool SendMsg(char *pBuff, int len);

	bool AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal);

public:
	CBaseMsgReceiveState* CreateRceiveState(char cType);
	void RemoveRceiveState(CBaseMsgReceiveState* pState);
protected:
	CBaseMsgReceiveState* pCurMsgReceive;
	std::vector<char> m_vBuff;

public:

	void EventReturnFun(int nSendID, CScriptStack& ParmInfo);

public:

};

