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
#include "zByteArray.h"
#include "ScriptConnector.h"
#include "MsgHeadProtocol.h"
#include "sha1.h"
#include "base64.h"
#include "md5.h"
#include <sstream>
namespace zlnetwork
{
	CBaseHeadProtocol::CBaseHeadProtocol() : m_curMsgData(MAX_MSG_LEN)
	{

	}

	bool CBaseHeadProtocol::GetData(std::vector<char>& vBuff, unsigned int len)
	{
		return m_curMsgData.Get(vBuff,len);
	}

	CWebSocketHeadProtocol::CWebSocketHeadProtocol()
	{
		m_nState = E_CONNECT_INIT;
		m_nDataLen = 0;
		m_nCurLoadedDataLen = 0;
	}

	int CWebSocketHeadProtocol::OnProcess()
	{
		if (m_pConnector == nullptr)
		{
			return E_RETURN_ERROR;
		}
		int nResult = E_RETURN_CONTINUE;
		switch (m_nState)
		{
		case E_CONNECT_INIT:
			nResult = this->OnState_Init();
			break;
		case E_CONNECT_SHAKE_HAND:
			nResult = this->OnState_Shake_Hand();
			break;
		case E_CONNECT_GET_MSG_HEAD:
			nResult = this->OnState_Get_Head();
			break;
		case E_CONNECT_GET_MSG_LEN:
			nResult = this->OnState_Get_Len();
			break;
		case E_CONNECT_GET_MSG_MASK:
			nResult = this->OnState_Get_Mask();
			break;
		case E_CONNECT_GET_MSG_BODY:
			nResult = this->OnState_Get_Data();
			break;
		default:
			return E_RETURN_ERROR;
		}
		return nResult;
	}

	int CWebSocketHeadProtocol::OnState_Init()
	{
		m_pConnector->GetData2(m_vReadTempBuf,512);
		unsigned int pos = m_vReadShakeHandDataBuf.size();
		m_vReadShakeHandDataBuf.resize(m_vReadShakeHandDataBuf.size() + m_vReadTempBuf.size());
		memcpy(&m_vReadShakeHandDataBuf[pos], &m_vReadTempBuf[0], m_vReadTempBuf.size());
		m_vReadTempBuf.clear();

		//处理消息
		if (m_vReadShakeHandDataBuf.size() >= 3)
		{
			if (m_vReadShakeHandDataBuf[0] == 'G' && m_vReadShakeHandDataBuf[1] == 'E' && m_vReadShakeHandDataBuf[3] == 'T')
			{
				m_nState = E_CONNECT_SHAKE_HAND;
			}
			else
			{
				return E_RETURN_ERROR;
			}
		}

		return E_RETURN_NEXT;
	}

	int CWebSocketHeadProtocol::OnState_Shake_Hand()
	{
		m_pConnector->GetData2(m_vReadTempBuf, 512);
		unsigned int pos = m_vReadShakeHandDataBuf.size();
		m_vReadShakeHandDataBuf.resize(m_vReadShakeHandDataBuf.size() + m_vReadTempBuf.size());
		memcpy(&m_vReadShakeHandDataBuf[pos], &m_vReadTempBuf[0], m_vReadTempBuf.size());
		m_vReadTempBuf.clear();

		pos = 0;
		std::string strOneSentence;
		std::string strType;
		std::string strVal;
		//unsigned int pos = 0;
		bool bFinish = false;
		unsigned int nFinishPos = 0;
		for (; pos < m_vReadShakeHandDataBuf.size(); pos++)
		{
			if (m_vReadShakeHandDataBuf[pos] == '/n')
			{
				nFinishPos = pos;

				continue;
			}
			else if (m_vReadShakeHandDataBuf[pos] == '/r')
			{
				nFinishPos = pos;
				if (strOneSentence.empty())
				{
					//意味着握手信息接收完毕
					bFinish = true;
				}
				else
				{
					//一句读完
					unsigned int i = 0;
					for (; i < strOneSentence.size(); i++)
					{
						if (strOneSentence[i] == ':')
						{
							i++;
							if (strOneSentence[i] == ' ')
								i++;
							break;
						}
						strType.push_back(strOneSentence[i]);
					}

					for (; i < strOneSentence.size(); i++)
					{
						if (strOneSentence[i] == '\r')
						{
							break;
						}
						else if (strOneSentence[i] == '\n')
						{
							break;
						}
						strVal.push_back(strOneSentence[i]);
					}

					m_HandFlag[strType] = strVal;
				}
			}
			else
			{
				strOneSentence.push_back(m_vReadShakeHandDataBuf[pos]);
			}
		}
		//检测握手信息是否接收完毕
		if (bFinish || (nFinishPos == pos && m_HandFlag.find("Sec-WebSocket-Key") != m_HandFlag.end()))
		{
			std::string result = "HTTP/1.1 101 Switching Protocols\r\n";
			result += "Connection: upgrade\r\n";
			result += "Sec-WebSocket-Accept: ";
			//返回握手信息
			std::string strGUID = m_HandFlag["Sec-WebSocket-Key"] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			SHA1 sha;
			unsigned int message_digest[5];
			sha.Reset();
			sha << strGUID.c_str();
			sha.Result(message_digest);
			for (int i = 0; i < 5; i++) {
				message_digest[i] = htonl(message_digest[i]);
			}
			base64 base;
			result += base.base64_encode(reinterpret_cast<const unsigned char*>(message_digest), 20);
			result += "\r\n";
			result += "Upgrade: websocket\r\n\r\n";
			m_pConnector->SendData(result.c_str(), result.size());
			m_nState = E_CONNECT_GET_MSG_HEAD;
			return E_RETURN_NEXT;
		}
		else
		{
			int nNewSize = m_vReadShakeHandDataBuf.size() - nFinishPos;
			memcpy(&m_vReadShakeHandDataBuf[0], &m_vReadShakeHandDataBuf[pos], nNewSize);
			m_vReadShakeHandDataBuf.resize(nNewSize);
		}
		return E_RETURN_NEXT;
	}

