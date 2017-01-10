
// WindowClientDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include "ClientThread.h"
#include "CodecManager.h"
#include "ScreenWnd.h"

typedef OPENED_FILE_INFO VS_RAW_INFO;

// CWindowClientDlg 대화 상자
class CWindowClientDlg : public CDialog
{
// 생성입니다.
public:
	CWindowClientDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	virtual ~CWindowClientDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_WINDOWCLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	static int	RecvProcCallback(void*			pObject, 
								 unsigned int	uiSvcCode, 
								 unsigned int	uiDataLen,
								 char*			pData);

	void		InsertVideoList		(char* pData);
	void		InsertVideoHeight	(char* pData);
	int 		SetDecoder			(char* pData);
	int			ProcFrameData		(char* pData);
	void		SetPlayStatus		(char* pData);
	void        SetResolution(BOOL bResetResolution = FALSE);

	int 		CreateScreenWnd		(int nWidth, int nHeight);
	void		DestroyScreenWnd();

public:
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnBnClickedButtonConnect();
	afx_msg void	OnBnClickedButtonReqVideoList();
	afx_msg void	OnBnClickedButtonPlay();
	afx_msg void	OnBnClickedButtonStop();
	afx_msg void	OnCbnSelchangeComboHeight();

	afx_msg LRESULT OnRecvScreenCreateMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRecvScreenCloseMsg(WPARAM wParam, LPARAM lParam);

private:
	CClientThread*	m_pClientThread;
	
	CCodecManager*	m_pCodecManager;
	CCodec*			m_pDecoder;

	CListBox		m_VideoList;
	CComboBox		m_ComboHeight;
	CString			m_StrLastVideoFileName;
	
	VS_RAW_INFO		m_VsRawInfo;

	// Test
	FILE*			m_fpVideoScalingFile;
	FILE*			m_fpAudioResampleFile;

	BOOL			m_bVideoPlayRunning;
	BOOL			m_bPause;

	CScreenWnd*		m_pScreenWnd;
	int				m_nScreenWidth;
	int				m_nScreenHeight;
};
