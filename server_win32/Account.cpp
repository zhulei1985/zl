#include "Account.h"
#include "zByteArray.h"

CAccount::CAccount()
{
	nConnectPointIndex = 0;

	RegisterClassFun(SetAccountName, this, &CAccount::SetAccountName2Script);
	RegisterClassFun(GetAccountName, this, &CAccount::GetAccountName2Script);
	RegisterClassFun(SetPassword, this, &CAccount::SetPassword2Script);
	RegisterClassFun(GetPassword, this, &CAccount::GetPassword2Script);

	RegisterClassFun(GetNickName, this, &CAccount::GetNickName2Script);
	RegisterSyncClassFun(SetNickName, this, &CAccount::SetNickName2Script);

	RegisterClassFun(SetConnect, this, &CAccount::SetConnect2Script);
	RegisterClassFun(GetConnect, this, &CAccount::GetConnect2Script);
}

CAccount::~CAccount()
{

}

void CAccount::Init2Script()
{
	RegisterClassType("CAccount", CAccount);

	RegisterClassFun1("SetAccountName", CAccount);
	RegisterClassFun1("GetAccountName", CAccount);
	RegisterClassFun1("SetPassword", CAccount);
	RegisterClassFun1("GetPassword", CAccount);
	RegisterClassFun1("SetNickName", CAccount);
	RegisterClassFun1("GetNickName", CAccount);

	RegisterClassFun1("SetConnectID", CAccount);
	RegisterClassFun1("GetConnectID", CAccount);
}

int CAccount::SetAccountName2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strAccountName = pState->PopCharVarFormStack();
	pState->ClearFunParam();

	return ECALLBACK_FINISH;
}

int CAccount::GetAccountName2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(strAccountName.c_str());
	return ECALLBACK_FINISH;
}

int CAccount::SetPassword2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strPassword = pState->PopCharVarFormStack();

	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

int CAccount::GetPassword2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(strPassword.c_str());
	return ECALLBACK_FINISH;
}

int CAccount::SetNickName2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strNickName = pState->PopCharVarFormStack();

	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

int CAccount::GetNickName2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushVarToStack(strNickName.c_str());
	return ECALLBACK_FINISH;
}

int CAccount::SetConnect2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	nConnectPointIndex = pState->PopClassPointFormStack();

	pState->ClearFunParam();

	return ECALLBACK_FINISH;
}

int CAccount::GetConnect2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->ClearFunParam();
	pState->PushClassPointToStack(nConnectPointIndex);
	return ECALLBACK_FINISH;
}

bool CAccount::AddAllData2Bytes(std::vector<char>& vBuff)
{
	AddString2Bytes(vBuff, (char*)strAccountName.c_str());
	AddString2Bytes(vBuff, (char*)strNickName.c_str());
	return true;
}

bool CAccount::DecodeData4Bytes(char* pBuff, int& pos, int len)
{
	char strbuff[256] = { 0 };
	DecodeBytes2String(pBuff, pos, len, strbuff, 128);
	strAccountName = strbuff;
	strbuff[0] = 0;
	DecodeBytes2String(pBuff,pos,len,strbuff,128);
	strNickName = strbuff;
	return true;
}

