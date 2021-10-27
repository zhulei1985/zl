#include "DataBase.h"
#include "ScriptSuperPointer.h"
#include "SyncScriptPointInterface.h"

CDataBase::CDataBase()
{
#ifndef NO_SQL
	mysql_init(&mysql);
#endif
}

CDataBase::~CDataBase()
{
	m_Lock.lock();
#ifndef NO_SQL
	mysql_close(&mysql);
#endif
	m_Lock.unlock();
}


int CDataBase::InitDB2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string host = pState->GetStringVarFormStack(0);
	std::string acc = pState->GetStringVarFormStack(1);
	std::string pwd = pState->GetStringVarFormStack(2);
	std::string dbname = pState->GetStringVarFormStack(3);
	int port = pState->GetIntVarFormStack(4);
	int bResult = 1;
#ifdef NO_SQL
	bResult = 0;
#else
	if (mysql_real_connect(&mysql, host.c_str(), acc.c_str(), pwd.c_str(), dbname.c_str(), port, NULL, 0) == NULL)
	{
		bResult = 0;
	}
	else
	{
		char value = 1;
		mysql_options(&mysql, MYSQL_OPT_RECONNECT, &value);
	}
#endif
	pState->SetResult((__int64)bResult);
	return ECALLBACK_FINISH;
}

int CDataBase::InitTable2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strSql = "CREATE TABLE IF NOT EXISTS ";
	std::string strClassName = pState->GetStringVarFormStack(0);
	auto& classcache = m_mapQueryCache[strClassName];
	int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
