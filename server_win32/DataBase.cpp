#include "DataBase.h"
#include "SyncScriptPointInterface.h"

CDataBase::CDataBase()
{
	AddClassObject(this->GetScriptPointIndex(), this);

	RegisterClassFun(InitDB, this, &CDataBase::InitDB2Script);
	RegisterClassFun(InitTable, this, &CDataBase::InitTable2Script);
	RegisterClassFun(Query, this, &CDataBase::Query2Script);
	RegisterClassFun(QueryArray, this, &CDataBase::QueryArray2Script);
	RegisterClassFun(Save, this, &CDataBase::Save2Script);
	RegisterClassFun(Delete, this, &CDataBase::Delete2Script);
	RegisterClassFun(Ping, this, &CDataBase::Ping2Script);
	RegisterClassFun(Close, this, &CDataBase::Close2Scipt);
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

void CDataBase::Init2Script()
{
	RegisterClassType("CDataBase", CDataBase);

	RegisterClassFun1("InitDB", CDataBase);
	RegisterClassFun1("InitTable", CDataBase);
	RegisterClassFun1("Query", CDataBase);
	RegisterClassFun1("QueryArray", CDataBase);
	RegisterClassFun1("Save", CDataBase);
	RegisterClassFun1("Delete", CDataBase);
	RegisterClassFun1("Ping", CDataBase);
	RegisterClassFun1("Close", CDataBase);
}


int CDataBase::InitDB2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string host = pState->PopCharVarFormStack();
	std::string acc = pState->PopCharVarFormStack();
	std::string pwd = pState->PopCharVarFormStack();
	std::string dbname = pState->PopCharVarFormStack();
	int port = pState->PopIntVarFormStack();
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
	pState->ClearFunParam();
	pState->PushVarToStack(bResult);
	return ECALLBACK_FINISH;
}

int CDataBase::InitTable2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strSql = "CREATE TABLE IF NOT EXISTS ";
	std::string strClassName = pState->PopCharVarFormStack();
#ifdef NO_SQL
	CScriptSuperPointerMgr::GetInstance()->SetClassDBIdCount(strClassName, 1);
