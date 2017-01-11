#include "StdAfx.h"
#include "SocketThread.h"

typedef struct _SendData
{
    int nDataLen;
    char* pData;
} SendData;


CSocketThread::CSocketThread()
{
	m_hSendThread			= NULL;
	m_hRecvThread			= NULL;

	m_bRunSendThread		= false;
	m_bRunRecvThread		= false;

    m_pSendDataQueue		= NULL;
}

CSocketThread::~CSocketThread(void)
{
    if (0 < m_pSendDataQueue->size())
    {
        for (m_SendDataQueueIt = m_pSendDataQueue->begin(); m_SendDataQueueIt != m_pSendDataQueue->end(); ++m_SendDataQueueIt)
        {
            SendData* pSendData = (SendData*)*m_SendDataQueueIt;
            if (NULL != pSendData)
            {
                SAFE_DELETE_ARRAY(pSendData->pData);
                SAFE_DELETE(pSendData);
            }
        }
        m_pSendDataQueue->clear();
    }

    SAFE_DELETE(m_pSendDataQueue);

#ifdef _WINDOWS
    CloseHandle(m_hSendDataQueueMutex);
	m_hSendDataQueueMutex = NULL;
#else
	pthread_mutex_destroy(&m_hSendDataQueueMutex);
#endif
}

bool CSocketThread::InitSocketThread()
{
	SAFE_DELETE(m_pSendDataQueue);

    m_pSendDataQueue	= new SendDataQueue;
    m_pSendDataQueue->clear();

#ifdef _WINDOWS
	m_hSendDataQueueMutex = CreateMutex(NULL, FALSE, NULL);

    m_hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
    if (NULL != m_hSendThread)
    {
        m_bRunSendThread = true;
    }

    //m_hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvThread, this, 0, NULL);
    //if (NULL != m_hRecvThread)
    //{
    //    m_bRunRecvThread = true;
    //}
#else
	pthread_mutex_init(&m_hSendDataQueueMutex, NULL);

	int nRet = pthread_create(&m_hSendThread, NULL, SendThread, this);
	if (0 == nRet)
	{
		m_bRunSendThread = true;
	}

	nRet = pthread_create(&m_hRecvThread, NULL, RecvThread, this);
	if (0 == nRet)
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
#ifdef _WINDOWS
	if (NULL != m_hSendThread)
    {
        m_bRunSendThread = false;
		WaitForSingleObject(m_hSendThread, INFINITE);
    }

	if (NULL != m_hRecvThread)
    {
		// escape blocking mode
		closesocket(GetSocket()->GetConSocket());

        m_bRunRecvThread = false;

		WaitForSingleObject(m_hRecvThread, INFINITE);
    }
#else
	close(GetSocket()->GetConSocket());
#endif
}

void CSocketThread::Send(char* pData, int nDataLen)
{
    if ((NULL == pData) || (0 >= nDataLen))
    {
        return;
    }

    SendData* pSendData = new SendData;
    pSendData->nDataLen = nDataLen;
    pSendData->pData    = pData;

#ifdef _WINDOWS
	WaitForSingleObject(m_hSendDataQueueMutex, INFINITE);
#else
	pthread_mutex_lock(&m_hSendDataQueueMutex);
#endif

    m_pSendDataQueue->push_back((void*)pSendData);

#ifdef _WINDOWS
    ReleaseMutex(m_hSendDataQueueMutex);
#else
	pthread_mutex_unlock(&m_hSendDataQueueMutex);
#endif
}

