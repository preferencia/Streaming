#pragma once

#include "ConnectSocket.h"

#include <list>
using namespace std;

typedef struct _SendData
{
    int nDataLen;
    char* pData;
} SendData;

typedef list<SendData*>                 SendDataQueue;
typedef list<SendData*>::iterator       SendDataQueueIt;

typedef void (*ProcSvcCallback)(void*, IN int, IN void*, IN int, IN HWND);
typedef void (*RecvProcCallback)(void*, IN WPARAM, IN LPARAM, OUT int&, OUT int&, OUT int&, OUT char**);

class CSocketThread
{
public:
    CSocketThread(void);
    ~CSocketThread(void);

    void            SetSocket(SOCKET socket)                {   m_Socket = socket;     }
    virtual void    Send(char* pData, int nDataLen);
    virtual void    ProcRecvSvcData(WPARAM wParam, LPARAM lParam)   {};
    virtual void    ProcRecvSvcMsg(WPARAM wParam, LPARAM lParam)    {};
    virtual void    ProcUserMsg(int nMsgCode)                       {};

    void            SaveDataLog(int nLogType, int nLen, char* pszFmt, ...);

protected:
    virtual CConnectSocket* GetSocket() {   return NULL;    }
    virtual BOOL            InitSocketThread();
    virtual void            SetCryptionData(char* pszCryptionKey, char* pszKeySrcValue, int nCryptionKeyLen, int nKeySrcValueLen) = 0;

    static unsigned int __stdcall   SendThread(void* lpParam);

protected:
    BOOL                    m_bRunSendThread;
    HANDLE                  m_hSendThread;
    SOCKET                  m_Socket;

    SendDataQueue*          m_pSendDataQueue;
    SendDataQueueIt         m_SendDataQueueIt;

    HANDLE                  m_hSendDataQueueMutex;

    int                     m_nThreadType;
};
