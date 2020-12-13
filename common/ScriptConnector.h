#pragma once
#include "SocketConnectorMgr.h"
#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
#include "MsgReceive.h"
#include "MsgHeadProtocol.h"
#include <atomic>
#include <set>
#include <unordered_map>
using namespace zlnetwork;
using namespace zlscript;

class CBaseScriptConnector : public CScriptPointInterface, public CScriptExecFrame
{
public:
	CBaseScriptConnector();
	~CBaseScriptConnector();

	static void Init2Script();

	//这个GetID现在发现有些多余，脚本中直接就以脚本指针索引即可
	int GetID2Script(CScriptRunState* pState);

	int GetPort2Script(CScriptRunState* pState);

	int IsConnect2Script(CScriptRunState* pState);
	int RunScript2Script(CScriptRunState* pState);

	int SetAccount2Script(CScriptRunState* pState);
	int GetAccount2Script(CScriptRunState* pState);

	int SetScriptLimit2Script(CScriptRunState* pState);
	int CheckScriptLimit2Script(CScriptRunState* pState);

	int SetRemoteFunction2Script(CScriptRunState* pState);

	int SetRoute2Script(CScriptRunState* pState);
	int SetRouteInitScript2Script(CScriptRunState* pState);

	int SetHeadProtocol2Script(CScriptRunState* pState);
public:
	//virtual __int64 GetID()
	//{
	//	return CScriptExecFrame::GetEventIndex();
	//}
	virtual int GetSocketPort() { return 0; }
	virtual bool IsSocketClosed() { return true; }

	virtual void OnInit();
	virtual bool OnProcess();
	virtual void OnDestroy();
	virtual bool SendMsg(char* pBuff, int len)=0;
	virtual bool SendMsg(CBaseMsgReceiveState* pMsg)=0;
	virtual bool RunMsg(CBaseMsgReceiveState* pMsg);

	virtual bool AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal) = 0;

	virtual void SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint);
	virtual void SyncUpClassFunRun(__int64 classID, std::string strFunName, CScriptStack& stack);
	virtual void SyncDownClassFunRun(__int64 classID, std::string strFunName, CScriptStack& stack);
	virtual void SendSyncClassData(__int64 classID, std::vector<char>& data);

	virtual void EventReturnFun(__int64 nSendID, CScriptStack& ParmInfo);
	virtual void EventRunFun(__int64 nSendID, CScriptStack& ParmInfo);

	virtual void EventUpSyncFun(__int64 nSendID, CScriptStack& ParmInfo);
	virtual void EventDownSyncFun(__int64 nSendID, CScriptStack& ParmInfo);

	virtual void EventUpSyncData(__int64 nSendID, CScriptStack& ParmInfo);
	virtual void EventDownSyncData(__int64 nSendID, CScriptStack& ParmInfo);

	virtual void SetHeadProtocol(CBaseHeadProtocol* pProtocol){}
public:
	//"我"是指本连接对应的进程

	//"别人"要求"我"执行脚本
	virtual void RunFrom(std::string funName, CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex);
	//"别人"向"我"返回执行脚本的结果
	virtual void ResultFrom(CScriptStack& pram, __int64 nReturnID);

public:
	virtual void SetRouteInitScript(const char*) {}
	virtual bool RouteMsg(CBaseMsgReceiveState* pMsg) { return false; }
protected:
	//当前登录的账号
	std::string strAccountName;

	__int64 nRouteMode_ConnectID;

	//这个链接处理的镜像类
public:
	virtual __int64 GetImage4Index(__int64) = 0;
	virtual __int64 GetIndex4Image(__int64) = 0;
	virtual void SetImageAndIndex(__int64 nImageID, __int64 nLoaclID) = 0;
public:
	void SetScriptLimit(std::string strName);
	bool CheckScriptLimit(std::string strName);
	void RemoveScriptLimit(std::string strName);
