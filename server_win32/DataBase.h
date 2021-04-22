﻿#pragma once
#define NO_SQL
#include "ZLScript.h"
#include "ScriptPointInterface.h"
#include "winsock2i.h"
#ifndef NO_SQL
#include <mysql.h>
#endif
#include <unordered_map>
#include <unordered_set>
using namespace zlscript;


class CDataBase : public CScriptPointInterface/*,public IClassAttributeObserver*/
{
public:
	CDataBase();
	~CDataBase();
public:
	static void Init2Script();

	int InitDB2Script(CScriptRunState* pState);
	int InitTable2Script(CScriptRunState* pState);
	int Query2Script(CScriptRunState* pState);
	int QueryArray2Script(CScriptRunState* pState);
	int Save2Script(CScriptRunState* pState);
	int Delete2Script(CScriptRunState* pState);
	int Ping2Script(CScriptRunState* pState);
	int Close2Scipt(CScriptRunState* pState);
public:
	virtual void ChangeScriptAttribute(CBaseScriptClassAttribute* pAttr, StackVarInfo& old);
	virtual void RegisterScriptAttribute(CBaseScriptClassAttribute* pAttr);
	virtual void RemoveScriptAttribute(CBaseScriptClassAttribute* pAttr);

public:
	//下面函数不考虑线程安全，调用时要注意
	void Save2DB(CScriptPointInterface *pPoint);
	bool Delete2DB(CScriptPointInterface* pPoint);
private:
#ifndef NO_SQL
	MYSQL mysql;
#endif
	std::mutex m_Lock;

private:
	struct stIndex
	{
		bool bInit{ false };
		std::unordered_set<PointVarInfo,hash_PV> setIndex;
	};
	//缓存
	stIndex Query4Cache(std::string className, std::string attrName, StackVarInfo val);
	void Add2Cache(CScriptPointInterface *pPoint);
	void Remove2Cache(CScriptPointInterface* pPoint);
	void SetInit2Cache(std::string className, std::string attrName, StackVarInfo val);

	stIndex Query4Sql(std::string className, std::string attrName, StackVarInfo val);
private:
	//typedef std::set<__int64> SetIndex;

	typedef std::unordered_map<StackVarInfo, stIndex, hash_SV> MapValueToIndexs;
	typedef std::unordered_map<std::string, MapValueToIndexs> MapAttributeNameValue;
	std::unordered_map<std::string, MapAttributeNameValue> m_mapQueryCache;
	
	std::mutex m_QueryLock;
};
