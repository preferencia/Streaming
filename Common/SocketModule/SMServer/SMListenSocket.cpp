#include "StdAfx.h"
#include "SMListenSocket.h"

CSMListenSocket::CSMListenSocket(void)
{
    m_SMServerThreadMap.clear();
    m_pszCryptionKey    = NULL;
    m_pszKeySrcValue    = NULL;
    m_nCryptionKeyLen   = 0;
    m_nKeySrcValueLen   = 0;

    m_pProcObject       = NULL;
    m_pProcSvcCallback  = NULL;
    m_pRecvDataCallback = NULL;
    m_pRecvMsgCallback  = NULL;

    m_pParentWnd        = NULL;

    m_nCurThreadCount   = 0;
}

CSMListenSocket::~CSMListenSocket(void)
{
    if (0 < m_SMServerThreadMap.size())
    {
        for (m_SMServerThreadMapIt = m_SMServerThreadMap.begin(); 
            m_SMServerThreadMapIt != m_SMServerThreadMap.end(); ++m_SMServerThreadMapIt)
        {
            CSMServerThread* pSMServerThread = m_SMServerThreadMapIt->second;
            SAFE_DELETE(pSMServerThread);
        }

        m_SMServerThreadMap.clear();
    }    
}

void CSMListenSocket::SetCryptionData(char* pszCryptionKey, char* pszKeySrcValue, int nCryptionKeyLen, int nKeySrcValueLen)
{
    m_pszCryptionKey    = pszCryptionKey;
    m_pszKeySrcValue    = pszKeySrcValue;
    m_nCryptionKeyLen   = nCryptionKeyLen;
    m_nKeySrcValueLen   = nKeySrcValueLen;
}

void CSMListenSocket::SetProcCallbackData(  void* pOjbect, 
                                            ProcSvcCallback pProcSvcCallbackFunc, 
                                            RecvProcCallback pRecvDataCallbackFunc,
                                            RecvProcCallback pRecvMsgCallbackFunc)
{
    m_pProcObject       = pOjbect;
    m_pProcSvcCallback  = pProcSvcCallbackFunc;
    m_pRecvDataCallback = pRecvDataCallbackFunc;
    m_pRecvMsgCallback  = pRecvMsgCallbackFunc;
}

BOOL CSMListenSocket::SendRecvReplyData(void* pObject, int nSvcCode, void* pData, int nDataLen)
{
    if (0 < m_SMServerThreadMap.size())
    {
        for (m_SMServerThreadMapIt = m_SMServerThreadMap.begin(); 
            m_SMServerThreadMapIt != m_SMServerThreadMap.end(); ++m_SMServerThreadMapIt)
        {
            CSMServerThread* pSMServerThread = m_SMServerThreadMapIt->second;
            if (TRUE == pSMServerThread->GetSendOnlyStatus())
            {
                return pSMServerThread->SendRecvReplyData(pObject, nSvcCode, pData, nDataLen);
            }
        }
    }    

    return FALSE;
}

void CSMListenSocket::RemoveCloseSession(SOCKET Socket)
{
    if (0 < m_SMServerThreadMap.size())
    {
        m_SMServerThreadMapIt = m_SMServerThreadMap.find(Socket);
        if (m_SMServerThreadMap.end() != m_SMServerThreadMapIt)
        {
            CSMServerThread* pSMServerThread = m_SMServerThreadMapIt->second;
            SAFE_DELETE(pSMServerThread);
            m_SMServerThreadMap.erase(m_SMServerThreadMapIt);
        }
    }
}

void CSMListenSocket::OnAccept(int nErrorCode)
{
    if (ERROR_SUCCESS != nErrorCode)
    {
        return;
    }

    // New connection is being established
    CAsyncSocket AsyncSock;

    // Accept the connection using a temp CAsyncSocket object.
    if (FALSE == Accept(AsyncSock))
    {
        DWORD err = ::GetLastError();
        return;
    }

    CMsgWnd* pRecvMsgWnd  = new CMsgWnd;
    if (NULL != pRecvMsgWnd)
    {
        pRecvMsgWnd->Create(m_pParentWnd);
    }
    
    CSMServerThread* pSMServerThread = new CSMServerThread;
    SOCKET sock = AsyncSock.Detach();
    pSMServerThread->SetSocket(sock);
    pSMServerThread->SetCryptionData(m_pszCryptionKey, m_pszKeySrcValue, m_nCryptionKeyLen, m_nKeySrcValueLen);
    pSMServerThread->SetProcCallbackData(m_pProcObject, m_pProcSvcCallback, m_pRecvDataCallback, m_pRecvMsgCallback);
    pSMServerThread->Attach(pRecvMsgWnd);

    if (FALSE == pSMServerThread->InitSocketThread())
    {
        return;
    }

    if (SOCK_INDEX_RECV_REALTIME == m_SMServerThreadMap.size() % SOCK_INDEX_END)
    {
        pSMServerThread->SetSendOnly();
    }
    m_SMServerThreadMap[sock] = pSMServerThread;

    CAsyncSocket::OnAccept(nErrorCode);
}