#pragma once

//！！！！！！！！！线程安全性
//将解决：目前的互斥方案不是很好，
//没能彻底将读写线程和主线程互斥开

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



//见SetMsgRebuildBuf
//#include "Winsock2.h""

//关于iocompletionport   ioc 的使用
//使用ioc的CConnecttorWithQueue最好使用静态分配(再次被使用前，最好也先“冷却”一下)
//	原因：关闭socket的时候，内核中可能还有接收发送在使用对象中的发送，接收缓冲，
//		  关闭后，内核可能有一点延迟才能中止 以上处理
//
//发送线程如果发现没有后续的消息需要发送，就会中断发送过程
//外部请经常调用CheckSendReq,来激活发送（同时可以判断socket状态是否正常）

//1。用静态Connecttor结构（自增长的静态），这样可以去掉WaitClose队列		X
//2。判断socket是否关闭，直接用WSASend发送长度0的包判断是否关闭			X
//3。启动发送，直接调用WSASend应该就可以，不用post						X
//4。增加一个判定，收到packet判断dwkey是否==当前socket
//	(创建时dwKey=socket,如果不等于，说明socket重用了，并且保证同一块内存
//	被复用的时候的socket不能==上次的socket)
//		
//	
//关闭socket时发生的事情：
//	1。假设内核正在接收，那么它会结束当前接收，但有可能对buffer读写				
//		解决：	前提：系统的判断应该很快，不超过1秒
//		colddown
//		2。假设内核刚刚发送了一个io完成包，这个包将在后续被线程处理					X
//		（但由于socket已经关闭，所以必须避免错误将消息当作当前socket的消息）	
//		解决：	如果是重用，重用前判断是否与上次的socket一致， 如果一致，取消
//		发送接收线程中已经判断了dwKey==pConnecttor->socket

//可能以上考虑有错误
//以前 启动接收的做法是：Post一个消息到完成队列
//现在改成直接 使用WSARecv启动接收
//这样之后 好像closesocket后，内核能确认关闭 与socket相关的完成请求和完成消息
//需要测试
//	去掉静态分配 和 关闭延迟

//还是保持静态分配 dwKey由原来的socket句柄改成 一个随机值


//[2004/05/12]
//关闭socket时发生的事情：
//	1。假设内核正在接收，那么立即结束当前接收，
//		colddown
//MSDN:
//Any pending overlapped send and receive operations 
//(WSASend/WSASendTo/WSARecv/WSARecvFrom with an overlapped socket) 
//issued by any thread in this process are also canceled. Any event, completion routine, or completion port action specified for these overlapped operations is performed. The pending 
//overlapped operations fail with the error status WSA_OPERATION_ABORTED. 

//	2。假设内核刚刚发送了一个io完成包，这个包将在后续被线程处理					
//			重用Connecttor(静态分配)
//			使用key来区分是否时本Connecttor处理的packet


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

#define MAXMSGERRORCOUNT					5    //收到的错误消息序列最大数（指合法消息）

//一个消息最大10M
#define MSG_DATASIZE_TOBIG 10485760
#define CONNECTDATA_MAXSIZE 1048576
#define BUFFER_SIZE 16384
#define IOCP_BUFFER_SIZE 8192
//无消息接收，超时时间
#define TIMEOUT_NO_RECVDATA 600000
class CSocketConnector;
struct CBaseMsg;
class CDataQueue;

typedef CSocketConnector* (*C_CREATESOCKET)(SOCKET ,SOCKADDR_IN& );

struct SOVERLAPPEDPLUS
{
  //ol必须是第一个定义
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
	//压缩方法
	enum enumCompressMode
	{
		//一般，比如：50字节以下不压缩
		compressmode_normal		
		//必须压缩
		,compressmode_must
		//不需要压缩
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
	static LONG m_lActiveThreadNum;//活动线程数

	//监听端口们
	struct tagListen
	{
		tagListen()
		{
			m_nPort = 0;
			m_sListen = INVALID_SOCKET;
			m_pFun = NULL;
		}
		int m_nPort;//端口
		SOCKET m_sListen;//监听socket
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
	SOCKET      s;      // 对应的套接字句柄 
	sockaddr_in addr;   // 对方的地址

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
//和客户端的连接
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
  //最后一次收发消息时间
  long m_nLastActionTime;
  //long m_nLastRecvMsgTime;

//以下是操作重叠端口的部分
private://这是提供给IOCP的数据收发
	//int UpdataSendData(unsigned int iSentBytes);

	bool Recv(char *pBytes,int nLen);
  bool Recv(SOVERLAPPEDPLUS *pPlus);
 
private:

	//int		m_iRecvSize;
	//char m_szRecvBuf[IOCP_BUFFER_SIZE];//提供给iocp接受消息的缓存

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
	//保证同一时刻只有一个WSASend
	long	volatile	m_lHaveSentSendReq;
  long	volatile	m_lHaveSentThreadSafeReq;

  long volatile m_lSendDataCount;
  long volatile m_lHasPutRecvDataCount;
  long m_lCurRecvDataCount;
  ////确保安全机制
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
  ////接收到的数据，没有组成包
  //int m_iRecvRestSize;
  ////将接受到的字节组成消息的缓存
  //char m_sRecvRest[ BUFFER_SIZE ];
  std::vector<char> m_vRecvHeadCache;
  int m_nRecvDataSizeInQueue;
  int m_nNeedRecvData;

  //发送，保存上次发送调用未发送完的数据
  int	m_iSendRestSize;
  int m_nSendDataSizeInQueue;
  //剩余数据的起始位置
  char m_sSendRest[IOCP_BUFFER_SIZE];
private:

	//连接器实例重用的情况下
	//连接器特殊码，和指针一起标示是否是
	//被连接的完成消息
	DWORD		m_dwConnecttorSpecialKey;
protected:


	friend void __cdecl SendRecvFunc(LPVOID lpParam);

//以下是性能监测所需代码
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
