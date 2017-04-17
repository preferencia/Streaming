#include "stdAfx.h"
#include "ClientThread.h"

typedef struct _ProcSvcData
{
	int		nSvcCode;
	int     nSvcDataLen;
	char*   pSvcData;
} ProcSvcData;

CClientThread::CClientThread()
{
	memset(m_szServerIP, 0, _DEC_SERVER_ADDR_LEN);
	m_nServerPort				= 0;
	m_pClientSocket				= NULL;
	
	m_hProcSvcDataThread		= NULL;
	m_hProcSvcDataQueueMutex	= NULL;
	
	m_bRunProcSvcDataThread		= false;

	m_pProcSvcDataQueue			= NULL;

	m_pObject					= NULL;
	m_ProcCallbackFunc			= NULL;
}

CClientThread::~CClientThread(void)
{
	StopSocketThread();

	//SAFE_DELETE(m_pClientSocket);

	m_ProcCallbackFunc	= NULL;
	m_pObject			= NULL;
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
	m_pObject			= pObject;
	m_ProcCallbackFunc	= ProcCallbackFunc;
}

bool CClientThread::InitSocketThread()
{
	m_hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == m_hComPort)
	{
		return false;
	}

	SAFE_DELETE(m_pClientSocket);

	m_pClientSocket = new CClientSocket();
	if (NULL == m_pClientSocket)
	{
		return false;
	}

	m_pClientSocket->SetSocketThread(this);

	if ((0 == strlen(m_szServerIP)) || (0 >= m_nServerPort))
	{
		return false;
	}
	
	if (false == m_pClientSocket->Init(m_szServerIP, m_nServerPort))
	{
		return false;
	}

	SAFE_DELETE(m_pProcSvcDataQueue);

	m_pProcSvcDataQueue = new ProcSvcDataQueue;
	m_pProcSvcDataQueue->clear();

#ifdef _WINDOWS
	m_hProcSvcDataQueueMutex = CreateMutex(NULL, FALSE, NULL);

	m_hProcSvcDataThread = (HANDLE)_beginthreadex(NULL, 0, ProcSvcDataThread, this, 0, NULL);
	if (NULL != m_hProcSvcDataThread)
	{
		m_bRunProcSvcDataThread = true;
	}
#else
	pthread_mutex_init(&m_hProcSvcDataQueueMutex, NULL);

	if (0 == pthread_create(&m_hProcSvcDataThread, NULL, ProcSvcDataThread, this))
	{
		m_bRunProcSvcDataThread = true;
	}

	pthread_detach(m_hProcSvcDataThread);
#endif

    return CSocketThread::InitSocketThread();

$ERROR:
	m_bRunSendThread	= false;
	m_bRunRecvThread	= false;
	SAFE_DELETE(m_pClientSocket);
	return false;
}

void CClientThread::StopSocketThread()
{
    m_bRunProcSvcDataThread = false;

#ifdef _WINDOWS
	if (NULL != m_hProcSvcDataThread)
	{
		WaitForSingleObject(m_hProcSvcDataThread, INFINITE);
	}
#endif

	if (NULL != m_pProcSvcDataQueue)
	{
		if (0 < m_pProcSvcDataQueue->size())
		{
			for (m_ProcSvcDataQueueIt = m_pProcSvcDataQueue->begin(); m_ProcSvcDataQueueIt != m_pProcSvcDataQueue->end(); ++m_ProcSvcDataQueueIt)
			{
				ProcSvcData* pProcSvcData = (ProcSvcData*)*m_ProcSvcDataQueueIt;
				if (NULL != pProcSvcData)
				{
					SAFE_DELETE_ARRAY(pProcSvcData->pSvcData);
					SAFE_DELETE(pProcSvcData);
				}
			}

			m_pProcSvcDataQueue->clear();
		}
	}	

	SAFE_DELETE(m_pProcSvcDataQueue);

#ifdef _WINDOWS
	CloseHandle(m_hProcSvcDataQueueMutex);
	m_hProcSvcDataQueueMutex = NULL;
#else
	pthread_mutex_destroy(&m_hProcSvcDataQueueMutex);
#endif

	CSocketThread::StopSocketThread();
}

void CClientThread::PushSvcData(int nSvcCode, UINT uiSvcDataLen, char* pSvcData)
{
	if ((SVC_CODE_START >= nSvcCode) || (SVC_CODE_END <= nSvcCode))
	{
		return;
	}

	ProcSvcData* pProcSvcData	= new ProcSvcData;
    memset(pProcSvcData, 0, sizeof(ProcSvcData));
	pProcSvcData->nSvcCode		= nSvcCode;

    if ((0 < uiSvcDataLen) && (NULL != pSvcData))
    {
        pProcSvcData->nSvcDataLen	= uiSvcDataLen;
	    pProcSvcData->pSvcData		= new char[uiSvcDataLen];
	    memcpy(pProcSvcData->pSvcData, pSvcData, uiSvcDataLen);
    }

#ifdef _WINDOWS
	WaitForSingleObject(m_hProcSvcDataQueueMutex, INFINITE);
#else
	pthread_mutex_lock(&m_hProcSvcDataQueueMutex);
#endif

	m_pProcSvcDataQueue->push_back((void*)pProcSvcData);

#ifdef _WINDOWS
	ReleaseMutex(m_hProcSvcDataQueueMutex);
#else
	pthread_mutex_unlock(&m_hProcSvcDataQueueMutex);
#endif
}

