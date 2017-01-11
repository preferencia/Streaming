
// SMClient.h : 헤더 파일
//

#pragma once

#include "SMClientThread.h"

// CSMClient
class CSMClient : public CWnd
{
// 생성입니다.
public:
	CSMClient(CWnd* pParent = NULL);	// 표준 생성자입니다.
    ~CSMClient();

public:
    BOOL                Create(CWnd* pParentWnd);
    BOOL                ConnectServer(int nPort, char* pszCrytpionKey, int nCryptionKeyLen);
    void                DisconnectServer();
    BOOL                IsConnectedSever();
    int                 RequestSvc(int nSvcCode, char* pData, int nDataSize);

    void                SetCallbackFunc(ProcSvcCallback pProcSvcCallbackFunc)        {   m_pProcSvcCallback  = pProcSvcCallbackFunc;     }

// 구현입니다.
protected:
	// 생성된 메시지 맵 함수
    afx_msg int			OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void		OnDestroy();
	DECLARE_MESSAGE_MAP()

private:

private:
    CWnd*				m_pParent;

    CSMClientThread**   m_pSMClientThreads;
    char*               m_pszCryptionKey;
    int                 m_nCryptionKeyLen;
    int                 m_nCurOrdKeyNum;

    ProcSvcCallback     m_pProcSvcCallback;
};
