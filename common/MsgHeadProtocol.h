#pragma once
/****************************************************************************
	Copyright (c) 2020 ZhuLei
	Email:zhulei1985@foxmail.com

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
 ****************************************************************************/
#include <vector>
#include "RingBuffer.h"

namespace zlnetwork
{
#define MAX_MSG_LEN 32768
	enum E_HEAD_PROTOCOL_TYPE
	{
		E_HEAD_PROTOCOL_WEBSOCKET,
		E_HEAD_PROTOCOL_NONE,
		E_HEAD_PROTOCOL_INNER,
	};
	class CSocketConnector;
	class CBaseHeadProtocol
	{
	public:
		CBaseHeadProtocol();

		virtual int GetType() = 0;

		void SetConnector(CSocketConnector* pConnector)
		{
			m_pConnector = pConnector;
		}
	public:

		enum E_PROCESS_RETURN
		{
			E_RETURN_CONTINUE,//在本轮询中继续处理
			E_RETURN_NEXT,//下个轮询再处理
			E_RETURN_COMPLETE,
			E_RETURN_ERROR,
			E_RETURN_SHAKE_HAND_COMPLETE,//握手完成
		};
		virtual int OnProcess() = 0;

		bool GetData(std::vector<char>&, unsigned int);

		virtual int Ping() = 0;//向客户端发送ping消息
		virtual int SendHead(unsigned int len) = 0;
		virtual void ShakeHand(){}//主动发送握手消息

		virtual unsigned __int64 GetDataLen() = 0;

		virtual void SetServer(bool val) { ; }
		virtual bool IsServer() { return false; }
		virtual void SetPassword(const char* str){}

		virtual __int64 GetLastConnectID() { return 0; }
	protected:
		CSocketConnector* m_pConnector;
		int m_nState;

		std::vector<char> m_vReadTempBuf;
		std::vector<char> m_vReadShakeHandDataBuf;

		CByteRingBuffer m_curMsgData;
	};

	class CWebSocketHeadProtocol : public CBaseHeadProtocol
	{
	public:
		CWebSocketHeadProtocol();

		int GetType()
		{
			return E_HEAD_PROTOCOL_WEBSOCKET;
		}
	public:
		enum E_CONNECT_STATE
		{
			E_CONNECT_INIT,//初始化
			E_CONNECT_SHAKE_HAND,//握手
			E_CONNECT_GET_MSG_HEAD,//等待消息头
			E_CONNECT_GET_MSG_LEN,//等待消息额外长度
			E_CONNECT_GET_MSG_MASK,//获取消息掩码
			E_CONNECT_GET_MSG_BODY,//等待消息体
		};
	public:

		virtual int OnProcess();

		int OnState_Init();
		int OnState_Shake_Hand();
		int OnState_Get_Head();
		int OnState_Get_Len();
		int OnState_Get_Mask();
		int OnState_Get_Data();

		virtual int Ping();
		int SendHead(unsigned int len);

		unsigned __int64 GetDataLen();
	private:
		std::map<std::string, std::string> m_HandFlag;
		char cHeadFlag1;
		char cHeadFlag2;

		bool IsFin();
		char GetOpcode();
		bool HasMask();
		char GetDataLenMode();//0,没有额外长度，1,额外16位长度，2,额外64位长度
		

		unsigned __int64 m_nDataLen;
		char m_Mask[4];
		int m_nMaskIndex;

		unsigned __int64 m_nCurLoadedDataLen;//当前已经读取的长度
	};

	class CNoneHeadProtocol : public CBaseHeadProtocol
	{
	public:
		CNoneHeadProtocol();

		int GetType()
		{
			return E_HEAD_PROTOCOL_NONE;
		}

		virtual int OnProcess();

	public:
		enum E_CONNECT_STATE
		{
			E_CONNECT_INIT,//初始化
			E_CONNECT_SHAKE_HAND,//握手
			E_CONNECT_GET_MSG_LEN,//等待消息头
			E_CONNECT_GET_MSG_BODY,//等待消息体
		};
	public:
		int OnState_Get_Len();

		int OnState_Get_Data();

		virtual int Ping();
		int SendHead(unsigned int len);

		unsigned __int64 GetDataLen();
	public:
		unsigned int m_nDataLen;

		unsigned __int64 m_nCurLoadedDataLen;//当前已经读取的长度
	};

	class CInnerHeadProtocol : public CNoneHeadProtocol
	{
	public:
		CInnerHeadProtocol();

		int GetType()
		{
			return E_HEAD_PROTOCOL_INNER;
		}

		virtual int OnProcess();

		int OnState_Init();
		int OnState_Shake_Hand();
	public:
		virtual void SetServer(bool val)
		{
			bServer = val;
		}
		bool IsServer()
		{
			return bServer;
		}
		void SetPassword(const char* str);
	private:
		bool bServer;
		std::string m_strPassword;

		int nMD5StringLen;
		std::string strMD5String;

		__int64 nLastConnectID;
	};

	class CHeadProtocolMgr
	{
	public:
		CHeadProtocolMgr();
		~CHeadProtocolMgr();
	public:
		CBaseHeadProtocol* Create(int nType);
		void Remove(CBaseHeadProtocol* pProtocol);
	public:
		static CHeadProtocolMgr* GetInstance()
		{
			return &s_Instance;
		}
	private:
		static CHeadProtocolMgr s_Instance;
	};
}
