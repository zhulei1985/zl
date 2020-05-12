#pragma once
enum E_MSG_TYPE
{
	E_RUN_SCRIPT = 64,
	E_RUN_SCRIPT_RETURN,
	E_SYNC_CLASS_DATA,//ͬ��һ���������
	E_SYNC_DOWN_PASSAGE,//����ͬ��ͨ��
	E_SYNC_UP_PASSAGE,//����ͬ��ͨ��
};

class CClient;
#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
using namespace zlscript;

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
	virtual bool OnProcess(CClient*);
private:
	int nClassNameStringLen;
	std::string strClassName;
	__int64 nClassID;//�漰������ID

	CSyncScriptPointInterface* m_pPoint;

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
	virtual bool OnProcess(CClient*);

private:
	__int64 nClassID;//�漰������ID

	int nFunNameStringLen;
	std::string strFunName;

	//�����Ƕ�ȡ�ű���������ʱ��Ҫ�ı���
	CScriptStack m_scriptParm;
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
	virtual bool OnProcess(CClient*);

private:
	__int64 nClassID;//�漰������ID

	int nFunNameStringLen;
	std::string strFunName;

	//�����Ƕ�ȡ�ű���������ʱ��Ҫ�ı���
	CScriptStack m_scriptParm;
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