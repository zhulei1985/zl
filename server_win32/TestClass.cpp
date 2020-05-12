#include "TestClass.h"

CTestClass::CTestClass()
{

}
CTestClass::~CTestClass()
{

}
void CTestClass::Init2Script()
{
}

int CTestClass::Fun12Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}
