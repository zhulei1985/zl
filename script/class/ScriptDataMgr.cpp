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

#include "ScriptVirtualMachine.h"

#include "ScriptDataMgr.h"

#include "zByteArray.h"
namespace zlscript
{
	CScriptHashMap::CScriptHashMap()
	{
		AddClassObject(CScriptPointInterface::GetScriptPointIndex(), this);

		//RegisterClassFun(GetVal, this, &CScriptHashMap::GetVal2Script);
		//RegisterClassFun(SetVal, this, &CScriptHashMap::SetVal2Script);
		//RegisterClassFun(Remove, this, &CScriptHashMap::Remove2Script);
		//RegisterClassFun(Clear, this, &CScriptHashMap::Clear2Script);
	}
	CScriptHashMap::~CScriptHashMap()
	{
	}
	void CScriptHashMap::Init2Script()
	{
		RegisterClassType("HashMap", CScriptHashMap);

		//RegisterClassFun1("GetVal", CScriptHashMap);
		//RegisterClassFun1("SetVal", CScriptHashMap);
		//RegisterClassFun1("Remove", CScriptHashMap);

		//RegisterClassFun1("Clear", CScriptHashMap);
	}
	int CScriptHashMap::GetVal2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		auto key = pState->GetVarFormStack(0);

