#include "stdAfx.h"
#include "ClientThread.h"

CClientThread::CClientThread()
{
	memset(m_szServerIP, 0, _DEC_SERVER_ADDR_LEN);
	m_nServerPort	= 0;
	m_pClientSocket	= NULL;
}

CClientThread::~CClientThread(void)
{
	StopSocketThread();
	SAFE_DELETE(m_pClientSocket);
}

void CClientThread::SetServerInfo(char* pszSeverIP, int nServerPort)
{
	if ((NULL != pszSeverIP) && (0 < nServerPort))
	{
		if (_DEC_SERVER_ADDR_LEN > strlen(pszSeverIP))
		{
			strcpy(m_szServerIP, pszSeverIP);
		}

		m_nServerPort = nServerPort;
	}
}

void CClientThread::SetProcCallbakcInfo(void* pObject, ProcCallback ProcCallbackFunc)
{
	if (NULL != m_pClientSocket)
	{
		m_pClientSocket->SetProcCallbakcInfo(pObject, ProcCallbackFunc);
	}
}

bool CClientThread::InitSocketThread(bool bRunSendThread /* = true */, bool bRunRecvThread /* = true */)
{
	SAFE_DELETE(m_pClientSocket);

	m_pClientSocket = new CClientSocket();
	if (NULL == m_pClientSocket)
	{
		return false;
	}

	if ((0 == strlen(m_szServerIP)) || (0 >= m_nServerPort))
	{
		return false;
	}
	
	if (false == m_pClientSocket->Init(m_szServerIP, m_nServerPort))
	{
		return false;
	}

	m_pClientSocket->SetSocketThread(this);

    return CSocketThread::InitSocketThread(bRunSendThread, bRunRecvThread);

$ERROR:
	m_bRunSendThread	= false;
	m_bRunRecvThread	= false;
	SAFE_DELETE(m_pClientSocket);
	return false;
}