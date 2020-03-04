#pragma once

//�������������������̰߳�ȫ��
//�������Ŀǰ�Ļ��ⷽ�����Ǻܺã�
//û�ܳ��׽���д�̺߳����̻߳��⿪

/*/
MainThread,SendRecvThread
MainThread:
	GetMsg
	SendMsg
	Close
	Reset
	CheckSendReq

SendRecvThread
	PrepareSendBuf
	PushBuf2Msg
	
/*/



//��SetMsgRebuildBuf
//#include "Winsock2.h""

//����iocompletionport   ioc ��ʹ��
//ʹ��ioc��CConnecttorWithQueue���ʹ�þ�̬����(�ٴα�ʹ��ǰ�����Ҳ�ȡ���ȴ��һ��)
//	ԭ�򣺹ر�socket��ʱ���ں��п��ܻ��н��շ�����ʹ�ö����еķ��ͣ����ջ��壬
//		  �رպ��ں˿�����һ���ӳٲ�����ֹ ���ϴ���
//
//�����߳��������û�к�������Ϣ��Ҫ���ͣ��ͻ��жϷ��͹���
//�ⲿ�뾭������CheckSendReq,������ͣ�ͬʱ�����ж�socket״̬�Ƿ�������

//1���þ�̬Connecttor�ṹ���������ľ�̬������������ȥ��WaitClose����		X
//2���ж�socket�Ƿ�رգ�ֱ����WSASend���ͳ���0�İ��ж��Ƿ�ر�			X
//3���������ͣ�ֱ�ӵ���WSASendӦ�þͿ��ԣ�����post						X
//4������һ���ж����յ�packet�ж�dwkey�Ƿ�==��ǰsocket
//	(����ʱdwKey=socket,��������ڣ�˵��socket�����ˣ����ұ�֤ͬһ���ڴ�
//	�����õ�ʱ���socket����==�ϴε�socket)
//		
//	
//�ر�socketʱ���������飺
//	1�������ں����ڽ��գ���ô���������ǰ���գ����п��ܶ�buffer��д				
//		�����	ǰ�᣺ϵͳ���ж�Ӧ�úܿ죬������1��
//		colddown
//		2�������ں˸ոշ�����һ��io��ɰ�����������ں������̴߳���					X
//		��������socket�Ѿ��رգ����Ա�����������Ϣ������ǰsocket����Ϣ��	
//		�����	��������ã�����ǰ�ж��Ƿ����ϴε�socketһ�£� ���һ�£�ȡ��
//		���ͽ����߳����Ѿ��ж���dwKey==pConnecttor->socket

//�������Ͽ����д���
//��ǰ �������յ������ǣ�Postһ����Ϣ����ɶ���
//���ڸĳ�ֱ�� ʹ��WSARecv��������
//����֮�� ����closesocket���ں���ȷ�Ϲر� ��socket��ص��������������Ϣ
//��Ҫ����
//	ȥ����̬���� �� �ر��ӳ�

//���Ǳ��־�̬���� dwKey��ԭ����socket����ĳ� һ�����ֵ


//[2004/05/12]
//�ر�socketʱ���������飺
//	1�������ں����ڽ��գ���ô����������ǰ���գ�
//		colddown
//MSDN:
//Any pending overlapped send and receive operations 
//(WSASend/WSASendTo/WSARecv/WSARecvFrom with an overlapped socket) 
//issued by any thread in this process are also canceled. Any event, completion routine, or completion port action specified for these overlapped operations is performed. The pending 
//overlapped operations fail with the error status WSA_OPERATION_ABORTED. 

//	2�������ں˸ոշ�����һ��io��ɰ�����������ں������̴߳���					
//			����Connecttor(��̬����)
//			ʹ��key�������Ƿ�ʱ��Connecttor�����packet


#include "winsock2i.h"
#include <mswsock.h>
#include "DataQueue.h"
#include "Single.h"
#include "ThreadBase.h"
#include <map>
#include <list>
#include <vector>
#include "Encrypt.h"
#include "Allocator.h"
#include "OrderList.h"
#include <mutex>
//#include "LockPtr.h"

#define MAXMSGERRORCOUNT					5    //�յ��Ĵ�����Ϣ�����������ָ�Ϸ���Ϣ��

