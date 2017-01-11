#include "StdAfx.h"
#include "SMClientSocket.h"
#include "SocketThread.h"

CSMClientSocket::CSMClientSocket(void)
{
}

CSMClientSocket::~CSMClientSocket(void)
{
}

void CSMClientSocket::OnClose(int nErrorCode)
{
    CConnectSocket::OnClose(nErrorCode);
    ((CSocketThread*)m_pParentThread)->ProcRecvSvcData((WPARAM)SVC_DISCONNECT, (LPARAM)NULL);
}

int CSMClientSocket::ProcessReceive(char* lpBuf, int nDataLen)
{
    if (NULL == lpBuf)
    {
        TRACELOG(LEVEL_DBG, _T("Data is Wrong"));
        if (NULL != m_pParentThread)
        {
            ((CSocketThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Data is Wrong");
        }
        return -1;
    }

    FS_HEADER* pHeader      = (FS_HEADER*)lpBuf;
    unsigned int uiChecksum = pHeader->uiSvcCode ^ pHeader->uiErrCode ^ pHeader->uiDataLen;
    if (pHeader->uiChecksum != uiChecksum)
    {
        TRACELOG(LEVEL_DBG, _T("Checksum is Wrong - recv[%u], result[%u]"), pHeader->uiChecksum, uiChecksum);
        if (NULL != m_pParentThread)
        {
            ((CSocketThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Checksum is Wrong - recv[%u], result[%u]", pHeader->uiChecksum, uiChecksum);
        }
        return -2;
    }

    int nRecvSvcCode        = pHeader->uiSvcCode;
    int nRecvErrCode        = pHeader->uiErrCode;
    int nRecvDataLen        = pHeader->uiDataLen;
    int nTotHeaderSize      = (SVC_AUTH >= nRecvSvcCode) ? sizeof(FS_HEADER) : sizeof(FS_HEADER) + sizeof(FS2SM_CNT_BLOCK);
    if ((0 < nRecvDataLen) && (nDataLen < (nTotHeaderSize + nRecvDataLen)))
    {
        TRACELOG(LEVEL_DBG, _T("Need More Data - Total Data Len = %d, Receive Data Len = %d"), nRecvDataLen, nDataLen - nTotHeaderSize);
        if (NULL != m_pParentThread)
        {
            ((CSocketThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Need More Data - Total Data Len = %d, Receive Data Len = %d", 
                                                           nRecvDataLen, nDataLen - nTotHeaderSize);
        }
        return (nTotHeaderSize + nRecvDataLen) - nDataLen;
    }

    TRACELOG(LEVEL_DBG, _T("nRecvSvcCode = %d, nRecvErrCode = %d, nRecvDataLen = %d"), nRecvSvcCode, nRecvErrCode, nRecvDataLen);
    if (NULL != m_pParentThread)
    {
        ((CSocketThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "Recv (Packet_Size = %d, Data_Len= %d)", nDataLen, nRecvDataLen);
        ((CSocketThread*)m_pParentThread)->SaveDataLog(enumLogTypeData, nDataLen, lpBuf);
    }

    char*   pProcData       = NULL;
    BOOL    bDataLenOver    = FALSE;
    int     nInputDataLen   = nDataLen;
    int     nOverDataLen    = 0;

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

    // FS2SM_CNT_BLOCK까지 같이 넘겨줌
    char* pRecvData         = (char*)(pProcData + sizeof(FS_HEADER));

    if (SVC_AUTH == nRecvSvcCode)
    {
        FS_AUTH_FS2SM* pAuth = (FS_AUTH_FS2SM*)pRecvData;
        if (NULL == pAuth)
        {
            return -3;
        }

        if (NULL != pAuth->pNewAuthData)
		{
			if (NULL != m_pszCryptionKey)
			{
				//TRACELOG(LEVEL_DBG, _T("Key[Before] = %hs"), m_pszCryptionKey);
				memcpy(m_pszCryptionKey, pAuth->pNewAuthData, nRecvDataLen);
				m_nCryptionKeyLen = nRecvDataLen;
				//TRACELOG(LEVEL_DBG, _T("Key[After] = %hs"), m_pszCryptionKey);
			}
		}
	}
    else
    {
        if ((NULL != m_pParentThread) && (NULL != pRecvData))
        {
            TRACELOG(LEVEL_DBG, _T("Svc Code = %d, Err Code = %d, Data Size = %d"), nRecvSvcCode, nRecvErrCode, nRecvDataLen);
            ((CSocketThread*)m_pParentThread)->ProcRecvSvcData((WPARAM)nRecvSvcCode, (LPARAM)pRecvData);
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
}
