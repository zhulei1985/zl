#include "TestDataClass.h"
#include "zByteArray.h"
CTestDataClass::CTestDataClass()
{

}
CTestDataClass::~CTestDataClass()
{

}
void CTestDataClass::Init2Script()
{
}

bool CTestDataClass::AddAllData2Bytes(std::vector<char>& vBuff, std::vector<PointVarInfo>& vOutClassPoint)
{
	AddInt2Bytes(vBuff, varInt);
	return true;
}

bool CTestDataClass::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len, std::vector<PointVarInfo>& vOutClassPoint)
{
	varInt = DecodeBytes2Int(pBuff, pos, len);
	return true;
}

int CTestDataClass::Fun12Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	return ECALLBACK_FINISH;
}