//һ����Ϣ���10M
#define MSG_DATASIZE_TOBIG 10485760
#define CONNECTDATA_MAXSIZE 1048576
#define BUFFER_SIZE 16384
#define IOCP_BUFFER_SIZE 8192
//����Ϣ���գ���ʱʱ��
#define TIMEOUT_NO_RECVDATA 600000
class CSocketConnector;
struct CBaseMsg;
class CDataQueue;

typedef CSocketConnector* (*C_CREATESOCKET)(SOCKET ,SOCKADDR_IN& );

struct SOVERLAPPEDPLUS
{
  //ol�����ǵ�һ������
  OVERLAPPED	ol;
  WSABUF		wsabuf;
  CSocketConnector* pConnecttor;
  int	iOpCode;
  char data[IOCP_BUFFER_SIZE];
  long nDataIndex;
#define OP_READ   1 
#define OP_WRITE 2 
#define OP_STOPTHREAD 3
public:
  SOVERLAPPEDPLUS();
  ~SOVERLAPPEDPLUS();
  void init()
  {
    wsabuf.buf = data;
    wsabuf.len = 0;
    nDataIndex = 0;
    pConnecttor = NULL;
    iOpCode = 0;
    memset( &ol,0,sizeof(ol) );
  }
  void clear()
  {
  }
  long GetID()
  {
    return nDataIndex;
  }
public:
};

class CIOCPConnector : public CThreadBase
{
public:
	CIOCPConnector();
	virtual ~CIOCPConnector();
	//ѹ������
	enum enumCompressMode
	{
		//һ�㣬���磺50�ֽ����²�ѹ��
		compressmode_normal		
		//����ѹ��
		,compressmode_must
		//����Ҫѹ��
		,compressmode_neednot
	};

public:
	bool SocketStartUp();
	void SocketCleanUp();

	virtual	void	OnInit();
	virtual bool	OnProcess();
	virtual void	OnDestroy();

	bool StartIoCompletionPort(int iThreadNum = -1);
	void StopIoCompletionPort();

public:
	static LONG GetActiveThreadNum() { return m_lActiveThreadNum; }
	static void SetActiveThreadNum(LONG val) { m_lActiveThreadNum = val; }

	static HANDLE GetHIoCompletionPort() { return hIoCompletionPort; }
	static void SetHIoCompletionPort(HANDLE val) { hIoCompletionPort = val; }

	void SetListen(int nPort,C_CREATESOCKET pFun);
	//void SetPort(int nval)
	//{
	//	m_nPort = nval;
	//}
	//int GetConnectorCount() const { return m_nConnectorCount; }
	//void SetConnectorCount(int val) { m_nConnectorCount = val; }

	bool GetRun() const { return m_bRun; }
	void SetRun(bool val) { m_bRun = val; }
protected:

	static HANDLE hIoCompletionPort;
	static LONG m_lActiveThreadNum;//��߳���

	//�����˿���
	struct tagListen
	{
		tagListen()
		{
			m_nPort = 0;
			m_sListen = INVALID_SOCKET;
			m_pFun = NULL;
		}
		int m_nPort;//�˿�
		SOCKET m_sListen;//����socket
		C_CREATESOCKET m_pFun;
	};
	std::map<int,tagListen> m_mapListen;
  std::mutex	m_xListenCtrl;
	//typedef std::map<int,CSocketConnector*> MapConnectors;
	//MapConnectors m_mapConnect;
	//std::list<CSocketConnector*> m_listCloseConnect;
	//int m_nConnectorCount;

	bool m_bRun;

public:
  static CAllocator<SOVERLAPPEDPLUS,8192> s_OverlappedCache;
  //static CAllocator<tagOverlappedBuff,32768> s_OverlappedDataBuffCache;
	friend void __cdecl SendRecvFunc(LPVOID lpParam);
};

//SOVERLAPPEDPLUS m_overlappedsend;
typedef struct _PER_HANDLE_DATA 
{ 
	SOCKET      s;      // ��Ӧ���׽��־�� 
	sockaddr_in addr;   // �Է��ĵ�ַ

}PER_HANDLE_DATA, *PPER_HANDLE_DATA;

struct tagConnectInfo
{
	tagConnectInfo()
	{
		memset(strIP,0,sizeof(strIP));
		nPort = 0;
		nServerID = 0;
	}
	char strIP[16];
	int nPort;
	__int64 nServerID;
};
//�Ϳͻ��˵�����
class CSocketConnector
{
public:
	CSocketConnector(char *pPassword=NULL);
	virtual ~CSocketConnector(void);

protected:
	char strIP[16];
	int m_nPort;

