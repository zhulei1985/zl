#pragma once
enum E_MSG_TYPE
{
	E_RUN_SCRIPT = 64,
	E_RUN_SCRIPT_RETURN,
	//TODO ����ͬ����������ͬ������Ҫ˼��
	//ͬ��������Ҫ��״̬��ͬ���������ݻ���ֻͬ���ı�����
	E_SYNC_CLASS_INFO,//ͬ����״̬
	E_SYNC_CLASS_DATA,//ͬ��һ���������

	E_SYNC_DOWN_PASSAGE,//����ͬ��ͨ��
	E_SYNC_UP_PASSAGE,//����ͬ��ͨ��

	E_ROUTE_FRONT,//���ڿͻ��˷���������
	E_ROUTE_BACK,//���ڷ����������ͻ���
};

class CScriptConnector;
class CBaseScriptConnector;
#include "ZLScript.h"
#include "SyncScriptPointInterface.h"
#include <functional>
using namespace zlscript;

class CBaseMsgReceiveState
{
public:
	~CBaseMsgReceiveState();

	virtual int GetType() = 0;
public:
	virtual void Clear() {}
	virtual bool Recv(CScriptConnector*) = 0;
	virtual bool Send(CBaseScriptConnector*);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient,std::vector<char>& vBuff) = 0;
	//��������Ӻ�·�����Ӷ��п���ʹ��run
	virtual bool Run(CBaseScriptConnector* pClient) = 0;

public:
	void SetGetDataFun(std::function<bool(std::vector<char>&, unsigned int)> fun);
protected:
	//bool CBaseConnector::GetData(std::vector<char>& vOut, unsigned int nSize)
	std::function<bool(std::vector<char>&, unsigned int)> m_GetData;
};
class CScriptMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CScriptMsgReceiveState()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
		nScriptFunNameLen = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_RUN_SCRIPT;
	}
	virtual void Clear()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
		nScriptFunNameLen = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;

		strScriptFunName.clear();
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	//__int64 nEventListIndex;//�ű�ִ������ID
	__int64 nReturnID;//�ű�ִ�з����õ�ID

	std::string strScriptFunName;//�ű���
	CScriptStack m_scriptParm;
protected:
	//�����Ƕ�ȡʱ����ʱ����
	int nScriptFunNameLen;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};

class CReturnMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CReturnMsgReceiveState()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_RUN_SCRIPT_RETURN;
	}
	virtual void Clear()
	{
		//nEventListIndex = -1;
		nReturnID = -1;
		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	//int nEventListIndex;//�ű�ִ������ID
	__int64 nReturnID;//�ű�ִ��״̬��ID
	CScriptStack m_scriptParm;
protected:
	//�����Ƕ�ȡʱ����ʱ����

	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};

class CSyncClassInfoMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncClassInfoMsgReceiveState()
	{
		nClassID = -1;

		nClassNameStringLen = -1;
		nDataLen = -1;
		m_pPoint = nullptr;
	}
	int GetType()
	{
		return E_SYNC_CLASS_INFO;
	}
	virtual void Clear()
	{
		nClassID = -1;

		nRootServerID = -1;
		nRootClassID = -1;
		nTier = -1;

		nClassNameStringLen = -1;
		nDataLen = -1;
		m_pPoint = nullptr;

		strClassName.clear();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	//2020.10.28 ��ͬ����Ϣ��Ҫ�������Ϣ
	int nRootServerID;
	__int64 nRootClassID;
	int nTier;
	std::string strClassName;
	CSyncScriptPointInterface* m_pPoint;
private:
	//�����Ƕ�ȡʱ����ʱ����
	int nClassNameStringLen;
	__int64 nClassID;//�漰������ID
	int nDataLen;
};
class CSyncClassDataReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncClassDataReceiveState()
	{
		nClassID = -1;

		nClassNameStringLen = -1;
		nDataLen = -1;

		strClassName.clear();
	}
	int GetType()
	{
		return E_SYNC_CLASS_DATA;
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	std::string strClassName;
	__int64 nClassID;//��ID
	std::vector<char> vData;//ͬ������
private:
	//�����Ƕ�ȡʱ����ʱ����
	int nClassNameStringLen;

	int nDataLen;
};

class CSyncUpMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncUpMsgReceiveState()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_SYNC_UP_PASSAGE;
	}
	virtual void Clear()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
		strFunName.clear();
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//�漰������ID

	std::string strFunName;
	CScriptStack m_scriptParm;
protected:
	//�����Ƕ�ȡʱ����ʱ����
	int nFunNameStringLen;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};
class CSyncDownMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CSyncDownMsgReceiveState()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
	}
	int GetType()
	{
		return E_SYNC_DOWN_PASSAGE;
	}
	virtual void Clear()
	{
		nClassID = -1;

		nFunNameStringLen = -1;

		nScriptParmNum = -1;
		nCurParmType = -1;
		nStringLen = -1;
		strFunName.clear();
		strClassName.clear();
		while (m_scriptParm.size())
			m_scriptParm.pop();
	}
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);
public:
	__int64 nClassID;//�漰������ID
	std::string strFunName;
	CScriptStack m_scriptParm;

protected:
	//�����Ƕ�ȡʱ����ʱ����
	int nFunNameStringLen;
	int nScriptParmNum;
	int nCurParmType;
	int nStringLen;
	std::string strClassName;
};

class CRouteFrontMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CRouteFrontMsgReceiveState()
	{
		nConnectID = -1;
		nMsgType = 0;
		pState = nullptr;
	}
	int GetType()
	{
		return E_ROUTE_FRONT;
	}
	virtual void Clear();
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	__int64 nConnectID;
	CBaseMsgReceiveState* pState;

protected:
	//�����Ƕ�ȡʱ����ʱ����
	int nMsgType;
};

class CRouteBackMsgReceiveState : public CBaseMsgReceiveState
{
public:
	CRouteBackMsgReceiveState()
	{
		nConnectID = -1;
		nMsgType = 0;
		pState = nullptr;
	}
	int GetType()
	{
		return E_ROUTE_BACK;
	}
	virtual void Clear();
	virtual bool Recv(CScriptConnector*);
	virtual bool Run(CBaseScriptConnector* pClient);
	virtual bool AddAllData2Bytes(CBaseScriptConnector* pClient, std::vector<char>& vBuff);

public:
	__int64 nConnectID;
	CBaseMsgReceiveState* pState;

protected:
	//�����Ƕ�ȡʱ����ʱ����
	int nMsgType;
};

class CMsgReceiveMgr
{
public:
	CBaseMsgReceiveState* CreateRceiveState(char cType);
	void RemoveRceiveState(CBaseMsgReceiveState* pState);
public:
	static CMsgReceiveMgr* GetInstance()
	{
		return &s_Instance;
	}
protected:
	static CMsgReceiveMgr s_Instance;
};