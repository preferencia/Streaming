#include "StdAfx.h"
#include "SMClientThread.h"

CSMClientThread::CSMClientThread(void)
{
    memset(m_szServerIP, 0, _DEC_IP_ADDR_LEN);
    m_uiPort                = 0;

    m_pProcObject           = NULL;
    m_pProcSvcCallback      = NULL;
    
    m_hProcSvcCallbackMutex = NULL;
}

CSMClientThread::~CSMClientThread(void)
{
	//CloseHandle(m_hProcSvcCallbackMutex);
	//m_hProcSvcCallbackMutex = NULL;
}

BOOL CSMClientThread::InitSocketThread()
{
    USES_CONVERSION;

    if (FALSE == CSocketThread::InitSocketThread())
    {
        return FALSE;
    }

    if (FALSE == m_SMClientSocket.Create())
    {
        DWORD err = ::GetLastError();
        return FALSE;
    }

    if (FALSE == m_SMClientSocket.Connect(A2T(m_szServerIP), m_uiPort))
    {
        DWORD err = ::GetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            return FALSE;
        }
    }

    m_SMClientSocket.SetParentThread(this);

    //m_hProcSvcCallbackMutex = CreateMutex(NULL, FALSE, NULL);
    //if (NULL == m_hProcSvcCallbackMutex)
    //{
    //    return FALSE;
    //}

    return TRUE;
}

void CSMClientThread::SetCryptionData(char* pszCryptionKey, char* pszSrcValue, int nCryptionKeyLen, int nSrcValueSize)
{
	if (NULL != m_SMClientSocket)
		m_SMClientSocket.SetCryptionData(pszCryptionKey, pszSrcValue, nCryptionKeyLen, nSrcValueSize);
}

void CSMClientThread::ProcRecvSvcData(WPARAM wParam, LPARAM lParam)
{
    // Callback Call
    if (NULL != m_pProcSvcCallback)
    {
        char*   pData       = NULL;
        int     nDataCount  = -1;

        if ((0 < (int)wParam) && (NULL != lParam))
        {
            PFS2SM_CNT_BLOCK    pCntBlock   = (PFS2SM_CNT_BLOCK)lParam;

            nDataCount  = pCntBlock->uiCount; 
            pData       = (char*)((char*)lParam + sizeof(FS2SM_CNT_BLOCK));
            //TRACELOG(LEVEL_DBG, _T("Data = %hs, Count = %d"), pData, nDataCount);
        }
        else
        {
            if (SVC_DISCONNECT == (int)wParam)
            {
                nDataCount = 0;
            }
            pData       = (char*)lParam;
            //TRACELOG(LEVEL_DBG, _T("Error Data = %hs"), pData);
        }

        m_pProcSvcCallback(m_pProcObject, (int)wParam, (void*)pData, nDataCount, NULL);
    }
}

void CSMClientThread::SetProcCallbackData(void* pOjbect, ProcSvcCallback pProcSvcCallbackFunc)
{
    m_pProcObject       = pOjbect;
    m_pProcSvcCallback  = pProcSvcCallbackFunc;
}

void CSMClientThread::SetServerIP(char* pszServerIP)
{
    if ((NULL != pszServerIP) && (0 < strlen(pszServerIP)))
    {
        strncpy(m_szServerIP, pszServerIP, sizeof(m_szServerIP));
    }
}