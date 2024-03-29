﻿#include "scriptcommon.h"
#include "ScriptRunState.h"
#include "ScriptCallBackFunion.h"
#include "ScriptDebugPrint.h"
#include "EScriptSentenceType.h"
#include "EScriptVariableType.h"
#include <chrono>

namespace zlscript
{
	__int64 CScriptCallState::GetMasterID()
	{
		return m_pMaster ? m_pMaster->GetId() : 0;
	}
	//bool CScriptCallState::PushEmptyVarToStack()
	//{
	//	StackVarInfo var;
	//	var.cType = EScriptVal_None;
	//	var.Int64 = 0;

	//	m_stackRegister.push(var);

	//	return true;
	//}
	//bool CScriptCallState::PushVarToStack(int nVal)
	//{
	//	ScriptVector_PushVar(m_stackRegister, (__int64)nVal);

	//	return true;
	//}

	//bool CScriptCallState::PushVarToStack(__int64 nVal)
	//{
	//	ScriptVector_PushVar(m_stackRegister, nVal);

	//	return true;
	//}
	//bool CScriptCallState::PushVarToStack(double Double)
	//{
	//	ScriptVector_PushVar(m_stackRegister, Double);

	//	return true;
	//}
	//bool CScriptCallState::PushVarToStack(const char* pstr)
	//{
	//	ScriptVector_PushVar(m_stackRegister, pstr);

	//	return true;
	//}
	//bool CScriptCallState::PushClassPointToStack(__int64 nIndex)
	//{
	//	StackVarInfo var;
	//	var.cType = EScriptVal_ClassPoint;
	//	var.pPoint = CScriptSuperPointerMgr::GetInstance()->PickupPointer(nIndex);
	//	m_stackRegister.push(var);

	//	return true;
	//}

	//bool CScriptCallState::PushClassPointToStack(CScriptBasePointer* pPoint)
	//{
	//	ScriptVector_PushVar(m_stackRegister, pPoint);

	//	return true;
	//}

	//bool CScriptCallState::PushVarToStack(StackVarInfo& Val)
	//{
	//	m_stackRegister.push(Val);

	//	return true;
	//}


	//__int64 CScriptCallState::PopIntVarFormStack()
	//{
	//	return ScriptStack_GetInt(m_stackRegister);
	//}
	//double CScriptCallState::PopDoubleVarFormStack()
	//{
	//	return ScriptStack_GetFloat(m_stackRegister);
	//}
	//const char* CScriptCallState::PopCharVarFormStack()
	//{
	//	strBuffer = ScriptStack_GetString(m_stackRegister);

	//	return strBuffer.c_str();
	//}
	//PointVarInfo CScriptCallState::PopClassPointFormStack()
	//{
	//	return ScriptStack_GetClassPoint(m_stackRegister);
	//}
	//StackVarInfo CScriptCallState::PopVarFormStack()
	//{
	//	StackVarInfo var = m_stackRegister.top();
	//	m_stackRegister.pop();
	//	return var;
	//}
	__int64 CScriptCallState::GetIntVarFormStack(unsigned int index)
	{
		__int64 nReturn = 0;
		if (index < m_stackRegister.nIndex)
		{
			SCRIPTVAR_GET_INT(m_stackRegister.m_vData[index], nReturn);
		}
		return nReturn;
	}
	double CScriptCallState::GetDoubleVarFormStack(unsigned int index)
	{
		double fReturn = 0;
		if (index < m_stackRegister.nIndex)
		{
			SCRIPTVAR_GET_FLOAT(m_stackRegister.m_vData[index], fReturn);
		}
		return fReturn;
	}
	std::string CScriptCallState::GetStringVarFormStack(unsigned int index)
	{
		std::string strReturn;
		if (index < m_stackRegister.nIndex)
		{
			SCRIPTVAR_GET_STRING(m_stackRegister.m_vData[index], strReturn);
		}
		return strReturn;
	}
	PointVarInfo CScriptCallState::GetClassPointFormStack(unsigned int index)
	{
		if (index < m_stackRegister.nIndex)
		{
			StackVarInfo &var = m_stackRegister.m_vData[index];
			if (var.cType == EScriptVal_ClassPoint)
				return PointVarInfo(var.pPoint);
		}

		return PointVarInfo();
	}
	StackVarInfo CScriptCallState::GetVarFormStack(unsigned int index)
	{
		if (index < m_stackRegister.nIndex)
		{
			return m_stackRegister.m_vData[index];
		}

		return StackVarInfo();
	}
	int CScriptCallState::GetParamNum()
	{
		return m_stackRegister.nIndex;
	}

