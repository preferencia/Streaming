// ScreenWnd.cpp : implementation file
//

#include "stdafx.h"
#include "WindowClient.h"
#include "ScreenWnd.h"
#include "afxdialogex.h"


// CScreenWnd dialog

IMPLEMENT_DYNAMIC(CScreenWnd, CDialog)

CScreenWnd::CScreenWnd(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SCREEN_WND, pParent)
{
	m_pParentWnd	= pParent;

	m_nScreenWidth	= 0;
	m_nScreenHeight = 0;
}

CScreenWnd::~CScreenWnd()
{
	m_pParentWnd	= NULL;
}

void CScreenWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CScreenWnd, CDialog)
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()


// CScreenWnd message handlers

BOOL CScreenWnd::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	int nExtraWidth		= (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE)) * 2;
	int nExtraHeight	= GetSystemMetrics(SM_CYCAPTION) + (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) * 2;

	SetWindowPos(NULL, 0, 0, m_nScreenWidth + nExtraWidth, m_nScreenHeight + nExtraHeight, 0);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CScreenWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{

	}
	else
	{
		switch (nID)
		{
		case SC_CLOSE:
			{
				if (NULL != m_pParentWnd)
				{
					m_pParentWnd->SendMessage(UM_SCREEN_CLOSE_MSG, 0, 0);
				}
			}
			break;
		default:
			break;
		}

		CDialog::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CScreenWnd::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CScreenWnd::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CScreenWnd::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (WM_KEYDOWN == pMsg->message)
	{
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
		case VK_RETURN:
			return TRUE;

		default:
			break;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CScreenWnd::SetScreenSize(int nWidth, int nHeight)
{
	m_nScreenWidth	= nWidth;
	m_nScreenHeight = nHeight;
}