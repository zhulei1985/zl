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

enum E_SCRIPT_EVENT_TYPE_CONNECT
{
	E_SCRIPT_EVENT_TYPE_CONNECT_REMOVE = 100,//
	E_SCRIPT_EVENT_TYPE_CONNECT_CHANGE,
};
class CNetConnector : public CSocketConnector
{
public:
	CNetConnector(std::string type, std::string flag, std::string password);
	~CNetConnector();

	void OnInit();
	bool OnProcess();
	void OnDestroy();

	bool SendMsg(char* pBuff, int len);
public:
	void SetMaster(CScriptConnector* pConnector);
	bool SetHeadProtocol(std::string& strName, std::string& flag, std::string& password);

	unsigned int GetMsgSize();
	CBaseMsgReceiveState* PopMsg();
private:
	CBaseHeadProtocol* m_pHeadProtocol;
	std::mutex m_LockProtocol;

	bool m_bIsWebSocket;

	CScriptConnector* m_pMaster;

	std::list<CBaseMsgReceiveState*> m_listMsg;
	std::mutex m_LockMsgList;
public:
	bool CanSend()
	{
		return m_bCanSend;
	}
protected:
	std::atomic_bool m_bCanSend{ false };
public:
	void SetInitScrript(std::string str);
	std::string& GetInitScript();
	void SetDisconnectScrript(std::string str);
	std::string &GetDisconnectScript();
protected:
	std::string strInitScript;
	std::string strDisconnectScript;
};
class CBaseScriptConnector : public CScriptPointInterface, public CScriptExecFrame
{
public:
	CBaseScriptConnector();
	~CBaseScriptConnector();

	static void Init2Script();

	//这个GetID现在发现有些多余，脚本中直接就以脚本指针索引即可
	CLASS_SCRIPT_FUN(CBaseScriptConnector, GetID);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, GetPort);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, Close);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, IsConnect);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, RunScript);

	CLASS_SCRIPT_FUN(CBaseScriptConnector, SetAllScriptLimit);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, SetScriptLimit);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, CheckScriptLimit);

	//CLASS_SCRIPT_FUN(CBaseScriptConnector, SetRemoteFunction);

	CLASS_SCRIPT_FUN(CBaseScriptConnector, SetRoute);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, SetRouteInitScript);

	//int SetHeadProtocol2Script(CScriptRunState* pState);

	CLASS_SCRIPT_FUN(CBaseScriptConnector, GetVal);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, SetVal);

	CLASS_SCRIPT_FUN(CBaseScriptConnector, Merge);
	CLASS_SCRIPT_FUN(CBaseScriptConnector, SetDisconnectScript);
public:
	//virtual __int64 GetID()
	//{
	//	return CScriptExecFrame::GetEventIndex();
	//}
	virtual int GetSocketPort() { return 0; }
	virtual bool IsSocketClosed() { return true; }

	virtual void Close(){}
	virtual void OnInit();
	virtual bool OnProcess();
	virtual void OnDestroy();
	virtual bool SendMsg(char* pBuff, int len) { return false; }
	virtual bool SendMsg(CBaseMsgReceiveState* pMsg) { return false; }
	virtual bool RunMsg(CBaseMsgReceiveState* pMsg);

	//virtual bool AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal) { return false; }
	virtual bool AddVar2Bytes(std::vector<char>& vBuff, CScriptBasePointer* pPoint) {
		return false;
	}
	virtual void SendNoSyncClassMsg(std::string strClassName, CScriptPointInterface* pPoint);
	virtual void SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint);
	virtual void SyncUpClassFunRun(__int64 classID, std::string strFunName, tagScriptVarStack& stack, std::list<__int64> listRoute);
	virtual void SyncDownClassFunRun(__int64 classID, std::string strFunName, tagScriptVarStack& stack);
	//virtual void SendSyncClassData(__int64 classID, std::vector<char>& data);
	//virtual void SendReturnSyncFun(std::list<__int64> routeList, CScriptStack& stack);
	virtual void SendRemoveSyncUp(__int64 classID);
	virtual void SendRemoveSyncDown(__int64 classID);
	virtual void SendRemoveRoute(__int64 nConnectID);
	virtual void SendChangeRoute(__int64 oldid, __int64 newid);

	virtual void EventReturnFun(__int64 nSendID, tagScriptVarStack& ParmInfo);
	virtual void EventRunFun(__int64 nSendID, tagScriptVarStack& ParmInfo);

	virtual void EventUpSyncFun(__int64 nSendID, tagScriptVarStack& ParmInfo);
	virtual void EventDownSyncFun(__int64 nSendID, tagScriptVarStack& ParmInfo);

	virtual void EventUpSyncData(__int64 nSendID, tagScriptVarStack& ParmInfo);
	virtual void EventDownSyncData(__int64 nSendID, tagScriptVarStack& ParmInfo);

	virtual void EventReturnSyncFun(__int64 nSendID, tagScriptVarStack& ParmInfo);

	virtual void EventRemoveUpSync(__int64 nSendID, tagScriptVarStack& ParmInfo);
	virtual void EventRemoveDownSync(__int64 nSendID, tagScriptVarStack& ParmInfo);

	virtual void EventRemoveRoute(__int64 nSendID, tagScriptVarStack& ParmInfo);
	virtual void EventChangeRoute(__int64 nSendID, tagScriptVarStack& ParmInfo);

	//virtual void SetHeadProtocol(CBaseHeadProtocol* pProtocol){}

	void SetDisconnectScript(std::string val);