#ifdef NO_SQL
	CScriptSuperPointerMgr::GetInstance()->SetClassDBIdCount(strClassName, 1);
	auto pInfo = CScriptSuperPointerMgr::GetInstance()->GetBaseClassInfo(nClassType);
	if (pInfo)
	{
		auto itAttr = pInfo->mapDicString2ParamInfo.begin();
		for (int index = 0; itAttr != pInfo->mapDicString2ParamInfo.end(); itAttr++)
		{
			auto param = itAttr->second;
			if (param.m_flag & CBaseScriptClassAttribute::E_FLAG_DB)
			{
				if (param.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
				{
				}
				else if (param.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
				{
					auto& attr = classcache.mapAttributeIndex[param.m_strAttrName];
				}
				else if (param.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_INDEX)
				{
					auto& attr = classcache.mapAttributeIndex[param.m_strAttrName];
				}
			}
		}
	}
#else
	auto pInfo = CScriptSuperPointerMgr::GetInstance()->GetBaseClassInfo(nClassType);
	if (pInfo)
	{
		//auto pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
		//if (pMgr)
		//{
			//auto pPoint = pMgr->New(SCRIPT_NO_USED_AUTO_RELEASE);
			//if (pPoint)
			//{
		std::lock_guard<std::mutex> Lock(m_Lock);
		strSql += strClassName;
		strSql += "(";
		auto attributes = pInfo->mapDicString2ParamInfo;
		auto itAttr = attributes.begin();
		for (int nIndex = 0; itAttr != attributes.end(); itAttr++)
		{
			auto attr = itAttr->second;
			if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB == 0)
			{
				continue;
			}
			if (nIndex > 0)
			{
				strSql += ",";
			}
			nIndex++;
			strSql += attr.m_strAttrName;
			strSql += " ";
			strSql += attr.strType;

			if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
			{
				strSql += " NOT NULL PRIMARY KEY";
			}
			else if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
			{
				strSql += " NOT NULL UNIQUE KEY";
			}
			else if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_INDEX)
			{
				strSql += " NOT NULL,INDEX(";
				strSql += attr.m_strAttrName;
				strSql += ")";
			}
		}
		//注意！！！mysql5.7以上会出现only_full_group_by问题，需要在mysql上设置好
		strSql += ") ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4;\n";

		////查询数据
		MYSQL_RES* res;
		MYSQL_ROW row;
		SCRIPT_PRINT("sql", "%s", strSql.c_str());
		int queryResult = mysql_query(&mysql, strSql.c_str());
		if (queryResult == 0)
		{
			strSql = "DESC ";
			strSql += strClassName;
			strSql += ";";
			SCRIPT_PRINT("sql", "%s", strSql.c_str());
			queryResult = mysql_query(&mysql, strSql.c_str());
			if (queryResult == 0)
			{
				//获取结果集
				res = mysql_store_result(&mysql);
				//检查表的字段
				std::map<std::string, std::string> resultDic;
				if (res)
				{
					while (row = mysql_fetch_row(res))
					{
						resultDic[row[0]] = row[1];
					}
				}

				mysql_free_result(res);

				//auto attributes = pPoint->GetDBAttributes();
				itAttr = attributes.begin();
				for (; itAttr != attributes.end(); itAttr++)
				{
					auto attr = itAttr->second;
					if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB == 0)
					{
						continue;
					}
					if (resultDic.find(attr.m_strAttrName) == resultDic.end())
					{
						std::string sqlAddField = "ALTER TABLE ";
						sqlAddField += strClassName;
						sqlAddField += " ADD ";
						sqlAddField += attr.m_strAttrName;
						sqlAddField += " ";
						sqlAddField += attr.strType;

						if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
						{
							sqlAddField += " NOT NULL PRIMARY KEY;";
						}
						else if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
						{
							sqlAddField += "NOT NULL  UNIQUE KEY;";
						}
						else if (attr.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_INDEX)
						{
							sqlAddField += "NOT NULL,";
							sqlAddField += "ADD INDEX ";
							sqlAddField += attr.m_strAttrName;
							sqlAddField += "(";
							sqlAddField += attr.m_strAttrName;
							sqlAddField += "),";
						}
						mysql_query(&mysql, sqlAddField.c_str());
					}
				}
			}
			else
			{
				//错误
				const char* pStrError = mysql_error(&mysql);
				SCRIPT_PRINT("sqlerror", "%s", pStrError);
			}

			//查询自增变量
			strSql = "show table status where Name =\'";
			strSql += strClassName;
			strSql += "\';";
			SCRIPT_PRINT("sql", "%s", strSql.c_str());
			queryResult = mysql_query(&mysql, strSql.c_str());
			if (queryResult == 0)
			{
				//获取结果集
				res = mysql_store_result(&mysql);
				//检查表的字段
				if (res)
				{
					while (row = mysql_fetch_row(res))
					{
						if (row[0] == "Auto_increment")
						{
							__int64 auto_Id = _atoi64(row[1]);
							CScriptSuperPointerMgr::GetInstance()->SetClassDBIdCount(strClassName, auto_Id);
						}
					}
				}

				mysql_free_result(res);
			}
			else
			{
				//错误
				const char* pStrError = mysql_error(&mysql);
				SCRIPT_PRINT("sqlerror", "%s", pStrError);
			}
		}
		else
		{
			//错误
			const char* pStrError = mysql_error(&mysql);
			SCRIPT_PRINT("sqlerror", "%s", pStrError);
		}
		//pMgr->Release(pPoint);
	//}
	}
#endif

	return ECALLBACK_FINISH;
}

int CDataBase::Query2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strClassName = pState->GetStringVarFormStack(0);
	std::string strFieldName = pState->GetStringVarFormStack(1);
	StackVarInfo Val = pState->GetVarFormStack(2);
	auto cache = Query4Sql(strClassName, strFieldName, Val);
	//if (cache.bInit)
	{
		auto it = cache.begin();
		if (it != cache.end())
		{
			pState->SetClassPointResult((*it).pPoint);
		}
	}

	return ECALLBACK_FINISH;
}

int CDataBase::QueryArray2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	int nArrayClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(std::string("CArray"));
	auto pArrayMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nArrayClassType);
	CScriptArray* pArray = nullptr;
	if (pArrayMgr)
	{
		pArray = dynamic_cast<CScriptArray*>(pArrayMgr->New(SCRIPT_NO_USED_AUTO_RELEASE));
	}
	if (pArray)
	{
		std::string strClassName = pState->GetStringVarFormStack(0);
		std::string strFieldName = pState->GetStringVarFormStack(1);
		StackVarInfo Val = pState->GetVarFormStack(2);
		int begin = pState->GetIntVarFormStack(3);
		int size = pState->GetIntVarFormStack(4);
		std::lock_guard<std::mutex> Lock(m_Lock);
		auto cache = Query4Sql(strClassName, strFieldName, Val,begin,size);
		auto it = cache.begin();
		for (; it != cache.end(); it++)
		{
			pArray->GetVars().push_back((*it).pPoint);
		}
	}

	pState->SetClassPointResult(pArray);
	return ECALLBACK_FINISH;
}

