#include "Client.h"


std::atomic_int CClient::m_SendSize = 0;
std::atomic_int CClient::m_RecvSize = 0;
CClient::CClient()
{
}


CClient::~CClient()
{
}
bool CClient::OnProcess()
{
	CSocketConnector::OnProcess();

	//从接收队列里取出消息处理
	std::vector<char> vOut;
	while (GetData2(vOut, 4096))
	{
		m_RecvSize += vOut.size();
		SendData(&vOut[0], vOut.size());
		m_SendSize += vOut.size();
	}

	return true;
}
bool CClient::SendMsg(char *pBuff, int len)
{
	CSocketConnector::SendData(pBuff, len);
	return true;
}