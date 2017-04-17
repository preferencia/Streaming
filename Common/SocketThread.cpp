#include "stdafx.h"
#include "SocketThread.h"

typedef struct _SendData
{
    SOCKET  hSocket;
    int     nDataLen;
    char*   pData;
} SendData;

CSocketThread::CSocketThread()
{
	m_nSessionThreadNum		= 0;
#ifdef _WIN32
	m_hComPort				= NULL;

    m_hSendThread			= NULL;
	m_hRecvThread			= NULL;
	m_hSendDataQueueMutex	= NULL;
#else
    m_pEpollEvents          = NULL;
    m_nEpollFd              = 0;
#endif

	m_bRunSendThread		= false;
	m_bRunRecvThread		= false;

    m_SendDataQueue.clear();
}

CSocketThread::~CSocketThread(void)
{
	if (0 < m_ConnectSocketMap.size())
	{
		m_ConnectSocketMapIt = m_ConnectSocketMap.begin();
		while (m_ConnectSocketMapIt != m_ConnectSocketMap.end())
		{
			CConnectSocket* pConnectSocket = m_ConnectSocketMapIt->second;
			SAFE_DELETE(pConnectSocket);
			m_ConnectSocketMap.erase(m_ConnectSocketMapIt++);
		}
	}
}

bool CSocketThread::InitSocketThread()
{
#ifdef _WIN32
	m_hSendDataQueueMutex = CreateMutex(NULL, FALSE, NULL);

	m_hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
	if (NULL != m_hSendThread)
	{
		m_bRunSendThread = true;
	}

	m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvThread, this, 0, NULL);
	if (NULL != m_hRecvThread)
	{
		m_bRunRecvThread = true;
	}
#else
	pthread_mutex_init(&m_hSendDataQueueMutex, NULL);

	if (0 == pthread_create(&m_hSendThread, NULL, SendThread, this))
	{
		m_bRunSendThread = true;
	}

	if (0 == pthread_create(&m_hRecvThread, NULL, RecvThread, this))
	{
		m_bRunRecvThread = true;
	}

	pthread_detach(m_hSendThread);
	pthread_detach(m_hRecvThread);
#endif

    return true;
}

void CSocketThread::StopSocketThread()
{
	// escape blocking mode
	if (0 < m_ConnectSocketMap.size())
	{
		for (m_ConnectSocketMapIt = m_ConnectSocketMap.begin(); m_ConnectSocketMapIt != m_ConnectSocketMap.end(); ++m_ConnectSocketMapIt)
		{
            CConnectSocket* pConnectSocket = m_ConnectSocketMapIt->second;
			if (NULL != pConnectSocket)
			{
#ifdef _WIN32
				SOCKET hSock = pConnectSocket->GetSocket();
				closesocket(pConnectSocket->GetSocket());
#else
				close(pConnectSocket->GetSocket());
#endif				
			}
        }
	}

	m_bRunSendThread = false;
	m_bRunRecvThread = false;

#ifdef _WIN32
	if (NULL != m_hSendThread)
    {
		WaitForSingleObject(m_hSendThread, INFINITE);
    }

	if (NULL != m_hRecvThread)
    {        
		WaitForSingleObject(m_hRecvThread, INFINITE);
    }
#endif

    if (0 < m_SendDataQueue.size())
	{
		m_SendDataQueueIt = m_SendDataQueue.begin();
		while (m_SendDataQueueIt != m_SendDataQueue.end())
		{
			SendData* pSendData = (SendData*)*m_SendDataQueueIt;
			if (NULL != pSendData)
			{
				SAFE_DELETE_ARRAY(pSendData->pData);
				SAFE_DELETE(pSendData);				
			}
			m_SendDataQueue.erase(m_SendDataQueueIt++);
		}
	}

#ifdef _WIN32
    CloseHandle(m_hSendDataQueueMutex);
	m_hSendDataQueueMutex = NULL;
#else
	pthread_mutex_destroy(&m_hSendDataQueueMutex);
#endif
}

