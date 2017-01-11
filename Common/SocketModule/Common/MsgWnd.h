#pragma once

#include <list>

#define _USE_PARAM_LIST

typedef struct ProcRecvSvcPktThreadParam
{
    int nProcType;
    int nDataParam;
    unsigned char* pData;
} ProcRecvSvcPktThreadParam;

typedef std::list<ProcRecvSvcPktThreadParam*>            ProcRecvSvcPktList;
typedef std::list<ProcRecvSvcPktThreadParam*>::iterator  ProcRecvSvcPktListIt;

class CMsgWnd : public CWnd
{
public:
    CMsgWnd(CWnd* pWnd = NULL);
    virtual ~CMsgWnd(void);

    BOOL                    Create(CWnd* pParentWnd);
    void                    SetParentThread(void* pParentThread)    {   m_pParentThread = pParentThread;    }

    // �����Դϴ�.
protected:
    // ������ �޽��� �� �Լ�
    afx_msg int			    OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void		    OnDestroy();
    DECLARE_MESSAGE_MAP()
    afx_msg LRESULT         OnRecvData(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT         OnRecvMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT         OnRecvRealNotiMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT         OnRecvSimulNotiMsg(WPARAM wParam, LPARAM lParam);

private:
    static unsigned int __stdcall ProcRecvSvcPktThread(void* lpParam);

private:
    void*                   m_pParentThread;
    HANDLE                  m_hCallbackMutex;
    HANDLE                  m_hProcRecvSvcPktListMutex;
    HANDLE                  m_hProcRecvSvcPktThread;
    BOOL                    m_bRunProcRecvSvcPktThread;

    ProcRecvSvcPktList      m_ProcRecvSvcPktList;
    ProcRecvSvcPktListIt    m_ProcRecvSvcPktListIt;
};
