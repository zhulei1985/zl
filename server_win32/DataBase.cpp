#include "DataBase.h"
#include "SyncScriptPointInterface.h"
CDataBase::CDataBase()
{
	RegisterClassFun(InitDB, this, &CDataBase::InitDB2Script);
	RegisterClassFun(InitTable, this, &CDataBase::InitTable2Script);
	RegisterClassFun(Query, this, &CDataBase::Query2Script);
	RegisterClassFun(Save, this, &CDataBase::Save2Script);
	RegisterClassFun(Ping, this, &CDataBase::Ping2Script);
	RegisterClassFun(Close, this, &CDataBase::Close2Scipt);

	mysql_init(&mysql);

	mysql_options(&mysql, MYSQL_OPT_RECONNECT,nullptr);
}

CDataBase::~CDataBase()
{
	m_Lock.lock();
	mysql_close(&mysql);
	m_Lock.unlock();
}

void CDataBase::Init2Script()
{
	RegisterClassType("CDataBase", CDataBase);

	RegisterClassFun1("InitDB", CDataBase);
	RegisterClassFun1("InitTable", CDataBase);
	RegisterClassFun1("Query", CDataBase);
	RegisterClassFun1("Save", CDataBase);
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
	if (mysql_real_connect(&mysql, host.c_str(), acc.c_str(), pwd.c_str(), dbname.c_str(), port, NULL, 0) == NULL)
	{
		bResult = 0;
	}

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
	int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
	auto pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
	if (pMgr)
	{
		auto pPoint = pMgr->New();
		if (pPoint)
		{
			std::lock_guard<std::mutex> Lock(m_Lock);
			strSql += strClassName;
			strSql += "(";
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
			strSql += ") ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8mb4;";
			strSql += "DESC ";
			strSql += strClassName;
			strSql += ";";
			//查询数据
			MYSQL_RES* res;
			MYSQL_ROW row;
			mysql_query(&mysql, strSql.c_str());
			//获取结果集
			res = mysql_store_result(&mysql);
			//检查表的字段
			std::map<std::string, std::string> resultDic;
			while (row = mysql_fetch_row(res))
			{
				resultDic[row[0]] = row[1];
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

			pMgr->Release(pPoint);
		}
	}

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
	std::string strVal = pState->PopCharVarFormStack();
	int nClassType = CScriptSuperPointerMgr::GetInstance()->GetClassType(strClassName);
	auto pMgr = CScriptSuperPointerMgr::GetInstance()->GetClassMgr(nClassType);
	if (pMgr)
	{
		std::lock_guard<std::mutex> Lock(m_Lock);
		auto pPoint = pMgr->New();
		std::string strSql = "SELECT ";
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
		strSql += " FROM ";
		strSql += strClassName;
		strSql += " WHERE ";
		strSql += strFieldName;
		strSql += " =\'";
		strSql += strVal;
		strSql += " \'";
		//查询数据
		MYSQL_RES* res;
		MYSQL_ROW row;
		mysql_query(&mysql, strSql.c_str());
		//获取结果集
		res = mysql_store_result(&mysql);
		//检查表的字段
		if (row = mysql_fetch_row(res))
		{
			itAttr = attributes.begin();
			for (int i = 0; itAttr != attributes.end(); itAttr++,i++)
			{
				itAttr->second->SetVal(row[i]);
			}
		}
		else
		{
			//如果查询不成功
			pMgr->Release(pPoint);
			pPoint = nullptr;
		}
		mysql_free_result(res);		

		pState->ClearFunParam();
		pState->PushClassPointToStack(pPoint);
		return ECALLBACK_FINISH;
	}
	pState->ClearFunParam();
	pState->PushEmptyVarToStack();
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
	__int64 index = pState->PopClassPointFormStack();
	bool bOk = false;
	CScriptBasePointer* pPoint = CScriptSuperPointerMgr::GetInstance()->PickupPointer(index);
	if (pPoint)
	{
		pPoint->Lock();
		const char *pClassName = pPoint->ClassName();
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
		CScriptSuperPointerMgr::GetInstance()->ReturnPointer(pPoint);
	}
	if (bOk)
	{
		std::string strSql = strHeadSql + strValSql;
		std::lock_guard<std::mutex> Lock(m_Lock);
		mysql_query(&mysql, strSql.c_str());
	}
	pState->ClearFunParam();
	return ECALLBACK_FINISH;
}

int CDataBase::Ping2Script(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	if (m_Lock.try_lock())
	{
		mysql_ping(&mysql);
		m_Lock.unlock();
	}
	return ECALLBACK_FINISH;
}

int CDataBase::Close2Scipt(CScriptRunState* pState)
{
	if (pState == nullptr)
	{
		return ECALLBACK_ERROR;
	}
	std::lock_guard<std::mutex> Lock(m_Lock);
	
	mysql_close(&mysql);
	return ECALLBACK_FINISH;
}