#ifdef _WINDOWS
unsigned int __stdcall	CSocketThread::SendThread(void* lpParam)
#else
void*					CSocketThread::SendThread(void* lpParam)
#endif
{
    CSocketThread* pThis    = (CSocketThread*)lpParam;
    if (NULL == pThis)
    {
#ifdef _WINDOWS
        return -1;
#else
		return NULL;
#endif
    }

    while (true == pThis->m_bRunSendThread)
    {
        if (0 == pThis->m_pSendDataQueue->size())
        {
#ifdef _WINDOWS
            Sleep(1);
#else
			usleep(1000);
#endif
            continue;
        }

#ifdef _WINDOWS
		WaitForSingleObject(pThis->m_hSendDataQueueMutex, INFINITE);
#else
		pthread_mutex_lock(&pThis->m_hSendDataQueueMutex);
#endif

        SendData* pSendData = (SendData*)pThis->m_pSendDataQueue->front();
        if ((NULL != pSendData) && (NULL != pSendData->pData) && (0 < pSendData->nDataLen))
        {
            pThis->GetSocket()->Send(pSendData->pData, pSendData->nDataLen);

            SAFE_DELETE_ARRAY(pSendData->pData);
            SAFE_DELETE(pSendData);
        } 
        pThis->m_pSendDataQueue->pop_front();

#ifdef _WINDOWS
        ReleaseMutex(pThis->m_hSendDataQueueMutex);
#else
		pthread_mutex_unlock(&pThis->m_hSendDataQueueMutex);
#endif
    }

#ifdef _WINDOWS
    CloseHandle(pThis->m_hSendThread);
    pThis->m_hSendThread = NULL;

    return 1;
#else
	return NULL;
#endif 
}

#ifdef _WINDOWS
unsigned int __stdcall	CSocketThread::RecvThread(void* lpParam)
#else
void*					CSocketThread::RecvThread(void* lpParam)
#endif
{
	CSocketThread* pThis    = (CSocketThread*)lpParam;
    if (NULL == pThis)
    {
#ifdef _WINDOWS
        return -1;
#else
		return NULL;
#endif
    }

	char    RecvProcBuf[_DEC_MAX_BUF_SIZE]		= {0, };
    int     nReadLen                            = 0;
	UINT	uiBufIndex							= 0;

	while (true == pThis->m_bRunRecvThread)
	{
#ifdef _WINDOWS
		nReadLen = recv(pThis->GetSocket()->GetConSocket(), RecvProcBuf, _DEC_MAX_BUF_SIZE, 0);
#else
		nReadLen = read(pThis->GetSocket()->GetConSocket(), RecvProcBuf, _DEC_MAX_BUF_SIZE);
#endif
		if (0 >= nReadLen)
		{
			TraceLog("Read Length = %d", nReadLen);
#ifdef _WINDOWS
			Sleep(10);
#else
			usleep(10000);
#endif
            break;
		}

		UINT uiResult = pThis->GetSocket()->ProcessReceive(RecvProcBuf, nReadLen);
		if (0 < uiResult)
		{
			UINT	uiRemainLen		= uiResult;
			char*	pNewRecvProcBuf	= new char[_DEC_MAX_BUF_SIZE + uiResult];
			memset(pNewRecvProcBuf, 0,              _DEC_MAX_BUF_SIZE + uiResult);
			memcpy(pNewRecvProcBuf, RecvProcBuf,    _DEC_MAX_BUF_SIZE);
			uiBufIndex += _DEC_MAX_BUF_SIZE;
        
			nReadLen	= 0;
			while (0 < uiRemainLen)
			{
#ifdef _WINDOWS
				nReadLen = recv(pThis->GetSocket()->GetConSocket(), pNewRecvProcBuf + uiBufIndex, uiRemainLen, 0);
#else
				nReadLen = read(pThis->GetSocket()->GetConSocket(), pNewRecvProcBuf + uiBufIndex, uiRemainLen);
#endif

				TraceLog("Extra Read Length = %d", nReadLen);
				if (0 >= nReadLen)
				{
					continue;
				}				

				uiRemainLen -= nReadLen;
				uiBufIndex	+= nReadLen;

				TraceLog("Cur Buffer Index = %d, Remain Data = %d", uiBufIndex, uiRemainLen);
			}

	        uiResult = pThis->GetSocket()->ProcessReceive(pNewRecvProcBuf, uiBufIndex);
		    SAFE_DELETE_ARRAY(pNewRecvProcBuf);
		}

		uiBufIndex = 0;
	}

#ifdef _WINDOWS
    CloseHandle(pThis->m_hRecvThread);
    pThis->m_hRecvThread = NULL;

    return 1;
#else
	return NULL;
#endif
}