#else
	int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
	auto pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
	if (pMgr)
	{
		auto pPoint = pMgr->New(SCRIPT_NO_USED_AUTO_RELEASE);
		if (pPoint)
		{
			std::lock_guard<std::mutex> Lock(m_Lock);
			strSql += strClassName;
			strSql += "(";
			auto attributes = pPoint->GetDBAttributes();
			auto itAttr = attributes.begin();
			for (; itAttr != attributes.end(); itAttr++)
			{
				auto pAttr = itAttr->second;
				if (itAttr != attributes.begin())
				{
					strSql += ",";
				}
				strSql += itAttr->first;
				strSql += " ";
				strSql += pAttr->ToType();

				if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
				{
					strSql += " NOT NULL PRIMARY KEY";
				}
				else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
				{
					strSql += " NOT NULL UNIQUE KEY";
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
						auto pAttr = itAttr->second;
						if (resultDic.find(itAttr->first) == resultDic.end())
						{
							std::string sqlAddField = "ALTER TABLE ";
							sqlAddField += strClassName;
							sqlAddField += " ADD ";
							sqlAddField += itAttr->first;
							sqlAddField += " ";
							if (pAttr->ToType() == "TEXT")
							{
								if ((pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
									| pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
								{
									sqlAddField += "VARCHAR(128)";
								}
							}
							else
							{
								sqlAddField += pAttr->ToType();
							}
							if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
							{
								sqlAddField += " NOT NULL PRIMARY KEY";
							}
							else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
							{
								sqlAddField += " UNIQUE";
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
			pMgr->Release(pPoint);
		}
	}
#endif
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

int CDataBase::Query2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strClassName = pState->PopCharVarFormStack();
	std::string strFieldName = pState->PopCharVarFormStack();
	StackVarInfo Val = pState->PopVarFormStack();
	auto cache = Query4Sql(strClassName, strFieldName, Val);
	//if (cache.bInit)
	{
		pState->ClearFunParam();
		auto it = cache.setIndex.begin();
		if (it != cache.setIndex.end())
		{
			pState->PushClassPointToStack((*it).pPoint);
		}
		else
		{
			pState->PushEmptyVarToStack();
		}
		return ECALLBACK_FINISH;
	}


	pState->ClearFunParam();
	pState->PushEmptyVarToStack();
	return ECALLBACK_FINISH;
}

int CDataBase::QueryArray2Script(CScriptRunState* pState)
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
		std::string strClassName = pState->PopCharVarFormStack();
		std::string strFieldName = pState->PopCharVarFormStack();
		StackVarInfo Val = pState->PopVarFormStack();
		std::lock_guard<std::mutex> Lock(m_Lock);
		auto cache = Query4Sql(strClassName, strFieldName, Val);
		auto it = cache.setIndex.begin();
		for (; it != cache.setIndex.end(); it++)
		{
			pArray->GetVars().push_back((*it).pPoint);
		}
	}
	pState->ClearFunParam();
	pState->PushClassPointToStack(pArray);
	return ECALLBACK_FINISH;
}

int CDataBase::Save2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::string strHeadSql = "INSERT INTO ";
	std::string strValSql = "VALUE(";
	PointVarInfo pointVar = pState->PopClassPointFormStack();
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
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

int CDataBase::Delete2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	PointVarInfo pointVar = pState->PopClassPointFormStack();
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
	pState->ClearFunParam();
	pState->PushVarToStack((__int64)(bOk ? 1 : 0));
	return ECALLBACK_FINISH;
}

int CDataBase::Ping2Script(CScriptRunState* pState)
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
	pState->ClearFunParam();
	pState->PushVarToStack(nResult);
	return ECALLBACK_FINISH;
}

int CDataBase::Close2Scipt(CScriptRunState* pState)
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
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

void CDataBase::ChangeScriptAttribute(CBaseScriptClassAttribute* pAttr, StackVarInfo& old)
{
	std::lock_guard<std::mutex> Lock(m_QueryLock);
	if (pAttr == nullptr)
	{
		return;
	}
	if (pAttr->m_pMaster == nullptr)
	{
		return;
	}
	if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
	{
		auto& classcache = m_mapQueryCache[pAttr->m_pMaster->getClassInfo()->strClassName];
		auto& attrcache = classcache[pAttr->m_strAttrName];
		stIndex& oldindex = attrcache[old];
		//oldindex.setIndex.erase(pAttr->m_pMaster->GetScriptPointIndex());
		//if (oldindex.setIndex.size() == 0)
		{
			attrcache.erase(old);
		}
		stIndex& index = attrcache[pAttr->ToScriptVal()];
		index.setIndex.clear();
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());
		index.bInit = true;
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
	{
		auto& classcache = m_mapQueryCache[pAttr->m_pMaster->getClassInfo()->strClassName];
		auto& attrcache = classcache[pAttr->m_strAttrName];
		stIndex& oldindex = attrcache[old];
		oldindex.setIndex.erase(pAttr->m_pMaster->GetScriptPointIndex());
		if (oldindex.setIndex.empty())
		{
			attrcache.erase(old);
		}
		stIndex& index = attrcache[pAttr->ToScriptVal()];
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());
	}
}

void CDataBase::RegisterScriptAttribute(CBaseScriptClassAttribute* pAttr)
{
	std::lock_guard<std::mutex> Lock(m_QueryLock);
	if (pAttr == nullptr)
	{
		return;
	}
	if (pAttr->m_pMaster == nullptr)
	{
		return;
	}
	if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
	{
		auto& classcache = m_mapQueryCache[pAttr->m_pMaster->getClassInfo()->strClassName];
		auto& attrcache = classcache[pAttr->m_strAttrName];
		stIndex& index = attrcache[pAttr->ToScriptVal()];
		index.setIndex.clear();
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());
		index.bInit = true;
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
	{
		auto& classcache = m_mapQueryCache[pAttr->m_pMaster->getClassInfo()->strClassName];
		auto& attrcache = classcache[pAttr->m_strAttrName];
		stIndex& index = attrcache[pAttr->ToScriptVal()];
		index.setIndex.insert(pAttr->m_pMaster->GetScriptPointIndex());
	}
}

