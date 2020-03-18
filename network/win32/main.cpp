
#include <atomic>
#include <thread>
#include <chrono>
#include "winsock2i.h"
#include "ClientConnectMgr.h"
std::atomic_int g_nThreadRunState = 0;//0 退出 1 运行 2 暂停

int main()
{
	if (!initSocket())
	{
		//PrintDebug("debug", "网络初始化失败");
	}

	CClientConnectMgr::GetInstance()->OnInit();
	CClientConnectMgr::GetInstance()->SetListen(9999, CClientConnectMgr::CreateNew);
	while (true)
	{
		CClientConnectMgr::GetInstance()->OnProcess();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}