#include "Account.h"
#include "zByteArray.h"
#include "md5.h"

CAccount::CAccount()
{
	INIT_SYNC_AND_DB_ATTRIBUTE_PRIMARY(1, Id);
	INIT_SYNC_AND_DB_ATTRIBUTE_UNIQUE(2, strAccountName);
	INIT_DB_ATTRIBUTE(3, strPassword);
	INIT_SYNC_AND_DB_ATTRIBUTE(4, strNickName);

	AddClassObject(this->GetScriptPointIndex(), this);

	RegisterClassFun(SetAccountName, this, &CAccount::SetAccountName2Script);
	RegisterClassFun(GetAccountName, this, &CAccount::GetAccountName2Script);
	RegisterClassFun(SetPassword, this, &CAccount::SetPassword2Script);
	RegisterClassFun(GetPassword, this, &CAccount::GetPassword2Script);

	RegisterClassFun(GetNickName, this, &CAccount::GetNickName2Script);
	RegisterSyncClassFun(SetNickName, this, &CAccount::SetNickName2Script, 0);

	RegisterClassFun(GetVal, this, &CAccount::GetVal2Script);
	RegisterClassFun(SetVal, this, &CAccount::SetVal2Script);

	RegisterClassFun(MakeConnectString, this, &CAccount::MakeConnectString2Script);
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

	RegisterClassFun1("GetVal", CAccount);
	RegisterClassFun1("SetVal", CAccount);

	RegisterClassFun1("MakeConnectString", CAccount);
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

int CAccount::GetVal2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string str = pState->PopCharVarFormStack();

	pState->ClearFunParam();
	auto it = m_mapData.find(str);
	if (it != m_mapData.end())
	{
		pState->PushVarToStack(it->second);
		return ECALLBACK_FINISH;
	}
	pState->PushEmptyVarToStack();
	return ECALLBACK_FINISH;
}

int CAccount::SetVal2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string str = pState->PopCharVarFormStack();
	m_mapData[str] = pState->PopVarFormStack();

	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}
int CAccount::MakeConnectString2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	tagByteArray vBuff;
	AddInt642Bytes(vBuff, time(nullptr));
	AddInt2Bytes(vBuff, rand());
	MD5 md5(&vBuff[0], vBuff.size());
	pState->ClearFunParam();
	pState->PushVarToStack(md5.toString().c_str());
	return ECALLBACK_FINISH;
}
//bool CAccount::AddAllData2Bytes(std::vector<char>& vBuff)
//{
//	AddString2Bytes(vBuff, (char*)strAccountName.c_str());
//	AddString2Bytes(vBuff, (char*)strNickName.c_str());
//	return true;
//}
//
//bool CAccount::DecodeData4Bytes(char* pBuff, int& pos, int len)
//{
//	char strbuff[256] = { 0 };
//	DecodeBytes2String(pBuff, pos, len, strbuff, 128);
//	strAccountName = strbuff;
//	strbuff[0] = 0;
//	DecodeBytes2String(pBuff,pos,len,strbuff,128);
//	strNickName = strbuff;
//	return true;
//}