	StackVarInfo& CScriptCallState::GetResult()
	{
		return m_varReturn;
	}

	void CScriptCallState::SetResult(__int64 nVal)
	{
		m_varReturn = nVal;
	}

	void CScriptCallState::SetResult(double Val)
	{
		m_varReturn = Val;
	}

	void CScriptCallState::SetResult(const char* pStr)
	{
		m_varReturn = pStr;
	}

	void CScriptCallState::SetClassPointResult(__int64 nIndex)
	{
		m_varReturn.Clear();
		m_varReturn.cType = EScriptVal_ClassPoint;
		m_varReturn.pPoint = CScriptSuperPointerMgr::GetInstance()->PickupPointer(nIndex);
	}

	void CScriptCallState::SetClassPointResult(CScriptBasePointer* pPoint)
	{
		m_varReturn = pPoint;
	}

	void CScriptCallState::SetResult(StackVarInfo& Val)
	{
		m_varReturn = Val;
	}


	__int64 CScriptRunState::s_nIDSum = 0;
	CScriptRunState::CScriptRunState()
	{
		m_pMachine = nullptr;
		//_intUntilTime = 0;
		m_WatingTime = 0;

		//nNetworkID = 0;
		nCallEventIndex = -1;
		m_CallReturnId = 0;
		nRunCount = 0;
		//m_pBattle = nullptr;
		//m_pShape = nullptr;
		if (s_nIDSum == 0)
		{
			s_nIDSum++;//跳过0
		}
		m_id = s_nIDSum++;

		SetEnd(false);
	}
	CScriptRunState::~CScriptRunState()
	{
		ClearStack();
		ClearExecBlock();
	}
	void CScriptRunState::SetWatingTime(unsigned int nTime)
	{
		auto nowTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
		m_WatingTime = nowTime.time_since_epoch().count();
		m_WatingTime += nTime;
	}

	//void CScriptRunState::CopyToStack(CScriptStack* pStack, int nNum)
	//{
	//	if (pStack == nullptr)
	//	{
	//		return;
	//	}
	//	if (m_BlockStack.size() > 0)
	//	{
	//		CScriptExecBlock* pBlock = m_BlockStack.top();
	//		if (pBlock)
	//		{
	//			for (unsigned int i = pBlock->m_stackRegister.nIndex - nNum; i < pBlock->m_stackRegister.nIndex; i++)
	//			{
	//				StackVarInfo var;
	//				STACK_GET_INDEX(pBlock->m_stackRegister, var,i);
	//				pStack->push(var);
	//			}
	//		}
	//	}
	//}


	void CScriptRunState::SetResultRegister(StackVarInfo& var)
	{
		if (m_BlockStack.size() > 0)
		{
			CScriptExecBlock* pBlock = m_BlockStack.top();
			if (pBlock && pBlock->m_cReturnRegisterIndex < R_SIZE)
			{
				SCRIPTVAR_SET_VAR(pBlock->m_register[pBlock->m_cReturnRegisterIndex], var);
			}
		}
	}

