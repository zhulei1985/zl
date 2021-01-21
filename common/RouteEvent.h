#pragma once
//主要用于解决连接和路由虚拟连接之间的通信问题
#include <map>
#include <vector>
#include <mutex>
#include <unordered_map>

class CBaseMsgReceiveState;
struct stRouteEvent
{
	stRouteEvent()
	{
		nIndex = 0;
		bFront = true;
		pMsg = nullptr;
	}
	//发送者索引
	__int64 nIndex;

	bool bFront;
	//转发的消息
	CBaseMsgReceiveState* pMsg;
};
class CRouteEventMgr
{
public:
	CRouteEventMgr();
	~CRouteEventMgr();

	bool Register(__int64 nID);
	bool Unregister(__int64 nID);
	bool SendEvent(__int64 nRecvID, bool bFront, CBaseMsgReceiveState* pMsg, __int64 nSendID = 0);
	bool GetEvent(__int64 nEventIndex, std::vector<stRouteEvent*>& vOut);

	//TODO 日后加上缓存
	stRouteEvent* NewEvent();
	void ReleaseEvent(stRouteEvent* pPoint);
protected:
	std::mutex m_LockEvent;
	typedef std::list<stRouteEvent*> tagListEvents;

	std::unordered_map<__int64, tagListEvents> m_mapEvents;
public:
	static CRouteEventMgr* GetInstance()
	{
		return &s_Instance;
	}
private:
	static CRouteEventMgr s_Instance;
};