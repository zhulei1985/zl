#pragma once
#include "IOCPConnector.h"
#include <atomic>
using namespace zlnetwork;
class CClient : public CSocketConnector
{
public:
	CClient();
	~CClient();

public:
	//virtual void	OnInit();
	bool OnProcess();
	virtual bool SendMsg(char *pBuff, int len);

	static std::atomic_int m_SendSize;
	static std::atomic_int m_RecvSize;
};

