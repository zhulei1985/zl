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
#include<string.h>
#include "RingBuffer.h"

CByteRingBuffer::CByteRingBuffer()
{
	m_vBuffer.resize(4);
	nHeadPos = 0;
	nLastPos = 0;

	m_nSize = 0;
}

CByteRingBuffer::CByteRingBuffer(unsigned int nSize)
{
	m_vBuffer.resize(nSize);

	nHeadPos = 0;
	nLastPos = 0;

	m_nSize = 0;
}

CByteRingBuffer::~CByteRingBuffer()
{
}

void CByteRingBuffer::SetMaxSize(unsigned int nSize)
{
	if (nSize > m_vBuffer.size())
		m_vBuffer.resize(nSize);
}

bool CByteRingBuffer::Push(const char* pBuff, unsigned int nSize)
{
	if (nSize > GetEmptySize())
		return false;
	if (m_vBuffer.size() >= nLastPos + nSize)
	{
		memcpy(&m_vBuffer[nLastPos], pBuff, nSize);
	}
	else
	{
		int nCopySize = m_vBuffer.size() - nLastPos;
		memcpy(&m_vBuffer[nLastPos], pBuff, nCopySize);

		memcpy(&m_vBuffer[0], pBuff+ nCopySize, nSize-nCopySize);
	}

	nLastPos += nSize;
	while (nLastPos >= m_vBuffer.size())
	{
		nLastPos -= m_vBuffer.size();
	}
	m_nSize += nSize;
	return true;
}

bool CByteRingBuffer::Get(std::vector<char>& vOut, unsigned int nSize)
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


	if (m_vBuffer.size() >= nHeadPos + nSize)
	{
		memcpy(&vOut[0],&m_vBuffer[nHeadPos], nSize);
	}
	else
	{
		int nCopySize = m_vBuffer.size() - nHeadPos;
		memcpy(&vOut[0], &m_vBuffer[nHeadPos], nCopySize);

		memcpy(&vOut[nCopySize], &m_vBuffer[0], nSize-nCopySize);
	}
	nHeadPos += nSize;
	while (nHeadPos >= m_vBuffer.size())
	{
		nHeadPos -= m_vBuffer.size();
	}
	m_nSize -= nSize;
	return true;
}

bool CByteRingBuffer::Get2(std::vector<char>& vOut, unsigned int nSize)
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


	if (m_vBuffer.size() >= nHeadPos + nCurSize)
	{
		memcpy(&vOut[0], &m_vBuffer[nHeadPos], nCurSize);
	}
	else
	{
		int nCopySize = m_vBuffer.size() - nHeadPos;
		memcpy(&vOut[0], &m_vBuffer[nHeadPos], nCopySize);

		memcpy(&vOut[nCopySize], &m_vBuffer[0], nCurSize - nCopySize);
	}
	nHeadPos += nCurSize;
	while (nHeadPos >= m_vBuffer.size())
	{
		nHeadPos -= m_vBuffer.size();
	}
	m_nSize -= nCurSize;
	return true;
}

//bool CRingBuffer::Pop(unsigned int nSize)
//{
//	unsigned int nCurSize = m_nSize;
//	if (nSize < nCurSize)
//	{
//		nCurSize = nSize;
//	}
//	if (nCurSize == 0)
//	{
//		return false;
//	}
//	nHeadPos += nCurSize;
//	m_nSize -= nCurSize;
//
//	while (nHeadPos > m_vBuffer.size())
//	{
//		nHeadPos -= m_vBuffer.size();
//	}
//	return true;
//}

void CByteRingBuffer::Clear()
{
	nHeadPos = 0;
	nLastPos = 0;

	m_nSize = 0;
}

unsigned int CByteRingBuffer::GetSize()
{
	return m_nSize;
}

unsigned int CByteRingBuffer::GetEmptySize()
{
	return m_vBuffer.size() - m_nSize;
}
