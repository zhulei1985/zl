#include "RouteEvent.h"

CRouteEventMgr CRouteEventMgr::s_Instance;

CRouteEventMgr::CRouteEventMgr()
{
}

CRouteEventMgr::~CRouteEventMgr()
{
}

bool CRouteEventMgr::Register(__int64 nID)
{
	std::lock_guard<std::mutex> Lock(m_LockEvent);
	auto it = m_mapEvents.find(nID);
	if (it == m_mapEvents.end())
	{
		m_mapEvents[nID];
	}
	return true;
}

bool CRouteEventMgr::Unregister(__int64 nID)
{
	std::lock_guard<std::mutex> Lock(m_LockEvent);
	auto it = m_mapEvents.find(nID);
	if (it != m_mapEvents.end())
	{
		//TODO 如果还有没有处理完的事件，打印错误信息
		m_mapEvents.erase(it);
		return true;
	}

	return false;
}

bool CRouteEventMgr::SendEvent(__int64 nRecvID, bool bFront, CBaseMsgReceiveState* pMsg, __int64 nSendID)
{
	std::lock_guard<std::mutex> Lock(m_LockEvent);
	auto it = m_mapEvents.find(nRecvID);
	if (it != m_mapEvents.end())
	{
		stRouteEvent* pEvent = NewEvent();
		if (pEvent)
		{
			pEvent->nIndex = nSendID;
			pEvent->bFront = bFront;
			pEvent->pMsg = pMsg;
			it->second.push_back(pEvent);
			return true;
		}
	}
	return false;
}

bool CRouteEventMgr::GetEvent(__int64 nEventIndex, std::vector<stRouteEvent*>& vOut)
{
	std::lock_guard<std::mutex> Lock(m_LockEvent);
	auto it = m_mapEvents.find(nEventIndex);
	if (it != m_mapEvents.end())
	{
		for (auto itEvent = it->second.begin(); itEvent != it->second.end(); itEvent++)
		{
			vOut.push_back(*itEvent);
		}
		it->second.clear();
	}
	return false;
}

stRouteEvent* CRouteEventMgr::NewEvent()
{
	return new stRouteEvent;
}

void CRouteEventMgr::ReleaseEvent(stRouteEvent* pPoint)
{
	if (pPoint)
	{
		delete pPoint;
	}
}