int CDataBase::Save2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strHeadSql = "INSERT INTO ";
	std::string strValSql = "VALUE(";
	PointVarInfo pointVar = pState->GetClassPointFormStack(0);
#ifdef NO_SQL
	CScriptBasePointer* pPoint = pointVar.pPoint;
	if (pPoint)
	{
		pPoint->Lock();
		Add2Cache(pPoint->GetPoint());
		pPoint->Unlock();
	}
#else
	bool bOk = false;
	CScriptBasePointer* pPoint = pointVar.pPoint;
	if (pPoint)
	{
		pPoint->Lock();
		const char* pClassName = pPoint->ClassName();
		if (pClassName)
			strHeadSql += pClassName;
		strHeadSql += "(";
		CScriptPointInterface* pClassPoint = dynamic_cast<CScriptPointInterface*>(pPoint->GetPoint());
		if (pClassPoint)
		{
			auto attributes = pClassPoint->GetDBAttributes();
			auto itAttr = attributes.begin();
			for (; itAttr != attributes.end(); itAttr++)
			{
				if (itAttr != attributes.begin())
				{
					strHeadSql += ",";
					strValSql += ",";
				}
				strHeadSql += itAttr->first;
				strValSql += '\'';
				strValSql += itAttr->second->ToString();
				strValSql += '\'';
			}

			bOk = true;
		}
		strHeadSql += ")";
		strValSql += ")";
		pPoint->Unlock();

	}
	if (bOk)
	{
		std::string strSql = strHeadSql + strValSql;
		std::lock_guard<std::mutex> Lock(m_Lock);
		SCRIPT_PRINT("sql", "%s", strSql.c_str());
		int queryResult = mysql_query(&mysql, strSql.c_str());
		if (queryResult != 0)
		{
			//错误
			const char* pStrError = mysql_error(&mysql);
			SCRIPT_PRINT("sqlerror", "%s", pStrError);
		}
		else
		{
			if (pPoint)
			{
				pPoint->Lock();
				Add2Cache(pPoint->GetPoint());
				pPoint->Unlock();
			}
		}
	}
#endif

	return ECALLBACK_FINISH;
}

int CDataBase::Delete2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	PointVarInfo pointVar = pState->GetClassPointFormStack(0);
	bool bOk = false;
	std::string strSql = "DELETE FROM ";
#ifdef NO_SQL
	bOk = true;
	CScriptBasePointer* pPoint = pointVar.pPoint;
	if (pPoint)
	{
		pPoint->Lock();
		Remove2Cache(pPoint->GetPoint());
		pPoint->Unlock();
	}
#else
	CScriptBasePointer* pPoint = pointVar.pPoint;
	if (pPoint)
	{
		pPoint->Lock();
		CScriptPointInterface* pClassPoint = dynamic_cast<CScriptPointInterface*>(pPoint->GetPoint());
		if (pClassPoint)
		{
			strSql += pClassPoint->getClassInfo()->strClassName;
			strSql += " WHERE ";
			auto attributes = pClassPoint->GetDBAttributes();
			auto itAttr = attributes.begin();
			bool bOk = false;
			for (; itAttr != attributes.end(); itAttr++)
			{
				if (itAttr->second->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
				{
					strSql += itAttr->first;
					strSql += "=\'";
					strSql += itAttr->second->ToString();;
					strSql += "\';";
					bOk = true;
					break;
				}
			}
		}
		pPoint->Unlock();
	}
	if (bOk)
	{
		std::lock_guard<std::mutex> Lock(m_Lock);
		SCRIPT_PRINT("sql", "%s", strSql.c_str());
		int queryResult = mysql_query(&mysql, strSql.c_str());
		if (queryResult != 0)
		{
			//错误
			const char* pStrError = mysql_error(&mysql);
			SCRIPT_PRINT("sqlerror", "%s", pStrError);
			bOk = false;
		}
		else
		{
			if (pPoint)
			{
				pPoint->Lock();
				Remove2Cache(pPoint->GetPoint());
				pPoint->Unlock();
			}
		}
	}
#endif
	pState->SetResult((__int64)(bOk ? 1 : 0));
	return ECALLBACK_FINISH;
}

