#include "Account.h"
#include "zByteArray.h"
#include "md5.h"

CAccount::CAccount()
{
}

CAccount::~CAccount()
{

}

void CAccount::Init2Script()
{
	RegisterClassType("CAccount", CAccount);

}

int CAccount::SetAccountName2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strAccountName = pState->GetStringVarFormStack(0).c_str();

	return ECALLBACK_FINISH;
}

int CAccount::GetAccountName2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->SetResult(strAccountName.c_str());
	return ECALLBACK_FINISH;
}

int CAccount::SetPassword2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strPassword = pState->GetStringVarFormStack(0).c_str();

	return ECALLBACK_FINISH;
}

int CAccount::GetPassword2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->SetResult(strPassword.c_str());
	return ECALLBACK_FINISH;
}

int CAccount::SetNickName2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	strNickName = pState->GetStringVarFormStack(0).c_str();

	return ECALLBACK_FINISH;
}

int CAccount::GetNickName2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}

	pState->SetResult(strNickName.c_str());
	return ECALLBACK_FINISH;
}

int CAccount::GetVal2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string str = pState->GetStringVarFormStack(0);

	auto it = m_mapData.find(str);
	if (it != m_mapData.end())
	{
		pState->SetResult(it->second);
		return ECALLBACK_FINISH;
	}
	return ECALLBACK_FINISH;
}

int CAccount::SetVal2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string str = pState->GetStringVarFormStack(0);
	m_mapData[str] = pState->GetVarFormStack(1);

	return ECALLBACK_FINISH;
}
int CAccount::MakeConnectString2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	tagByteArray vBuff;
	AddInt642Bytes(vBuff, time(nullptr));
	AddInt2Bytes(vBuff, rand());
	MD5 md5(&vBuff[0], vBuff.size());

	pState->SetResult(md5.toString().c_str());
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

