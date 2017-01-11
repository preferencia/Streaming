#include "stdAfx.h"
#include "SMServerThread.h"

CSMServerThread::CSMServerThread(void)
{
    m_bSendOnly             = FALSE;

    m_bRunProcSvcThread     = FALSE;
    m_hProcSvcThread        = NULL;

    m_pProcSvcDataQueue     = NULL;
    m_hProcSvcCallbackMutex = NULL;

    m_pProcObject           = NULL;
    m_pProcSvcCallback      = NULL;
    m_pRecvDataCallback     = NULL;
    m_pRecvMsgCallback      = NULL;

    m_pRecvMsgWnd           = NULL;

    m_nThreadType           = 0;
}

CSMServerThread::~CSMServerThread(void)
{
    Dettach(m_pRecvMsgWnd);

    if (NULL != m_hProcSvcThread)
    {
        m_bRunProcSvcThread = FALSE;
        while (NULL != m_hProcSvcThread)
        {
            Sleep(100);
        }
    }

    if ((NULL != m_pProcSvcDataQueue) && (0 < m_pProcSvcDataQueue->size()))
    {
        for (m_ProcSvcDataQueueIt = m_pProcSvcDataQueue->begin(); m_ProcSvcDataQueueIt != m_pProcSvcDataQueue->end(); ++m_ProcSvcDataQueueIt)
        {
            ProcSvcData* pProcSvcData = *m_ProcSvcDataQueueIt;
            if (NULL != pProcSvcData)
            {
                SAFE_DELETE_ARRAY(pProcSvcData->pData);
                SAFE_DELETE(pProcSvcData);
            }
        }
        m_pProcSvcDataQueue->clear();
    }

    SAFE_DELETE(m_pProcSvcDataQueue);

    if (NULL != m_hProcSvcCallbackMutex)
	{
		CloseHandle(m_hProcSvcCallbackMutex);
		m_hProcSvcCallbackMutex = NULL;
	}
}

BOOL CSMServerThread::InitSocketThread()
{
    if (FALSE == CSocketThread::InitSocketThread())
    {
        return FALSE;
    }

    m_SMServerSocket.SetParentThread(this);

    m_pProcSvcDataQueue = new ProcSvcDataQueue;
    m_pProcSvcDataQueue->clear();

    m_hProcSvcCallbackMutex = CreateMutex(NULL, FALSE, NULL);
    if (NULL == m_hProcSvcCallbackMutex)
    {
        return FALSE;
    }

    m_hProcSvcThread = (HANDLE)_beginthreadex(NULL, 0, ProcSvcThread, this, 0, NULL);
    if (NULL != m_hProcSvcThread)
    {
        m_bRunProcSvcThread = TRUE;
    }

    return TRUE;
}

void CSMServerThread::SetCryptionData(char* pszCryptionKey, char* pszKeySrcValue, int nCryptionKeyLen, int nKeySrcValueLen)
{
    m_SMServerSocket.SetCryptionData(pszCryptionKey, pszKeySrcValue, nCryptionKeyLen, nKeySrcValueLen);
}

BOOL CSMServerThread::SendRecvReplyData(void* pObject, int nSvcCode, void* pData, int nDataLen)
{
    if (m_pProcObject != pObject)
    {
        TRACELOG(LEVEL_DBG, _T("Call From Invalid Object"));
        return FALSE;
    }

    if ((NULL == pData) || (0 == nDataLen))
    {
        TRACELOG(LEVEL_DBG, _T("Data is Wrong"));
        return FALSE;
    }

    //if ((SVC_FSO01 > nSvcCode) || (SVC_CODE_END <= nSvcCode))
    //{
    //    TRACELOG(LEVEL_DBG, _T("Wrong Code [%d]"), nSvcCode);
    //    return FALSE;
    //}

    int     nTotPktSize = 0;
    if (0 > nSvcCode)
    {
        nTotPktSize = sizeof(FS_HEADER) + nDataLen;
    }
    else
    {
        nTotPktSize = sizeof(FS_HEADER) + sizeof(FS2SM_CNT_BLOCK) + nDataLen;
    }
    
    int     nSendBufIndex   = 0;
    char*   pSendBuffer     = new char[nTotPktSize];
    memset(pSendBuffer, 0, nTotPktSize);

    PFS_HEADER          pHeader     = (PFS_HEADER)pSendBuffer;
    PFS2SM_CNT_BLOCK    pCntBlock   = (0 > nSvcCode) ? NULL : (PFS2SM_CNT_BLOCK)(pSendBuffer + sizeof(FS_HEADER));
    if (NULL != pCntBlock)
    {
        pCntBlock->uiCount              = 1;
    }    

    MAKE_FS_HEADER(pHeader, nSvcCode, 0, nDataLen);
    nSendBufIndex += sizeof(FS_HEADER);
    if (NULL != pCntBlock)
    {
        memcpy(pSendBuffer + nSendBufIndex, pCntBlock, sizeof(FS2SM_CNT_BLOCK));
        nSendBufIndex += sizeof(FS2SM_CNT_BLOCK);
    }
    
    memcpy(pSendBuffer + nSendBufIndex, pData, nDataLen);
    Send(pSendBuffer, nTotPktSize);

    return TRUE;
}