void CSocketThread::Send(SOCKET hSocket, int nDataLen, char* pData)
{
    if ((INVALID_SOCKET == hSocket) || (0 >= nDataLen) || (NULL == pData))
    {
        return;
    }

    SendData* pSendData = new SendData;
    pSendData->hSocket  = hSocket;
    pSendData->nDataLen = nDataLen;
    pSendData->pData    = pData;

#ifdef _WIN32
	WaitForSingleObject(m_hSendDataQueueMutex, INFINITE);
#else
	pthread_mutex_lock(&m_hSendDataQueueMutex);
#endif

    m_SendDataQueue.push_back((void*)pSendData);

#ifdef _WIN32
    ReleaseMutex(m_hSendDataQueueMutex);
#else
	pthread_mutex_unlock(&m_hSendDataQueueMutex);
#endif
}

#ifdef _WIN32
unsigned int __stdcall CSocketThread::SendThread(void* lpParam)
{
	CSocketThread* pThis = (CSocketThread*)lpParam;
	if (NULL == pThis)
	{
		return -1;
	}

	SOCKET				hSessionSocket = INVALID_SOCKET;
	DWORD               dwTransBytes = 0;
	DWORD				dwFlags = 0;
	POVERLAPPED_IO_DATA pOvlpInfo = NULL;
	SendData*           pSendData = NULL;

	while (true == pThis->m_bRunSendThread)
	{
		if (0 >= pThis->m_SendDataQueue.size())
		{
			Sleep(10);
			continue;
		}

		pOvlpInfo = new OVERLAPPED_IO_DATA;
		if (NULL == pOvlpInfo)
		{
			continue;
		}

		memset(pOvlpInfo, 0, sizeof(OVERLAPPED_IO_DATA));
		pOvlpInfo->nRWMode = _DEC_MODE_WRITE;

		WaitForSingleObject(pThis->m_hSendDataQueueMutex, INFINITE);

		pSendData = (SendData*)pThis->m_SendDataQueue.front();
		if ((NULL != pSendData) && (INVALID_SOCKET != pSendData->hSocket) && (0 < pSendData->nDataLen) && (NULL != pSendData->pData))
		{
			hSessionSocket			= pSendData->hSocket;
			pOvlpInfo->wsaBuf.len	= pSendData->nDataLen;
            pOvlpInfo->wsaBuf.buf	= new char[pOvlpInfo->wsaBuf.len];
            memcpy(pOvlpInfo->wsaBuf.buf, pSendData->pData, pOvlpInfo->wsaBuf.len);

            TraceLog("CSocketThread::SendThread - OvlpInfo = 0x%08x", pOvlpInfo);

			WSASend(hSessionSocket, &pOvlpInfo->wsaBuf, 1, &dwTransBytes, 0, &pOvlpInfo->Ovlp, NULL);

            TraceLog("CSocketThread::SendThread - Send Bytes = %d", dwTransBytes);

			SAFE_DELETE_ARRAY(pSendData->pData);
			SAFE_DELETE(pSendData);

            TraceLog("CSocketThread::SendThread - Remove Send Data");
		}
		pThis->m_SendDataQueue.pop_front();

		ReleaseMutex(pThis->m_hSendDataQueueMutex);
	}

	CloseHandle(pThis->m_hSendThread);
	pThis->m_hSendThread = NULL;

	return 0;
}