int CDataBase::Ping2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	int nResult = 1;
#ifdef NO_SQL

#else
	if (m_Lock.try_lock())
	{
		nResult = mysql_ping(&mysql);
		m_Lock.unlock();
	}
#endif

	pState->SetResult((__int64)nResult);
	return ECALLBACK_FINISH;
}

int CDataBase::Close2Script(CScriptCallState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::lock_guard<std::mutex> Lock(m_Lock);

#ifdef NO_SQL
#else
	mysql_close(&mysql);
#endif

	return ECALLBACK_FINISH;
}

void CDataBase::ChangeScriptAttribute(CBaseScriptClassAttribute* pAttr, StackVarInfo& old)
{
	if (pAttr == nullptr)
	{
		return;
	}
	if (pAttr->m_pMaster == nullptr)
	{
		return;
	}
	auto itClass = m_mapQueryCache.find(pAttr->m_pMaster->getClassInfo()->strClassName);
	if (itClass == m_mapQueryCache.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，class分类在初始化时就要建好
		return;
	}
	auto& classcache = itClass->second;
	if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
	{
		std::lock_guard<std::mutex> Lock(classcache.m_PrimaryLock);
		classcache.mapCachePrimary[pAttr->ToScriptVal()] = pAttr->m_pMaster->GetScriptPointIndex();
		classcache.mapCachePrimary.erase(old);
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
	{
		auto itAttr = classcache.mapAttributeIndex.find(pAttr->m_strAttrName);
		if (itAttr == classcache.mapAttributeIndex.end())
		{
			//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
			return;
		}
		auto& attrcache = itAttr->second;
		std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);

		stIndex& index = attrcache.mapIndexs[pAttr->ToScriptVal()];
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());

		attrcache.mapIndexs.erase(old);
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_INDEX)
	{
		auto itAttr = classcache.mapAttributeIndex.find(pAttr->m_strAttrName);
		if (itAttr == classcache.mapAttributeIndex.end())
		{
			//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
			return;
		}
		auto& attrcache = itAttr->second;
		std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);

		stIndex& index = attrcache.mapIndexs[pAttr->ToScriptVal()];
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());

		stIndex& oldindex = attrcache.mapIndexs[old];
		oldindex.setIndex.erase(pAttr->m_pMaster->GetScriptPointIndex());
		if (oldindex.setIndex.empty())
		{
			attrcache.mapIndexs.erase(old);
		}
	}
}

void CDataBase::RegisterScriptAttribute(CBaseScriptClassAttribute* pAttr)
{
	if (pAttr == nullptr)
	{
		return;
	}
	if (pAttr->m_pMaster == nullptr)
	{
		return;
	}
	auto itClass = m_mapQueryCache.find(pAttr->m_pMaster->getClassInfo()->strClassName);
	if (itClass == m_mapQueryCache.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，class分类在初始化时就要建好
		return;
	}
	auto& classcache = itClass->second;
	if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
	{
		std::lock_guard<std::mutex> Lock(classcache.m_PrimaryLock);
		classcache.mapCachePrimary[pAttr->ToScriptVal()] = pAttr->m_pMaster->GetScriptPointIndex();
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
	{
		auto itAttr = classcache.mapAttributeIndex.find(pAttr->m_strAttrName);
		if (itAttr == classcache.mapAttributeIndex.end())
		{
			//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
			return;
		}
		auto& attrcache = itAttr->second;
		std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);
		stIndex& index = attrcache.mapIndexs[pAttr->ToScriptVal()];
		index.setIndex.clear();
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_INDEX)
	{
		auto itAttr = classcache.mapAttributeIndex.find(pAttr->m_strAttrName);
		if (itAttr == classcache.mapAttributeIndex.end())
		{
			//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
			return;
		}
		auto& attrcache = itAttr->second;
		std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);
		stIndex& index = attrcache.mapIndexs[pAttr->ToScriptVal()];
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());
	}
}

