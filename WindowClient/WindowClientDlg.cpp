
// WindowClientDlg.cpp : 구현 파일
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

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
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


// CWindowClientDlg 대화 상자




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

	m_pScreenWnd			= NULL;
	m_nScreenWidth			= 0;
	m_nScreenHeight			= 0;

	memset(&m_VsRawInfo, 0, sizeof(m_VsRawInfo));
}

CWindowClientDlg::~CWindowClientDlg()
{
	if (true == m_bVideoPlayRunning)
	{
		OnBnClickedButtonStop();
	}

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

	DestroyScreenWnd();
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
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CWindowClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_REQ_VIDEO_LIST, &CWindowClientDlg::OnBnClickedButtonReqVideoList)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CWindowClientDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CWindowClientDlg::OnBnClickedButtonStop)

	ON_MESSAGE(UM_SCREEN_CREATE_MSG, OnRecvScreenCreateMsg)
	ON_MESSAGE(UM_SCREEN_CLOSE_MSG, OnRecvScreenCloseMsg)
	ON_CBN_SELCHANGE(IDC_COMBO_HEIGHT, &CWindowClientDlg::OnCbnSelchangeComboHeight)
END_MESSAGE_MAP()


// CWindowClientDlg 메시지 처리기

BOOL CWindowClientDlg::OnInitDialog()
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
	SetDlgItemText(IDC_IPADDRESS_SERVER, _T("127.0.0.1"));
	SetDlgItemText(IDC_EDIT_PORT, _T("12345"));

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CWindowClientDlg::OnPaint()
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

			pThis->m_pScreenWnd->ShowWindow(FALSE);
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

	m_nScreenWidth	= pRepData->uiWidth;
	m_nScreenHeight = pRepData->uiHeight;
	PostMessage(UM_SCREEN_CREATE_MSG, 0, 0);

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

		m_bVideoPlayRunning = false;
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

		int nSendSize = sizeof(VS_HEADER) + sizeof(VS_SELECT_RESOLUTION_REQ);
		char* pSendBuf = new char[_DEC_MAX_BUF_SIZE];
		memset(pSendBuf, 0, _DEC_MAX_BUF_SIZE);

		PVS_HEADER pHeader = (PVS_HEADER)pSendBuf;
		MAKE_VS_HEADER(pHeader, SVC_SELECT_RESOLUTION, 0, sizeof(VS_SELECT_RESOLUTION_REQ));

		PVS_SELECT_RESOLUTION_REQ pReqData = (PVS_SELECT_RESOLUTION_REQ)(pSendBuf + sizeof(VS_HEADER));
		pReqData->uiResetResolution = (TRUE == bResetResolution) ? 1 : 0;
		pReqData->uiHeight = uiHeight;

		for (int nIndex = 0; nIndex < _DEC_VIDEO_HEIGHT_SET_CNT; ++nIndex)
		{
			if (g_nVideoHeightSet[nIndex] == pReqData->uiHeight)
			{
				pReqData->uiWidth = g_nVideoWidthSet[nIndex];
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
	}
}

int CWindowClientDlg::CreateScreenWnd(int nWidth, int nHeight)
{
	if (NULL != m_pScreenWnd)
	{
		DestroyScreenWnd();
	}

	m_pScreenWnd = new CScreenWnd(this);
	if (NULL == m_pScreenWnd)
	{
		return -1;
	}

	m_pScreenWnd->SetScreenSize(nWidth, nHeight);

	if (FALSE == m_pScreenWnd->Create(IDD_SCREEN_WND, this))
	{
		return -2;
	}

	m_pScreenWnd->SetWindowText(_T("Test.mp4"));
	m_pScreenWnd->ShowWindow(TRUE);

	return 0;
}

void CWindowClientDlg::DestroyScreenWnd()
{
	if (NULL != m_pScreenWnd)
	{
		if (NULL != m_pScreenWnd->GetSafeHwnd())
		{
			m_pScreenWnd->DestroyWindow();
		}

		SAFE_DELETE(m_pScreenWnd);
	}
}

BOOL CWindowClientDlg::PreTranslateMessage(MSG* pMsg)
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

void CWindowClientDlg::OnBnClickedButtonConnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
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
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
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
	// TODO: Add your control notification handler code here
	if (true == m_bVideoPlayRunning)
	{
		SetResolution(TRUE);
	}	
}


LRESULT CWindowClientDlg::OnRecvScreenCreateMsg(WPARAM wParam, LPARAM lParam)
{
	CreateScreenWnd(m_nScreenWidth, m_nScreenHeight);
	return 1;
}


LRESULT CWindowClientDlg::OnRecvScreenCloseMsg(WPARAM wParam, LPARAM lParam)
{
	OnBnClickedButtonStop();
	return 1;
}