	SOCKET m_Socket;

	__int64 m_nID;

	static __int64 s_nIDCount;
public:
	static CSocketConnector* CreateNew(SOCKET sRemote,SOCKADDR_IN& saAdd);
public:
	__int64 GetID() const
	{
		return m_nID;
	}
	void SetID(__int64 val)
	{
		m_nID = val;
	}
	int GetPort() const
	{
		return m_nPort;
	}
	void SetPort(int val)
	{
		m_nPort = val;
	}
	virtual void SetIP(char *);
	virtual char *GetIP();
	SOCKET GetSocket() const { return m_Socket; }
	void SetSocket(SOCKET val) { m_Socket = val; }
	bool IsSocketClosed();

	bool Open();
	virtual void	OnInit();
	virtual bool	OnProcess();
	virtual void	OnDestroy();

	virtual bool	EnableIoCompletionPort();
  virtual bool	CheckOverlappedIOCompleted();
public:
	bool SendData(unsigned char *pBuff,int len);
	char *GetData(int &len);
	bool PopData();
	unsigned int GetDataSize();
public:

	virtual void Close();
	virtual void clear();

	//long GetLastSendMsgTime() const { return m_nLastSendMsgTime; }
	//void SetLastSendMsgTime(long val) { m_nLastSendMsgTime = val; }
	long GetLastActionMsgTime() const { return m_nLastActionTime; }
	void SetLastActionMsgTime(long val) { m_nLastActionTime = val; }

  unsigned int GetSendMsgQueueSize();
  unsigned int GetRecvMsgQueueSize();
protected:
  std::mutex	m_xRecvMsgCtrl;
  CDataQueue m_msgRecvQueue;
  std::mutex	m_xSendMsgCtrl;
  CDataQueue m_msgSendQueue;

  std::mutex	m_xSocketCtrl;
  //���һ���շ���Ϣʱ��
  long m_nLastActionTime;
  //long m_nLastRecvMsgTime;

//�����ǲ����ص��˿ڵĲ���
private://�����ṩ��IOCP�������շ�
	//int UpdataSendData(unsigned int iSentBytes);

	bool Recv(char *pBytes,int nLen);
  bool Recv(SOVERLAPPEDPLUS *pPlus);
 
private:

	//int		m_iRecvSize;
	//char m_szRecvBuf[IOCP_BUFFER_SIZE];//�ṩ��iocp������Ϣ�Ļ���

	unsigned __int64 m_nRecvDataSize;
	unsigned __int64 m_nRecvErrorDataSize;

public:
	void SetEncryptKey(char *pPassword);
	void CancelEncrypt();


private:

	CEncrypt m_Encrypt;
	bool m_bNeedEncrypt;

public:
  long GetHaveSentThreadSafeReq()
  {
    return m_lHaveSentThreadSafeReq;
  }
  void IncHaveSentThreadSafeReq()
  {
    InterlockedIncrement(&m_lHaveSentThreadSafeReq);
  }
  void DecHaveSentThreadSafeReq()
  {
    InterlockedDecrement(&m_lHaveSentThreadSafeReq);
  }
  long GetHasSendReq()
  {
    return m_lHaveSentSendReq;
  }
protected:

  bool SendByOverlapped(const char *pBuff, unsigned int nLen);
  bool EndSendByOverlapped(SOVERLAPPEDPLUS* pPlus);
  bool RecvByOverlapped();
  bool RecvEndByOverlapped(SOVERLAPPEDPLUS* pPlus);
private:
	//��֤ͬһʱ��ֻ��һ��WSASend
	long	volatile	m_lHaveSentSendReq;
  long	volatile	m_lHaveSentThreadSafeReq;

  long volatile m_lSendDataCount;
  long volatile m_lHasPutRecvDataCount;
  long m_lCurRecvDataCount;
  ////ȷ����ȫ����
	//long	volatile	m_lHaveSentPolicyFileRequestReq;
	//SOVERLAPPEDPLUS m_overlappedsend;
	//SOVERLAPPEDPLUS m_overlappedrecv;
  //std::list<SOVERLAPPEDPLUS*> m_listSendOverlapped;
  std::mutex	m_xSendOverlappedCtrl;
  //std::list<SOVERLAPPEDPLUS*> m_listRecvOverlapped;
  //std::map<long,SOVERLAPPEDPLUS*> m_mapRecvOverlapped;
  std::mutex	m_xRecvOverlappedCtrl;
  //CAllocator<SOVERLAPPEDPLUS,16> m_AllocatorOverlapped;

