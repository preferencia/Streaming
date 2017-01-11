
// SMClient.h : ��� ����
//

#pragma once

#include "SMClientThread.h"

// CSMClient
class CSMClient : public CWnd
{
// �����Դϴ�.
public:
	CSMClient(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.
    ~CSMClient();

public:
    BOOL                Create(CWnd* pParentWnd);
    BOOL                ConnectServer(int nPort, char* pszCrytpionKey, int nCryptionKeyLen);
    void                DisconnectServer();
    BOOL                IsConnectedSever();
    int                 RequestSvc(int nSvcCode, char* pData, int nDataSize);

    void                SetCallbackFunc(ProcSvcCallback pProcSvcCallbackFunc)        {   m_pProcSvcCallback  = pProcSvcCallbackFunc;     }

// �����Դϴ�.
protected:
	// ������ �޽��� �� �Լ�
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