	int CWebSocketHeadProtocol::OnState_Get_Head()
	{
		if (m_pConnector->GetData(m_vReadTempBuf, 2))
		{
			cHeadFlag1 = m_vReadTempBuf[0];
			cHeadFlag2 = m_vReadTempBuf[2];

			m_nCurLoadedDataLen = 0;
			m_nDataLen = 0;
			if (GetDataLenMode() > 0)
			{
				m_nState = E_CONNECT_GET_MSG_LEN;
			}
			else if (HasMask())
			{
				m_nState = E_CONNECT_GET_MSG_MASK;
			}
			else
			{
				m_nState = E_CONNECT_GET_MSG_BODY;
			}

		}
		return E_RETURN_CONTINUE;
	}

	int CWebSocketHeadProtocol::OnState_Get_Len()
	{
		if (GetDataLenMode() == 1)
		{
			if (m_pConnector->GetData(m_vReadTempBuf, 2))
			{
				int nPos = 0;
				m_nDataLen = DecodeBytes2Short(&m_vReadTempBuf[0], nPos, 2);
				m_curMsgData.SetMaxSize(m_nDataLen);
			}
			else
			{
				return E_RETURN_NEXT;
			}
		}
		else if(GetDataLenMode() == 2)
		{
			if (m_pConnector->GetData(m_vReadTempBuf, 8))
			{
				int nPos = 0;
				m_nDataLen = DecodeBytes2Short(&m_vReadTempBuf[0], nPos, 8);
				m_curMsgData.SetMaxSize(m_nDataLen);
			}
			else
			{
				return E_RETURN_NEXT;
			}
		}
		else
		{
			m_nDataLen = cHeadFlag2 & 0x7f;
			m_curMsgData.SetMaxSize(m_nDataLen);
		}
		if (HasMask())
		{
			m_nState = E_CONNECT_GET_MSG_MASK;
		}
		else
		{
			m_nState = E_CONNECT_GET_MSG_BODY;
		}

		return E_RETURN_CONTINUE;
	}

	int CWebSocketHeadProtocol::OnState_Get_Mask()
	{
		m_nMaskIndex = 0;
		if (HasMask())
		{
			if (m_pConnector->GetData(m_vReadTempBuf, 4))
			{
				m_Mask[0] = m_vReadTempBuf[0];
				m_Mask[1] = m_vReadTempBuf[1];
				m_Mask[2] = m_vReadTempBuf[2];
				m_Mask[3] = m_vReadTempBuf[3];
			}
			else
			{
				return E_RETURN_NEXT;
			}
		}

		m_nState = E_CONNECT_GET_MSG_BODY;
		
		return E_RETURN_CONTINUE;
	}

	int CWebSocketHeadProtocol::OnState_Get_Data()
	{

		m_pConnector->GetData2(m_vReadTempBuf, GetDataLen() - m_nCurLoadedDataLen);
		m_nCurLoadedDataLen += m_vReadTempBuf.size();
		//计算掩码
		if (HasMask())
		{
			for (unsigned int i = 0; i < m_vReadTempBuf.size(); i++)
			{
				m_vReadTempBuf[i] = m_vReadTempBuf[i] ^ m_Mask[(m_nMaskIndex++)%4];
			}
		}
		m_curMsgData.Push(&m_vReadTempBuf[0], m_vReadTempBuf.size());
		
		if (IsFin() && m_nCurLoadedDataLen >= GetDataLen())
		{
			return E_RETURN_COMPLETE;
		}
		return E_RETURN_NEXT;
	}

