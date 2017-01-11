#pragma once

#include "CommonDef.h"
#include "Common_Protocol.h"
#include "SMServerThread.h"
#include <map>

using namespace std;
typedef std::map<SOCKET, CSMServerThread*>            SMServerThreadMap;
typedef std::map<SOCKET, CSMServerThread*>::iterator  SMServerThreadMapIt;

class CSMListenSocket : public CAsyncSocket
{
public:
    CSMListenSocket(void);
    virtual ~CSMListenSocket(void);

    void            SetParentWnd(CWnd* pParentWnd)  { m_pParentWnd = pParentWnd; }
    void            SetCryptionData(char* pszCryptionKey, char* pszKeySrcValue, int nCrpytionKeyLen, int nKeySrcValueLen);
    void            SetProcCallbackData(void* pOjbect, 
                                        ProcSvcCallback pProcSvcCallbackFunc, 
                                        RecvProcCallback pRecvDataCallbackFunc, 
                                        RecvProcCallback pRecvMsgCallbackFunc);
    BOOL            SendRecvReplyData(void* pObject, int nSvcCode, void* pData, int nDataLen);

    void            RemoveCloseSession(SOCKET Socket);

protected:
    virtual void    OnAccept(int nErrorCode);

private:
    //CSMServerThread*  m_pServerThread;
    SMServerThreadMap   m_SMServerThreadMap;
    SMServerThreadMapIt m_SMServerThreadMapIt;

    char*               m_pszCryptionKey;
    char*               m_pszKeySrcValue;
    int                 m_nCryptionKeyLen;
    int                 m_nKeySrcValueLen;

    void*               m_pProcObject;
    ProcSvcCallback     m_pProcSvcCallback;
    RecvProcCallback    m_pRecvDataCallback;
    RecvProcCallback    m_pRecvMsgCallback;

    CWnd*               m_pParentWnd;

    int                 m_nCurThreadCount;
};
