
// WindowClientDlg.cpp : ���� ����
//

#include "stdafx.h"
#include "WindowClient.h"
#include "WindowClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static int g_nVideoWidthSet[]	= { 1920,	1280,	640, };
static int g_nVideoHeightSet[]	= { 1080,	720,	360, };

#define _DEC_VIDEO_WIDTH_SET_CNT	(sizeof(g_nVideoWidthSet) / sizeof(*g_nVideoWidthSet))
#define _DEC_VIDEO_HEIGHT_SET_CNT	(sizeof(g_nVideoHeightSet) / sizeof(*g_nVideoHeightSet))

// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

// �����Դϴ�.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CWindowClientDlg ��ȭ ����




CWindowClientDlg::CWindowClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWindowClientDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pClientThread			= NULL;

	m_pCodecManager			= NULL;
	m_pDecoder				= NULL;

	m_fpVideoScalingFile	= NULL;
	m_fpAudioResampleFile	= NULL;

	m_bVideoPlayRunning		= false;
	m_bPause				= false;

	memset(&m_VsRawInfo,		0,	sizeof(m_VsRawInfo));
}

CWindowClientDlg::~CWindowClientDlg()
{
	OnBnClickedButtonStop();

	m_pCodecManager->DestroyCodec(&m_pDecoder);
	SAFE_DELETE(m_pCodecManager);
	SAFE_DELETE(m_pClientThread);

	if (NULL != m_fpVideoScalingFile)
	{
		fclose(m_fpVideoScalingFile);
		m_fpVideoScalingFile = NULL;
	}

	if (NULL != m_fpAudioResampleFile)
	{
		fclose(m_fpAudioResampleFile);
		m_fpAudioResampleFile = NULL;
	}
}

void CWindowClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_VIDEO_LSIT, m_VideoList);
	DDX_Control(pDX, IDC_COMBO_HEIGHT, m_ComboHeight);
}

BEGIN_MESSAGE_MAP(CWindowClientDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_CONNECT,			&CWindowClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_REQ_VIDEO_LIST,	&CWindowClientDlg::OnBnClickedButtonReqVideoList)
	ON_BN_CLICKED(IDC_BUTTON_PLAY,				&CWindowClientDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP,				&CWindowClientDlg::OnBnClickedButtonStop)
    ON_CBN_SELCHANGE(IDC_COMBO_HEIGHT, &CWindowClientDlg::OnCbnSelchangeComboHeight)
END_MESSAGE_MAP()


// CWindowClientDlg �޽��� ó����

BOOL CWindowClientDlg::OnInitDialog()
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
	SetDlgItemText(IDC_IPADDRESS_SERVER, _T("127.0.0.1"));
	SetDlgItemText(IDC_EDIT_PORT,		 _T("23456"));

	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

void CWindowClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CWindowClientDlg::OnPaint()
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
HCURSOR CWindowClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int CWindowClientDlg::RecvProcCallback(	void*			pObject, 
										unsigned int	uiSvcCode, 
										unsigned int	uiDataLen,
										char*			pData)
{
	if (NULL == pObject)
	{
		TraceLog("Object is NULL.");
		return -1;
	}

	CWindowClientDlg* pThis = (CWindowClientDlg*)pObject;

	switch (uiSvcCode)
	{
	case SVC_GET_LIST:
		{
			pThis->InsertVideoList(pData);
		}
		break;

	case SVC_FILE_OPEN:
		{
			pThis->InsertVideoHeight(pData);
		}
		break;

	case SVC_SELECT_RESOLUTION:
		{
			pThis->SetDecoder(pData);
		}
		break;

	case SVC_FRAME_DATA:
		{
			pThis->ProcFrameData(pData);
		}
		break;

	case SVC_FILE_CLOSE:
		{
			pThis->m_bVideoPlayRunning	= false;
			pThis->m_bPause				= false;
			pThis->GetDlgItem(IDC_BUTTON_PLAY)->SetWindowText(_T("Play"));
			pThis->GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

			if (NULL != pThis->m_fpVideoScalingFile)
			{
				fclose(pThis->m_fpVideoScalingFile);
				pThis->m_fpVideoScalingFile = NULL;
			}

			if (NULL != pThis->m_fpAudioResampleFile)
			{
				fclose(pThis->m_fpAudioResampleFile);
				pThis->m_fpAudioResampleFile = NULL;
			}
		}
		break;

	case SVC_SET_PLAY_STATUS:
		{
			pThis->SetPlayStatus(pData);
		}
		break;

	default:
		{
		}
		break;
	}

	return 0;
}

