#pragma once


// CScreenWnd dialog

#define UM_SCREEN_CREATE_MSG	(WM_USER + 0x1000)
#define UM_SCREEN_CLOSE_MSG		(WM_USER + 0x1001)

class CScreenWnd : public CDialog
{
	DECLARE_DYNAMIC(CScreenWnd)

public:
	CScreenWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CScreenWnd();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SCREEN_WND };
#endif

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

public:
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	void			SetScreenSize(int nWidth, int nHeight);

private:
	int				m_nScreenWidth;
	int				m_nScreenHeight;
};
