#pragma once
#include <vector>
#include <string>
#include "nowin.h"
namespace zlscript
{
	class CSyncScriptPointInterface;
	struct CBaseSyncAttribute
	{
		CBaseSyncAttribute()
		{
			m_flag = 0;
			m_index = 0;
			m_master = nullptr;
		}
		virtual ~CBaseSyncAttribute()
		{

		}
		enum
		{
			E_FLAG_SYNC = 1,
			E_FLAG_DB = 2,
			E_FLAG_DB_PRIMARY = 4,
			E_FLAG_DB_UNIQUE = 8,
		};
		void init(unsigned short flag, unsigned short index,CSyncScriptPointInterface* master);
		virtual void AddData2Bytes(std::vector<char>& vBuff) = 0;
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len) = 0;

		unsigned short m_flag;
		unsigned short m_index;
		CSyncScriptPointInterface* m_master;
	
	};
	struct CSyncCharAttribute : public CBaseSyncAttribute
	{
		char* pPoint;
		virtual void AddData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);
	};
	struct CSyncShortAttribute : public CBaseSyncAttribute
	{
		short* pPoint;
		virtual void AddData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);
	};
	struct CSyncIntAttribute : public CBaseSyncAttribute
	{
		int m_val;
		operator int();
		int operator =(int& val);
		virtual void AddData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);
	};
	struct CSyncInt64Attribute : public CBaseSyncAttribute
	{
		__int64 m_val;
		operator __int64();
		__int64 operator =(__int64& val);
		virtual void AddData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);
	};
	struct CSyncFloatAttribute : public CBaseSyncAttribute
	{
		float m_val;
		operator float();
		float operator =(float& val);
		virtual void AddData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);
	};
	struct CSyncDoubleAttribute : public CBaseSyncAttribute
	{
		double m_val;
		operator double();
		double operator =(double& val);
		virtual void AddData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);
	};
	struct CSyncStringAttribute : public CBaseSyncAttribute
	{
		static const int s_maxStrLen;
		std::string m_val;
		operator std::string&();
		std::string& operator =(std::string& val);
		std::string& operator =(char* val);
		std::string& operator =(const char* val);
		const char* c_str();
		virtual void AddData2Bytes(std::vector<char>& vBuff);
		virtual bool DecodeData4Bytes(char* pBuff, int& pos, unsigned int len);
	};
	//class CSyncAttributeMake
	//{
	//public:
	//	CSyncAttributeMake();
	//	template<typename T>
	//	CBaseSyncAttribute* Create(T &val);
	//	void Remove(CBaseSyncAttribute* pAttr);
	//public:
	//	static CSyncAttributeMake* GetInstance()
	//	{
	//		return &s_Instance;
	//	}
	//private:
	//	static CSyncAttributeMake s_Instance;
	//};
}