void CDataBase::RemoveScriptAttribute(CBaseScriptClassAttribute* pAttr)
{
	std::lock_guard<std::mutex> Lock(m_QueryLock);
	if (pAttr == nullptr)
	{
		return;
	}
	if (pAttr->m_pMaster == nullptr)
	{
		return;
	}

	if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
	{
		auto classcacheIt = m_mapQueryCache.find(pAttr->m_pMaster->getClassInfo()->strClassName);
		if (classcacheIt != m_mapQueryCache.end())
		{
			auto attrcacheIt = classcacheIt->second.find(pAttr->m_strAttrName);
			if (attrcacheIt != classcacheIt->second.end())
			{
				auto indexIt = attrcacheIt->second.find(pAttr->ToScriptVal());
				if (indexIt != attrcacheIt->second.end())
				{
					attrcacheIt->second.erase(indexIt);
				}
			}
		}
	}
	else if (pAttr->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_UNIQUE)
	{
		auto classcacheIt = m_mapQueryCache.find(pAttr->m_pMaster->getClassInfo()->strClassName);
		if (classcacheIt != m_mapQueryCache.end())
		{
			auto attrcacheIt = classcacheIt->second.find(pAttr->m_strAttrName);
			if (attrcacheIt != classcacheIt->second.end())
			{
				auto indexIt = attrcacheIt->second.find(pAttr->ToScriptVal());
				if (indexIt != attrcacheIt->second.end())
				{
					indexIt->second.setIndex.erase(pAttr->m_pMaster->GetScriptPointIndex());
					if (indexIt->second.setIndex.empty())
					{
						attrcacheIt->second.erase(indexIt);
					}
				}
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

CDataBase::stIndex CDataBase::Query4Cache(std::string className, std::string attrName, StackVarInfo val)
{
	std::lock_guard<std::mutex> Lock(m_QueryLock);
	auto classcacheIt = m_mapQueryCache.find(className);
	if (classcacheIt != m_mapQueryCache.end())
	{
		auto attrcacheIt = classcacheIt->second.find(attrName);
		if (attrcacheIt != classcacheIt->second.end())
		{
			auto indexIt = attrcacheIt->second.find(val);
			if (indexIt != attrcacheIt->second.end())
			{
				return indexIt->second;
			}
		}
	}
	// TODO: 在此处插入 return 语句
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

void CDataBase::SetInit2Cache(std::string className, std::string attrName, StackVarInfo val)
{
	std::lock_guard<std::mutex> Lock(m_QueryLock);
	auto& classcache = m_mapQueryCache[className];
	auto& attrcache = classcache[attrName];
	stIndex& cache = attrcache[val];
	cache.bInit = true;
}

CDataBase::stIndex CDataBase::Query4Sql(std::string className, std::string attrName, StackVarInfo val)
{
	stIndex cache = Query4Cache(className, attrName, val);
	if (!cache.bInit)
	{
#ifdef NO_SQL
		SetInit2Cache(className, attrName, val);
		cache.bInit = true;
#else
		std::unordered_set<StackVarInfo, hash_SV> setCheckHas;
		auto it = cache.setIndex.begin();
		for (; it != cache.setIndex.end(); it++)
		{
			if ((*it).pPoint && (*it).pPoint->GetPoint())
			{
				auto attributes = (*it).pPoint->GetPoint()->GetDBAttributes();
				auto itAttr = attributes.begin();
				for (int i = 0; itAttr != attributes.end(); itAttr++, i++)
				{
					if (itAttr->second->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
					{
						setCheckHas.insert(itAttr->second->ToScriptVal());
					}
				}
			}
		}
		int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(className);
		auto pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
		if (pMgr)
		{
			std::string strSql = "SELECT ";
			{
				auto pPoint = pMgr->New(SCRIPT_NO_USED_AUTO_RELEASE);
				auto attributes = pPoint->GetDBAttributes();
				auto itAttr = attributes.begin();
				for (; itAttr != attributes.end(); itAttr++)
				{
					if (itAttr != attributes.begin())
					{
						strSql += ",";
					}
					strSql += itAttr->first;
				}
				pMgr->Release(pPoint);
				pPoint = nullptr;
			}
			strSql += " FROM ";
			strSql += className;
			strSql += " WHERE ";
			strSql += attrName;
			strSql += " =\'";
			strSql += GetString_StackVar(&val);
			strSql += " \'";
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
						//通过辨认主键检查是否已存在与缓存中
						if (itAttr->second->m_flag & CBaseScriptClassAttribute::E_FLAG_DB_PRIMARY)
						{
							if (setCheckHas.find(itAttr->second->ToScriptVal()) != setCheckHas.end())
							{
								hasInCache = true;
							}
						}
					}
					if (!hasInCache)
					{
						Add2Cache(pPoint);
						cache.setIndex.insert(pPoint->GetScriptPointIndex());
					}
				}

				mysql_free_result(res);
				//设置缓存对应数据为初始化完成
				SetInit2Cache(className, attrName, val);
				cache.bInit = true;
			}
			else
			{
				//错误
				const char* pStrError = mysql_error(&mysql);
				SCRIPT_PRINT("sqlerror", "%s", pStrError);
			}
		}
#endif
	}
	return cache;
}
