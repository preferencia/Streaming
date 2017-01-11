
// SMServer.h : 헤더 파일
//

#pragma once

#include "SMListenSocket.h"

// CSMServer
class CSMServer : public CWnd
{
// 생성입니다.
public:
	CSMServer(CWnd* pParent = NULL);	// 표준 생성자입니다.
    ~CSMServer();

public:
    char*               GetCryptionKey()    {   return m_pszCryptionKey;   }
    BOOL                Create(CWnd* pParentWnd);
    void                SetCallbackFunc(ProcSvcCallback pProcSvcCallbackFunc, RecvProcCallback pRecvDataCallbackFunc, RecvProcCallback pRecvMsgCallbackFunc);
    BOOL                SendRecvReplyData(void* pObject, int nSvcCode, void* pData, int nDataLen);
    void                RemoveCloseSession(SOCKET Socket);

// 구현입니다.
protected:
	// 생성된 메시지 맵 함수
    afx_msg int			OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void		OnDestroy();
	DECLARE_MESSAGE_MAP()

private:
    BOOL                CreateSMListener();

private:
    CWnd*				m_pParent;

    CSMListenSocket     m_SMListenSocket;
    char                m_szKeySrcValue[_DEC_KEY_SRC_VALUE_LEN + 1];
    char*               m_pszCryptionKey;

    ProcSvcCallback     m_pProcSvcCallback;
    RecvProcCallback    m_pRecvDataCallback;
    RecvProcCallback    m_pRecvMsgCallback;
};
