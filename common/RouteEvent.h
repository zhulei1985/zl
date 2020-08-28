#pragma once
//��Ҫ���ڽ�����Ӻ�·����������֮���ͨ������
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
	//����������
	__int64 nIndex;

	bool bFront;
	//ת������Ϣ
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

	//TODO �պ���ϻ���
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