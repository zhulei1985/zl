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

int CTestClass::Fun12Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	return ECALLBACK_FINISH;
}
