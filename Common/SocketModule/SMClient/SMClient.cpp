
// CSMClientDlg.cpp : 구현 파일
//

#include "stdAfx.h"
#include "SMClient.h"
#include "CommonDef.h"
#include "Common_Protocol.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSMClient::CSMClient(CWnd* pParent /*=NULL*/)
{
    m_pParent               = pParent;
    m_pSMClientThreads      = NULL;
    m_pszCryptionKey        = NULL;
    m_pProcSvcCallback      = NULL;
    m_nCurOrdKeyNum         = 0;
}

CSMClient::~CSMClient()
{
    DestroyWindow();

    SAFE_DELETE_ARRAY(m_pszCryptionKey);

    for (int nIndex = 0; nIndex < _DEC_CONNECT_CNT; ++nIndex)
    {
        if (NULL != m_pSMClientThreads[nIndex])
        {
            SAFE_DELETE(m_pSMClientThreads[nIndex]);
        }
    }
    SAFE_DELETE_ARRAY(m_pSMClientThreads);
}

// CSMClient 메시지 처리기

BEGIN_MESSAGE_MAP(CSMClient, CWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CSMClient::Create(CWnd* pParentWnd)
{
    m_pParent           = pParentWnd;

    TRACELOG(LEVEL_DBG, _T("Call CWnd::Create - Parent Window = 0x%x"), m_pParent);
    return CWnd::Create(NULL, _T("SMClient"), WS_CHILD, CRect(0, 0, 0, 0), pParentWnd, 100);
}

BOOL CSMClient::ConnectServer(int nPort, char* pszCrytpionKey, int nCryptionKeyLen)
{
    if ((NULL == pszCrytpionKey) || (0 >= nCryptionKeyLen))
    {
        return FALSE;
    }

    SAFE_DELETE_ARRAY(m_pszCryptionKey);
    SAFE_DELETE_ARRAY(m_pSMClientThreads);

    m_nCryptionKeyLen   = nCryptionKeyLen;
    m_pszCryptionKey    = new char[m_nCryptionKeyLen + 1];
    strcpy(m_pszCryptionKey, pszCrytpionKey);
    m_pszCryptionKey[m_nCryptionKeyLen] = 0;

    m_pSMClientThreads = new CSMClientThread*[_DEC_CONNECT_CNT];
    for (int nIndex = 0; nIndex < _DEC_CONNECT_CNT; ++nIndex)
    {
        m_pSMClientThreads[nIndex] = new CSMClientThread;
        if (NULL != m_pSMClientThreads[nIndex])
        {
            m_pSMClientThreads[nIndex]->SetServerIP(_DEC_SERVER_NAME);
            m_pSMClientThreads[nIndex]->SetServerPort(nPort);
            if (FALSE == m_pSMClientThreads[nIndex]->InitSocketThread())
            {
                return FALSE;
            }

            m_pSMClientThreads[nIndex]->SetCryptionData(m_pszCryptionKey, NULL, strlen(m_pszCryptionKey), 0);
            m_pSMClientThreads[nIndex]->SetProcCallbackData(m_pParent, m_pProcSvcCallback);
        }
        else
        {
            return FALSE;
        }
    }

    // 접속 후 인증
    RequestSvc(SVC_AUTH, m_pszCryptionKey, m_nCryptionKeyLen);

    return TRUE;
}

void CSMClient::DisconnectServer()
{
    if (NULL != m_pSMClientThreads)
    {
        for (int nIndex = 0; nIndex < _DEC_CONNECT_CNT; ++nIndex)
        {
            if (NULL != m_pSMClientThreads[nIndex])
            {
                m_pSMClientThreads[nIndex]->GetSocket()->DoClose();
            }
        }
    }
}

BOOL CSMClient::IsConnectedSever()
{
    if (NULL != m_pSMClientThreads)
    {
        for (int nIndex = 0; nIndex < _DEC_CONNECT_CNT; ++nIndex)
        {
            if (NULL != m_pSMClientThreads[nIndex])
            {
                if (FALSE == m_pSMClientThreads[nIndex]->GetSocket()->IsConnected())
                {
                    return FALSE;
                }
            }
        }

        return TRUE;
    }

    return FALSE;
}

int CSMClient::RequestSvc(int nSvcCode, char* pData, int nDataSize)
{
    if ((NULL == pData) || (0 >= nDataSize))
    {
        TRACELOG(LEVEL_DBG, _T("Data is wrong."));
        return -1;
    }

    if (NULL == m_pSMClientThreads)
    {
        TRACELOG(LEVEL_DBG, _T("Thread Array is NULL"));
        return -2;
    }

    CSMClientThread* pSMClientThread = NULL;
    int              nRet            = 0;

    if (SVC_FSO01 > nSvcCode)
    {
        // AUTH & QUERY
        pSMClientThread = m_pSMClientThreads[SOCK_INDEX_QUERY];
    }
    else if (SVC_FSR01 > nSvcCode)
    {
        // ORDER
        pSMClientThread = m_pSMClientThreads[SOCK_INDEX_ORDER];

        if (_DEC_MAX_ORD_KEY_NUM <= m_nCurOrdKeyNum)
        {
            m_nCurOrdKeyNum = 0;
        }

        PFS_ORD_COMMON pFSOrdCommon = (PFS_ORD_COMMON)pData;
        if (NULL != pFSOrdCommon)
        {
            pFSOrdCommon->nOrdKeyNum = m_nCurOrdKeyNum++;
        }

        nRet = pFSOrdCommon->nOrdKeyNum;
    }

    if (NULL == pSMClientThread)
    {
        TRACELOG(LEVEL_DBG, _T("Thread is NULL"));
        return -3;
    }

    int nSendBufSize    = 0;
    int nTotalDataSize  = (SVC_AUTH == nSvcCode) ? nDataSize : (nDataSize + m_nCryptionKeyLen);
    char* pBuf = new char[4096];
    memset(pBuf, 0, 4096);

    PFS_HEADER pHeader = (PFS_HEADER)pBuf;
    MAKE_FS_HEADER(pHeader, nSvcCode, 0, nTotalDataSize);
    nSendBufSize += sizeof(FS_HEADER);
    memcpy(pBuf + nSendBufSize, pData, nDataSize);
    nSendBufSize += nDataSize;
    if (SVC_AUTH < nSvcCode)
    {
        memcpy(pBuf + nSendBufSize, m_pszCryptionKey, m_nCryptionKeyLen);
        nSendBufSize += m_nCryptionKeyLen;
    }

    TRACELOG(LEVEL_DBG, _T("SVC Code = %d, Total Data Size = %d, Send Buf Size = %d"), nSvcCode, nTotalDataSize, nSendBufSize);

    pSMClientThread->Send(pBuf, nSendBufSize);

    return nRet;
}

BOOL CSMClient::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (-1 == CWnd::OnCreate(lpCreateStruct))
    {
        TRACELOG(LEVEL_DBG, _T("SMClient Create Failed!"));
        return -1;
    }

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
    if (FALSE == AfxSocketInit())
    {
        TRACELOG(LEVEL_DBG, _T("Socket Init Failed!"));
        return FALSE;
    }

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CSMClient::OnDestroy()
{
    CWnd::OnDestroy();
}