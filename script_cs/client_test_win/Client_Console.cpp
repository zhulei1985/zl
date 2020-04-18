// Client_Console.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "ZLScript.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <direct.h>
#include "getFileNameFromPath.h"
#include "Client.h"
#include "ClientConnectMgr.h"
std::atomic_int g_nThreadRunState = 0;//0 退出 1 运行 2 暂停
void BackGroundThreadFun()
{
	zlscript::CScriptVirtualMachine Machine;
	Machine.InitEvent(zlscript::E_SCRIPT_EVENT_RETURN,
		std::bind(&zlscript::CScriptVirtualMachine::EventReturnFun, &Machine, std::placeholders::_1, std::placeholders::_2));
	Machine.InitEvent(zlscript::E_SCRIPT_EVENT_RUNSCRIPT,
		std::bind(&zlscript::CScriptVirtualMachine::EventRunScriptFun, &Machine, std::placeholders::_1, std::placeholders::_2));
	auto oldtime = std::chrono::steady_clock::now();
	while (g_nThreadRunState > 0)
	{
		if (g_nThreadRunState == 2)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		auto nowTime = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - oldtime);
		oldtime = nowTime;
		if (Machine.Exec(duration.count(), 9999) != 0)
		{
			break;
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}


int main()
{
	zlscript::InitScript();
	InitNetworkConnect();
	CClient::Init2Script();

	//设置脚本DEBUG输出
	std::function<void(const char*)> debugfun = [](const char* pstr)
	{
		std::cout << pstr;
	};
	zlscript::CScriptDebugPrintMgr::GetInstance()->RegisterCallBack_PrintFun(debugfun);

	//读取所有脚本
	char buf1[1024];
	_getcwd(buf1, sizeof(buf1));

	std::vector<std::string> vFilename;
	std::string scrioptfullpath = buf1;
	scrioptfullpath += "\\script\\";
	vFilename = GetFileNameFromPath(scrioptfullpath.c_str());
	for (unsigned int i = 0; i < vFilename.size(); i++)
	{
		std::string strFilename = "script/";
		strFilename += vFilename[i].c_str();
		zlscript::LoadFile(strFilename.c_str());
	}
	//zlscript::LoadFile("script/main.script");
	g_nThreadRunState = 1;

	for (int i = 0; i < 5; i++)
	{
		std::thread tbg(BackGroundThreadFun);
		tbg.detach();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	zlscript::RunScript("main");
	while (1)
	{
		CClientConnectMgr::GetInstance()->OnProcess();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	g_nThreadRunState = 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