void CDataBase::RemoveScriptAttribute(CBaseScriptClassAttribute* pAttr)
{
	if (pAttr == nullptr)
	{
		return;
	}
	if (pAttr->m_pMaster == nullptr)
	{
		return;
	}
	auto itClass = m_mapQueryCache.find(pAttr->m_pMaster->getClassInfo()->strClassName);
	if (itClass == m_mapQueryCache.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，class分类在初始化时就要建好
		return;
	}
	auto& classcache = itClass->second;
	if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
	{
		std::lock_guard<std::mutex> Lock(classcache.m_PrimaryLock);

		classcache.mapCachePrimary.erase(pAttr->ToScriptVal());
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
	{
		auto itAttr = classcache.mapAttributeIndex.find(pAttr->m_strAttrName);
		if (itAttr == classcache.mapAttributeIndex.end())
		{
			//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
			return;
		}
		auto& attrcache = itAttr->second;
		std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);
		attrcache.mapIndexs.erase(pAttr->ToScriptVal());
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_INDEX)
	{
		auto itAttr = classcache.mapAttributeIndex.find(pAttr->m_strAttrName);
		if (itAttr == classcache.mapAttributeIndex.end())
		{
			//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
			return;
		}
		auto& attrcache = itAttr->second;
		std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);

		auto indexIt = attrcache.mapIndexs.find(pAttr->ToScriptVal());
		if (indexIt != attrcache.mapIndexs.end())
		{
			indexIt->second.setIndex.erase(pAttr->m_pMaster->GetScriptPointIndex());
			if (indexIt->second.setIndex.empty())
			{
				attrcache.mapIndexs.erase(indexIt);
			}
		}

	}
}

void CDataBase::Save2DB(CScriptPointInterface* pPoint)
{
	if (pPoint == nullptr && pPoint->getClassInfo())
	{
		return;
	}
#ifdef NO_SQL
	Add2Cache(pPoint);
#else
	std::string strHeadSql = "INSERT INTO ";
	std::string strValSql = "VALUE(";
	strHeadSql += pPoint->getClassInfo()->strClassName;
	strHeadSql += "(";

	auto attributes = pPoint->GetDBAttributes();
	auto itAttr = attributes.begin();
	for (; itAttr != attributes.end(); itAttr++)
	{
		if (itAttr != attributes.begin())
		{
			strHeadSql += ",";
			strValSql += ",";
		}
		strHeadSql += itAttr->first;
		strValSql += '\'';
		strValSql += itAttr->second->ToString();
		strValSql += '\'';
	}
	strHeadSql += ")";
	strValSql += ")";

	std::string strSql = strHeadSql + strValSql;
	std::lock_guard<std::mutex> Lock(m_Lock);
	SCRIPT_PRINT("sql", "%s", strSql.c_str());
	int queryResult = mysql_query(&mysql, strSql.c_str());
	if (queryResult != 0)
	{
		//错误
		const char* pStrError = mysql_error(&mysql);
		SCRIPT_PRINT("sqlerror", "%s", pStrError);
	}
	else
	{
		Add2Cache(pPoint);
	}
#endif
}

