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
namespace zlnetwork
{
	//�ڻ��λ������Ļ����ϣ�����һ����������������ʱ��������Ĺ���
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
		virtual void OnInit() = 0;
		virtual bool OnProcess() = 0;
		virtual void OnDestroy() = 0;
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

	public:
		//������Ϣ
		virtual void SendData(const char *pData,unsigned int nSize);

		//������Ϣ����
		bool GetData(std::vector<char>& vOut, unsigned int nSize);
		//������Ϣ����
		bool GetData2(std::vector<char>& vOut, unsigned int nSize);
	protected:
		//���ڵȴ����յ���Ϣ����
		unsigned int m_nWaitRecvDataLen;
	protected:
		//���ڻ��潫Ҫ���͵���Ϣ����
		CConnectorDataBuffer m_Buffer_Send;
		//���ڻ����Ѿ����յ�����Ϣ����
		CConnectorDataBuffer m_Buffer_Recv;
	};

}