public:
	virtual void RunTo(std::string funName, tagScriptVarStack& pram, __int64 nReturnID, __int64 nEventIndex) {}
	//"我"向"别人"返回执行脚本的结果
	virtual void ResultTo(tagScriptVarStack& pram, __int64 nReturnID, __int64 nEventIndex) {}

	//"我"是指本连接对应的进程

	//"别人"要求"我"执行脚本
	virtual void RunFrom(std::string funName, tagScriptVarStack& pram, __int64 nReturnID, __int64 nEventIndex);
	//"别人"向"我"返回执行脚本的结果
	virtual void ResultFrom(tagScriptVarStack& pram, __int64 nReturnID);

public:
	virtual void SetRouteInitScript(const char*) {}
	virtual void ChangeRoute(__int64 nOldId, __int64 nNewId){}
	virtual void RemoveRoute(__int64 nId){}
	virtual bool RouteMsg(CRouteFrontMsgReceiveState* pMsg) { return false; }
protected:
	std::unordered_map<std::string, StackVarInfo> m_mapData;

	__int64 nRouteMode_ConnectID;

	//这个链接处理的镜像类
public:
	virtual __int64 GetImage4Index(__int64) {
		return 0;
	}
	virtual __int64 GetIndex4Image(__int64) {
		return 0;
	}
	virtual void SetImageAndIndex(__int64 nImageID, __int64 nLoaclID){}
	virtual PointVarInfo MakeNoSyncImage(std::string strClassName, __int64 index) { return PointVarInfo(); }
	virtual PointVarInfo GetNoSyncImage4Index(__int64 index) { return PointVarInfo(); }
public:
	void SetScriptLimit(std::string strName);
	bool CheckScriptLimit(std::string strName);
	void RemoveScriptLimit(std::string strName);
protected:
	//可用脚本，不再此表里的脚本函数不能执行，如果是路由模式，不在此表的脚本函数会转发给对应的链接
	std::map<std::string, int> m_mapScriptLimit;
	bool m_bIgnoreScriptLimit;

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

	std::string m_strDisconnectScript;
public:
	virtual bool CanSend() { return true; }
protected:
	std::atomic_int64_t m_WaitCanSendStateID{ 0 };
};

class CScriptRouteConnector;
class CScriptConnector : public CBaseScriptConnector
{
public:
	CScriptConnector();
	~CScriptConnector();

	//__int64 GetID()
	//{
	//	return CScriptExecFrame::GetEventIndex();
	//}
public:
	//virtual int GetSocketPort();
	virtual bool IsSocketClosed();
	//virtual void Close();
	//bool CanDelete();

	virtual void OnInit();
	bool OnProcess();
	virtual void OnDestroy();
	virtual bool SendMsg(char *pBuff, int len);
	virtual bool SendMsg(CBaseMsgReceiveState* pMsg);

	//bool AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal);
	bool AddVar2Bytes(std::vector<char>& vBuff, CScriptBasePointer* pPoint);

	void SetNetConnector(CNetConnector *pConnector);
	//void SetHeadProtocol(CBaseHeadProtocol* pProtocol);

