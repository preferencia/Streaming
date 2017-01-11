#pragma once

#include "MsgWnd.h"
#include "SMServerSocket.h"
#include "SocketThread.h"
#include <map>

class ProcSvcData : public SendData
{
public:
    HWND hTargetWnd;
    int nSvcCode;
};

typedef list<ProcSvcData*>              ProcSvcDataQueue;
typedef list<ProcSvcData*>::iterator    ProcSvcDataQueueIt;

typedef map<int, int>                   OrderRqIdMap;

class CSMServerThread : public CSocketThread
{
public:
    CSMServerThread(void);
    virtual ~CSMServerThread(void);

    virtual BOOL            InitSocketThread();
    virtual void            SetCryptionData(char* pszCryptionKey, char* pszKeySrcValue, int nCryptionKeyLen, int nKeySrcValueLen);
    virtual CConnectSocket* GetSocket() { return &m_SMServerSocket; }
    virtual void            ProcRecvSvcData(WPARAM wParam, LPARAM lParam);
    virtual void            ProcRecvSvcMsg(WPARAM wParam, LPARAM lParam);
    virtual void            ProcUserMsg(int nMsgCode);

    BOOL                    SendRecvReplyData(void* pObject, int nSvcCode, void* pData, int nDataLen);

    void                    Attach(CMsgWnd* pRecvMsgWnd);
    void                    Dettach(CMsgWnd* pRecvMsgWnd);
    void                    SetProcCallbackData(void* pOjbect, 
                                                ProcSvcCallback pProcSvcCallbackFunc, 
                                                RecvProcCallback pRecvDataCallbackFunc,
                                                RecvProcCallback pRecvMsgCallbackFunc);
    void                    ProcReqSvcData(int nSvcCode, char* pData, int nDataLen, HWND hTargetWnd = NULL);
    
    void                    SetSendOnly(BOOL bSendOnly = TRUE)  {   m_bSendOnly = bSendOnly;    }
    BOOL                    GetSendOnlyStatus()                 {   return m_bSendOnly;         }

protected:
    static unsigned int __stdcall ProcSvcThread(void* lpParam);

private:
    BOOL                    m_bSendOnly;        // for RealTime Data 

    BOOL                    m_bRunProcSvcThread;
    HANDLE                  m_hProcSvcThread;

    CSMServerSocket         m_SMServerSocket;

    ProcSvcDataQueue*       m_pProcSvcDataQueue;
    ProcSvcDataQueueIt      m_ProcSvcDataQueueIt;

    OrderRqIdMap            m_MapOrderRqId;

    //CRITICAL_SECTION        m_csProcSvcCallback;
    HANDLE                  m_hProcSvcCallbackMutex;

    void*                   m_pProcObject;
    ProcSvcCallback         m_pProcSvcCallback;     // 조회, 주문 전송용 콜백
    RecvProcCallback        m_pRecvDataCallback;    // FreeCap 서버에서 데이터를 받았을 경우 파싱용 콜백
    RecvProcCallback        m_pRecvMsgCallback;     // FreeCap 서버에서 메시지(주로 에러)를 받았을 경우 파싱용 콜백

    CMsgWnd*                m_pRecvMsgWnd;
};