	void CScriptRunState::CopyFromStack(tagScriptVarStack& stack)
	{		
		if (m_BlockStack.size() > 0)
		{
			CScriptExecBlock* pBlock = m_BlockStack.top();
			if (pBlock)
			{
				for (unsigned int i = 0; i < stack.nIndex; i++)
				{
					StackVarInfo var;
					STACK_GET_INDEX(stack, var,i);
					STACK_PUSH(pBlock->m_stackRegister, var);
				}
			}
		}
		while (stack.nIndex > 0)
		{
			STACK_POP(stack);
		}
	}
	//__int64 CScriptRunState::GetIntParamVar(int index)
	//{
	//	index = index + CurCallFunParamNum;
	//	if (m_varRegister.size() > index)
	//	{
	//		return 0;
	//	}
	//	StackVarInfo & var = m_varRegister.GetVal(index);
	//	__int64 nReturn = 0;
	//	switch (var.cType)
	//	{
	//	case EScriptVal_Int:
	//		nReturn = var.Int64;
	//		break;
	//	case EScriptVal_Double:
	//		nReturn = (__int64)(var.Double + 0.5f);
	//		break;
	//	case EScriptVal_String:
	//		if (var.pStr)
	//		{
	//			nReturn = atoi(var.pStr);
	//
	//			if (var.cExtend > 0)
	//			{
	//				SAFE_DELETE_ARRAY(var.pStr);
	//			}
	//		}
	//		else
	//		{
	//			nReturn = 0;
	//		}
	//		break;
	//	}
	//	return nReturn;
	//}
	//double CScriptRunState::GetDoubleParamVar(int index)
	//{
	//	index = index + CurCallFunParamNum;
	//	if (m_varRegister.size() > index)
	//	{
	//		return 0;
	//	}
	//	StackVarInfo& var = m_varRegister.GetVal(index);
	//	double qReturn = 0;
	//	switch (var.cType)
	//	{
	//	case EScriptVal_Int:
	//		qReturn = (double)var.Int64;
	//		break;
	//	case EScriptVal_Double:
	//		qReturn = var.Double;
	//		break;
	//	case EScriptVal_String:
	//		if (var.pStr)
	//		{
	//			qReturn = atof(var.pStr);
	//
	//			if (var.cExtend > 0)
	//			{
	//				SAFE_DELETE_ARRAY(var.pStr);
	//			}
	//		}
	//		else
	//		{
	//			qReturn = 0;
	//		}
	//		break;
	//	}
	//
	//	m_varRegister.pop();
	//	return qReturn;
	//}
	//char* CScriptRunState::GetCharParamVar(int index)
	//{
	//	static char strbuff[2048];
	//	memset(strbuff,0,sizeof(strbuff));
	//
	//	index = index + CurCallFunParamNum;
	//	if (m_varRegister.size() > index)
	//	{
	//		return strbuff;
	//	}
	//
	//	StackVarInfo& var = m_varRegister.GetVal(index);
	//	char* pReturn = nullptr;
	//	switch (var.cType)
	//	{
	//	case EScriptVal_Int:
	//		ltoa(var.Int64,strbuff,10);
	//		break;
	//	case EScriptVal_Double:
	//		gcvt(var.Double,8,strbuff);
	//		break;
	//	case EScriptVal_String:
	//		strcpy(strbuff,var.pStr);
	//
	//		if (var.cExtend > 0)
	//		{
	//			SAFE_DELETE_ARRAY(var.pStr);
	//		}
	//		break;
	//	}
	//	return strbuff;
	//}
	void CScriptRunState::ClearFunParam()
	{
		if (m_BlockStack.size() > 0)
		{
			CScriptExecBlock* pBlock = m_BlockStack.top();
			if (pBlock)
			{
				//pBlock->ClearFunParam();
			}
		}
	}

