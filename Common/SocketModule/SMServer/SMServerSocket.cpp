#include "StdAfx.h"
#include "SMServerSocket.h"
#include "SMServerThread.h"

CSMServerSocket::CSMServerSocket(void)
{
}

CSMServerSocket::~CSMServerSocket(void)
{
}

void CSMServerSocket::OnClose(int nErrorCode)
{
    CConnectSocket::OnClose(nErrorCode);

    // 시그널메이커 접속 해제 전달
    ((CSMServerThread*)m_pParentThread)->ProcUserMsg(WPARAM_CONNECTION_CLOSE);
}

int CSMServerSocket::ProcessReceive(char* lpBuf, int nDataLen)
{
    if (NULL == lpBuf)
    {
        TRACELOG(LEVEL_DBG, _T("Buffer is NULL"));
        if (NULL != m_pParentThread)
        {
            ((CSMServerThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Buffer is NULL");
        }
        return -1;
    }

    FS_HEADER* pHeader      = (FS_HEADER*)lpBuf;
    unsigned int uiChecksum = pHeader->uiSvcCode ^ pHeader->uiErrCode ^ pHeader->uiDataLen;
    if (pHeader->uiChecksum != uiChecksum)
    {
        TRACELOG(LEVEL_DBG, _T("Checksum Error"));
        if (NULL != m_pParentThread)
        {
            ((CSMServerThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Checksum Error");
        }
        return -2;
    }

    int nRecvSvcCode    = pHeader->uiSvcCode;
    int nRecvDataLen    = pHeader->uiDataLen;
    int nTotHeaderSize  = sizeof(FS_HEADER);
    if ((0 < nRecvDataLen) && (nDataLen < (nTotHeaderSize + nRecvDataLen)))
    {
        TRACELOG(LEVEL_DBG, _T("Need More Data - Total Data Len = %d, Receive Data Len = %d"), nRecvDataLen, nDataLen - nTotHeaderSize);
        if (NULL != m_pParentThread)
        {
            ((CSMServerThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Need More Data - Total Data Len = %d, Receive Data Len = %d", 
                                                             nRecvDataLen, nDataLen - nTotHeaderSize);
        }
        return (nTotHeaderSize + nRecvDataLen) - nDataLen;
    }

    char*   pProcData       = NULL;
    char*   pRecvAuthData   = NULL;
    BOOL    bDataLenOver    = FALSE;
    int     nAuthDataLen    = 0;
    int     nErr            = 0;
    int     nInputDataLen   = nDataLen;
    int     nOverDataLen    = 0;

    TRACELOG(LEVEL_DBG, _T("Recv SVC Code = %d, Data Length = %d"), nRecvSvcCode, nInputDataLen);
    if (NULL != m_pParentThread)
    {
        ((CSMServerThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Recv (SVC_Code = %d, Packet_size= %d, sock= %d)", nRecvSvcCode, nInputDataLen, m_hSocket);
        ((CSMServerThread*)m_pParentThread)->SaveDataLog(enumLogTypeData, nInputDataLen, lpBuf);
    }

    // exist extra data
    if (nInputDataLen > (nTotHeaderSize + nRecvDataLen))
    {
        bDataLenOver    = TRUE;
        nOverDataLen    = nInputDataLen - (nTotHeaderSize + nRecvDataLen);
        pProcData       = new char[(nTotHeaderSize + nRecvDataLen)];
        memcpy(pProcData, lpBuf, (nTotHeaderSize + nRecvDataLen));
        TRACELOG(LEVEL_DBG, _T("Over Data Length = %d"), nOverDataLen);
    }
    else
    {
        pProcData       = lpBuf;
    }

    switch (nRecvSvcCode)
    {
    case SVC_AUTH:
        {
            nAuthDataLen    = nRecvDataLen;
            pRecvAuthData   = (char*)(pProcData + sizeof(FS_HEADER));

            if (NULL == pRecvAuthData)
            {
                nErr = -1;
                goto $ERROR_BAILED;
            }
        }
        break;

    case SVC_FSQ01:
        {
            FSQ01_SM2FS* pFSQ01_SM2FS = (FSQ01_SM2FS*)(pProcData + sizeof(FS_HEADER));
            if ((NULL == pFSQ01_SM2FS) || (NULL == pFSQ01_SM2FS->pAuthData))
            {
                nErr = -1;
                goto $ERROR_BAILED;
            }

            nAuthDataLen    = nRecvDataLen - sizeof(FSQ01_SM2FS);
            pRecvAuthData   = pFSQ01_SM2FS->pAuthData;
        }
        break;

    case SVC_FSQ02:
        {
            FSQ02_SM2FS* pFSQ02_SM2FS = (FSQ02_SM2FS*)(pProcData + sizeof(FS_HEADER));
            if ((NULL == pFSQ02_SM2FS) || (NULL == pFSQ02_SM2FS->pAuthData))
            {
                nErr = -1;
                goto $ERROR_BAILED;
            }

            nAuthDataLen    = nRecvDataLen - sizeof(FSQ02_SM2FS);
            pRecvAuthData   = pFSQ02_SM2FS->pAuthData;
        }
        break;

    case SVC_FSQ03:
        {
            FSQ03_SM2FS* pFSQ03_SM2FS = (FSQ03_SM2FS*)(pProcData + sizeof(FS_HEADER));
            if ((NULL == pFSQ03_SM2FS) || (NULL == pFSQ03_SM2FS->pAuthData))
            {
                nErr = -1;
                goto $ERROR_BAILED;
            }

            nAuthDataLen    = nRecvDataLen - sizeof(FSQ03_SM2FS);
            pRecvAuthData   = pFSQ03_SM2FS->pAuthData;
        }
        break;

    case SVC_FSO01:
        {
            FSO01_SM2FS* pFSO01_SM2FS = (FSO01_SM2FS*)(pProcData + sizeof(FS_HEADER));
            if ((NULL == pFSO01_SM2FS) || (NULL == pFSO01_SM2FS->pAuthData))
            {
                nErr = -1;
                goto $ERROR_BAILED;
            }

            nAuthDataLen    = nRecvDataLen - sizeof(FSO01_SM2FS);
            pRecvAuthData   = pFSO01_SM2FS->pAuthData;
        }
        break;

    case SVC_FSO02:
        {
            FSO02_SM2FS* pFSO02_SM2FS = (FSO02_SM2FS*)(pProcData + sizeof(FS_HEADER));
            if ((NULL == pFSO02_SM2FS) || (NULL == pFSO02_SM2FS->pAuthData))
            {
                nErr = -1;
                goto $ERROR_BAILED;
            }

            nAuthDataLen    = nRecvDataLen - sizeof(FSO02_SM2FS);
            pRecvAuthData   = pFSO02_SM2FS->pAuthData;
        }
        break;

    case SVC_FSO03:
        {
            FSO03_SM2FS* pFSO03_SM2FS = (FSO03_SM2FS*)(pProcData + sizeof(FS_HEADER));
            if ((NULL == pFSO03_SM2FS) || (NULL == pFSO03_SM2FS->pAuthData))
            {
                nErr = -1;
                goto $ERROR_BAILED;
            }

            nAuthDataLen    = nRecvDataLen - sizeof(FSO03_SM2FS);
            pRecvAuthData   = pFSO03_SM2FS->pAuthData;
        }
        break;

    default:
        break;
    }

    //TRACELOG(LEVEL_DBG, _T("Recv Key = %hs, Key Lengh = %d"), pRecvAuthData, nAuthDataLen);

    // Auth Check
    nErr = CheckRecvCryptionKey(pRecvAuthData, nAuthDataLen);
    if (0 > nErr)
    {
        //TRACELOG(LEVEL_DBG, _T("Check Receive Cryption Key Error - Err Code = %d"), nErr);
        if (NULL != m_pParentThread)
        {
            ((CSMServerThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "  Check Receive Cryption Key Error [%d]", nErr);
        }
        goto $ERROR_BAILED;
    }

    // 인증은 바로 처리
    if (SVC_AUTH == nRecvSvcCode)
    {
        m_nCryptionKeyLen = MakeNewCryptionKey(m_pszKeySrcValue, m_nKeySrcValueLen, &m_pszCryptionKey);
        if (0 >= m_nCryptionKeyLen)
        {
            TRACELOG(LEVEL_DBG, _T("Make New Cryption Key Error"));
            nErr = -5;
            goto $ERROR_BAILED;
        }

        //TRACELOG(LEVEL_DBG, _T("New Send Key = %hs"), m_pszCryptionKey);

        int nSendBufLen = sizeof(FS_HEADER) + m_nCryptionKeyLen;
        char* pSendBuf  = new char[nSendBufLen];
        memset(pSendBuf, 0, nSendBufLen);
        MAKE_FS_HEADER((FS_HEADER*)pSendBuf, nRecvSvcCode, 0, m_nCryptionKeyLen);
        memcpy(pSendBuf + sizeof(FS_HEADER), m_pszCryptionKey, m_nCryptionKeyLen);
        DoSend(pSendBuf, nSendBufLen);

        // 인증 성공 전달
        if (NULL != m_pParentThread)
        {
            ((CSMServerThread*)m_pParentThread)->ProcUserMsg(WPARAM_AUTH_SUCCESS);
        }
    }
    else
    {
        if (NULL != m_pParentThread)
        {
            ((CSMServerThread*)m_pParentThread)->ProcReqSvcData(nRecvSvcCode, (char*)(pProcData + sizeof(FS_HEADER)), nRecvDataLen - nAuthDataLen);
        }
    }    

    if (TRUE == bDataLenOver)
    {
        SAFE_DELETE(pProcData);
        lpBuf = (char*)(lpBuf + (nTotHeaderSize + nRecvDataLen));
		return ProcessReceive(lpBuf, nOverDataLen);
    }
	else
		return 0;

$ERROR_BAILED:
    int nSendErrBufLen = sizeof(FS_HEADER);
    char* pSendErrBuf  = new char[nSendErrBufLen];
    memset(pSendErrBuf, 0, nSendErrBufLen);
    MAKE_FS_HEADER((FS_HEADER*)pSendErrBuf, nRecvSvcCode, nRecvSvcCode * nErr, 0);
    DoSend(pSendErrBuf, nSendErrBufLen);
    return 0;
}

void CSMServerSocket::DoSend(char* pBuf, int nBufLen)
{
    if (NULL != m_pParentThread)
    {
        ((CSMServerThread*)m_pParentThread)->Send(pBuf, nBufLen);
    }
}
