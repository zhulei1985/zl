#pragma once
#include "ZLScript.h"

namespace zlscript
{
	class CTempScriptRunState : public CScriptRunState
	{
	public:
		CTempScriptRunState();
		~CTempScriptRunState();
	public:
		virtual bool PushEmptyVarToStack();
		virtual bool PushVarToStack(int nVal);
		virtual bool PushVarToStack(__int64 nVal);
		virtual bool PushVarToStack(double Double);
		virtual bool PushVarToStack(const char* pstr);
		virtual bool PushClassPointToStack(__int64 nIndex);

		virtual bool PushVarToStack(StackVarInfo& Val);

		virtual __int64 PopIntVarFormStack();
		virtual double PopDoubleVarFormStack();
		virtual char* PopCharVarFormStack();
		virtual __int64 PopClassPointFormStack();
		virtual StackVarInfo PopVarFormStack();

		virtual int GetParamNum();

		virtual void CopyToStack(CScriptStack* pStack, int nNum);
		virtual void CopyFromStack(CScriptStack* pStack);

		//��ȡ��������
		virtual void ClearFunParam();

	protected:
		//��ջ
		CScriptStack m_varRegister;
	};
}