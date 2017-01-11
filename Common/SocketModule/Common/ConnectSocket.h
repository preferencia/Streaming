#pragma once

#include "CommonDef.h"
#include "Common_Protocol.h"

class CConnectSocket : public CAsyncSocket
{
public:
    CConnectSocket(void);
    virtual ~CConnectSocket(void);

    void            SetParentThread(void* pParentThread)    {   m_pParentThread = pParentThread;    }
    void            SetCryptionData(char* pszCryptionKey, char* pszKeySrcValue, int nCryptionKeyLen, int nKeySrcValueLen);

    int             DoSendData(const void* lpBuf, int nBufLen, int nFlags = 0);
    void            DoClose();

    BOOL            IsConnected()   {   return m_bIsConnected;    }

protected:
    virtual int     ProcessReceive(char* lpBuf, int nDataLen) = 0; // define in subclasses
    
    void            AbortConnection(DWORD err);
    int             CheckRecvCryptionKey(char* pszCryptionKey, int nCryptionKeyLen);
    int             MakeNewCryptionKey(char* pszIn, int nInputLen, char** pszOut);

    virtual void    OnConnect(int nErrorCode);
    virtual void    OnClose(int nErrorCode);
    virtual void    OnReceive(int nErrorCode);
    virtual void    OnSend(int nErrorCode);
    virtual int     Receive(void* lpBuf, int nBufLen, int nFlags = 0);
    virtual int     Send(const void* lpBuf, int nBufLen, int nFlags = 0);

protected:
    void*           m_pParentThread;
    char*           m_pszCryptionKey;
    char*           m_pszKeySrcValue;
    int             m_nCryptionKeyLen;
    int             m_nKeySrcValueLen;
    BOOL            m_bIsConnected;
};
