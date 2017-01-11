
// CSMServerDlg.cpp : 구현 파일
//

#include "stdAfx.h"
#include "SMServer.h"
#include "ClientBase64Url.h"
#include "FSCryption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSMServer::CSMServer(CWnd* pParent /*=NULL*/)
{
    m_pParent               = pParent;
    memset(m_szKeySrcValue, 0, _DEC_KEY_SRC_VALUE_LEN + 1);
    m_pszCryptionKey        = NULL;
    m_pProcSvcCallback      = NULL;
    m_pRecvDataCallback     = NULL;
    m_pRecvMsgCallback      = NULL;
}

CSMServer::~CSMServer()
{
    DestroyWindow();
    SAFE_DELETE_ARRAY(m_pszCryptionKey);
}

// CSMServer 메시지 처리기

BEGIN_MESSAGE_MAP(CSMServer, CWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CSMServer::Create(CWnd* pParentWnd)
{
    m_pParent           = pParentWnd;

    TRACELOG(LEVEL_DBG, _T("Call CWnd::Create - Parent Window = 0x%x"), m_pParent);
    return CWnd::Create(NULL, _T("SMServer"), WS_CHILD, CRect(0, 0, 0, 0), pParentWnd, 100);
}

void CSMServer::SetCallbackFunc(ProcSvcCallback pProcSvcCallbackFunc, RecvProcCallback pRecvDataCallbackFunc, RecvProcCallback pRecvMsgCallbackFunc)
{
    m_pProcSvcCallback  = pProcSvcCallbackFunc;
    m_pRecvDataCallback = pRecvDataCallbackFunc;
    m_pRecvMsgCallback  = pRecvMsgCallbackFunc;

    m_SMListenSocket.SetProcCallbackData(m_pParent, m_pProcSvcCallback, m_pRecvDataCallback, m_pRecvMsgCallback);

    ::PostMessage(m_pParent->GetSafeHwnd(), UM_SYS_TRADING_PROC, WPARAM_SETUP_COMPLETE, LPARAM_COMPLETE_SET_CALLBACK_FUNCTION);
}

BOOL CSMServer::SendRecvReplyData(void* pObject, int nSvcCode, void* pData, int nDataLen)
{
    return m_SMListenSocket.SendRecvReplyData(pObject, nSvcCode, pData, nDataLen);
}

void CSMServer::RemoveCloseSession(SOCKET Socket)
{
    m_SMListenSocket.RemoveCloseSession(Socket);
}

BOOL CSMServer::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (-1 == CWnd::OnCreate(lpCreateStruct))
    {
        TRACELOG(LEVEL_DBG, _T("SMServer Create Failed!"));
        return -1;
    }

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
    if (FALSE == AfxSocketInit())
    {
        TRACELOG(LEVEL_DBG, _T("Socket Init Failed!"));
        return FALSE;
    }

    if (0 >= MakeCryptionKey(m_szKeySrcValue, _DEC_KEY_SRC_VALUE_LEN, &m_pszCryptionKey))
    {
        TRACELOG(LEVEL_DBG, _T("Make Cryption Key Failed!"));
        return FALSE;
    }

    if (FALSE == CreateSMListener())
    {
        TRACELOG(LEVEL_DBG, _T("SMServer Create Listener Failed!"));
        return FALSE;
    }

    TRACELOG(LEVEL_DBG, _T("Src = %hs, Key = %hs"), m_szKeySrcValue, m_pszCryptionKey);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CSMServer::OnDestroy()
{
    CWnd::OnDestroy();
}

BOOL CSMServer::CreateSMListener()
{
    if (FALSE == m_SMListenSocket.Create(_DEC_LISTEN_PORT))
    {
        ::PostMessage(m_pParent->GetSafeHwnd(), UM_SYS_TRADING_PROC, WPARAM_SETUP_FAILED, LPARAM_ALREADY_EXSIT_LISTEN_SOCKET);
        return FALSE;
    }

    m_SMListenSocket.SetParentWnd(this);
    m_SMListenSocket.SetCryptionData(m_pszCryptionKey, m_szKeySrcValue, strlen(m_pszCryptionKey), _DEC_KEY_SRC_VALUE_LEN);
    m_SMListenSocket.Listen();

    ::PostMessage(m_pParent->GetSafeHwnd(), UM_SYS_TRADING_PROC, WPARAM_SETUP_COMPLETE, LPARAM_COMPLETE_CREATE_LISTEN_SOCKET);

    return TRUE;
}