void CSMServerThread::Attach(CMsgWnd* pRecvMsgWnd)
{
    m_pRecvMsgWnd = pRecvMsgWnd;
    if (NULL != m_pRecvMsgWnd)
    {
        m_pRecvMsgWnd->SetParentThread(this);
    }
}

void CSMServerThread::Dettach(CMsgWnd* pRecvMsgWnd)
{
    if (NULL != m_pRecvMsgWnd)
    {
        m_pRecvMsgWnd->SetParentThread(NULL);
        m_pRecvMsgWnd->DestroyWindow();
        SAFE_DELETE(m_pRecvMsgWnd);
    }
}

void CSMServerThread::ProcRecvSvcData(WPARAM wParam, LPARAM lParam)
{
	int nSendSvcCode    = 0;
	int nErr            = 0;

	if ((NULL == m_pProcObject) || (NULL == m_pRecvDataCallback))
		return;

	int nRqID           = -1;
	int nOutDataCount   = -1;
	int nOutDataSize    = -1;
	char* pOutData      = NULL;

	m_pRecvDataCallback(m_pProcObject, wParam, lParam, nRqID, nOutDataCount, nOutDataSize, &pOutData);

	TRACELOG(LEVEL_DBG, _T("RQ ID = %d, Data Count = %d, Data Length = %d"), nRqID, nOutDataCount, nOutDataSize);
	//TRACELOG(LEVEL_DBG, _T("Data = %hs"), pOutData);

	if ((0 > nRqID) && (0 > nOutDataCount) && (0 > nOutDataSize))
	{
		this->SaveDataLog(enumLogTypeStr, 0, "  ProcRecvSvcData: RQ ID = %d, Data Count = %d, Data Length = %d", nRqID, nOutDataCount, nOutDataSize);
		return;
	}

	switch (nRqID)
	{
	case RQ_ORD_NONSIGN_LIST:
	case RQ_ORD_SIGN_LIST:
	case RQ_ORD_LIST_ALL:
		{
			nSendSvcCode = SVC_FSQ03;
		}
		break;

	case RQ_BALANCE_LIST:
		{
			nSendSvcCode = SVC_FSQ02;
		}
		break;

	case RQ_ACC_DEPOSIT:
		{
			nSendSvcCode = SVC_FSQ01;
		}
		break;

	case RQ_FS_ORD_NEW:
	case RQ_HANA_ORD_NEW:
	case RQ_KR_ORD_NEW:
	case RQ_EBEST_ORD_NEW:
		{
			nSendSvcCode = SVC_FSO01;
		}
		break;

	case RQ_FS_ORD_MODIFY:
	case RQ_HANA_ORD_MODIFY:
	case RQ_KR_ORD_MODIFY:
	case RQ_EBEST_ORD_MODIFY:
		{
			nSendSvcCode = SVC_FSO02;
		}
		break;

	case RQ_FS_ORD_CANCEL:
	case RQ_HANA_ORD_CANCEL:
	case RQ_KR_ORD_CANCEL:
	case RQ_EBEST_ORD_CANCEL:
		{
			nSendSvcCode = SVC_FSO03;
		}
		break;

	default:
		{
		}
		break;
	}

	if (0 > nOutDataCount)
	{
		nErr = -1;
		goto $ERROR_BAILED;
	}

	if (0 > nOutDataSize)
	{
		nErr = -2;
		goto $ERROR_BAILED;
	}

	int     nTotPktSize = sizeof(FS_HEADER) + sizeof(FS2SM_CNT_BLOCK) + nOutDataSize;
	char*   pSendBuffer = new char[nTotPktSize];
	memset(pSendBuffer, 0, nTotPktSize);

	PFS_HEADER          pHeader     = (PFS_HEADER)pSendBuffer;
	PFS2SM_CNT_BLOCK    pCntBlock   = (PFS2SM_CNT_BLOCK)(pSendBuffer + sizeof(FS_HEADER));
	pCntBlock->uiCount              = nOutDataCount;

	MAKE_FS_HEADER(pHeader, nSendSvcCode, nErr, nOutDataSize);
	memcpy(pSendBuffer + sizeof(FS_HEADER), pCntBlock, sizeof(FS2SM_CNT_BLOCK));
	memcpy(pSendBuffer + sizeof(FS_HEADER) + sizeof(FS2SM_CNT_BLOCK), pOutData, nOutDataSize);
	Send(pSendBuffer, nTotPktSize);

	SAFE_DELETE_ARRAY(pOutData);

	return;

$ERROR_BAILED:
	TRACELOG(LEVEL_DBG, _T("Error Proc - SVC Code = %d, Err Code = %d"), nSendSvcCode, nErr);
	int nSendErrBufLen = sizeof(FS_HEADER);
	char* pSendErrBuf  = new char[nSendErrBufLen];
	memset(pSendErrBuf, 0, nSendErrBufLen);
	MAKE_FS_HEADER((FS_HEADER*)pSendErrBuf, nSendSvcCode, nErr, 0);
	Send(pSendErrBuf, nSendErrBufLen);
	SAFE_DELETE_ARRAY(pOutData);
	return;
}

