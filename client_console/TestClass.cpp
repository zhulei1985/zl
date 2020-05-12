#include "TestClass.h"

int CTestClass::Fun12Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	if (GetProcessID() == 0)
	{
		//本地
	}
	else
	{
		//镜像，上传

	}
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}