	void CScriptRunState::ClearStack()
	{
		if (m_BlockStack.size() > 0)
		{
			CScriptExecBlock* pBlock = m_BlockStack.top();
			if (pBlock)
			{
				pBlock->ClearStack();
			}
		}
	}
	void CScriptRunState::ClearExecBlock(bool bPrint)
	{
		//if (!m_BlockStack.empty())
		//	zlscript::CScriptDebugPrintMgr::GetInstance()->Print("Script Run Error");
		while (!m_BlockStack.empty())
		{

			CScriptExecBlock* pBlock = m_BlockStack.top();
			if (bPrint)
			{
				zlscript::CScriptDebugPrintMgr::GetInstance()->Print(pBlock->GetCurSourceWords().c_str());
			}
			SAFE_DELETE(pBlock);
			m_BlockStack.pop();
		}
	}
	void CScriptRunState::PrintExecBlock()
	{
		//if (!m_BlockStack.empty())
		//	zlscript::CScriptDebugPrintMgr::GetInstance()->Print("Script Run Error");
		//while (!m_BlockStack.empty())
		//{

		//	CScriptExecBlock* pBlock = m_BlockStack.top();
		//	if (bPrint)
		//	{
		//		zlscript::CScriptDebugPrintMgr::GetInstance()->Print(pBlock->GetCurSourceWords().c_str());
		//	}
		//	SAFE_DELETE(pBlock);
		//	m_BlockStack.pop();
		//}
	}
	void CScriptRunState::ClearAll()
	{
		ClearExecBlock();
		ClearStack();
		ClearFunParam();
		nCallEventIndex = -1;
		m_CallReturnId = 0;
		FunName = "";
		nRunCount = 0;
		m_WatingTime = 0;
	}
	int CScriptRunState::CallFun_CallBack(CScriptVirtualMachine* pMachine, int FunIndex, CScriptCallState* pCallState)
	{
		int nReturn = ECALLBACK_FINISH;
		C_CallBackScriptFunion pFun = CScriptCallBackFunion::GetFun(FunIndex);
		if (pFun)
		{
			nReturn = pFun(pMachine, pCallState);
		}
		else
		{
			SCRIPT_PRINT("script", "Error: script callfun: %d", FunIndex);
			return ECALLBACK_ERROR;
		}
		return nReturn;
	}
	int CScriptRunState::CallFun_Script(CScriptVirtualMachine* pMachine, int FunIndex, tagScriptVarStack& ParmStack, int nParmNum,char cRegisterIndex, bool bIsBreak)
	{
		int nReturn = ECALLBACK_FINISH;
		{
			tagCodeData* pCodeData = CScriptCodeLoader::GetInstance()->GetCode(FunIndex);
			if (pCodeData)
			{
				CScriptExecBlock* pBlock =
					new CScriptExecBlock(pCodeData, this);

				if (pBlock)
				{
					pBlock->m_cReturnRegisterIndex = cRegisterIndex;
					//提取参数
					int nOffset = ParmStack.nIndex - nParmNum;
					if (nOffset < 0)
						nOffset = 0;
					if (pBlock->m_pTempVar)
					{
						int nSize = nParmNum < pBlock->m_nTempVarSize ? nParmNum : pBlock->m_nTempVarSize;
						for (int i = 0; i < nSize; i++)
						{
							StackVarInfo var;
							STACK_GET_INDEX( ParmStack, var,(i + nOffset));

							SCRIPTVAR_SET_VAR(pBlock->m_pTempVar[i],var);
							
						}
					}

					m_BlockStack.push(pBlock);
					nReturn = ECALLBACK_CALLSCRIPTFUN;
				}
				else
				{
					nReturn = ECALLBACK_ERROR;
				}
			}
			else
			{
				SCRIPT_PRINT("script", "Error: script scriptfun: %d", FunIndex);
				return ECALLBACK_ERROR;
			}
		}
		return nReturn;
	}
	int CScriptRunState::CallFunImmediately(CScriptVirtualMachine* pMachine, const char* pFunName, tagScriptVarStack& ParmStack)
	{
		int nReturn = ERunTime_Complete;

		tagCodeData* pCode = CScriptCodeLoader::GetInstance()->GetCode(pFunName);
		if (pCode)
		{
			CScriptExecBlock* pBlock =
				new CScriptExecBlock(pCode, this);

			if (pBlock)
			{
				//注入参数
				if (pBlock->m_pTempVar)
				{
					for (unsigned int i = 0; i < ParmStack.nIndex && i < pBlock->m_nTempVarSize; i++)
					{
						StackVarInfo var;
						STACK_GET_INDEX(ParmStack,var,i);

						SCRIPTVAR_SET_VAR(pBlock->m_pTempVar[i], var);

					}
				}


				m_BlockStack.push(pBlock);
				nReturn = Exec(0xffffffff, pMachine);
			}
			else
			{
				nReturn = ERunTime_Error;
			}
		}

		return nReturn;
	}
	unsigned int CScriptRunState::Exec(unsigned int unTimeRes, CScriptVirtualMachine* pMachine)
	{
		if (unTimeRes == 0)
		{
			unTimeRes = 1;
		}
		while (!m_BlockStack.empty() && unTimeRes > 0)
		{
			CScriptExecBlock* pBlock = m_BlockStack.top();
			if (pBlock == nullptr)
			{
				m_BlockStack.pop();
				continue;
			}
			bool bNeedNext = false;

			while (unTimeRes > 0)
			{
				bool bBreak = false;
				unsigned int nResult = pBlock->ExecBlock(pMachine);
				switch (nResult)
				{
				case CScriptExecBlock::ERESULT_END:
				{
					m_BlockStack.pop();
					if (m_BlockStack.empty())
					{
						m_varReturn = pBlock->GetReturnVar();
						SAFE_DELETE(pBlock);
						return ERunTime_Complete;
					}
					else
					{
						auto pCurBlock = m_BlockStack.top();
						if (pCurBlock)
						{
							//for (unsigned int i = 0; i < pBlock->GetVarSize(); i++)
							//{
							//	auto pVar = pBlock->GetVar(i);
							//	if (pVar)
							//		pCurBlock->PushVar(*pVar);
							//	else
							//	{
							//		StackVarInfo var;
							//		pCurBlock->PushVar(var);
							//	}
							//}
							if (pBlock->GetReturnRegisterIndex() < R_SIZE)
							{
								pCurBlock->m_register[pBlock->GetReturnRegisterIndex()] = pBlock->GetReturnVar();
							}
							else
							{
								STACK_PUSH(pCurBlock->m_stackRegister, pBlock->GetReturnVar());
							}
						}
						SAFE_DELETE(pBlock);
					}
					bBreak = true;
					break;
				}
				case CScriptExecBlock::ERESULT_ERROR:
				{
					//2020/4/23 执行错误后不再退出整个堆栈，而是只终结当前代码块并且返回默认值
						//ClearExecBlock(true);
					SCRIPT_PRINT("script", "Error script 执行错误: %s", FunName.c_str());
#ifdef  _SCRIPT_DEBUG
					for (unsigned int i = 0; i < m_BlockStack.size(); i++)
					{
						auto pBlock = m_BlockStack.get(i);
						if (pBlock->m_pCurCode)
						{
							auto souceInfo = CScriptCodeLoader::GetInstance()->GetSourceWords(pBlock->m_pCurCode->nSoureWordIndex);
							SCRIPT_PRINT("script", "Stack %d|file:%s,line:%d,word:%s", i, souceInfo.strCurFileName.c_str(),
								souceInfo.nLineNum, souceInfo.strLineWords.c_str());
						}
					}
#endif
					//退出本代码块，返回默认返回值
					m_BlockStack.pop();
					if (pBlock->GetDefaultReturnType() != EScriptVal_None)
					{
						if (m_BlockStack.empty())
						{
							StackVarInfo var;
							m_varReturn = var;
							return ERunTime_Complete;
						}
						else
						{
							auto pCurBlock = m_BlockStack.top();
							if (pCurBlock)
							{
								StackVarInfo var;
								STACK_PUSH(pCurBlock->m_stackRegister, var);
							}

						}
					}
					SAFE_DELETE(pBlock);
					bBreak = true;
					break;
				}
				case CScriptExecBlock::ERESULT_WAITING:
				{
					return ERunTime_Waiting;
				}
				case CScriptExecBlock::ERESULT_CALLSCRIPTFUN:
				{
					//检查是否有被打断的函数
					bBreak = true;
				}
				break;
				case CScriptExecBlock::ERESULT_NEXTCONTINUE:
				{
					bNeedNext = true;
				}
				break;
				}
				if (bNeedNext)
				{
					break;
				}
				if (bBreak)
				{
					break;
				}

				unTimeRes--;
				nRunCount++;
			}
			if (bNeedNext)
			{
				break;
			}
		}

		if (m_BlockStack.empty())
		{
			return ERunTime_Complete;
		}
		//if (nRunCount > 50000)
		//{
		//	ClearExecBlock();
		//	PrintDebug("script","Error script 执行步骤过长: %s",FunName.c_str());
		//	return ERunTime_Error;
		//}
		return ERunTime_Continue;
	}