void CSMServerThread::ProcRecvSvcMsg(WPARAM wParam, LPARAM lParam)
{
	if ((NULL == m_pProcObject) || (NULL == m_pRecvMsgCallback))
		return;

	int nSendSvcCode    = -1;
	int nRqID           = -1;
	int nErrCode        = -1;
	int nOutDataSize    = -1;
	char* pOutData      = NULL;

	m_pRecvMsgCallback(m_pProcObject, wParam, lParam, nRqID, nErrCode, nOutDataSize, &pOutData);

	TRACELOG(LEVEL_DBG, _T("RQ ID = %d, Err Code = %d, Data Length = %d"), nRqID, nErrCode, nOutDataSize);

	if (0 >= nErrCode)
	{
		this->SaveDataLog(enumLogTypeStr, 0, "  ProcRecvSvcMsg: RQ ID = %d, Err Code = %d, Data Length = %d", nRqID, nErrCode, nOutDataSize);
		SAFE_DELETE_ARRAY(pOutData);
		return;
	}

	//TRACELOG(LEVEL_DBG, _T("Data = %hs"), pOutData);

	switch (nRqID)
	{
	case RQ_ORD_NONSIGN_LIST:
	case RQ_ORD_SIGN_LIST:
	case RQ_ORD_LIST_ALL:
		{
			nSendSvcCode = SVC_FSQ03;
		}
		break;

	case RQ_BALANCE_LIST:
		{
			nSendSvcCode = SVC_FSQ02;
		}
		break;

	case RQ_ACC_DEPOSIT:
		{
			nSendSvcCode = SVC_FSQ01;
		}
		break;

	case RQ_FS_ORD_NEW:
	case RQ_HANA_ORD_NEW:
	case RQ_KR_ORD_NEW:
	case RQ_EBEST_ORD_NEW:
		{
			nSendSvcCode = SVC_FSO01;
		}
		break;

	case RQ_FS_ORD_MODIFY:
	case RQ_HANA_ORD_MODIFY:
	case RQ_KR_ORD_MODIFY:
	case RQ_EBEST_ORD_MODIFY:
		{
			nSendSvcCode = SVC_FSO02;
		}
		break;

	case RQ_FS_ORD_CANCEL:
	case RQ_HANA_ORD_CANCEL:
	case RQ_KR_ORD_CANCEL:
	case RQ_EBEST_ORD_CANCEL:
		{
			nSendSvcCode = SVC_FSO03;
		}
		break;

	default:
		{
		}
		break;
	}

	nSendSvcCode *= -1;

	int     nTotPktSize = sizeof(FS_HEADER) + nOutDataSize;
	char*   pSendBuffer = new char[nTotPktSize];
	memset(pSendBuffer, 0, nTotPktSize);

	PFS_HEADER          pHeader     = (PFS_HEADER)pSendBuffer;
	MAKE_FS_HEADER(pHeader, nSendSvcCode, nErrCode, nOutDataSize);
	memcpy(pSendBuffer + sizeof(FS_HEADER), pOutData, nOutDataSize);
	Send(pSendBuffer, nTotPktSize);

	SAFE_DELETE_ARRAY(pOutData);
}

