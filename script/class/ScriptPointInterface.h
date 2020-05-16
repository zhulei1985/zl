﻿/****************************************************************************
	Copyright (c) 2019 ZhuLei
	Email:zhulei1985@foxmail.com

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
 ****************************************************************************/

#pragma once

#include <mutex>
#include <atomic>
#include <map>
#include <functional> 
#include <string>
#include <vector>
#include <set>

namespace zlscript
{
	class CBaseScriptClassMgr;

	template<class T>
	class CScriptClassMgr;
	template<class T>
	CBaseScriptClassMgr* GetScriptClassMgr();

	enum
	{
		ECALLBACK_ERROR,
		ECALLBACK_FINISH,
		ECALLBACK_WAITING,
		ECALLBACK_CALLSCRIPTFUN,
		ECALLBACK_NEXTCONTINUE,
	};
	template<typename TFuncAddr, typename TFunc>
	inline void GetMemberFuncAddr(TFuncAddr& addr, TFunc func)
	{
		union
		{
			TFunc _func;
			TFuncAddr _addr;
		}ut;
		ut._func = func;
		addr = ut._addr;
	}
	template<typename TFuncAddr, typename TFunc>
	inline void GetFuncForAddr(TFuncAddr addr, TFunc& func)
	{
		union
		{
			TFunc _func;
			TFuncAddr _addr;
		}ut;
		ut._addr = addr;
		func = ut._func;
	}
	class CScriptRunState;
	typedef int (*ScriptClassFunHandler)(CScriptRunState*);
	struct CScriptBaseClassFunInfo
	{
		struct tagParameterInfo
		{
			int type;//1 数值 2 浮点 3 字符串 4 类指针
			int classtype;//如果是类指针，类类型值
		};
		//virtual void init() = 0;
		std::vector<tagParameterInfo> vParmeterInfo;
		virtual int RunFun(CScriptRunState* pState) = 0;
		virtual CScriptBaseClassFunInfo* Copy() = 0;
		virtual const char* GetFunName() = 0;
	};

	class CScriptPointInterface
	{
	public:
		CScriptPointInterface();
		//{
		//	m_nID = 0;
		//}
		~CScriptPointInterface();
		virtual void InitScriptPointIndex();
		bool IsInitScriptPointIndex();
		__int64 GetScriptPointIndex();
		void ClearScriptPointIndex();

		void SetFun(int id, CScriptBaseClassFunInfo* pInfo);
		int RunFun(int id, CScriptRunState* pState);

		CScriptPointInterface(const CScriptPointInterface& val);
		virtual CScriptPointInterface& operator=(const CScriptPointInterface& val);


	protected:
		bool m_bInit;//是否初始化了
		//用于所有脚本可用的类实例索引，作用范围是本地
		__int64 m_nScriptPointIndex;
		static __int64 s_nScriptPointIndexCount;




		std::map<int, CScriptBaseClassFunInfo*> m_mapScriptClassFun;

		std::mutex m_FunLock;
		//std::shared_ptr<std::mutex> m_FunLock;

	};

}