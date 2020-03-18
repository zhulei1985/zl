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

//�ֽڵĻ��λ�����,��������������һ��һ��
class CByteRingBuffer
{
public:
	CByteRingBuffer(unsigned int nSize);
	~CByteRingBuffer();

public:
	//���д������ݴ��ڿ���ռ䣬��ô����д�벢�ҷ���false
	virtual bool Push(const char* pBuff, unsigned int nSize);
	//�����Ҫ��ȡ�����ݴ����������ݣ���ô����ȡ�����ҷ���false
	virtual bool Get(std::vector<char> &vOut, unsigned int nSize);
	//�����Ҫ��ȡ�����ݴ����������ݣ���ô��ȡ������
	virtual bool Get2(std::vector<char>& vOut, unsigned int nSize);
	//�����Ҫɾ�������ݴ����������ݣ���ɾ��������������
	//bool Pop(unsigned int nSize);

	unsigned int GetSize();
	unsigned int GetEmptySize();
protected:

	std::atomic_uint m_nSize;
	//ָ���һ������
	unsigned int nHeadPos;
	//ָ���һ��������
	unsigned int nLastPos;
	
	std::vector<char> m_vBuffer;
};

template <class T>
class CRingBuffer
{
public:
	CRingBuffer(unsigned int nSize);
	~CRingBuffer();

	//��������������ͷŵ����ָ��
	virtual void Push(T*);
	//�������Ϊ�ջ�newһ��
	virtual T* Get();

	unsigned int GetSize();
	unsigned int GetEmptySize();
public:
	std::atomic_uint m_nSize;
	//ָ���һ������
	unsigned int nHeadPos;
	//ָ���һ��������
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