bool CDataBase::Delete2DB(CScriptPointInterface* pPoint)
{
	if (pPoint == nullptr && pPoint->getClassInfo())
	{
		return false;
	}
	bool bOk = false;
#ifdef NO_SQL
	Remove2Cache(pPoint);
#else
	std::string strSql = "DELETE FROM ";
	strSql += pPoint->getClassInfo()->strClassName;
	strSql += " WHERE ";
	auto attributes = pPoint->GetDBAttributes();
	auto itAttr = attributes.begin();

	for (; itAttr != attributes.end(); itAttr++)
	{
		if (itAttr->second->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
		{
			strSql += itAttr->first;
			strSql += "=\'";
			strSql += itAttr->second->ToString();;
			strSql += "\';";
			bOk = true;
			break;
		}
	}
	if (bOk)
	{
		std::lock_guard<std::mutex> Lock(m_Lock);
		SCRIPT_PRINT("sql", "%s", strSql.c_str());
		int queryResult = mysql_query(&mysql, strSql.c_str());
		if (queryResult != 0)
		{
			//错误
			const char* pStrError = mysql_error(&mysql);
			SCRIPT_PRINT("sqlerror", "%s", pStrError);
			bOk = false;
		}
		else
		{
			Remove2Cache(pPoint);
		}
	}
#endif
	return bOk;
}

PointVarInfo CDataBase::QueryByPrimary(std::string className, StackVarInfo val)
{
	auto itClass = m_mapQueryCache.find(className);
	if (itClass == m_mapQueryCache.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，class分类在初始化时就要建好
		return PointVarInfo();
	}
	auto& classcache = itClass->second;
	std::lock_guard<std::mutex> Lock(classcache.m_PrimaryLock);
	auto it = classcache.mapCachePrimary.find(val);
	if (it != classcache.mapCachePrimary.end())
	{
		return it->second;
	}
	return PointVarInfo();
}

CDataBase::stIndex CDataBase::Query4Cache(std::string className, std::string attrName, StackVarInfo val)
{
	auto itClass = m_mapQueryCache.find(className);
	if (itClass == m_mapQueryCache.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，class分类在初始化时就要建好
		return stIndex();
	}
	auto& classcache = itClass->second;
	auto itAttr = classcache.mapAttributeIndex.find(attrName);
	if (itAttr == classcache.mapAttributeIndex.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
		return stIndex();
	}
	auto& attrcache = itAttr->second;
	std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);

	auto indexIt = attrcache.mapIndexs.find(val);
	if (indexIt != attrcache.mapIndexs.end())
	{
		return indexIt->second;
	}

	return stIndex();
}

void CDataBase::Add2Cache(CScriptPointInterface* pPoint)
{
	if (pPoint == nullptr)
	{
		return;
	}
	//stCache& stCache = m_mapCache[pPoint->GetScriptPointIndex()];
	auto attributes = pPoint->GetDBAttributes();
	auto itAttr = attributes.begin();
	for (; itAttr != attributes.end(); itAttr++)
	{
		auto pAttr = itAttr->second;
		pAttr->AddObserver(this);
	}
}

void CDataBase::Remove2Cache(CScriptPointInterface* pPoint)
{
	if (pPoint == nullptr)
	{
		return;
	}

	auto attributes = pPoint->GetDBAttributes();
	auto itAttr = attributes.begin();
	for (; itAttr != attributes.end(); itAttr++)
	{
		auto pAttr = itAttr->second;

		pAttr->RemoveObserver(this);
	}
}

void CDataBase::SetInit2Cache(std::string className, std::string attrName, StackVarInfo val, bool bInit)
{
	auto itClass = m_mapQueryCache.find(className);
	if (itClass == m_mapQueryCache.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，class分类在初始化时就要建好
		return;
	}
	auto& classcache = itClass->second;
	auto itAttr = classcache.mapAttributeIndex.find(attrName);
	if (itAttr == classcache.mapAttributeIndex.end())
	{
		//TODO 输出错误信息，考虑到多线程性能，属性分类也要在初始化时建好
		return;
	}
	auto& attrcache = itAttr->second;
	std::lock_guard<std::mutex> Lock(attrcache.m_IndexLock);
	stIndex& cache = attrcache.mapIndexs[val];
	cache.bInit = bInit;
}

std::list<PointVarInfo> CDataBase::QueryByIndex(std::string className, stIndex indexs)
{
	std::list<PointVarInfo> list;
	std::lock_guard<std::mutex> Lock(m_QueryLock);
	auto& classcache = m_mapQueryCache[className];
	for (auto it = indexs.setIndex.begin(); it != indexs.setIndex.end(); it++)
	{
		auto itPoint = classcache.mapCachePrimary.find(*it);
		if (itPoint != classcache.mapCachePrimary.end())
		{
			list.push_back(itPoint->second);
		}
	}
	return std::move(list);
}