unsigned int __stdcall CSocketThread::RecvThread(void* lpParam)
{
	CSocketThread* pThis = (CSocketThread*)lpParam;
	if (NULL == pThis)
	{
		return -1;
	}

	char*               pRecvProcBuf		= NULL;
	UINT                uiReadLen			= 0;
	UINT	            uiBufIndex			= 0;
	UINT                uiProcRecvResult	= 0;
	UINT				uiBufSize			= _DEC_MAX_BUF_SIZE;

	DWORD               dwTransBytes		= 0;
	DWORD				dwFlags				= 0;
	PSOCKET_INFO_DATA   pSocketInfo			= NULL;
	POVERLAPPED_IO_DATA pOvlpInfo			= NULL;
	CConnectSocket*     pConnectSocket		= NULL;

	while (true == pThis->m_bRunRecvThread)
	{
		if (0 >= pThis->m_ConnectSocketMap.size())
		{
			Sleep(10);
			continue;
		}

        GetQueuedCompletionStatus(pThis->m_hComPort, &dwTransBytes, (PULONG_PTR)&pSocketInfo, (LPOVERLAPPED*)&pOvlpInfo, INFINITE);

		if (NULL == pOvlpInfo)
		{
			continue;
		}

		if (_DEC_MODE_READ == pOvlpInfo->nRWMode)
		{
            TraceLog("CSocketThread::RecvThread - READ : OvlpInfo = 0x%08x", pOvlpInfo);
			if (0 == dwTransBytes)  // EOF Àü¼Û ½Ã
			{
				TraceLog("CSocketThread::RecvThread - Disconnect %d session.", pSocketInfo->hSessionSocket);
				closesocket(pSocketInfo->hSessionSocket);

				// Remove session socket map
				pThis->m_ConnectSocketMapIt = pThis->m_ConnectSocketMap.find(pSocketInfo->hSessionSocket);
				if (pThis->m_ConnectSocketMapIt != pThis->m_ConnectSocketMap.end())
				{
					pConnectSocket = pThis->m_ConnectSocketMapIt->second;
					SAFE_DELETE(pConnectSocket);
					pThis->m_ConnectSocketMap.erase(pThis->m_ConnectSocketMapIt);
				}

				SAFE_DELETE_ARRAY(pOvlpInfo->wsaBuf.buf);
                SAFE_DELETE_ARRAY(pOvlpInfo->wsaSavedBuf.buf);
				SAFE_DELETE(pOvlpInfo);
				SAFE_DELETE(pSocketInfo);

				continue;
			}

			if (NULL == pThis->GetConSocket(pSocketInfo->hSessionSocket))
			{
				continue;
			}

			TraceLog("CSocketThread::RecvThread - Recv Bytes = %ld", dwTransBytes);

            // If exist saved data
            if ((NULL != pOvlpInfo->wsaSavedBuf.buf) && (0 < pOvlpInfo->wsaSavedBuf.len))
            {
                pRecvProcBuf            = new char[pOvlpInfo->wsaSavedBuf.len + dwTransBytes];
                memcpy(pRecvProcBuf, pOvlpInfo->wsaSavedBuf.buf, pOvlpInfo->wsaSavedBuf.len);
                uiBufIndex              += pOvlpInfo->wsaSavedBuf.len;
                memcpy(pRecvProcBuf + uiBufIndex, pOvlpInfo->wsaBuf.buf, dwTransBytes);

                SAFE_DELETE_ARRAY(pOvlpInfo->wsaBuf.buf);
                
                pOvlpInfo->wsaBuf.len   = pOvlpInfo->wsaSavedBuf.len + dwTransBytes;
                pOvlpInfo->wsaBuf.buf   = pRecvProcBuf;
                dwTransBytes            = pOvlpInfo->wsaBuf.len;
                pRecvProcBuf            = NULL;

                TraceLog("CSocketThread::RecvThread - Total Recv Byte = %ld", dwTransBytes);
            }

			uiProcRecvResult = pThis->GetConSocket(pSocketInfo->hSessionSocket)->ProcessReceive(pOvlpInfo->wsaBuf.buf, dwTransBytes);
			if (0 < uiProcRecvResult)    // Incomplete data recieve
			{
                // Recieved data copy
				uiBufSize       = uiProcRecvResult;
				pRecvProcBuf    = new char[dwTransBytes];
				memset(pRecvProcBuf, 0, dwTransBytes);
				memcpy(pRecvProcBuf, pOvlpInfo->wsaBuf.buf, dwTransBytes);
			}
			else
			{
				uiBufSize       = _DEC_MAX_BUF_SIZE;
			}

			SAFE_DELETE_ARRAY(pOvlpInfo->wsaBuf.buf);
            SAFE_DELETE_ARRAY(pOvlpInfo->wsaSavedBuf.buf);
			SAFE_DELETE(pOvlpInfo);

			if (INVALID_SOCKET != pSocketInfo->hSessionSocket)
			{
				pOvlpInfo = new OVERLAPPED_IO_DATA;
				if (NULL == pOvlpInfo)
				{
					continue;
				}

                TraceLog("CSocketThread::RecvThread - READ : new OvlpInfo = 0x%08x", pOvlpInfo);

				memset(pOvlpInfo, 0, sizeof(OVERLAPPED_IO_DATA));
				pOvlpInfo->nRWMode = _DEC_MODE_READ;
				pOvlpInfo->wsaBuf.len = uiBufSize;
				pOvlpInfo->wsaBuf.buf = new char[pOvlpInfo->wsaBuf.len];
				memset(pOvlpInfo->wsaBuf.buf, 0, pOvlpInfo->wsaBuf.len);
				if (NULL != pRecvProcBuf)   // Save recieved data
				{
                    pOvlpInfo->wsaSavedBuf.buf = new char[dwTransBytes];
					memcpy(pOvlpInfo->wsaSavedBuf.buf, pRecvProcBuf, dwTransBytes);
                    pOvlpInfo->wsaSavedBuf.len = dwTransBytes;
					SAFE_DELETE(pRecvProcBuf);
				}

				WSARecv(pSocketInfo->hSessionSocket, &pOvlpInfo->wsaBuf, 1, &dwTransBytes, &dwFlags, &pOvlpInfo->Ovlp, NULL);
			}

            pOvlpInfo   = NULL;
			uiBufIndex  = 0;
		}
		else
		{
            TraceLog("CSocketThread::RecvThread - WRITE : wrtie bytes = %ld, OvlpInfo = 0x%08x", dwTransBytes, pOvlpInfo);
            if (dwTransBytes < pOvlpInfo->wsaBuf.len)
            {                
                pOvlpInfo->wsaBuf.len -= dwTransBytes;
                pOvlpInfo->wsaBuf.buf += dwTransBytes;

                TraceLog("CSocketThread::RecvThread - Reamined Bytes = %ld", pOvlpInfo->wsaBuf.len);

                while (0 < pOvlpInfo->wsaBuf.len)
                {                    
                    WSASend(pSocketInfo->hSessionSocket, &pOvlpInfo->wsaBuf, 1, &dwTransBytes, 0, &pOvlpInfo->Ovlp, NULL);

                    TraceLog("CSocketThread::RecvThread - Send Remained Bytes = %d", dwTransBytes);

                    pOvlpInfo->wsaBuf.len -= dwTransBytes;
                    pOvlpInfo->wsaBuf.buf += dwTransBytes;
                }
            }

            // SendThread's OVERLAPPED_IO_DATA structure delete
            SAFE_DELETE_ARRAY(pOvlpInfo->wsaBuf.buf);
		    SAFE_DELETE(pOvlpInfo);

            TraceLog("CSocketThread::RecvThread - WRITE : Remove OvlpInfo");
		}
	}
    
	CloseHandle(pThis->m_hRecvThread);
	pThis->m_hRecvThread = NULL;

	return 0;
}
#else
void*	CSocketThread::SendThread(void* lpParam)
{
	CSocketThread* pThis = (CSocketThread*)lpParam;
	if (NULL == pThis)
	{
		return NULL;
	}

	SendData*           pSendData = NULL;

	while (true == pThis->m_bRunSendThread)
	{
		if (0 >= pThis->m_SendDataQueue.size())
		{
			usleep(10000);
			continue;
		}

		pthread_mutex_lock(&pThis->m_hSendDataQueueMutex);

		pSendData = (SendData*)pThis->m_SendDataQueue.front();
		if ( (NULL != pSendData) 
			&& (INVALID_SOCKET != pSendData->hSocket) 
			&& (NULL != pSendData->pData) 
			&& (0 < pSendData->nDataLen) )
		{
			int nSendBytes = pThis->GetConSocket(pSendData->hSocket)->Send(pSendData->pData, pSendData->nDataLen);
			TraceLog("CSocketThread::SendThread - Send Bytes = %d", nSendBytes);
			
			SAFE_DELETE_ARRAY(pSendData->pData);
			SAFE_DELETE(pSendData);
		} 
		pThis->m_SendDataQueue.pop_front();

		pthread_mutex_unlock(&pThis->m_hSendDataQueueMutex);
	}

	return NULL;
}

