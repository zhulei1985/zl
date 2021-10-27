#pragma once
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

	CLASS_SCRIPT_FUN(CDataBase, InitDB);
	CLASS_SCRIPT_FUN(CDataBase, InitTable);
	CLASS_SCRIPT_FUN(CDataBase, Query);
	CLASS_SCRIPT_FUN(CDataBase, QueryArray);
	CLASS_SCRIPT_FUN(CDataBase, Save);
	CLASS_SCRIPT_FUN(CDataBase, Delete);
	CLASS_SCRIPT_FUN(CDataBase, Ping);
	CLASS_SCRIPT_FUN(CDataBase, Close);
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
		std::unordered_set<StackVarInfo, hash_SV> setIndex;
	};
	//缓存
	PointVarInfo QueryByPrimary(std::string className, StackVarInfo val);
	stIndex Query4Cache(std::string className, std::string attrName, StackVarInfo val);
	void Add2Cache(CScriptPointInterface *pPoint);
	void Remove2Cache(CScriptPointInterface* pPoint);
	void SetInit2Cache(std::string className, std::string attrName, StackVarInfo val, bool bInit);

	std::list<PointVarInfo> QueryByIndex(std::string className, stIndex indexs);

	std::list<PointVarInfo> Query4Sql(std::string className, std::string attrName, StackVarInfo val, unsigned int begin=0, unsigned int size=0);
	//忽略缓存直接从sql读取数据
	std::list<PointVarInfo> Load4Sql(std::string className, std::string attrName, StackVarInfo val, unsigned int begin, unsigned int size);
private:

	struct stAttributeToIndex
	{
		std::mutex m_IndexLock;
		std::unordered_map<StackVarInfo, stIndex, hash_SV> mapIndexs;
	};
	struct stScriptClassCache
	{
		std::mutex m_PrimaryLock;
		std::unordered_map<StackVarInfo, PointVarInfo, hash_SV> mapCachePrimary;//基于主键的缓存
		std::unordered_map<std::string, stAttributeToIndex> mapAttributeIndex;
	};
	std::unordered_map<std::string, stScriptClassCache> m_mapQueryCache;
	
	std::mutex m_QueryLock;
};
