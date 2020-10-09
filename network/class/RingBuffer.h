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
#include <atomic>
#include <mutex>

 //固定大小的字节的环形缓冲区,不加锁，仅限于一进一出
//TODO 将用于连接对象上，连接对象将修改成固定大小，并能通过一大块整内存缓存
template<unsigned int MaxSize>
class CharRingBuffer
{
public:
	CharRingBuffer()
	{
		nHeadPos = 0;
		nLastPos = 0;

		m_nSize = 0;
	}
	~CharRingBuffer()
	{

	}

public:
	//如果写入的数据大于空余空间，那么不会写入并且返回false
	virtual bool Push(const char* pBuff, unsigned int nSize)
	{
		if (nSize > GetEmptySize())
			return false;
		if (MaxSize >= nLastPos + nSize)
		{
			memcpy(&m_Buffer[nLastPos], pBuff, nSize);
		}
		else
		{
			int nCopySize = MaxSize - nLastPos;
			memcpy(&m_Buffer[nLastPos], pBuff, nCopySize);

			memcpy(&m_Buffer[0], pBuff + nCopySize, nSize - nCopySize);
		}

		nLastPos += nSize;
		while (nLastPos >= MaxSize)
		{
			nLastPos -= MaxSize;
		}
		m_nSize += nSize;
		return true;
	}
	//如果想要获取的数据大于已有数据，那么不会取出并且返回false
	virtual bool Get(std::vector<char>& vOut, unsigned int nSize)
	{
		if (nSize == 0)
		{
			vOut.clear();
			return true;
		}
		if (nSize > m_nSize)
		{
			return false;
		}

		vOut.resize(nSize);


		if (MaxSize >= nHeadPos + nSize)
		{
			memcpy(&vOut[0], &m_Buffer[nHeadPos], nSize);
		}
		else
		{
			int nCopySize = MaxSize - nHeadPos;
			memcpy(&vOut[0], &m_Buffer[nHeadPos], nCopySize);

			memcpy(&vOut[nCopySize], &m_Buffer[0], nSize - nCopySize);
		}
		nHeadPos += nSize;
		while (nHeadPos >= MaxSize)
		{
			nHeadPos -= MaxSize;
		}
		m_nSize -= nSize;
		return true;
	}
	//如果想要获取的数据大于已有数据，那么会取出所有
	virtual bool Get2(std::vector<char>& vOut, unsigned int nSize)
	{
		int nCurSize = m_nSize;
		if (nSize < nCurSize)
		{
			nCurSize = nSize;
		}
		if (nCurSize == 0)
		{
			vOut.clear();
			return false;
		}
		vOut.resize(nCurSize);


		if (MaxSize >= nHeadPos + nCurSize)
		{
			memcpy(&vOut[0], &m_Buffer[nHeadPos], nCurSize);
		}
		else
		{
			int nCopySize = MaxSize - nHeadPos;
			memcpy(&vOut[0], &m_Buffer[nHeadPos], nCopySize);

			memcpy(&vOut[nCopySize], &m_Buffer[0], nCurSize - nCopySize);
		}
		nHeadPos += nCurSize;
		while (nHeadPos >= MaxSize)
		{
			nHeadPos -= MaxSize;
		}
		m_nSize -= nCurSize;
		return true;
	}
	//如果想要删除的数据大于已有数据，会删除所有已有数据
	//bool Pop(unsigned int nSize);

	virtual void Clear()
	{
		nHeadPos = 0;
		nLastPos = 0;

		m_nSize = 0;
	}

	unsigned int GetSize()
	{
		return m_nSize;
	}
	unsigned int GetEmptySize()
	{
		return MaxSize - m_nSize;
	}
protected:

	std::atomic_uint m_nSize;
	//指向第一个数据
	unsigned int nHeadPos;
	//指向第一个空数据
	unsigned int nLastPos;

	char m_Buffer[MaxSize];
};

//字节的环形缓冲区,不加锁，仅限于一进一出
class CByteRingBuffer
{
public:
	CByteRingBuffer();
	CByteRingBuffer(unsigned int nSize);
	~CByteRingBuffer();

	void SetMaxSize(unsigned int nSize);
public:
	//如果写入的数据大于空余空间，那么不会写入并且返回false
	virtual bool Push(const char* pBuff, unsigned int nSize);
	//如果想要获取的数据大于已有数据，那么不会取出并且返回false
	virtual bool Get(std::vector<char> &vOut, unsigned int nSize);
	//如果想要获取的数据大于已有数据，那么会取出所有
	virtual bool Get2(std::vector<char>& vOut, unsigned int nSize);
	//如果想要删除的数据大于已有数据，会删除所有已有数据
	//bool Pop(unsigned int nSize);

	virtual void Clear();

	unsigned int GetSize();
	unsigned int GetEmptySize();
protected:

	std::atomic_uint m_nSize;
	//指向第一个数据
	unsigned int nHeadPos;
	//指向第一个空数据
	unsigned int nLastPos;
	
	std::vector<char> m_vBuffer;
};

template <class T>
class CRingBuffer
{
public:
	CRingBuffer(unsigned int nSize);
	~CRingBuffer();

	//如果缓存已满会释放掉这个指针
	virtual void Push(T*);
	//如果缓存为空会new一个
	virtual T* Get();

	unsigned int GetSize();
	unsigned int GetEmptySize();
public:
	std::atomic_uint m_nSize;
	//指向第一个数据
	unsigned int nHeadPos;
	//指向第一个空数据
	unsigned int nLastPos;

	std::vector<T*> m_vBuffer;

	std::mutex m_Lock;
};

template<class T>
inline CRingBuffer<T>::CRingBuffer(unsigned int nSize)
{
	m_vBuffer.resize(nSize,nullptr);
	nHeadPos = 0;
	nLastPos = 0;
	m_nSize = 0;
}

template<class T>
inline CRingBuffer<T>::~CRingBuffer()
{
	for (auto it = m_vBuffer.begin(); it != m_vBuffer.end(); it++)
	{
		T* pPoint = *it;
		if (pPoint)
		{
			delete pPoint;
		}
	}
}

template<class T>
inline void CRingBuffer<T>::Push(T* pPoint)
{
	std::lock_guard<std::mutex> Lock(m_Lock);
	if (pPoint == nullptr)
	{
		return;
	}
	if (m_vBuffer.size() > m_nSize)
	{
		m_vBuffer[nLastPos++] = pPoint;
		if (nLastPos >= m_vBuffer.size())
		{
			nLastPos = 0;
		}
		m_nSize++;
	}
	else
	{
		delete pPoint;
	}
}

template<class T>
inline T* CRingBuffer<T>::Get()
{
	std::lock_guard<std::mutex> Lock(m_Lock);
	if (m_nSize == 0)
	{
		return new T;
	}
	else
	{
		T* pPoint = m_vBuffer[nHeadPos++];
		if (nHeadPos >= m_vBuffer.size())
		{
			nHeadPos = 0;
		}
		m_nSize--;
		return pPoint;
	}
}