void*	CSocketThread::RecvThread(void* lpParam)
{
	CSocketThread* pThis = (CSocketThread*)lpParam;
	if (NULL == pThis)
	{
		return NULL;
	}

	char*               pRecvProcBuf		= NULL;
	char*				pExpProcBuf			= NULL;
	UINT                uiReadLen			= 0;
	UINT	            uiBufIndex			= 0;
	UINT                uiProcRecvResult	= 0;
	UINT				uiBufSize			= _DEC_MAX_BUF_SIZE;

	SOCKET				hSessionSocket		= INVALID_SOCKET;
	int					nEpollEventCnt		= 0;

	pRecvProcBuf = new char[uiBufSize];
	memset(pRecvProcBuf, 0, uiBufSize);

	while (true == pThis->m_bRunRecvThread)
	{
		if (NULL == pThis->m_pEpollEvents)
		{
			usleep(10000);
			continue;
		}

		nEpollEventCnt	= epoll_wait(pThis->m_nEpollFd, pThis->m_pEpollEvents, _DEC_EPOLL_SIZE, -1);
		if (-1 == nEpollEventCnt)
		{
			TraceLog("epoll_wait() error!");
			break;
		}

		for (int nIndex = 0; nIndex < nEpollEventCnt; ++nIndex)
		{
			hSessionSocket = pThis->m_pEpollEvents[nIndex].data.fd;
			if (INVALID_SOCKET == hSessionSocket)
			{
				continue;
			}

			uiReadLen = read(hSessionSocket, pRecvProcBuf, uiBufSize);

			if (0 == uiReadLen)	// close request
			{
                TraceLog("Disconnect %d session.", hSessionSocket);
				epoll_ctl(pThis->m_nEpollFd, EPOLL_CTL_DEL, hSessionSocket, NULL);

				// Remove session socket map
				pThis->m_ConnectSocketMapIt = pThis->m_ConnectSocketMap.find(hSessionSocket);
				if (pThis->m_ConnectSocketMapIt != pThis->m_ConnectSocketMap.end())
				{
					CConnectSocket* pConnectSocket = pThis->m_ConnectSocketMapIt->second;
					SAFE_DELETE(pConnectSocket);
					pThis->m_ConnectSocketMap.erase(pThis->m_ConnectSocketMapIt);
				}

				close(hSessionSocket);
			}
			else
			{
				uiProcRecvResult = pThis->GetConSocket(hSessionSocket)->ProcessReceive(pRecvProcBuf, uiReadLen);
				if (0 < uiProcRecvResult)    // Incomplete data recieve
				{
					// Recieved data copy
					uiBufSize		= uiReadLen + uiProcRecvResult;
					pExpProcBuf		= new char[uiBufSize];
					memset(pExpProcBuf, 0, uiBufSize);
					memcpy(pExpProcBuf, pRecvProcBuf, uiReadLen);
					uiBufIndex		+= uiReadLen;

					while (0 < uiProcRecvResult)
					{
						uiReadLen = read(hSessionSocket, pExpProcBuf + uiBufIndex, uiProcRecvResult);

						if (0 > uiReadLen)
						{
							if (EAGAIN == errno)
							{
								break;
							}								
						}

						uiBufIndex			+= uiReadLen;
						uiProcRecvResult	-= uiReadLen;
					}

					pThis->GetConSocket(hSessionSocket)->ProcessReceive(pExpProcBuf, uiBufSize);

					SAFE_DELETE(pExpProcBuf);
				}

				uiBufSize = _DEC_MAX_BUF_SIZE;
				memset(pRecvProcBuf, 0, uiBufSize);
			}
		}
	}

	SAFE_DELETE(pRecvProcBuf);
	SAFE_DELETE(pExpProcBuf);

	return NULL;
}
#endif