void CWindowClientDlg::InsertVideoList(char* pData)
{
	if (NULL == pData)
	{
		return;
	}

	unsigned int	uiBufIndex  = 0;

	PVS_LIST_REP	pRepData		= (PVS_LIST_REP)pData;
	uiBufIndex						= sizeof(pRepData->nCnt);

	for (int nIndex = 0; nIndex < pRepData->nCnt; ++nIndex)
	{
		PVideoListData pVideoListData = (PVideoListData)(pData + uiBufIndex);
		if (NULL != pVideoListData)
		{
			uiBufIndex += sizeof(pVideoListData->nFileNameLen);
			char* pszFileName = (char*)(pData + uiBufIndex);
			m_VideoList.InsertString(nIndex, (LPCTSTR)pszFileName);
			uiBufIndex += pVideoListData->nFileNameLen + 1;
		}		
	}
}

void CWindowClientDlg::InsertVideoHeight(char* pData)
{
	if (NULL == pData)
	{
		return;
	}

	// Set Video Height List
	if (0 >= m_ComboHeight.GetCount())
	{
		int					nAddCount = 0;
		PVS_FILE_OPEN_REP	pRepData = (PVS_FILE_OPEN_REP)pData;

		for (int nIndex = 0; nIndex < _DEC_VIDEO_HEIGHT_SET_CNT; ++nIndex)
		{
			if (g_nVideoHeightSet[nIndex] <= pRepData->uiHeight)
			{
				CString StrHeight;
				StrHeight.Format(_T("%d"), g_nVideoHeightSet[nIndex]);
				m_ComboHeight.InsertString(nAddCount++, StrHeight);
			}
		}

		m_ComboHeight.SetCurSel(nAddCount / 2);
		memcpy(&m_VsRawInfo, pRepData, sizeof(VS_FILE_OPEN_REP));
	}

    SetResolution();
}

int CWindowClientDlg::SetDecoder(char* pData)
{
	if (NULL == pData)
	{
		return -1;
	}

	/* register all formats and codecs */
	av_register_all();

	if (NULL == m_pCodecManager)
	{
		m_pCodecManager = new CCodecManager;
	}

	if (NULL == m_pCodecManager)
	{
		return -2;
	}

	if (NULL != m_pDecoder)
	{
		m_pCodecManager->DestroyCodec(&m_pDecoder);
	}

	if ((0 > m_pCodecManager->CreateCodec(&m_pDecoder, CODEC_TYPE_DECODER)) || (NULL == m_pDecoder))
	{
		return -3;
	}

	m_pDecoder->SetAudioSrcInfo((AVSampleFormat)m_VsRawInfo.uiSampleFmt, m_VsRawInfo.uiChannelLayout, 
								m_VsRawInfo.uiChannels, m_VsRawInfo.uiSampleRate, m_VsRawInfo.uiFrameSize);

	PVS_SELECT_RESOLUTION_REP pRepData	= (PVS_SELECT_RESOLUTION_REP)pData;

	AVRational AvTimeBase				= { 0, };
	AvTimeBase.num						= pRepData->uiNum;
	AvTimeBase.den						= pRepData->uiDen;

	if (0 > m_pDecoder->InitVideoCtx((AVPixelFormat)pRepData->uiPixFmt, (AVCodecID)pRepData->uiVideoCodecID, AvTimeBase,
									 pRepData->uiWidth, pRepData->uiHeight, pRepData->uiGopSize, pRepData->uiVideoBitrate))
	{
		return -4;
	}

	if (0 > m_pDecoder->InitAudioCtx((AVSampleFormat)pRepData->uiSampleFmt, (AVCodecID)pRepData->uiAudioCodecID, 
									 pRepData->uiChannelLayout, pRepData->uiChannels,
									 pRepData->uiSampleRate, pRepData->uiFrameSize, pRepData->uiAudioBitrate))
	{
		return -5;
	}

	m_pDecoder->SetStreamIndex(AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO);

	if (NULL == m_fpVideoScalingFile)
	{
		char szVideoScalingFile[MAX_PATH * 2] = { 0, };
		sprintf(szVideoScalingFile, "%s_scaling_%dx%d.raw", m_StrLastVideoFileName.Left(m_StrLastVideoFileName.GetLength() - 4), pRepData->uiWidth, pRepData->uiHeight);

		m_fpVideoScalingFile = fopen(szVideoScalingFile, "wb");
	}

	if (NULL == m_fpAudioResampleFile)
	{
		char szAudioResampleFile[MAX_PATH * 2] = { 0, };
		sprintf(szAudioResampleFile, "%s_resample.pcm", m_StrLastVideoFileName.Left(m_StrLastVideoFileName.GetLength() - 4));

		m_fpAudioResampleFile = fopen(szAudioResampleFile, "wb");
	}

	m_ComboHeight.EnableWindow(TRUE);

	m_bVideoPlayRunning = true;
	GetDlgItem(IDC_BUTTON_PLAY)->SetWindowText(_T("Pause"));
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);

	return 0;
}