	int CWebSocketHeadProtocol::Ping()
	{
		tagByteArray vBuff;
		vBuff.push_back(0x89);
		vBuff.push_back(0x00);
		m_pConnector->SendData(&vBuff[0], vBuff.size());
		return 0;
	}

	int CWebSocketHeadProtocol::SendHead(unsigned int len)
	{
		tagByteArray vBuff;
		vBuff.push_back(0x80);
		//按搜到的文档说，服务器发送给客户端的数据不能有掩码
		if (len > 0xff)
		{
			vBuff.push_back((char)127);
			AddInt642Bytes(vBuff, len);
		}
		else if (len > 125)
		{
			vBuff.push_back((char)126);
			AddUShort2Bytes(vBuff, (unsigned short)len);
		}
		else
		{
			vBuff.push_back((char)len);
		}
		m_pConnector->SendData(&vBuff[0], vBuff.size());
		return 0;
	}

	bool CWebSocketHeadProtocol::IsFin()
	{
		return cHeadFlag1 >> 7 ? true:false;
	}

	char CWebSocketHeadProtocol::GetOpcode()
	{
		return cHeadFlag1 & 0x0f;
	}

	bool CWebSocketHeadProtocol::HasMask()
	{
		return cHeadFlag2>>7?true:false;
	}

	char CWebSocketHeadProtocol::GetDataLenMode()
	{
		switch (cHeadFlag2 & 0x7f)
		{
		case 126:
			return 1;
		case 127:
			return 2;
		}
		return 0;
	}

	unsigned __int64 CWebSocketHeadProtocol::GetDataLen()
	{
		return m_nDataLen;
	}


	CNoneHeadProtocol::CNoneHeadProtocol()
	{
		m_nState = E_CONNECT_GET_MSG_LEN;
	}

	int CNoneHeadProtocol::OnProcess()
	{
		int nResult = E_RETURN_CONTINUE;
		if (m_nState == E_CONNECT_GET_MSG_LEN)
		{
			nResult = OnState_Get_Len();
		}
		else if (m_nState == E_CONNECT_GET_MSG_BODY)
		{
			nResult = OnState_Get_Data();
		}
		return nResult;
	}

	int CNoneHeadProtocol::OnState_Get_Len()
	{
		int nResult = E_RETURN_NEXT;
		if (m_pConnector->GetData(m_vReadTempBuf, 4))
		{
			int nPos = 0;
			m_nDataLen = DecodeBytes2Int(&m_vReadTempBuf[0], nPos, m_vReadTempBuf.size());
			m_nCurLoadedDataLen = 0;
			nResult = E_RETURN_CONTINUE;
			m_nState = E_CONNECT_GET_MSG_BODY;
		}
		return nResult;
	}

	int CNoneHeadProtocol::OnState_Get_Data()
	{
		m_pConnector->GetData2(m_vReadTempBuf, m_nDataLen - m_nCurLoadedDataLen);
		m_nCurLoadedDataLen += m_vReadTempBuf.size();

		if (m_vReadTempBuf.size() > 0)
			m_curMsgData.Push(&m_vReadTempBuf[0], m_vReadTempBuf.size());

		if (m_nCurLoadedDataLen >= m_nDataLen)
		{
			m_nState = E_CONNECT_GET_MSG_LEN;
			return E_RETURN_COMPLETE;
		}
		return E_RETURN_NEXT;

	}

	int CNoneHeadProtocol::Ping()
	{
		return 0;
	}

	int CNoneHeadProtocol::SendHead(unsigned int len)
	{
		tagByteArray vBuff;
		AddInt2Bytes(vBuff, len);
		m_pConnector->SendData(&vBuff[0], vBuff.size());
		return 0;
	}

	unsigned __int64 CNoneHeadProtocol::GetDataLen()
	{
		return m_nDataLen;
	}

	CInnerHeadProtocol::CInnerHeadProtocol()
	{
		m_nState = E_CONNECT_INIT;
	}