	int CScriptRunState::CallFun(CScriptVirtualMachine* pMachine, CScriptExecBlock* pCurBlock, int nType, int FunIndex, int nParmNum, bool bIsBreak)
	{
		int nReturn = ECALLBACK_FINISH;

		switch (nType)
		{
		case 0:
		{
			//pCurBlock->SetCallFunParamNum(nParmNum);
			C_CallBackScriptFunion pFun = CScriptCallBackFunion::GetFun(FunIndex);
			if (pFun)
			{
				CACHE_NEW(CScriptCallState, pCallState, this);
				if (pCurBlock)
				{
					unsigned int nBegin = pCurBlock->m_stackRegister.nIndex - nParmNum;
					STACK_MOVE_ALL_BACK(pCallState->m_stackRegister, pCurBlock->m_stackRegister, nBegin);
					//for (int i = 0; i < nParmNum; i++)
					//{
					//	StackVarInfo var;
					//	STACK_GET_INDEX(pCurBlock->m_stackRegister, var, (pCurBlock->m_stackRegister.nIndex - nParmNum+i));
					//	STACK_PUSH(pCallState->m_stackRegister, var);
					//}
					////pBlock->SetCallFunParamNum(nParmNum);
					//for (int i = 0; i < nParmNum; i++)
					//{
					//	//pCurBlock->PopVar();
					//	STACK_POP(pCurBlock->m_stackRegister);
					//}
				}
				nReturn = pFun(pMachine, pCallState);
				CACHE_DELETE(pCallState);
			}
			else
			{
				SCRIPT_PRINT("script", "Error: script callfun: %d", FunIndex);
				return ECALLBACK_ERROR;
			}
		}
		break;
		case 1:
		{
			tagCodeData* pCodeData = CScriptCodeLoader::GetInstance()->GetCode(FunIndex);
			if (pCodeData)
			{
				CScriptExecBlock* pBlock =
					new CScriptExecBlock(pCodeData, this);

				if (pBlock)
				{
					//提取参数
					if (pCurBlock)
					{

						for (int i = 0; i < nParmNum; i++)
						{
							StackVarInfo var;
							STACK_GET_INDEX(pCurBlock->m_stackRegister, var, pCurBlock->m_stackRegister.nIndex - nParmNum + i);
							STACK_PUSH(pBlock->m_stackRegister,var);
						}
						//pBlock->SetCallFunParamNum(nParmNum);
						for (int i = 0; i < nParmNum; i++)
						{
							//pCurBlock->PopVar();
							STACK_POP(pCurBlock->m_stackRegister);
						}
					}

					m_BlockStack.push(pBlock);
					nReturn = ECALLBACK_CALLSCRIPTFUN;
				}
				else
				{
					nReturn = ECALLBACK_ERROR;
				}
			}
			else
			{
				SCRIPT_PRINT("script", "Error: script scriptfun: %d", FunIndex);
				return ECALLBACK_ERROR;
			}
		}
		}

		return nReturn;
	}

