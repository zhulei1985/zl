#pragma once
#include "SocketConnectorMgr.h"
#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
#include "MsgReceive.h"
#include <atomic>
using namespace zlnetwork;
using namespace zlscript;


class CClient : public CSocketConnector, public CScriptPointInterface
{
public:
	CClient();
	~CClient();
	static void Init2Script();

	int GetID2Script(CScriptRunState* pState);

	int IsConnect2Script(CScriptRunState* pState);
	int RunScript2Script(CScriptRunState* pState);

	int SetAccount2Script(CScriptRunState* pState);
	int GetAccount2Script(CScriptRunState* pState);

	int SetScriptLimit2Script(CScriptRunState* pState);
	int CheckScriptLimit2Script(CScriptRunState* pState);
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

	void SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint);
	void SyncUpClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack);
	void SyncDownClassFunRun(CSyncScriptPointInterface* pPoint,std::string strFunName, CScriptStack& stack);
protected:
	CBaseMsgReceiveState* pCurMsgReceive;
	std::vector<char> m_vBuff;

	//当前登录的账号
	std::string strAccountName;

public:

	void EventReturnFun(int nSendID, CScriptStack& ParmInfo);

public:
	__int64 GetImageIndex(__int64);
	void SetImageIndex(__int64, __int64);
private:
	std::map<__int64, __int64> m_mapClassImageIndex;

public:
	void SetScriptLimit(std::string strName);
	bool CheckScriptLimit(std::string strName);
	void RemoveScriptLimit(std::string strName);
private:
	//可用脚本
	std::map<std::string, int> m_mapScriptLimit;
};