	int CInnerHeadProtocol::OnProcess()
	{
		int nResult = E_RETURN_CONTINUE;
		switch (m_nState)
		{
		case E_CONNECT_INIT://初始化
			nResult = OnState_Init();
			break;
		case E_CONNECT_SHAKE_HAND://握手
			nResult = OnState_Shake_Hand();
			break;
		case E_CONNECT_GET_MSG_LEN:
			nResult = OnState_Get_Len();
			break;
		case E_CONNECT_GET_MSG_BODY:
			nResult = OnState_Get_Data();
			break;
		default:
			nResult = E_RETURN_ERROR;
			break;
		}
		return nResult;
	}

	int CInnerHeadProtocol::OnState_Init()
	{
		nMD5StringLen = -1;
		std::string headword = "zlnetwork";
		if (bServer)
		{
			tagByteArray vBuff;
			for (unsigned int i = 0; i < headword.size(); i++)
			{
				vBuff.push_back(headword.c_str()[i]);
			}
			//AddString2Bytes(vBuff, headword.c_str());
			AddInt2Bytes(vBuff, 1);//版本号
			AddInt642Bytes(vBuff, time(nullptr));
			m_pConnector->SendData(&vBuff[0], vBuff.size());

			AddString2Bytes(vBuff, m_strPassword.c_str());
			MD5 md5(&vBuff[headword.size()], vBuff.size() - headword.size());
			strMD5String.clear();
			strMD5String = md5.toString();
		}
		else
		{
			tagByteArray vBuff;
			if (m_pConnector->GetData(vBuff, headword.size()))
			{
				for (unsigned int i = 0; i < headword.size(); i++)
				{
					if (vBuff[i] != headword.c_str()[i])
					{
						return E_RETURN_ERROR;
					}
				}
			}
			else
			{
				return E_RETURN_NEXT;
			}
		}
		m_nState = E_CONNECT_SHAKE_HAND;
		return E_RETURN_CONTINUE;
	}

	int CInnerHeadProtocol::OnState_Shake_Hand()
	{
		tagByteArray vBuff;
		if (bServer)
		{
			if (nMD5StringLen < 0)
			{
				if (m_pConnector->GetData(vBuff, 4))
				{
					int pos = 0;
					nMD5StringLen = DecodeBytes2Int(&vBuff[0], pos, vBuff.size());
				}
				else
				{
					return E_RETURN_NEXT;
				}
			}
			if (m_pConnector->GetData(vBuff, nMD5StringLen))
			{
				int pos = 0;
				std::string strClientMD5String;
				for (unsigned int i = 0; i < vBuff.size(); i++)
				{
					strClientMD5String.push_back(vBuff[i]);
				}
				if (strClientMD5String == strMD5String)
				{
					m_nState = E_CONNECT_GET_MSG_LEN;
				}
				else
				{
					return E_RETURN_ERROR;
				}
			}
			else
			{
				return E_RETURN_NEXT;
			}
		}
		else
		{
			if (m_pConnector->GetData(vBuff, 12))
			{
				int pos = 0;
				//int nVer = DecodeBytes2Int(&vBuff[0], pos, vBuff.size());
				//__int64 time = DecodeBytes2Int64(&vBuff[0], pos, vBuff.size());
				AddString2Bytes(vBuff, m_strPassword.c_str());
				MD5 md5(&vBuff[0], vBuff.size());

				tagByteArray vReSend;
				AddString2Bytes(vReSend, md5.toString().c_str());
				m_pConnector->SendData(&vReSend[0], vReSend.size());

				m_nState = E_CONNECT_GET_MSG_LEN;
			}
			else
			{
				return E_RETURN_NEXT;
			}
		}
		return E_RETURN_COMPLETE;
	}

	void CInnerHeadProtocol::SetPassword(const char* str)
	{
		if (str)
		{
			m_strPassword = str;
		}
	}


	CHeadProtocolMgr CHeadProtocolMgr::s_Instance;
	CHeadProtocolMgr::CHeadProtocolMgr()
	{
	}
	CHeadProtocolMgr::~CHeadProtocolMgr()
	{
	}
	CBaseHeadProtocol* CHeadProtocolMgr::Create(int nType)
	{
		CBaseHeadProtocol *pProtocol = nullptr;
		switch (nType)
		{
		case E_HEAD_PROTOCOL_WEBSOCKET:
			pProtocol = new CWebSocketHeadProtocol;
			break;
		case E_HEAD_PROTOCOL_NONE:
			pProtocol = new CNoneHeadProtocol;
			break;
		case E_HEAD_PROTOCOL_INNER:
			pProtocol = new CInnerHeadProtocol;
			break;
		}
		return pProtocol;
	}

	void CHeadProtocolMgr::Remove(CBaseHeadProtocol* pProtocol)
	{
		if (pProtocol)
		{
			delete pProtocol;
		}
	}

}

