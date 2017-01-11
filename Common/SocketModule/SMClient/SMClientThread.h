#pragma once

#include "SocketThread.h"
#include "SMClientSocket.h"

class CSMClientThread : public CSocketThread
{
public:
    CSMClientThread(void);
    virtual ~CSMClientThread(void);

    virtual BOOL            InitSocketThread();
    virtual void            SetCryptionData(char* pszCryptionKey, char* pszSrcValue, int nCryptionKeyLen, int nSrcValueSize);
    virtual void            ProcRecvSvcData(WPARAM wParam, LPARAM lParam);
    virtual CConnectSocket* GetSocket() { return &m_SMClientSocket; }

    void                    SetProcCallbackData(void* pOjbect, ProcSvcCallback pProcSvcCallbackFunc);
    void                    SetServerIP(char* pszServerIP);
    void                    SetServerPort(UINT uiPort)  {   m_uiPort = uiPort;  }

private:
    CSMClientSocket         m_SMClientSocket;
    char                    m_szServerIP[_DEC_IP_ADDR_LEN];
    UINT                    m_uiPort;

    HANDLE                  m_hProcSvcCallbackMutex;

    void*                   m_pProcObject;
    ProcSvcCallback         m_pProcSvcCallback;     // 조회, 주문 응답 콜백
};