	//virtual void SendSyncClassMsg(std::string strClassName, CSyncScriptPointInterface* pPoint);
	//virtual void SyncUpClassFunRun(CSyncScriptPointInterface* pPoint, std::string strFunName, CScriptStack& stack);
	//virtual void SyncDownClassFunRun(CSyncScriptPointInterface* pPoint,std::string strFunName, CScriptStack& stack);
	void Merge(CScriptConnector* pOldConnect);
protected:
	std::chrono::time_point<std::chrono::steady_clock> tDisconnectTime;

	typedef std::function< int(CSocketConnector* pConnector)> StateFun;
	std::map<int, StateFun> m_mapStateFun;

	CNetConnector* m_pNetConnector{ nullptr };
	//bool m_bIsWebSocket;

	//CBaseHeadProtocol* m_pHeadProtocol;
	//std::mutex m_LockProtocol;
	//当前正在处理的消息
	//CBaseMsgReceiveState* pCurMsgReceive;
	
public://同步类的镜像索引
	__int64 GetImage4Index(__int64);
	__int64 GetIndex4Image(__int64);
	void SetImageAndIndex(__int64 nImageID, __int64 nLoaclID);
protected:
	std::map<__int64, __int64> m_mapClassImage2Index;
	std::map<__int64, __int64> m_mapClassIndex2Image;

public://非同步类的缓存
	PointVarInfo MakeNoSyncImage(std::string strClassName,__int64 index);
	PointVarInfo GetNoSyncImage4Index(__int64 index);
protected:
	struct tagNoSyncImage
	{
		tagNoSyncImage()
		{
			nCount = 0;
		}
		int nCount;
		PointVarInfo Point;
	};
	std::unordered_map<__int64, tagNoSyncImage> m_mapNoSyncImage;
public:
	//"我"是指本连接对应的进程
	//"我"要求"别人"执行脚本
	virtual void RunTo(std::string funName, tagScriptVarStack& pram, __int64 nReturnID, __int64 nEventIndex);
	//"我"向"别人"返回执行脚本的结果
	virtual void ResultTo(tagScriptVarStack& pram, __int64 nReturnID, __int64 nEventIndex);


	//路由模式
public:
	void SetRouteInitScript(const char* pStr);
	void ChangeRoute(__int64 nOldId, __int64 nNewId);
	void RemoveRoute(__int64 nId);
	bool RouteMsg(CRouteFrontMsgReceiveState* pMsg);

protected:
	std::string strRouteInitScript;
	std::map<__int64, CScriptRouteConnector*> m_mapRouteConnect;
//public:
//	bool CanSend()
//	{
//		return m_bCanSend;
//	}
//protected:
//	std::atomic_bool m_bCanSend{ false };
};

//路由模式下的链接
class CScriptRouteConnector : public CBaseScriptConnector
{
public:
	CScriptRouteConnector();

public:
	virtual void Close();
	virtual void OnInit();
	virtual bool OnProcess();
	virtual void OnDestroy();
	virtual bool SendMsg(char* pBuff, int len);
	virtual bool SendMsg(CBaseMsgReceiveState *pMsg);

	//bool AddVar2Bytes(std::vector<char>& vBuff, StackVarInfo* pVal);
	bool AddVar2Bytes(std::vector<char>& vBuff, CScriptBasePointer* pPoint);

public://同步类的镜像索引

	__int64 GetImage4Index(__int64);
	__int64 GetIndex4Image(__int64);
	void SetImageAndIndex(__int64 nImageID, __int64 nLoaclID);

	virtual PointVarInfo MakeNoSyncImage(std::string strClassName, __int64 index);
	virtual PointVarInfo GetNoSyncImage4Index(__int64 index);
public:
	void SetMaster(CScriptConnector* pConnect);
	void SetRouteID(__int64 nID);
protected:
	CScriptConnector* m_pMaster;
	__int64 m_RouteID;
	bool bRun;
public:
	//"我"是指本连接对应的进程
//"我"要求"别人"执行脚本
	virtual void RunTo(std::string funName, tagScriptVarStack& pram, __int64 nReturnID, __int64 nEventIndex);
	//"我"向"别人"返回执行脚本的结果
	virtual void ResultTo(tagScriptVarStack& pram, __int64 nReturnID, __int64 nEventIndex);

};