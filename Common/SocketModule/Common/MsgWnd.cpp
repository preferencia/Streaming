#include "stdAfx.h"
#include "MsgWnd.h"
#include "SocketThread.h"

#define WM_GET_DATA				WM_USER + 602		// 화면에게 조회응답데이터(RP) 처리를 하도록하는 메세지(STS)
#define WM_GET_MSG				WM_USER + 603       // 에러 메시지 처리
#define UM_MSG_REAL_NOTI        WM_USER + 5001      // 실시간 데이터 메시지 (실계좌)
#define UM_MSG_SIMUL_NOTI       WM_USER + 1101      // 실시간 데이터 메시지 (모의투자)
#define UM_RECV_DATA_MSG        WM_GET_DATA
#define UM_RECV_ERROR_MSG       WM_GET_MSG

#define _DEC_GET_PACKET_SIZE    4012    // sizeof(AS_SOCK_PACKET)

CMsgWnd::CMsgWnd(CWnd* pWnd /* = NULL */)
{
    m_pParentThread             = NULL;
    m_hCallbackMutex            = NULL;
    m_hProcRecvSvcPktListMutex  = NULL;
    m_hProcRecvSvcPktThread     = NULL;
    m_bRunProcRecvSvcPktThread  = FALSE;

    m_ProcRecvSvcPktList.clear();
}

CMsgWnd::~CMsgWnd(void)
{
    m_bRunProcRecvSvcPktThread = FALSE;
    WaitForSingleObject(m_hProcRecvSvcPktThread, INFINITE);

    if (NULL != m_hProcRecvSvcPktListMutex)
    {
        CloseHandle(m_hProcRecvSvcPktListMutex);
        m_hProcRecvSvcPktListMutex = NULL;
    }

    if (NULL != m_hCallbackMutex)
    {
        CloseHandle(m_hCallbackMutex);
        m_hCallbackMutex = NULL;
    }

    if (0 < m_ProcRecvSvcPktList.size())
    {
        for (m_ProcRecvSvcPktListIt = m_ProcRecvSvcPktList.begin(); m_ProcRecvSvcPktListIt != m_ProcRecvSvcPktList.end(); ++m_ProcRecvSvcPktListIt)
        {
            ProcRecvSvcPktThreadParam* pThreadParam = *m_ProcRecvSvcPktListIt;
            if (NULL != pThreadParam)
            {
                SAFE_DELETE_ARRAY(pThreadParam->pData);
                SAFE_DELETE(pThreadParam);
            }
        }
        m_ProcRecvSvcPktList.clear();
    }
}

BEGIN_MESSAGE_MAP(CMsgWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_MESSAGE(UM_RECV_DATA_MSG,    OnRecvData)
    ON_MESSAGE(UM_RECV_ERROR_MSG,   OnRecvMsg)
    ON_MESSAGE(UM_MSG_REAL_NOTI,    OnRecvRealNotiMsg)
    ON_MESSAGE(UM_MSG_SIMUL_NOTI,   OnRecvSimulNotiMsg)
END_MESSAGE_MAP()

BOOL CMsgWnd::Create(CWnd* pParentWnd)
{
    return CWnd::Create(NULL, _T("ThreadMsgWnd"), WS_CHILD, CRect(0, 0, 0, 0), pParentWnd, 10000);
}

BOOL CMsgWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (-1 == CWnd::OnCreate(lpCreateStruct))
    {
        return -1;
    }

    m_hCallbackMutex            = CreateMutex(NULL, FALSE, NULL);
    m_hProcRecvSvcPktListMutex  = CreateMutex(NULL, FALSE, NULL);

    m_hProcRecvSvcPktThread     = (HANDLE)_beginthreadex(NULL, 0, ProcRecvSvcPktThread, this, 0, NULL);
    if (NULL != m_hProcRecvSvcPktThread)
    {
        m_bRunProcRecvSvcPktThread = TRUE;
    }

    return TRUE;
}

void CMsgWnd::OnDestroy()
{
    CWnd::OnDestroy();
}