  COrderList<SOVERLAPPEDPLUS> m_listRecvOverlapped;
  ////���յ������ݣ�û����ɰ�
  //int m_iRecvRestSize;
  ////�����ܵ����ֽ������Ϣ�Ļ���
  //char m_sRecvRest[ BUFFER_SIZE ];
  std::vector<char> m_vRecvHeadCache;
  int m_nRecvDataSizeInQueue;
  int m_nNeedRecvData;

  //���ͣ������ϴη��͵���δ�����������
  int	m_iSendRestSize;
  int m_nSendDataSizeInQueue;
  //ʣ�����ݵ���ʼλ��
  char m_sSendRest[IOCP_BUFFER_SIZE];
private:

	//������ʵ�����õ������
	//�����������룬��ָ��һ���ʾ�Ƿ���
	//�����ӵ������Ϣ
	DWORD		m_dwConnecttorSpecialKey;
protected:


	friend void __cdecl SendRecvFunc(LPVOID lpParam);

//���������ܼ���������
//#ifdef _DEBUG
public:
  static std::mutex	s_xRecvDataAmountCtrl;
  static unsigned long s_unRecvDataAmount;
  static std::mutex	s_xSendDataAmountCtrl;
  static unsigned long s_unSendDataAmount;

  static void AddRecvDataAmount(unsigned int nSize);
  static void AddSendDataAmount(unsigned int nSize);
  static unsigned long GetRecvDataAmount();
  static unsigned long GetSendDataAmount();
  static void ClearDataAmount();

  static std::mutex	s_xRecvMsgAmountCtrl;
  static unsigned long s_unRecvMsgAmount;
  static std::mutex	s_xSendMsgAmountCtrl;
  static unsigned long s_unSendMsgAmount;

  static void AddRecvMsgAmount(unsigned int nSize);
  static void AddSendMsgAmount(unsigned int nSize);
  static unsigned long GetRecvMsgAmount();
  static unsigned long GetSendMsgAmount();
  static void ClearMsgAmount();
//#endif
};

#ifdef _DEBUG
#define ADD_RECV_DATA_AMOUNT(nSize) CSocketConnector::AddRecvDataAmount(nSize)
#define ADD_SEND_DATA_AMOUNT(nSize) CSocketConnector::AddSendDataAmount(nSize)
#define GET_RECV_DATA_AMOUNT() CSocketConnector::GetRecvDataAmount()
#define GET_SEND_DATA_AMOUNT() CSocketConnector::GetSendDataAmount()
#define CLEAR_DATA_AMOUNT() CSocketConnector::ClearDataAmount()

#define ADD_RECV_MSG_AMOUNT(nSize) CSocketConnector::AddRecvMsgAmount(nSize)
#define ADD_SEND_MSG_AMOUNT(nSize) CSocketConnector::AddSendMsgAmount(nSize)
#define GET_RECV_MSG_AMOUNT() CSocketConnector::GetRecvMsgAmount()
#define GET_SEND_MSG_AMOUNT() CSocketConnector::GetSendMsgAmount()
#define CLEAR_MSG_AMOUNT() CSocketConnector::ClearMsgAmount()
#else
#define ADD_RECV_DATA_AMOUNT(nSize) CSocketConnector::AddRecvDataAmount(nSize)
#define ADD_SEND_DATA_AMOUNT(nSize) CSocketConnector::AddSendDataAmount(nSize)
#define GET_RECV_DATA_AMOUNT() CSocketConnector::GetRecvDataAmount()
#define GET_SEND_DATA_AMOUNT() CSocketConnector::GetSendDataAmount()
#define CLEAR_DATA_AMOUNT() CSocketConnector::ClearDataAmount()

#define ADD_RECV_MSG_AMOUNT(nSize) CSocketConnector::AddRecvMsgAmount(nSize)
#define ADD_SEND_MSG_AMOUNT(nSize) CSocketConnector::AddSendMsgAmount(nSize)
#define GET_RECV_MSG_AMOUNT() CSocketConnector::GetRecvMsgAmount()
#define GET_SEND_MSG_AMOUNT() CSocketConnector::GetSendMsgAmount()
#define CLEAR_MSG_AMOUNT() CSocketConnector::ClearMsgAmount()
#endif
