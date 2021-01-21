#include "SyncScriptPointInterface.h"
#include "zByteArray.h"
#include "SyncAttributes.h"

namespace zlscript
{
	void CBaseSyncAttribute::init(unsigned short flag, unsigned short index, CSyncScriptPointInterface* master)
	{
		m_flag = flag;
		m_index = index;
		m_master = master;
	}
	CSyncIntAttribute::operator int()
	{
		return m_val;
	}
	int CSyncIntAttribute::operator =(int& val)
	{
		m_val = val;
		if (m_master)
		{
			if (m_flag & E_FLAG_SYNC)
				m_master->ChangeSyncAttibute(this);
			if (m_flag & E_FLAG_DB)
				m_master->ChangeDBAttibute(this);
		}
		return m_val;
	}
	void CSyncIntAttribute::AddData2Bytes(std::vector<char>& vBuff)
	{
		AddInt2Bytes(vBuff, m_val);
	}
	bool CSyncIntAttribute::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len)
	{
		m_val = DecodeBytes2Int(pBuff, pos, len);
		return true;
	}

	CSyncInt64Attribute::operator __int64()
	{
		return m_val;
	}

	__int64 CSyncInt64Attribute::operator=(__int64& val)
	{
		m_val = val;
		if (m_master)
		{
			if (m_flag & E_FLAG_SYNC)
				m_master->ChangeSyncAttibute(this);
			if (m_flag & E_FLAG_DB)
				m_master->ChangeDBAttibute(this);
		}
		return m_val;
	}

	void CSyncInt64Attribute::AddData2Bytes(std::vector<char>& vBuff)
	{
		AddInt642Bytes(vBuff, m_val);
	}

	bool CSyncInt64Attribute::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len)
	{
		m_val = DecodeBytes2Int64(pBuff, pos, len);
		return true;
	}

	CSyncFloatAttribute::operator float()
	{
		return m_val;
	}

	float CSyncFloatAttribute::operator=(float& val)
	{
		m_val = val;
		if (m_master)
		{
			if (m_flag & E_FLAG_SYNC)
				m_master->ChangeSyncAttibute(this);
			if (m_flag & E_FLAG_DB)
				m_master->ChangeDBAttibute(this);
		}
		return m_val;
	}

	void CSyncFloatAttribute::AddData2Bytes(std::vector<char>& vBuff)
	{
		AddFloat2Bytes(vBuff, m_val);
	}

	bool CSyncFloatAttribute::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len)
	{
		m_val = DecodeBytes2Float(pBuff, pos, len);
		return true;
	}

	CSyncDoubleAttribute::operator double()
	{
		return m_val;
	}

	double CSyncDoubleAttribute::operator=(double& val)
	{
		m_val = val;
		if (m_master)
		{
			if (m_flag & E_FLAG_SYNC)
				m_master->ChangeSyncAttibute(this);
			if (m_flag & E_FLAG_DB)
				m_master->ChangeDBAttibute(this);
		}
		return m_val;
	}

	void CSyncDoubleAttribute::AddData2Bytes(std::vector<char>& vBuff)
	{
		AddDouble2Bytes(vBuff, m_val);
	}

	bool CSyncDoubleAttribute::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len)
	{
		m_val = DecodeBytes2Double(pBuff, pos, len);
		return true;
	}
	const int CSyncStringAttribute::s_maxStrLen = 1024;

	CSyncStringAttribute::operator std::string&()
	{
		return m_val;
	}

	std::string& CSyncStringAttribute::operator=(std::string& val)
	{
		m_val = val;
		if (m_master)
		{
			if (m_flag & E_FLAG_SYNC)
				m_master->ChangeSyncAttibute(this);
			if (m_flag & E_FLAG_DB)
				m_master->ChangeDBAttibute(this);
		}
		return m_val;
	}

	std::string& CSyncStringAttribute::operator=(char* val)
	{
		m_val = val;
		if (m_master)
		{
			if (m_flag & E_FLAG_SYNC)
				m_master->ChangeSyncAttibute(this);
			if (m_flag & E_FLAG_DB)
				m_master->ChangeDBAttibute(this);
		}
		return m_val;
	}

	std::string& CSyncStringAttribute::operator=(const char* val)
	{
		m_val = val;
		if (m_master)
		{
			if (m_flag & E_FLAG_SYNC)
				m_master->ChangeSyncAttibute(this);
			if (m_flag & E_FLAG_DB)
				m_master->ChangeDBAttibute(this);
		}
		return m_val;
	}

	const char* CSyncStringAttribute::c_str()
	{
		return m_val.c_str();
	}

	void CSyncStringAttribute::AddData2Bytes(std::vector<char>& vBuff)
	{
		AddString2Bytes(vBuff, m_val.c_str());
	}

	bool CSyncStringAttribute::DecodeData4Bytes(char* pBuff, int& pos, unsigned int len)
	{
		//TODO 以后加入缓存
		char* strbuff = new char[s_maxStrLen];

		bool bResult = DecodeBytes2String(pBuff, pos, len, strbuff, s_maxStrLen);
		if (bResult)
		{
			m_val = strbuff;
		}
		delete [] strbuff;
		return bResult;
	}

	//CSyncAttributeMake CSyncAttributeMake::s_Instance;
	//CSyncAttributeMake::CSyncAttributeMake()
	//{
	//}

}