LRESULT CMsgWnd::OnRecvData(WPARAM wParam, LPARAM lParam)
{
    ProcRecvSvcPktThreadParam* pThreadParam = new ProcRecvSvcPktThreadParam;
    pThreadParam->nProcType     = 0;    // OnGetData
    pThreadParam->nDataParam    = (int)wParam;
    pThreadParam->pData         = new unsigned char[pThreadParam->nDataParam];
    memset(pThreadParam->pData, 0,              pThreadParam->nDataParam);
    memcpy(pThreadParam->pData, (void*)lParam,  pThreadParam->nDataParam);

    WaitForSingleObject(m_hProcRecvSvcPktListMutex, INFINITE);
    m_ProcRecvSvcPktList.push_back(pThreadParam);
    ReleaseMutex(m_hProcRecvSvcPktListMutex);

    delete (BYTE*)lParam;   // OnGetData()에서 lParam을 delete하므로 제거 필요
    
    return 0;
}

LRESULT CMsgWnd::OnRecvMsg(WPARAM wParam, LPARAM lParam)
{
    ProcRecvSvcPktThreadParam* pThreadParam = new ProcRecvSvcPktThreadParam;
    pThreadParam->nProcType     = 1;    // OnGetMsg
    pThreadParam->nDataParam    = (int)wParam;
    pThreadParam->pData         = new unsigned char[_DEC_GET_PACKET_SIZE];
    memset(pThreadParam->pData, 0,              _DEC_GET_PACKET_SIZE);
    memcpy(pThreadParam->pData, (void*)lParam,  _DEC_GET_PACKET_SIZE);

    WaitForSingleObject(m_hProcRecvSvcPktListMutex, INFINITE);
    m_ProcRecvSvcPktList.push_back(pThreadParam);
    ReleaseMutex(m_hProcRecvSvcPktListMutex);

    return 0;
}

LRESULT CMsgWnd::OnRecvRealNotiMsg(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CMsgWnd::OnRecvSimulNotiMsg(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

unsigned int __stdcall CMsgWnd::ProcRecvSvcPktThread(void* lpParam)
{
    CMsgWnd* pThis = (CMsgWnd*)lpParam;
    if (NULL == pThis)
    {
        return -1;
    }

    while (TRUE == pThis->m_bRunProcRecvSvcPktThread)
    {
        WaitForSingleObject(pThis->m_hProcRecvSvcPktListMutex, CLK_TCK);
        if (0 >= pThis->m_ProcRecvSvcPktList.size())
        {
            ReleaseMutex(pThis->m_hProcRecvSvcPktListMutex);
            Sleep(10);
            continue;
        }

        ProcRecvSvcPktThreadParam* pThreadParam = pThis->m_ProcRecvSvcPktList.front();
        if (NULL != pThreadParam)
        {
            TRACELOG(LEVEL_DBG, _T("Callback In. Proc Type = %s"), (0 == pThreadParam->nProcType) ? _T("Data") : _T("Msg"));

            WaitForSingleObject(pThis->m_hCallbackMutex, INFINITE);
            if (0 == pThreadParam->nProcType)
            {
                ((CSocketThread*)pThis->m_pParentThread)->ProcRecvSvcData((WPARAM)pThreadParam->nDataParam, (LPARAM)pThreadParam->pData);
            }
            else
            {
                ((CSocketThread*)pThis->m_pParentThread)->ProcRecvSvcMsg((WPARAM)pThreadParam->nDataParam, (LPARAM)pThreadParam->pData);
                SAFE_DELETE_ARRAY(pThreadParam->pData); // OnGetMsg() 내부에서 delete lparam을 하지 않으므로 삭제 처리 필요
            }
            ReleaseMutex(pThis->m_hCallbackMutex);

            TRACELOG(LEVEL_DBG, _T("Callback Out."));

            SAFE_DELETE(pThreadParam);
            pThis->m_ProcRecvSvcPktList.pop_front();
        }
        ReleaseMutex(pThis->m_hProcRecvSvcPktListMutex);
    }

    CloseHandle(pThis->m_hProcRecvSvcPktThread);
    pThis->m_hProcRecvSvcPktThread = NULL;

    return 0;
}