int CWindowClientDlg::ProcFrameData(char* pData)
{
	if (NULL == pData)
	{
		return -1;
	}

	if (NULL == m_pDecoder)
	{
		return -2;
	}

	PFRAME_DATA		pFrameData		= (PFRAME_DATA)pData;

	unsigned char*	pEncData		= (unsigned char*)pFrameData->pData;
	unsigned char*	pDecData		= NULL;
	unsigned int	uiEncSize		= pFrameData->uiFrameSize;
	unsigned int	uiDecSize		= 0;

	int				nStreamIndex	= -1;

	switch (pFrameData->uiFrameType)
	{
	case 'A':
		{
			nStreamIndex = AVMEDIA_TYPE_AUDIO;
		}
		break;

	default:
		{
			nStreamIndex = AVMEDIA_TYPE_VIDEO;
		}
		break;
	}

	if ((0 > m_pDecoder->Decode(nStreamIndex, pEncData, uiEncSize, &pDecData, uiDecSize)) || (NULL == pDecData) || (0 >= uiDecSize))
	{
		return -3;
	}

	switch (nStreamIndex)
	{
	case AVMEDIA_TYPE_VIDEO:
		{
			if (NULL != m_fpVideoScalingFile)
			{
				fwrite(pDecData, 1, uiDecSize, m_fpVideoScalingFile);
			}			
		}
		break;

	case AVMEDIA_TYPE_AUDIO:
		{
			if (NULL != m_fpAudioResampleFile)
			{
				fwrite(pDecData, 1, uiDecSize, m_fpAudioResampleFile);
			}			
		}
		break;

	default:
		break;
	}

	return 0;
}

void CWindowClientDlg::SetPlayStatus(char* pData)
{
	if (NULL == pData)
	{
		return;
	}

	PVS_SET_PLAY_STATUS pRepData = (PVS_SET_PLAY_STATUS)pData;

	GetDlgItem(IDC_BUTTON_PLAY)->SetWindowText((0 == pRepData->uiPlayStatus) ? _T("Pause") : _T("Play"));
    
    if (2 == pRepData->uiPlayStatus)
    {
        if (NULL != m_fpVideoScalingFile)
		{
			fclose(m_fpVideoScalingFile);
			m_fpVideoScalingFile = NULL;
		}

		if (NULL != m_fpAudioResampleFile)
		{
			fclose(m_fpAudioResampleFile);
			m_fpAudioResampleFile = NULL;
		}

        GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);
    }
}

void CWindowClientDlg::SetResolution(BOOL bResetResolution /* = FALSE */)
{
    // Send Video Resolution
	if (NULL != m_pClientThread)
	{
		UINT uiHeight = 0;
		CString StrHeight;
		m_ComboHeight.GetLBText(m_ComboHeight.GetCurSel(), StrHeight);
		if (0 >= StrHeight.GetLength())
		{
			uiHeight = g_nVideoHeightSet[_DEC_VIDEO_HEIGHT_SET_CNT - 1];
		}
		else
		{
			uiHeight = (UINT)_ttoi(StrHeight);
		}

		int nSendSize						= sizeof(VS_HEADER) + sizeof(VS_SELECT_RESOLUTION_REQ);
		char* pSendBuf						= new char[_DEC_MAX_BUF_SIZE];
		memset(pSendBuf, 0, _DEC_MAX_BUF_SIZE);

		PVS_HEADER pHeader					= (PVS_HEADER)pSendBuf;
		MAKE_VS_HEADER(pHeader, SVC_SELECT_RESOLUTION, 0, sizeof(VS_SELECT_RESOLUTION_REQ));

		PVS_SELECT_RESOLUTION_REQ pReqData	= (PVS_SELECT_RESOLUTION_REQ)(pSendBuf + sizeof(VS_HEADER));
        pReqData->uiResetResolution         = (TRUE == bResetResolution) ? 1 : 0;
		pReqData->uiHeight					= uiHeight;

		for (int nIndex = 0; nIndex < _DEC_VIDEO_HEIGHT_SET_CNT; ++nIndex)
		{
			if (g_nVideoHeightSet[nIndex] == pReqData->uiHeight)
			{
				pReqData->uiWidth           = g_nVideoWidthSet[nIndex];
				break;
			}
		}

		m_pClientThread->Send(pSendBuf, nSendSize);

		// Disable height combo to duplicate selecting resolution 
		m_ComboHeight.EnableWindow(FALSE);

        if (NULL != m_fpVideoScalingFile)
		{
			fclose(m_fpVideoScalingFile);
			m_fpVideoScalingFile = NULL;
		}

		if (NULL != m_fpAudioResampleFile)
		{
			fclose(m_fpAudioResampleFile);
			m_fpAudioResampleFile = NULL;
		}
	}
}