std::list<PointVarInfo> CDataBase::Query4Sql(std::string className, std::string attrName, StackVarInfo val, unsigned int begin, unsigned int size)
{
	bool bIsPrimary = false;
	bool bIndex = false;
	int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(className);
	auto pInfo = CScriptSuperPointerMgr::GetInstance()->GetBaseClassInfo(nClassType);
	if (pInfo)
	{
		auto it = pInfo->mapDicString2ParamInfo.find(attrName);
		if (!(it->second.m_flag & CBaseScriptClassAttribute::E_FLAG_DB))
		{
			return std::list<PointVarInfo>();
		}
		if (it->second.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
		{
			bIsPrimary = true;
		}
		if (it->second.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_INDEX)
		{
			bIndex = true;
		}
		if (it->second.m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
		{
			bIndex = true;
		}
	}
	std::list<PointVarInfo> list;
	if (bIsPrimary)
	{
		PointVarInfo point = QueryByPrimary(className, val);
		if (point.pPoint == nullptr)
		{
			list = Load4Sql(className, attrName, val, begin, size);
			auto it = list.begin();
			if (it != list.end())
			{
				if ((*it).pPoint)
					Add2Cache((*it).pPoint->GetPoint());
			}
		}
		else
		{
			list.push_back(point);
		}
		return std::move(list);
	}
	auto pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);

	stIndex cache = Query4Cache(className, attrName, val);
	if (cache.bInit)
	{
		list = QueryByIndex(className, cache);
	}
	else
	{
		list = Load4Sql(className, attrName, val, begin, size);
		if (bIndex)
			SetInit2Cache(className, attrName, val, true);
		for (auto it = list.begin(); it != list.end(); it++)
		{
			if ((*it).pPoint)
				Add2Cache((*it).pPoint->GetPoint());
		}
		}
	return std::move(list);
	}

std::list<PointVarInfo> CDataBase::Load4Sql(std::string className, std::string attrName, StackVarInfo val, unsigned int begin, unsigned int size)
{
#ifdef NO_SQL
	return std::list<PointVarInfo>();
#else
	std::list<PointVarInfo> list;
	int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(className);
	auto pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
	if (pMgr)
	{
		std::string strSql = "SELECT ";
		{
			auto pInfo = CScriptSuperPointerMgr::GetInstance()->GetBaseClassInfo(nClassType);
			if (pInfo)
			{
				auto itAttr = pInfo->mapDicString2ParamInfo.begin();
				for (int index = 0; itAttr != pInfo->mapDicString2ParamInfo.end(); itAttr++)
				{
					auto param = itAttr->second;
					if (param.m_flag & CBaseScriptClassAttribute::E_FLAG_DB)
					{
						if (index > 0)
						{
							strSql += ",";
						}
						index++;
						strSql += itAttr->first;
					}
				}
			}
		}
		strSql += " FROM ";
		strSql += className;
		strSql += " WHERE ";
		strSql += attrName;
		strSql += " =\'";
		strSql += GetString_StackVar(&val);
		strSql += " \'";
		if (size > 0)
		{
			char strBuff[64] = { 0 };
			sprintf(strBuff, " LIMIT %d,%d", begin, size);
			strSql += strBuff;
		}
		//查询数据
		MYSQL_RES* res;
		MYSQL_ROW row;
		SCRIPT_PRINT("sql", "%s", strSql.c_str());
		std::lock_guard<std::mutex> Lock(m_Lock);
		int queryResult = mysql_query(&mysql, strSql.c_str());
		if (queryResult == 0)
		{
			//获取结果集
			res = mysql_store_result(&mysql);
			//检查表的字段
			while (row = mysql_fetch_row(res))
			{
				auto pPoint = pMgr->New(SCRIPT_NO_USED_AUTO_RELEASE);
				auto attributes = pPoint->GetDBAttributes();
				auto itAttr = attributes.begin();
				bool hasInCache = false;
				for (int i = 0; itAttr != attributes.end(); itAttr++, i++)
				{
					itAttr->second->SetVal(row[i]);
				}
				list.push_back(pPoint->GetScriptPointIndex());
			}
		}
		else
		{
			//错误
			const char* pStrError = mysql_error(&mysql);
			SCRIPT_PRINT("sqlerror", "%s", pStrError);
		}
	}
	return std::move(list);
#endif
}
