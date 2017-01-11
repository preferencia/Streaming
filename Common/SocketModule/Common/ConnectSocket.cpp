#include "StdAfx.h"
#include "ConnectSocket.h"
#include "FSCryption.h"
#include "SMServerThread.h"

CConnectSocket::CConnectSocket(void)
{
    m_pParentThread     = NULL;
    m_pszCryptionKey    = NULL;
    m_pszKeySrcValue    = NULL;
    m_nCryptionKeyLen   = 0;
    m_nKeySrcValueLen   = 0;
    m_bIsConnected      = FALSE;
}

CConnectSocket::~CConnectSocket(void)
{
    //SAFE_DELETE_ARRAY(m_pszCryptionKey);
    //SAFE_DELETE_ARRAY(m_pszKeySrcValue);
}

void CConnectSocket::SetCryptionData(char* pszCryptionKey, char* pszKeySrcValue, int nCryptionKeyLen, int nKeySrcValueLen)
{
    m_pszCryptionKey    = pszCryptionKey;
    m_pszKeySrcValue    = pszKeySrcValue;
    m_nCryptionKeyLen   = nCryptionKeyLen;
    m_nKeySrcValueLen   = nKeySrcValueLen;

    //SAFE_DELETE_ARRAY(m_pszCryptionKey);
    //SAFE_DELETE_ARRAY(m_pszKeySrcValue);

    //m_pszCryptionKey = new char[m_nCryptionKeyLen + 1];
    //memset(m_pszCryptionKey, 0, m_nCryptionKeyLen + 1);
    //memcpy(m_pszCryptionKey, pszCryptionKey, m_nCryptionKeyLen);

    //m_pszKeySrcValue = new char[m_nKeySrcValueLen + 1];
    //memset(m_pszKeySrcValue, 0, m_nKeySrcValueLen + 1);
    //memcpy(m_pszKeySrcValue, pszKeySrcValue, m_nKeySrcValueLen);
}

int CConnectSocket::DoSendData(const void* lpBuf, int nBufLen, int nFlags /* = 0 */)
{
    return Send(lpBuf, nBufLen, nFlags);
}

void CConnectSocket::DoClose()
{
    ShutDown();
    Close();
    m_bIsConnected = FALSE;
}

void CConnectSocket::AbortConnection(DWORD err)
{
    DoClose();
}

int CConnectSocket::CheckRecvCryptionKey(char* pszCryptionKey, int nCryptionKeyLen)
{
    if ((NULL == pszCryptionKey) || (0 == nCryptionKeyLen))
    {
        return -1;
    }

    char* pszRecvCryptionKey = new char[nCryptionKeyLen + 1];
    memcpy(pszRecvCryptionKey, pszCryptionKey, nCryptionKeyLen);
    pszRecvCryptionKey[nCryptionKeyLen] = 0;

    TRACELOG(LEVEL_DBG, _T("Has %hs[%d], Recv = %hs[%d]"), m_pszCryptionKey, m_nCryptionKeyLen, pszRecvCryptionKey, nCryptionKeyLen);

    if ((m_nCryptionKeyLen != nCryptionKeyLen) || (0 != strncmp(m_pszCryptionKey, pszRecvCryptionKey, m_nCryptionKeyLen)))
    {
		if (NULL != m_pParentThread)
		{
			((CSMServerThread*)m_pParentThread)->SaveDataLog(enumLogTypeStr, 0, "  Has %hs[%d](%p), Recv = %hs[%d]", m_pszCryptionKey, m_nCryptionKeyLen, m_pszCryptionKey, pszRecvCryptionKey, nCryptionKeyLen);
		}
		SAFE_DELETE_ARRAY(pszRecvCryptionKey);
        return -2;
    }

	int rtn = 0;
    char* pszDecData    = NULL;
    int nDecDataLen     = DecodeCryptionKey(pszRecvCryptionKey, nCryptionKeyLen, &pszDecData);
    SAFE_DELETE_ARRAY(pszRecvCryptionKey);

    if ((NULL == pszDecData) || (0 >= nDecDataLen))
    {
        rtn = -3;
    }
    else if ((m_nKeySrcValueLen != nDecDataLen) || (0 != strncmp(m_pszKeySrcValue, pszDecData, m_nKeySrcValueLen)))
    {
        rtn = -4;
    }

    SAFE_DELETE_ARRAY(pszDecData);

    return rtn;
}

int CConnectSocket::MakeNewCryptionKey(char* pszIn, int nInputLen, char** pszOut)
{
	char* tmpCryptionKey = NULL;
    memset(pszIn, 0, nInputLen);
    int outLen = MakeCryptionKey(pszIn, nInputLen, &tmpCryptionKey);

	memcpy(*pszOut, tmpCryptionKey, outLen);
    SAFE_DELETE_ARRAY(tmpCryptionKey);

    return outLen;
}

void CConnectSocket::OnConnect(int nErrorCode)
{
    m_bIsConnected = TRUE;
    CAsyncSocket::OnConnect(nErrorCode);
}

void CConnectSocket::OnClose(int nErrorCode)
{
    CAsyncSocket::OnClose(nErrorCode);

    DoClose();
}

void CConnectSocket::OnReceive(int nErrorCode)
{
    if (ERROR_SUCCESS != nErrorCode)
    {
        // error occurred.
        AbortConnection(nErrorCode);
        return;
    }

    char    RecvProcBuf[_DEC_MAX_BUFFER_SIZE]   = {0, };
    int     nReadLen                            = 0;

    nReadLen = Receive(RecvProcBuf, _DEC_MAX_BUFFER_SIZE);
    if (0 >= nReadLen)
    {
        TRACELOG(LEVEL_DBG, _T("Receive Error"));
        return;
    }
    
    int nResult = ProcessReceive(RecvProcBuf, nReadLen);
    if (0 < nResult)
    {
        TRACELOG(LEVEL_DBG, _T("Extra Receive Data Size = %d"), nResult);
        char* pNewRecvProcBuf = new char[_DEC_MAX_BUFFER_SIZE + nResult];
        memset(pNewRecvProcBuf, 0,              _DEC_MAX_BUFFER_SIZE + nResult);
        memcpy(pNewRecvProcBuf, RecvProcBuf,    _DEC_MAX_BUFFER_SIZE);
        
        nReadLen = Receive(pNewRecvProcBuf + _DEC_MAX_BUFFER_SIZE, nResult);
        if (0 >= nReadLen)
        {
            TRACELOG(LEVEL_DBG, _T("Extra Data Receive Error"));
			SAFE_DELETE_ARRAY(pNewRecvProcBuf);
            return;
        }

        nResult = ProcessReceive(pNewRecvProcBuf, _DEC_MAX_BUFFER_SIZE + nResult);
        SAFE_DELETE_ARRAY(pNewRecvProcBuf);
    }

    CAsyncSocket::OnReceive(nErrorCode);
}

void CConnectSocket::OnSend(int nErrorCode)
{
    CAsyncSocket::OnSend(nErrorCode);
}

int CConnectSocket::Receive(void* lpBuf, int nBufLen, int nFlags /*= 0*/)
{
    return CAsyncSocket::Receive(lpBuf, nBufLen, nFlags);
}

int CConnectSocket::Send(const void* lpBuf, int nBufLen, int nFlags /*= 0*/)
{
    return CAsyncSocket::Send(lpBuf, nBufLen, nFlags);
}