int	CClientThread::ActiveConnectSocket(SOCKET hConnectSocket, SOCKADDR_IN* pConnectAdr)
{
	if ((INVALID_SOCKET == hConnectSocket) || (NULL == pConnectAdr))
	{
		return -1;
	}

	if (NULL == m_hComPort)
	{
		return -2;
	}

	DWORD				dwRecvBytes		= 0;
	DWORD				dwFlags			= 0;
	PSOCKET_INFO_DATA   pSocketInfo		= NULL;
	POVERLAPPED_IO_DATA pOvlpInfo		= NULL;

	pSocketInfo	 = new SOCKET_INFO_DATA;
	if (NULL == pSocketInfo)
	{
		closesocket(hConnectSocket);
		return -3;
	}

	memset(pSocketInfo, 0, sizeof(SOCKET_INFO_DATA));
	pSocketInfo->hSessionSocket = hConnectSocket;
	memcpy(&pSocketInfo->SessionAddr, pConnectAdr, sizeof(SOCKADDR_IN));

	CreateIoCompletionPort((HANDLE)hConnectSocket, m_hComPort, (ULONG_PTR)pSocketInfo, 0);

	pOvlpInfo	= new OVERLAPPED_IO_DATA;
	if (NULL == pOvlpInfo)
	{
		closesocket(hConnectSocket);
		return -4;
	}

	memset(pOvlpInfo, 0, sizeof(OVERLAPPED_IO_DATA));
	pOvlpInfo->nRWMode		= _DEC_MODE_READ;
	pOvlpInfo->wsaBuf.len	= _DEC_MAX_BUF_SIZE;
	pOvlpInfo->wsaBuf.buf	= new char[pOvlpInfo->wsaBuf.len];
	memset(pOvlpInfo->wsaBuf.buf, 0, pOvlpInfo->wsaBuf.len);

	m_ConnectSocketMap[hConnectSocket] = m_pClientSocket;

    TraceLog("CClientThread::ActiveConnectSocket - OvlpInfo = 0x%08x", pOvlpInfo);
	WSARecv(pSocketInfo->hSessionSocket, &pOvlpInfo->wsaBuf, 1, &dwRecvBytes, &dwFlags, &pOvlpInfo->Ovlp, NULL);

	return 0;
}

CConnectSocket* CClientThread::GetConSocket(SOCKET hConnectSocket)
{
	if (NULL == m_pClientSocket)
	{
		return NULL;
	}

	return (m_pClientSocket->GetSocket() == hConnectSocket) ? m_pClientSocket : NULL;
}

#ifdef _WINDOWS
unsigned int __stdcall	CClientThread::ProcSvcDataThread(void* lpParam)
#else
void*					CClientThread::ProcSvcDataThread(void* lpParam)
#endif
{
	CClientThread* pThis = (CClientThread*)lpParam;
	if (NULL == pThis)
	{
#ifdef _WINDOWS
		return -1;
#else
		return NULL;
#endif
	}

	if (NULL == pThis->m_pProcSvcDataQueue)
	{
#ifdef _WINDOWS
		return -2;
#else
		return NULL;
#endif
	}

	ProcSvcData*	pProcSvcData = NULL;

	while (true == pThis->m_bRunProcSvcDataThread)
	{
		if (0 >= pThis->m_pProcSvcDataQueue->size())
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			usleep(10000);
#endif
			continue;
		}

		WaitForSingleObject(pThis->m_hProcSvcDataQueueMutex, INFINITE);

		pProcSvcData = (ProcSvcData*)pThis->m_pProcSvcDataQueue->front();
		if ((NULL != pProcSvcData) && (SVC_CODE_START < pProcSvcData->nSvcCode) && (SVC_CODE_END > pProcSvcData->nSvcCode))
		{
			if ((NULL != pThis->m_pObject) && (NULL != pThis->m_ProcCallbackFunc))
			{
				pThis->m_ProcCallbackFunc(pThis->m_pObject, pProcSvcData->nSvcCode, pProcSvcData->nSvcDataLen, pProcSvcData->pSvcData);
			}

			SAFE_DELETE_ARRAY(pProcSvcData->pSvcData);
			SAFE_DELETE(pProcSvcData);
		}
		pThis->m_pProcSvcDataQueue->pop_front();

		ReleaseMutex(pThis->m_hProcSvcDataQueueMutex);
	}

#ifdef _WINDOWS
	CloseHandle(pThis->m_hProcSvcDataThread);
	pThis->m_hProcSvcDataThread = NULL;

	return 0;
#else
	return NULL;
#endif
}