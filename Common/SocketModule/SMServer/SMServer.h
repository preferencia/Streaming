
// SMServer.h : ��� ����
//

#pragma once

#include "SMListenSocket.h"

// CSMServer
class CSMServer : public CWnd
{
// �����Դϴ�.
public:
	CSMServer(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.
    ~CSMServer();

public:
    char*               GetCryptionKey()    {   return m_pszCryptionKey;   }
    BOOL                Create(CWnd* pParentWnd);
    void                SetCallbackFunc(ProcSvcCallback pProcSvcCallbackFunc, RecvProcCallback pRecvDataCallbackFunc, RecvProcCallback pRecvMsgCallbackFunc);
    BOOL                SendRecvReplyData(void* pObject, int nSvcCode, void* pData, int nDataLen);
    void                RemoveCloseSession(SOCKET Socket);

// �����Դϴ�.
protected:
	// ������ �޽��� �� �Լ�
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
