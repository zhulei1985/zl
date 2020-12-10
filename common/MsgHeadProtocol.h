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
			E_RETURN_CONTINUE,//�ڱ���ѯ�м�������
			E_RETURN_NEXT,//�¸���ѯ�ٴ���
			E_RETURN_COMPLETE,
			E_RETURN_ERROR,
		};
		virtual int OnProcess() = 0;

		bool GetData(std::vector<char>&, unsigned int);

		virtual int Ping() = 0;//��ͻ��˷���ping��Ϣ
		virtual int SendHead(unsigned int len) = 0;
		virtual void ShakeHand(){}//��������������Ϣ

		virtual unsigned __int64 GetDataLen() = 0;

		virtual void SetServer(bool val) { ; }
		virtual void SetPassword(const char* str){}
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
			E_CONNECT_INIT,//��ʼ��
			E_CONNECT_SHAKE_HAND,//����
			E_CONNECT_GET_MSG_HEAD,//�ȴ���Ϣͷ
			E_CONNECT_GET_MSG_LEN,//�ȴ���Ϣ���ⳤ��
			E_CONNECT_GET_MSG_MASK,//��ȡ��Ϣ����
			E_CONNECT_GET_MSG_BODY,//�ȴ���Ϣ��
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
		char GetDataLenMode();//0,û�ж��ⳤ�ȣ�1,����16λ���ȣ�2,����64λ����
		

		unsigned __int64 m_nDataLen;
		char m_Mask[4];
		int m_nMaskIndex;

		unsigned __int64 m_nCurLoadedDataLen;//��ǰ�Ѿ���ȡ�ĳ���
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
			E_CONNECT_INIT,//��ʼ��
			E_CONNECT_SHAKE_HAND,//����
			E_CONNECT_GET_MSG_LEN,//�ȴ���Ϣͷ
			E_CONNECT_GET_MSG_BODY,//�ȴ���Ϣ��
		};
	public:
		int OnState_Get_Len();

		int OnState_Get_Data();

		virtual int Ping();
		int SendHead(unsigned int len);

		unsigned __int64 GetDataLen();
	public:
		unsigned int m_nDataLen;

		unsigned __int64 m_nCurLoadedDataLen;//��ǰ�Ѿ���ȡ�ĳ���
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
		void SetPassword(const char* str);
	private:
		bool bServer;
		std::string m_strPassword;

		int nMD5StringLen;
		std::string strMD5String;
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