		//pState->ClearFunParam();
		auto it = m_mapData.find(key);
		if (it != m_mapData.end())
		{
			pState->SetResult(it->second);
			return ECALLBACK_FINISH;
		}
		//pState->PushEmptyVarToStack();
		return ECALLBACK_FINISH;
	}

	int CScriptHashMap::SetVal2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		auto key = pState->GetVarFormStack(0);
		m_mapData[key] = pState->GetVarFormStack(1);

		//pState->ClearFunParam();
		return ECALLBACK_FINISH;
	}

	int CScriptHashMap::Remove2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		auto key = pState->GetVarFormStack(0);
		{
			auto it = m_mapData.find(key);
			if (it != m_mapData.end())
			{
				m_mapData.erase(it);
			}
		}
		//pState->ClearFunParam();
		return ECALLBACK_FINISH;
	}

	int CScriptHashMap::Clear2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		m_mapData.clear();
		//pState->ClearFunParam();
		return ECALLBACK_FINISH;
	}
	CScriptArray::CScriptArray()
	{
		//AddClassObject(CScriptPointInterface::GetScriptPointIndex(), this);

		//RegisterClassFun(Add, this, &CScriptArray::Add2Script);
		//RegisterClassFun(Get, this, &CScriptArray::Get2Script);
		//RegisterClassFun(GetSize, this, &CScriptArray::GetSize2Script);
		//RegisterClassFun(Set, this, &CScriptArray::Set2Script);
		//RegisterClassFun(Remove, this, &CScriptArray::Remove2Script);

		//RegisterClassFun(Clear, this, &CScriptArray::Clear2Script);
	}

	CScriptArray::~CScriptArray()
	{
		//RemoveClassObject(CScriptPointInterface::GetScriptPointIndex());
	}

	void CScriptArray::Init2Script()
	{
		RegisterClassType("CArray", CScriptArray);

		//RegisterClassFun1("Add", CScriptArray);
		//RegisterClassFun1("Get", CScriptArray);
		//RegisterClassFun1("GetSize", CScriptArray);
		//RegisterClassFun1("Set", CScriptArray);
		//RegisterClassFun1("Remove", CScriptArray);

		//RegisterClassFun1("Clear", CScriptArray);
	}

	int CScriptArray::Add2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		m_vecVars.push_back(pState->GetVarFormStack(0));

		//pState->ClearFunParam();
		pState->SetResult((__int64)(m_vecVars.size() - 1));
		return ECALLBACK_FINISH;
	}

	int CScriptArray::Get2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		unsigned int nIndex = pState->GetIntVarFormStack(0);

		//pState->ClearFunParam();

		if (nIndex < m_vecVars.size())
		{
			pState->SetResult(m_vecVars[nIndex]);
			return ECALLBACK_FINISH;
		}
		
		//pState->s();
		return ECALLBACK_FINISH;
	}

	int CScriptArray::GetSize2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}

		//pState->ClearFunParam();
		pState->SetResult((__int64)m_vecVars.size());
		return ECALLBACK_FINISH;
	}

	int CScriptArray::Set2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		unsigned int nIndex = pState->GetIntVarFormStack(0);

		if (nIndex < m_vecVars.size())
		{
			m_vecVars[nIndex] = pState->GetVarFormStack(1);
			//pState->ClearFunParam();
			return ECALLBACK_FINISH;
		}

		//pState->ClearFunParam();
		return ECALLBACK_FINISH;
	}

	int CScriptArray::Remove2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		unsigned int nIndex = pState->GetIntVarFormStack(0);
		std::string str = pState->GetStringVarFormStack(1);
		
		if (nIndex < m_vecVars.size())
		{
			auto itDel = m_vecVars.begin();
			for (unsigned int i = 0; i < nIndex; i++)
			{
				itDel++;
			}
			m_vecVars.erase(itDel);
		}
		//pState->ClearFunParam();
		return ECALLBACK_FINISH;
	}

	int CScriptArray::Clear2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		m_vecVars.clear();

		//pState->ClearFunParam();
		return ECALLBACK_FINISH;
	}
	bool CScriptArray::AddAllData2Bytes(std::vector<char>& vBuff, std::vector<PointVarInfo>& vOutClassPoint)
	{
		AddUInt2Bytes(vBuff, m_vecVars.size());
		for (auto it = m_vecVars.begin(); it != m_vecVars.end(); it++)
		{
			AddVar2Bytes(vBuff, &(*it), vOutClassPoint);
		}
		return true;
	}

	bool CScriptArray::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len, std::vector<PointVarInfo>& vOutClassPoint)
	{
		unsigned int nSize = DecodeBytes2Int(pBuff, pos, len);
		m_vecVars.resize(nSize);
		for (unsigned int i = 0; i < nSize; i++)
		{
			m_vecVars[i] = DecodeVar4Bytes(pBuff, pos, len, vOutClassPoint);
		}
		return true;
	}

	CScriptSubData::CScriptSubData()
	{

	}
	CScriptSubData::~CScriptSubData()
	{

	}

	CScriptDataMgr CScriptDataMgr::s_Instance;

	CScriptData::CScriptData()
	{
		m_nUseCount = 0;
		//AddClassObject(CScriptPointInterface::GetScriptPointIndex(), this);
		//RegisterClassFun(GetVal, this, &CScriptData::GetVal2Script);
		//RegisterClassFun(SetVal, this, &CScriptData::SetVal2Script);
	}

	CScriptData::~CScriptData()
	{
		//RemoveClassObject(CScriptPointInterface::GetScriptPointIndex());
	}

	void CScriptData::Init2Script()
	{
		RegisterClassType("CData", CScriptData);

		//RegisterClassFun1("GetVal", CScriptData);
		//RegisterClassFun1("SetVal", CScriptData);
	}

	int CScriptData::GetVal2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		m_Lock.lock();
		StackVarInfo result;
		std::map<std::string, CScriptSubData*>* pMap = &m_mapChild;
		int nParmSize = pState->GetParamNum();
		for (int i = 0; i < nParmSize; i++)
		{
			if (pMap == nullptr)
			{
				result.Clear();
				break;
			}
			std::string strKey = pState->GetStringVarFormStack(i);
			auto it = pMap->find(strKey);
			if (it != pMap->end())
			{
				if (it->second)
				{
					result = it->second->m_var;
					pMap = &it->second->m_mapChild;
				}
				else
				{
					result.Clear();
				}
			}
			else
			{
				result.Clear();
				break;
			}
		}
		m_Lock.unlock();
		//pState->ClearFunParam();
		pState->SetResult(result);
		return ECALLBACK_FINISH;
	}

	int CScriptData::SetVal2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		m_Lock.lock();
		CScriptSubData* pArray = nullptr;
		std::map<std::string, CScriptSubData*>* pMap = &m_mapChild;
		int nParmSize = pState->GetParamNum() - 1;
		for (int i = 0; i < nParmSize; i++)
		{
			if (pMap == nullptr)
			{
				break;
			}
			std::string strKey = pState->GetStringVarFormStack(i);

			pArray = (*pMap)[strKey];
			if (pArray)
			{
				pMap = &pArray->m_mapChild;
			}
			else
			{
				pArray = new CScriptSubData;
				pMap->insert(std::make_pair(strKey, pArray));
				pMap = &pArray->m_mapChild;
			}
		}
		if (pArray)
		{
			pArray->m_var = pState->GetVarFormStack(nParmSize);
		}
		m_Lock.unlock();
		//pState->ClearFunParam();

		return ECALLBACK_FINISH;
	}

	int CScriptData::SaveFile2Script(CScriptCallState* pState)
	{

		return ECALLBACK_FINISH;
	}

	int CScriptData::LoadFile2Script(CScriptCallState* pState)
	{

		return ECALLBACK_FINISH;
	}

	CScriptDataMgr::CScriptDataMgr()
	{

	}

	CScriptDataMgr::~CScriptDataMgr()
	{
		Clear();
	}

	CScriptData* CScriptDataMgr::GetData(const char* pFlag)
	{
		if (pFlag == nullptr)
		{
			return nullptr;
		}
		m_Lock.lock();
		CScriptData* pData = nullptr;
		auto it = m_mapScriptData.find(pFlag);
		if (it != m_mapScriptData.end())
		{
			pData = it->second;
		}
		if (pData == nullptr)
		{
			pData = new CScriptData;
			pData->strFlag = pFlag;
			m_mapScriptData[pFlag] = pData;
		}
		if (pData)
		{
			pData->m_nUseCount++;
		}
		m_Lock.unlock();
		return pData;
	}

	void CScriptDataMgr::ReleaseData(CScriptData* pData)
	{
		if (pData == nullptr)
		{
			return;
		}
		m_Lock.lock();
		pData->m_nUseCount--;
		if (pData->m_nUseCount <= 0)
		{
			auto it = m_mapScriptData.find(pData->strFlag);
			if (it != m_mapScriptData.end())
			{
				m_mapScriptData.erase(it);
			}
			delete pData;
		}
		m_Lock.unlock();
	}

	void CScriptDataMgr::Clear()
	{
		std::map<std::string, CScriptData*>::iterator it = m_mapScriptData.begin();
		for (; it != m_mapScriptData.end(); it++)
		{
			CScriptData* pData = it->second;
			if (pData)
			{
				delete pData;
			}
		}
		m_mapScriptData.clear();

		std::set<CScriptArray*>::iterator itSet = m_setArray.begin();
		for (; itSet != m_setArray.end(); itSet++)
		{
			CScriptArray* pArray = *itSet;
			if (pArray)
			{
				delete pArray;
			}
		}
		m_setArray.clear();
	}

	CScriptArray* CScriptDataMgr::NewArray()
	{
		auto pArray = new CScriptArray;
		if (pArray)
			m_setArray.insert(pArray);
		return pArray;
	}

	void CScriptDataMgr::ReleaseArray(CScriptArray* pArray)
	{
		if (pArray)
		{
			auto it = m_setArray.find(pArray);
			if (it != m_setArray.end())
			{
				m_setArray.erase(it);
			}
			delete pArray;
		}
	}

}