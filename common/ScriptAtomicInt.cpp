#include "ScriptAtomicInt.h"

namespace zlscript
{
	CScriptAtomicInt::CScriptAtomicInt()
	{
		m_nValue = 0;

		AddClassObject(this->GetScriptPointIndex(), this);
	}
	CScriptAtomicInt::~CScriptAtomicInt()
	{
	}
	void CScriptAtomicInt::Init2Script()
	{
		RegisterClassType("CAtomicInt", CScriptAtomicInt);

	}
	int CScriptAtomicInt::Set2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		m_FunLock.lock();
		m_nValue = pState->GetIntVarFormStack(0);
		m_FunLock.unlock();
		//pState->ClearFunParam();

		return ECALLBACK_FINISH;
	}
	int CScriptAtomicInt::Get2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		//pState->ClearFunParam();
		std::lock_guard<std::mutex> Lock(m_FunLock);
		pState->SetResult(m_nValue);
		return ECALLBACK_FINISH;
	}
	int CScriptAtomicInt::Add2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		std::lock_guard<std::mutex> Lock(m_FunLock);
		__int64 nVal = 1;
		if (pState->GetParamNum() > 0)
		{
			nVal = pState->GetIntVarFormStack(0);
		}
		m_nValue += nVal;
		//pState->ClearFunParam();
		pState->SetResult(m_nValue);
		return ECALLBACK_FINISH;
	}
	int CScriptAtomicInt::Dec2Script(CScriptCallState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		std::lock_guard<std::mutex> Lock(m_FunLock);
		__int64 nVal = 1;
		if (pState->GetParamNum() > 0)
		{
			nVal = pState->GetIntVarFormStack(0);
		}
		m_nValue -= nVal;
		//pState->ClearFunParam();
		pState->SetResult(m_nValue);
		return ECALLBACK_FINISH;
	}
}