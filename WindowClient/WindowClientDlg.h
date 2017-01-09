
// WindowClientDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"
#include "ClientThread.h"
#include "CodecManager.h"

typedef OPENED_FILE_INFO VS_RAW_INFO;

// CWindowClientDlg ��ȭ ����
class CWindowClientDlg : public CDialog
{
// �����Դϴ�.
public:
	CWindowClientDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.
	virtual ~CWindowClientDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_WINDOWCLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
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
    void        SetResolution       (BOOL bResetResolution = FALSE);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonReqVideoList();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonStop();
    afx_msg void OnCbnSelchangeComboHeight();

private:
	CClientThread*	m_pClientThread;
	
	CCodecManager*	m_pCodecManager;
	CCodec*			m_pDecoder;

	CListBox		m_VideoList;
	CComboBox		m_ComboHeight;
	CString			m_StrLastVideoFileName;
	
	VS_RAW_INFO		m_VsRawInfo;

    BOOL			m_bVideoPlayRunning;
	BOOL			m_bPause;

	// Test
	FILE*			m_fpVideoScalingFile;
	FILE*			m_fpAudioResampleFile;
};