	int CScriptRunState::CallFun(CScriptVirtualMachine* pMachine, int nType, int FunIndex, tagScriptVarStack& ParmStack, bool bIsBreak)
	{
		int nReturn = ECALLBACK_FINISH;

		switch (nType)
		{
		case 0:
		{
			C_CallBackScriptFunion pFun = CScriptCallBackFunion::GetFun(FunIndex);
			if (pFun)
			{
				CACHE_NEW(CScriptCallState, pCallState, this);
				//注入参数

				for (int i = 0; i < ParmStack.nIndex; i++)
				{
					StackVarInfo var;
					STACK_GET_INDEX(ParmStack, var,i);
					STACK_PUSH(pCallState->m_stackRegister, var);
				}

				nReturn = pFun(pMachine, pCallState);
				CACHE_DELETE(pCallState);
			}
			else
			{
				SCRIPT_PRINT("script", "Error: script callfun: %d", FunIndex);
				return ECALLBACK_ERROR;
			}
		}
		break;
		case 1:
		{
			tagCodeData* pCodeData = CScriptCodeLoader::GetInstance()->GetCode(FunIndex);
			if (pCodeData)
			{
				CScriptExecBlock* pBlock =
					new CScriptExecBlock(pCodeData, this);

				if (pBlock)
				{
					//注入参数
					for (int i = 0; i < ParmStack.nIndex; i++)
					{
						StackVarInfo var;
						STACK_GET_INDEX(ParmStack, var, i);
						STACK_PUSH(pBlock->m_stackRegister, var);
					}
					if (bIsBreak && m_BlockStack.size() > 0)
					{
						CScriptExecBlock* pOldBlock = m_BlockStack.top();
						if (pOldBlock->GetFunType() == EICODE_FUN_CAN_BREAK)
						{
							m_BlockStack.pop();
						}
					}

					m_BlockStack.push(pBlock);
					nReturn = ECALLBACK_CALLSCRIPTFUN;
				}
				else
				{
					nReturn = ECALLBACK_ERROR;
				}
			}
			else
			{
				SCRIPT_PRINT("script", "Error: script scriptfun: %d", FunIndex);
				return ECALLBACK_ERROR;
			}
		}
		}

		return nReturn;
	}
	int CScriptRunState::CallFun(CScriptVirtualMachine* pMachine, const char* pFunName, tagScriptVarStack& ParmStack, bool bIsBreak)
	{
		int nReturn = ECALLBACK_FINISH;

		tagCodeData* pCode = CScriptCodeLoader::GetInstance()->GetCode(pFunName);
		if (pCode)
		{
			CScriptExecBlock* pBlock =
				new CScriptExecBlock(pCode, this);

			if (pBlock)
			{
				//注入参数
				if (pBlock->m_pTempVar)
				{
					for (unsigned int i = 0; i < ParmStack.nIndex && i < pBlock->m_nTempVarSize; i++)
					{
						StackVarInfo var;
						STACK_GET_INDEX(ParmStack, var, i);

						SCRIPTVAR_SET_VAR(pBlock->m_pTempVar[i], var);
					}
				}
				if (bIsBreak && m_BlockStack.size() > 0)
				{
					CScriptExecBlock* pOldBlock = m_BlockStack.top();
					if (pOldBlock->GetFunType() == EICODE_FUN_CAN_BREAK)
					{
						m_BlockStack.pop();
					}
				}

				m_BlockStack.push(pBlock);
				nReturn = ECALLBACK_CALLSCRIPTFUN;
			}
			else
			{
				nReturn = ECALLBACK_ERROR;
			}
		}
		else
		{

			C_CallBackScriptFunion pFun = CScriptCallBackFunion::GetFun(pFunName);
			if (pFun)
			{
				CACHE_NEW(CScriptCallState, pCallState, this);
				//注入参数

				for (int i = 0; i < ParmStack.nIndex; i++)
				{
					StackVarInfo var;
					STACK_GET_INDEX(ParmStack, var, i);
					STACK_PUSH(pCallState->m_stackRegister, var);
				}

				nReturn = pFun(pMachine, pCallState);
				CACHE_DELETE(pCallState);
			}
			else
			{
				SCRIPT_PRINT("script", "Error: script callfun: %s", pFunName);
				return ECALLBACK_ERROR;
			}
		}

		return nReturn;
	}
}