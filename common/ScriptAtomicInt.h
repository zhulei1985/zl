#pragma once
/****************************************************************************
	Copyright (c) 2020 ZhuLei
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
#include "ZLScript.h"
//因为脚本调用类函数时不再自动加锁，使用Mutex模仿atomic的功能
namespace zlscript
{
	class CScriptAtomicInt : public CScriptPointInterface
	{
	public:
		CScriptAtomicInt();
		~CScriptAtomicInt();

		static void Init2Script();

		int Set2Script(CScriptRunState* pState);
		int Get2Script(CScriptRunState* pState);
		int Add2Script(CScriptRunState* pState);
		int Dec2Script(CScriptRunState* pState);

	public:
		std::mutex m_FunLock;
		__int64 m_nValue;
	};
}