void CSMServerThread::ProcUserMsg(int nMsgCode)
{
    if (NULL != m_pProcObject)
    {
        ::PostMessage(((CWnd*)m_pProcObject)->GetSafeHwnd(), UM_SYS_TRADING_PROC, (WPARAM)nMsgCode, (LPARAM)m_Socket);
    }

    // 종료 세션 정리 시 소멸자에서 제거하면 죽어서 여기서 처리
    switch (nMsgCode)
    {
    case WPARAM_CONNECTION_CLOSE:
        {
            Dettach(m_pRecvMsgWnd);
        }
        break;

    default:
        break;
    }
}

void CSMServerThread::SetProcCallbackData(  void* pOjbect, 
                                            ProcSvcCallback pProcSvcCallbackFunc, 
                                            RecvProcCallback pRecvDataCallbackFunc,
                                            RecvProcCallback pRecvMsgCallbackFunc)
{
    m_pProcObject       = pOjbect;
    m_pProcSvcCallback  = pProcSvcCallbackFunc;
    m_pRecvDataCallback = pRecvDataCallbackFunc;
    m_pRecvMsgCallback  = pRecvMsgCallbackFunc;
}

void CSMServerThread::ProcReqSvcData(int nSvcCode, char* pData, int nDataLen, HWND hTaregtWnd /*= NULL*/)
{
    if ((NULL == pData) || (0 >= nDataLen))
    {
        return;
    }

    ProcSvcData* pProcSvcData   = new ProcSvcData;
    
    if (NULL == hTaregtWnd)
    {
        pProcSvcData->hTargetWnd    = m_pRecvMsgWnd->GetSafeHwnd();
    }
    else
    {
        pProcSvcData->hTargetWnd    = hTaregtWnd;
    }
    
    pProcSvcData->nSvcCode      = nSvcCode;
    pProcSvcData->nDataLen      = nDataLen;
    pProcSvcData->pData         = new char[nDataLen];
    memcpy(pProcSvcData->pData, pData, nDataLen);

    WaitForSingleObject(m_hProcSvcCallbackMutex, INFINITE);
    m_pProcSvcDataQueue->push_back(pProcSvcData);
    ReleaseMutex(m_hProcSvcCallbackMutex);
}

unsigned int __stdcall CSMServerThread::ProcSvcThread(void* lpParam)
{
    CSMServerThread* pThis  = (CSMServerThread*)lpParam;
    if (NULL == pThis)
    {
        return -1;
    }

    while (TRUE == pThis->m_bRunProcSvcThread)
    {
        if (0 == pThis->m_pProcSvcDataQueue->size())
        {
            Sleep(1);
            continue;
        }

        WaitForSingleObject(pThis->m_hProcSvcCallbackMutex, CLK_TCK);
        ProcSvcData* pProcSvcData = pThis->m_pProcSvcDataQueue->front();
        pThis->m_pProcSvcDataQueue->pop_front();
        ReleaseMutex(pThis->m_hProcSvcCallbackMutex);

        if (NULL != pProcSvcData)
        {
			if (NULL != pProcSvcData->pData)
			{
				if ((NULL != pThis->m_pProcObject) && (NULL != pThis->m_pProcSvcCallback) && (0 < pProcSvcData->nDataLen))
				{
					pThis->m_pProcSvcCallback(pThis->m_pProcObject, pProcSvcData->nSvcCode, pProcSvcData->pData, pProcSvcData->nDataLen, pProcSvcData->hTargetWnd);
				}

				SAFE_DELETE_ARRAY(pProcSvcData->pData);
			}
			SAFE_DELETE(pProcSvcData);
        } 
    }

    CloseHandle(pThis->m_hProcSvcThread);
    pThis->m_hProcSvcThread = NULL;

    return 1;
}