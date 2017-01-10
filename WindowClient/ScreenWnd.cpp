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

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
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

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	int nExtraWidth		= (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE)) * 2;
	int nExtraHeight	= GetSystemMetrics(SM_CYCAPTION) + (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) * 2;

	SetWindowPos(NULL, 0, 0, m_nScreenWidth + nExtraWidth, m_nScreenHeight + nExtraHeight, 0);

	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
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

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CScreenWnd::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CScreenWnd::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CScreenWnd::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
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