#include "TestClass.h"

int CTestClass::Fun12Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	if (GetProcessID() == 0)
	{
		//����
	}
	else
	{
		//�����ϴ�

	}
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}