protected:
	//可用脚本，不再此表里的脚本函数不能执行，如果是路由模式，不在此表的脚本函数会转发给对应的链接
	std::map<std::string, int> m_mapScriptLimit;

	//注册的远程函数
	std::set<std::string> m_setRemoteFunName;
protected:
	//每当"别人"要求"我"执行脚本并需要返回时，生成一个此状态
	struct tagReturnState
	{
		__int64 nEventIndex;
		__int64 nReturnID;
	};
	std::map<__int64, tagReturnState> m_mapReturnState;
	__int64 m_nReturnCount;
	__int64 AddReturnState(__int64 nEventIndex, __int64 nReturnID);
	tagReturnState* GetReturnState(__int64 nID);
	void RemoveReturnState(__int64 nID);
};
class CScriptRouteConnector;
class CScriptConnector : public CSocketConnector, public CBaseScriptConnector
{
public:
	CScriptConnector();
	~CScriptConnector();

	//__int64 GetID()
	//{
	//	return CScriptExecFrame::GetEventIndex();
	//}
public:
	virtual int GetSocketPort();
	virtual bool IsSocketClosed();

	virtual void OnInit();
	bool OnProcess();
	virtual void OnDestroy();
	virtual bool SendMsg(char *pBuff, int len);
	virtual bool SendMsg(CBaseMsgReceiveState* pMsg);

	bool AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal);

	void SetHeadProtocol(CBaseHeadProtocol* pProtocol);

	//virtual void SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint);
	//virtual void SyncUpClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack);
	//virtual void SyncDownClassFunRun(CSyncScriptPointInterface* pPoint,std::string strFunName, CScriptStack& stack);
protected:

	typedef std::function< int(CSocketConnector* pConnector)> StateFun;
	std::map<int, StateFun> m_mapStateFun;

	bool m_bIsWebSocket;

	CBaseHeadProtocol* m_pHeadProtocol;
	//当前正在处理的消息
	CBaseMsgReceiveState* pCurMsgReceive;
	
public://同步类的镜像索引
	__int64 GetImage4Index(__int64);
	__int64 GetIndex4Image(__int64);
	void SetImageAndIndex(__int64 nImageID, __int64 nLoaclID);
protected:
	std::map<__int64, __int64> m_mapClassImage2Index;
	std::map<__int64, __int64> m_mapClassIndex2Image;
public:
	//"我"是指本连接对应的进程
	//"我"要求"别人"执行脚本
	virtual void RunTo(std::string funName, CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex);
	//"我"向"别人"返回执行脚本的结果
	virtual void ResultTo(CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex);


	//路由模式
public:
	void SetRouteInitScript(const char* pStr);
	bool RouteMsg(CBaseMsgReceiveState* pMsg);

protected:
	std::string strRouteInitScript;
	std::map<__int64, CScriptRouteConnector*> m_mapRouteConnect;
};

//路由模式下的链接
class CScriptRouteConnector : public CBaseScriptConnector
{
public:
	CScriptRouteConnector();

public:
	virtual void OnInit();
	virtual bool OnProcess();
	virtual void OnDestroy();
	virtual bool SendMsg(char* pBuff, int len);
	virtual bool SendMsg(CBaseMsgReceiveState *pMsg);

	bool AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal);

public://同步类的镜像索引

	__int64 GetImage4Index(__int64);
	__int64 GetIndex4Image(__int64);
	void SetImageAndIndex(__int64 nImageID, __int64 nLoaclID);
public:
	void SetMaster(CScriptConnector* pConnect);
	void SetRouteID(__int64 nID);
protected:
	CScriptConnector* m_pMaster;
	__int64 m_RouteID;
public:
	//"我"是指本连接对应的进程
//"我"要求"别人"执行脚本
	virtual void RunTo(std::string funName, CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex);
	//"我"向"别人"返回执行脚本的结果
	virtual void ResultTo(CScriptStack& pram, __int64 nReturnID, __int64 nEventIndex);

};