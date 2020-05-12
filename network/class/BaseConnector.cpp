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

#include "BaseConnector.h"
#include "zByteArray.h"
namespace zlnetwork
{
	CConnectorDataBuffer::CConnectorDataBuffer(unsigned int nSize) :CByteRingBuffer(nSize)
	{

	}
	CConnectorDataBuffer::~CConnectorDataBuffer()
	{
		auto it = m_ListChain.begin();
		while (it != m_ListChain.end())
		{
			if (it->pData == nullptr)
			{
				it = m_ListChain.erase(it);
			}
			else
			{
				delete it->pData;
				it = m_ListChain.erase(it);
			}
		}
	}

	bool CConnectorDataBuffer::Push(const char* pBuff, unsigned int nSize)
	{
		if (!AddData2ChainIfChainNoEmpty(pBuff, nSize))
		{
			//当链表没有数据时，才能往ringbuffer里填入数据，不然会导致顺序错乱
			if (!CByteRingBuffer::Push(pBuff, nSize))
			{
				//压入失败，说明缓存空间不足，数据放入链表
				AddData2Chain(pBuff, nSize);
			}
		}
		return true;
	}

	bool CConnectorDataBuffer::Get(std::vector<char>& vOut, unsigned int nSize)
	{
		if (CByteRingBuffer::Get(vOut, nSize))
		{
			//取出了一些数据，尝试将链表中的数据放入一些到ringbuffer里
			PushChainData();
			return true;
		}
		return false;
	}

	bool CConnectorDataBuffer::Get2(std::vector<char>& vOut, unsigned int nSize)
	{
		bool bResult = false;
		if (CByteRingBuffer::Get2(vOut, nSize))
		{
			bResult = true;
		}
		//尝试将链表中的数据放入一些到ringbuffer里
		PushChainData();
		return bResult;
	}

	void CConnectorDataBuffer::AddData2Chain(const char* pBuff, unsigned int nSize)
	{
		std::lock_guard<std::mutex> Lock(m_LockChain);

		tagData data;
		data.pData = new char[nSize];
		data.nSize = nSize;
		m_ListChain.push_back(data);
	}

	bool CConnectorDataBuffer::AddData2ChainIfChainNoEmpty(const char* pBuff, unsigned int nSize)
	{
		std::lock_guard<std::mutex> Lock(m_LockChain);
		if (m_ListChain.empty())
			return false;

		tagData data;
		data.pData = new char[nSize];
		data.nSize = nSize;
		m_ListChain.push_back(data);
		return true;
	}

	void CConnectorDataBuffer::PushChainData()
	{
		std::lock_guard<std::mutex> Lock(m_LockChain);
		if (m_ListChain.empty())
			return;

		auto it = m_ListChain.begin();
		while (it != m_ListChain.end())
		{
			if (it->pData == nullptr)
			{
				it = m_ListChain.erase(it);
			}
			else if (CByteRingBuffer::Push(it->pData, it->nSize))
			{
				delete it->pData;
				it = m_ListChain.erase(it);
			}
			else
			{
				break;
			}
		}

	}
	std::atomic_ullong CBaseConnector::m_nAllotConnectID(0);

	CBaseConnector::CBaseConnector() :m_Buffer_Send(32768), m_Buffer_Recv(32768)
	{
		m_nID = 0;
		m_nWaitRecvDataLen = 0;
	}
	CBaseConnector::~CBaseConnector()
	{
	}
	void CBaseConnector::OnInit()
	{
		SetID(++m_nAllotConnectID);
	}
	void CBaseConnector::OnDestroy()
	{
		m_nID = 0;
	}
	void CBaseConnector::SendData(const char* pData, unsigned int nSize)
	{
		m_Buffer_Send.Push(pData, nSize);
	}
	bool CBaseConnector::GetData(std::vector<char>& vOut, unsigned int nSize)
	{
		//if (m_nWaitRecvDataLen == 0)
		//{
		//	//获取数据段大小
		//	if (!m_Buffer_Recv.Get(vOut, 2))
		//	{
		//		return false;
		//	}
		//	int nPos = 0;
		//	m_nWaitRecvDataLen = DecodeBytes2Short(&vOut[0], nPos, vOut.size());
		//}

		//if (m_Buffer_Recv.Get(vOut, m_nWaitRecvDataLen))
		//{
		//	return true;
		//}
		
		return m_Buffer_Recv.Get(vOut, nSize);
	}
	bool CBaseConnector::GetData2(std::vector<char>& vOut, unsigned int nSize)
	{
		return m_Buffer_Recv.Get2(vOut, nSize);
	}
}