BOOL CWindowClientDlg::PreTranslateMessage(MSG* pMsg)
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

void CWindowClientDlg::OnBnClickedButtonConnect()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	CString StrIP;
	CString StrPort;

	GetDlgItemText(IDC_IPADDRESS_SERVER, StrIP);
	GetDlgItemText(IDC_EDIT_PORT, StrPort);

	if ((0 == StrIP.GetLength()) || (0 == StrPort.GetLength()))
	{
		return;
	}

	SAFE_DELETE(m_pClientThread);

	m_pClientThread	= new CClientThread();
	if (NULL != m_pClientThread)
	{
		m_pClientThread->SetServerInfo(StrIP.GetBuffer(), atoi(StrPort.GetBuffer()));

		if (false == m_pClientThread->InitSocketThread())
		{
			AfxMessageBox(_T("Server Connection Failed!"));
			SAFE_DELETE(m_pClientThread);
			return;
		}

		m_pClientThread->SetProcCallbakcInfo(this, RecvProcCallback);
	}

	GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);
}

void CWindowClientDlg::OnBnClickedButtonReqVideoList()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	if (NULL != m_pClientThread)
	{
		m_VideoList.ResetContent();

		char* pSendBuf = new char[_DEC_MAX_BUF_SIZE];
		memset(pSendBuf, 0, _DEC_MAX_BUF_SIZE);

		PVS_HEADER pHeader = (PVS_HEADER)pSendBuf;
		MAKE_VS_HEADER(pHeader, SVC_GET_LIST, 0, 0);
		m_pClientThread->Send(pSendBuf, sizeof(VS_HEADER));
	}	
}


void CWindowClientDlg::OnBnClickedButtonPlay()
{
	// TODO: Add your control notification handler code here
	if (NULL == m_pClientThread)
	{
		return;
	}

	if (false == m_bVideoPlayRunning)
	{
		CString StrFileName;
		m_VideoList.GetText(m_VideoList.GetCurSel(), StrFileName);

		if (0 == StrFileName.GetLength())
		{
			AfxMessageBox(_T("Select a video!"));
			return;
		}

		if (m_StrLastVideoFileName != StrFileName)
		{
			m_StrLastVideoFileName = StrFileName;
			m_ComboHeight.ResetContent();
		}

		int nSendSize	= sizeof(VS_HEADER) + StrFileName.GetLength();
		char* pSendBuf	= new char[_DEC_MAX_BUF_SIZE];
		memset(pSendBuf, 0, _DEC_MAX_BUF_SIZE);

		PVS_HEADER pHeader = (PVS_HEADER)pSendBuf;
		MAKE_VS_HEADER(pHeader, SVC_FILE_OPEN, 0, StrFileName.GetLength());

		memcpy(pSendBuf + sizeof(VS_HEADER), StrFileName.GetBuffer(), StrFileName.GetLength());

		m_pClientThread->Send(pSendBuf, nSendSize);
	}
	else
	{
		m_bPause		= !m_bPause;

		int nSendSize	= sizeof(VS_HEADER) + sizeof(VS_SET_PLAY_STATUS);
		char* pSendBuf	= new char[_DEC_MAX_BUF_SIZE];
		memset(pSendBuf, 0, _DEC_MAX_BUF_SIZE);

		PVS_HEADER pHeader = (PVS_HEADER)pSendBuf;
		MAKE_VS_HEADER(pHeader, SVC_SET_PLAY_STATUS, 0, sizeof(VS_SET_PLAY_STATUS));

		PVS_SET_PLAY_STATUS pReqData	= (PVS_SET_PLAY_STATUS)(pSendBuf + sizeof(VS_HEADER));
		pReqData->uiPlayStatus			= (true == m_bPause) ? 1 : 0;

		m_pClientThread->Send(pSendBuf, nSendSize);
	}
	
}


void CWindowClientDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here
	if (NULL != m_pClientThread)
	{
		int nSendSize = sizeof(VS_HEADER) + sizeof(VS_SET_PLAY_STATUS);
		char* pSendBuf = new char[_DEC_MAX_BUF_SIZE];
		memset(pSendBuf, 0, _DEC_MAX_BUF_SIZE);

		PVS_HEADER pHeader = (PVS_HEADER)pSendBuf;
		MAKE_VS_HEADER(pHeader, SVC_SET_PLAY_STATUS, 0, sizeof(VS_SET_PLAY_STATUS));

		PVS_SET_PLAY_STATUS pReqData = (PVS_SET_PLAY_STATUS)(pSendBuf + sizeof(VS_HEADER));
		pReqData->uiPlayStatus = 2;

		m_pClientThread->Send(pSendBuf, nSendSize);
	}
}


void CWindowClientDlg::OnCbnSelchangeComboHeight()
{
    // TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
    SetResolution(TRUE);
}
