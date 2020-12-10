// server_win32.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "ZLScript.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <direct.h>
#include "getFileNameFromPath.h"
#include "ScriptConnector.h"
#include "ScriptConnectMgr.h"
#include "Account.h"
std::atomic_int g_nThreadRunState = 0;//0 退出 1 运行 2 暂停
void BackGroundThreadFun()
{
	zlscript::CScriptVirtualMachine Machine;
	Machine.InitEvent(zlscript::E_SCRIPT_EVENT_RETURN,
		std::bind(&zlscript::CScriptVirtualMachine::EventReturnFun, &Machine, std::placeholders::_1, std::placeholders::_2));
	Machine.InitEvent(zlscript::E_SCRIPT_EVENT_RUNSCRIPT,
		std::bind(&zlscript::CScriptVirtualMachine::EventRunScriptFun, &Machine, std::placeholders::_1, std::placeholders::_2));
	//Machine.InitEvent(zlscript::E_SCRIPT_EVENT_NETWORK_RUNSCRIPT,
	//	std::bind(&zlscript::CScriptVirtualMachine::EventNetworkRunScriptFun, &Machine, std::placeholders::_1, std::placeholders::_2));
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

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

int UserInput(CScriptVirtualMachine* pMachine, CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	char strTemp[1024];

	std::cin.getline(strTemp, 1024);

	pState->ClearFunParam();
	pState->PushVarToStack(strTemp);
	return ECALLBACK_FINISH;
}
int ExecuteCommand(CScriptVirtualMachine* pMachine, CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::vector<std::string> vStrings;
	std::string strString = pState->PopCharVarFormStack();
	std::string strTemp;
	for (size_t i = 0; i < strString.size(); i++)
	{
		char ch[2] = { 0,0 };
		if (strString[i] == ' ')
		{
			if (strTemp.size() > 0)
			{
				vStrings.push_back(strTemp);
				strTemp.clear();
			}
		}
		else
		{
			ch[0] = strString[i];
			strTemp += ch;
		}
	}
	if (strTemp.size() > 0)
	{
		vStrings.push_back(strTemp);
		strTemp.clear();
	}
	std::vector<StackVarInfo> vOtherVar;
	int nParmNum = pState->GetParamNum();
	for (int i = 1; i < nParmNum; i++)
	{
		vOtherVar.push_back(pState->PopVarFormStack());
	}
	pState->ClearFunParam();
	for (auto it = vOtherVar.rbegin(); it != vOtherVar.rend(); it++)
	{
		pState->PushVarToStack(*it);
	}
	for (auto it = vStrings.rbegin(); it != vStrings.rend(); it++)
	{
		pState->PushVarToStack((*it).c_str());
	}

	return ECALLBACK_FINISH;
}

int main()
{
    //std::cout << "Hello World!\n";
	zlscript::InitScript();
	InitNetworkConnect();
	CScriptConnector::Init2Script();
	CAccount::Init2Script();
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("UserInput", UserInput);
	zlscript::CScriptCallBackFunion::GetInstance()->RegisterFun("ExecuteCommand", ExecuteCommand);
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

	//for (int i = 0; i < 10; i++)
	//{
	//	std::thread tbg(BackGroundThreadFun);
	//	tbg.detach();
	//}
	zlscript::CScriptVirtualMachine Machine(10);
	Machine.InitEvent(zlscript::E_SCRIPT_EVENT_RETURN,
		std::bind(&zlscript::CScriptVirtualMachine::EventReturnFun, &Machine, std::placeholders::_1, std::placeholders::_2));
	Machine.InitEvent(zlscript::E_SCRIPT_EVENT_RUNSCRIPT,
		std::bind(&zlscript::CScriptVirtualMachine::EventRunScriptFun, &Machine, std::placeholders::_1, std::placeholders::_2));

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	auto oldtime = std::chrono::steady_clock::now();
	zlscript::RunScript("main");
	while (1)
	{
		CScriptConnectMgr::GetInstance()->OnProcess();
		auto nowTime = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - oldtime);
		oldtime = nowTime;
		Machine.Exec(duration.count(), 9999);
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
