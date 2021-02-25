#include "ScriptAtomicInt.h"

namespace zlscript
{
	CScriptAtomicInt::CScriptAtomicInt()
	{
		m_nValue = 0;

		AddClassObject(this->GetScriptPointIndex(), this);
		RegisterClassFun(Set, this, &CScriptAtomicInt::Set2Script);
		RegisterClassFun(Get, this, &CScriptAtomicInt::Get2Script);
		RegisterClassFun(Add, this, &CScriptAtomicInt::Add2Script);
		RegisterClassFun(Dec, this, &CScriptAtomicInt::Dec2Script);
	}
	CScriptAtomicInt::~CScriptAtomicInt()
	{
	}
	void CScriptAtomicInt::Init2Script()
	{
		RegisterClassType("CAtomicInt", CScriptAtomicInt);

		RegisterClassFun1("Set", CScriptAtomicInt);
		RegisterClassFun1("Get", CScriptAtomicInt);
		RegisterClassFun1("Add", CScriptAtomicInt);
		RegisterClassFun1("Dec", CScriptAtomicInt);
	}
	int CScriptAtomicInt::Set2Script(CScriptRunState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		m_FunLock.lock();
		m_nValue = pState->PopIntVarFormStack();
		m_FunLock.unlock();
		pState->ClearFunParam();

		return ECALLBACK_FINISH;
	}
	int CScriptAtomicInt::Get2Script(CScriptRunState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		pState->ClearFunParam();
		std::lock_guard<std::mutex> Lock(m_FunLock);
		pState->PushVarToStack(m_nValue);
		return ECALLBACK_FINISH;
	}
	int CScriptAtomicInt::Add2Script(CScriptRunState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		std::lock_guard<std::mutex> Lock(m_FunLock);
		__int64 nVal = 1;
		if (pState->GetParamNum() > 0)
		{
			nVal = pState->PopIntVarFormStack();
		}
		m_nValue += nVal;
		pState->ClearFunParam();
		pState->PushVarToStack(m_nValue);
		return ECALLBACK_FINISH;
	}
	int CScriptAtomicInt::Dec2Script(CScriptRunState* pState)
	{
		if (pState == nullptr)
		{
			return ECALLBACK_ERROR;
		}
		std::lock_guard<std::mutex> Lock(m_FunLock);
		__int64 nVal = 1;
		if (pState->GetParamNum() > 0)
		{
			nVal = pState->PopIntVarFormStack();
		}
		m_nValue -= nVal;
		pState->ClearFunParam();
		pState->PushVarToStack(m_nValue);
		return ECALLBACK_FINISH;
	}
}