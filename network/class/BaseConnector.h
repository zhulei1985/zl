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
#include "RingBuffer.h"
#include "nowin.h"
#include <list>
#include <mutex>
#include <atomic>
namespace zlnetwork
{
	//在环形缓冲区的基础上，加入一个超出缓冲则建立临时数据链表的功能
	class CConnectorDataBuffer : public CByteRingBuffer
	{
	public:
		CConnectorDataBuffer(unsigned int nSize);
		~CConnectorDataBuffer();

		virtual bool Push(const char* pBuff, unsigned int nSize);
		virtual bool Get(std::vector<char>& vOut, unsigned int nSize);
		virtual bool Get2(std::vector<char>& vOut, unsigned int nSize);

		virtual void AddData2Chain(const char* pBuff, unsigned int nSize);
		virtual bool AddData2ChainIfChainNoEmpty(const char* pBuff, unsigned int nSize);
		virtual void PushChainData();

	protected:
		struct tagData
		{
			char* pData;
			unsigned int nSize;
		};
		std::list<tagData> m_ListChain;
		std::mutex m_LockChain;
	};
	class CBaseConnector
	{
	public:
		CBaseConnector();
		~CBaseConnector();
	public:
		virtual void OnInit();
		virtual bool OnProcess() = 0;
		virtual void OnDestroy();
		virtual bool IsSocketClosed() = 0;
		virtual bool Open() = 0;
		virtual void Close() = 0;
		virtual void clear() = 0;

		virtual int GetPort() = 0;
		virtual void SetPort(int val) = 0;
		virtual void SetIP(const char*) = 0;
		virtual char* GetIP() = 0;

		virtual bool CheckOverlappedIOCompleted() = 0;
	public:
		__int64 GetID() const
		{
			return m_nID;
		}
		void SetID(__int64 val)
		{
			m_nID = val;
		}

	protected:
		__int64 m_nID;

		static std::atomic_ullong m_nAllotConnectID;
	public:
		//发送消息
		virtual void SendData(const char *pData,unsigned int nSize);

		//处理消息接收,如果实际缓存里数据小于nSize，返回错误并且不会动缓存里的数据
		bool GetData(std::vector<char>& vOut, unsigned int nSize);
		//处理消息接收，能取多少数据取多少，最大值不会大于nSize
		bool GetData2(std::vector<char>& vOut, unsigned int nSize);
	protected:
		//正在等待接收的消息长度
		unsigned int m_nWaitRecvDataLen;
	protected:
		//用于缓存将要发送的消息数据
		CConnectorDataBuffer m_Buffer_Send;
		//用于缓存已经接收到的消息数据
		CConnectorDataBuffer m_Buffer_Recv;
	};

}