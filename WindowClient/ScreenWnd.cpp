// ScreenWnd.cpp : implementation file
//

#include "stdafx.h"
#include "WindowClient.h"
#include "WindowClientDlg.h"
#include "ScreenWnd.h"

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC					10000000
#define REFTIMES_PER_MILLISEC				10000

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM	0x80000000
#endif

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto EXIT; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator	= __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator		= __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient				= __uuidof(IAudioClient);
const IID IID_IAudioRenderClient		= __uuidof(IAudioRenderClient);

// CScreenWnd dialog

IMPLEMENT_DYNAMIC(CScreenWnd, CDialog)
CScreenWnd::CScreenWnd(CWnd* pParent /*=NULL*/)
	: CDialog(CScreenWnd::IDD, pParent)
{
	m_pParentWnd                = pParent;

    m_nScreenWidth				= 0;
	m_nScreenHeight				= 0;

    m_bRunVideoRenderThread		= false;
	m_bRunAudioPlayThread		= false;

	m_hVideoRenderThread		= NULL;
	m_hAudioPlayThread			= NULL;

	m_hVideoQueueMutex			= NULL;
	m_hAudioQueueMutex			= NULL;

    m_llVideoFrameNum           = 0LL;
    m_llAudioFrameNum           = 0LL;

	m_pEnumerator				= NULL;
	m_pDevice					= NULL;
	m_pAudioClient				= NULL;
	m_pRenderClient				= NULL;
	m_pAudioBuffer				= NULL;

	m_hnsActualDuration			= 0;
	m_uiBufferSize		        = 0;
    m_uiBufferLengthOut         = 0;
    m_uiBufferIndex             = 0;
    m_nFrameSize                = 0;
	m_dwAudioClientFlags		= 0;

	memset(&m_Wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
}

CScreenWnd::~CScreenWnd()
{
    Stop();
    CleanUp();
	m_pParentWnd	= NULL;
}

void CScreenWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CScreenWnd, CDialog)
    ON_WM_CLOSE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

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

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	int nExtraWidth		= (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE)) * 2;
	int nExtraHeight	= GetSystemMetrics(SM_CYCAPTION) + (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) * 2;

	SetWindowPos(NULL, 0, 0, m_nScreenWidth + nExtraWidth, m_nScreenHeight + nExtraHeight, 0);

	if (S_OK != InitAudioClient())
    {
        AfxMessageBox(_T("Init Audio Client Failed!"));
        return FALSE;
    }

	// Run video render and create video queue mutex
	m_hVideoRenderThread = (HANDLE)_beginthreadex(NULL, 0, VideoRenderThread, this, 0, NULL);
	if (NULL != m_hVideoRenderThread)
	{        
		m_bRunVideoRenderThread = true;
		m_hVideoQueueMutex		= CreateMutex(NULL, FALSE, NULL);
	}

	// Run audio play and create audio queue mutex
	m_hAudioPlayThread = (HANDLE)_beginthreadex(NULL, 0, AudioPlayThread, this, 0, NULL);
	if (NULL != m_hAudioPlayThread)
	{        
		m_bRunAudioPlayThread	= true;
		m_hAudioQueueMutex		= CreateMutex(NULL, FALSE, NULL);
	}

    m_VideoQueue.clear();
    m_AudioQueue.clear();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// CScreenWnd message handlers
void CScreenWnd::OnClose()
{
    Stop();
    DestroyWindow();
    CDialog::OnClose();
}

void CScreenWnd::OnDestroy()
{
    CDialog::OnDestroy();
}

void CScreenWnd::PostNcDestroy()
{
	CDialog::PostNcDestroy();
}

void CScreenWnd::SetScreenSize(int nWidth, int nHeight)
{
	m_nScreenWidth	= nWidth;
	m_nScreenHeight = nHeight;
}

void CScreenWnd::SetWfx(int nChannels, int nSamplePerSec)
{
    // set waveformatex
	m_Wfx.Format.wFormatTag         = WAVE_FORMAT_PCM;
	m_Wfx.Format.nChannels          = nChannels;
	m_Wfx.Format.nSamplesPerSec     = nSamplePerSec;
	m_Wfx.Format.wBitsPerSample     = 16;
	m_Wfx.Format.nAvgBytesPerSec    = m_Wfx.Format.nChannels * m_Wfx.Format.nSamplesPerSec * m_Wfx.Format.wBitsPerSample / 8;
	m_Wfx.Format.nBlockAlign        = m_Wfx.Format.nChannels * m_Wfx.Format.wBitsPerSample / 8;
	m_Wfx.Format.cbSize             = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

	m_Wfx.Samples.wValidBitsPerSample = m_Wfx.Format.wBitsPerSample;

	if (m_Wfx.Format.wFormatTag == WAVE_FORMAT_PCM)
	{
		m_Wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	}
	else
	{
		m_Wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	}
	m_Wfx.Format.wFormatTag         = WAVE_FORMAT_EXTENSIBLE;

	// Note that the WAVEFORMATEX structure is valid for
	// representing wave formats with only 1 or 2 channels.
	if (m_Wfx.Format.nChannels == 1)
	{
		m_Wfx.dwChannelMask = SPEAKER_FRONT_CENTER;
	}
	else if (m_Wfx.Format.nChannels == 2)
	{
		m_Wfx.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	}
}

void CScreenWnd::Stop()
{
	m_bRunVideoRenderThread = false;
	m_bRunAudioPlayThread	= false;
}

void CScreenWnd::PushFrameData(int nFrameType, UINT uiFrameNum, UINT uiFrameSize, void* pData)
{
	if ((0 > uiFrameNum) || (0 == uiFrameSize) || (NULL == pData))
	{
		return;
	}

	if ((0 > nFrameType) || (1 < nFrameType))
	{
		return;
	}

    unsigned char* pBuffer  = new unsigned char[sizeof(FRAME_DATA) + uiFrameSize];
	FRAME_DATA* pFrameData	= (FRAME_DATA*)pBuffer;
    pFrameData->uiFrameNum	= uiFrameNum;
    pFrameData->uiFrameSize	= uiFrameSize;
    memcpy(pBuffer + sizeof(FRAME_DATA), pData, uiFrameSize);

	switch (nFrameType)
	{
	case 0:
		{
			WaitForSingleObject(m_hVideoQueueMutex, INFINITE);
			m_VideoQueue.push_back(pFrameData);
			ReleaseMutex(m_hVideoQueueMutex);
		}
		break;

	case 1:
		{
			WaitForSingleObject(m_hAudioQueueMutex, INFINITE);
			m_AudioQueue.push_back(pFrameData);
			ReleaseMutex(m_hAudioQueueMutex);
		}
		break;
	}
}

HRESULT CScreenWnd::InitAudioClient()
{
	HRESULT			hr						= E_FAIL;
    int             nFramSize               = m_Wfx.Format.nChannels * m_Wfx.Format.wBitsPerSample / 8;

	// Specify a sleep period of 500 milliseconds.
	DWORD			dwSleepPeriod			= 500;
	REFERENCE_TIME	hnsRequestedDuration	= dwSleepPeriod * REFTIMES_PER_MILLISEC;

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	hr = CoCreateInstance( CLSID_MMDeviceEnumerator, NULL,
						   CLSCTX_ALL, IID_IMMDeviceEnumerator,
						   (void**)&m_pEnumerator );
	EXIT_ON_ERROR(hr);

	//   Selects an endpoint rendering device
	hr = m_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice);
	EXIT_ON_ERROR(hr);

	hr = m_pDevice->Activate( IID_IAudioClient, CLSCTX_ALL,
							  NULL, (void**)&m_pAudioClient );
	EXIT_ON_ERROR(hr);

	hr = ValidateWaveFormatEx(&m_Wfx.Format);	
    EXIT_ON_ERROR(hr);

	hr = m_pAudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED,
									 AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
									 hnsRequestedDuration,
									 0,
									 &m_Wfx.Format,
									 NULL );
	EXIT_ON_ERROR(hr);

    m_hnsActualDuration     = hnsRequestedDuration;

    // Get the actual size of the allocated buffer.
	hr = m_pAudioClient->GetBufferSize(&m_uiBufferLengthOut);
	EXIT_ON_ERROR(hr);

    m_nFrameSize            = m_Wfx.Format.nChannels * m_Wfx.Format.wBitsPerSample / 8;
    m_uiBufferSize          = m_uiBufferLengthOut * m_nFrameSize;

    hr = m_pAudioClient->GetService( IID_IAudioRenderClient,
									 (void**)&m_pRenderClient );
    EXIT_ON_ERROR(hr);
	
	return hr;
EXIT:
	SAFE_RELEASE(m_pEnumerator);
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pAudioClient);
	SAFE_RELEASE(m_pRenderClient);
	return hr;
}

//
// Verify that WAVEFORMATEX structure is valid.
//
HRESULT CScreenWnd::ValidateWaveFormatEx(WAVEFORMATEX *pWfx)
{
	HRESULT hr = S_OK;

	if ((0 == pWfx->nChannels) ||
		(0 == pWfx->nSamplesPerSec) ||
		(0 == pWfx->nAvgBytesPerSec) ||
		(0 == pWfx->nBlockAlign) ||
		(1024 < pWfx->cbSize))
	{
		hr = E_INVALIDARG;
		goto Exit;
	}

	if (WAVE_FORMAT_PCM == pWfx->wFormatTag || WAVE_FORMAT_IEEE_FLOAT == pWfx->wFormatTag)
	{
		if ((0 != pWfx->cbSize) ||
			(0 != (pWfx->wBitsPerSample % 8)) ||
			(2 < pWfx->nChannels) ||
			(pWfx->nAvgBytesPerSec != (pWfx->nChannels * pWfx->nSamplesPerSec * pWfx->wBitsPerSample / 8)))
		{
			hr = E_INVALIDARG;
			goto Exit;
		}
	}
	else if (WAVE_FORMAT_EXTENSIBLE == pWfx->wFormatTag)
	{
		WAVEFORMATEXTENSIBLE *pWfxx = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pWfx);

		if (((sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) > pWfx->cbSize))
		{
			hr = E_INVALIDARG;
			goto Exit;
		}

		if ((0 == pWfxx->Samples.wValidBitsPerSample) ||
			(pWfx->wBitsPerSample < pWfxx->Samples.wValidBitsPerSample))
		{
			hr = E_INVALIDARG;
			goto Exit;
		}
		if ((KSDATAFORMAT_SUBTYPE_PCM == pWfxx->SubFormat) ||
			(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT == pWfxx->SubFormat))
		{
			if (pWfx->nAvgBytesPerSec != (pWfx->nChannels * pWfx->nSamplesPerSec * pWfx->wBitsPerSample / 8))
			{
				hr = E_INVALIDARG;
				goto Exit;
			}
		}
	}

Exit:
	return hr;
}

void CScreenWnd::TestAudio(char* pszPCMFileName)
{
    if ((NULL == pszPCMFileName) || (0 == strlen(pszPCMFileName)))
    {
        return;
    }

    FILE* fp = fopen(pszPCMFileName, "rb");
    if (NULL != fp)
    {
        char pBuffer[4096] = {0,};
        while (!feof(fp))
        {
            fread(pBuffer, 4096, 1, fp);
            AudioPlay(4096, pBuffer);
        }
        fclose(fp);
    }
}

void CScreenWnd::CleanUp()
{
	m_bRunVideoRenderThread = false;
	if (NULL != m_hVideoRenderThread)
	{
		WaitForSingleObject(m_hVideoRenderThread, INFINITE);
	}

	m_bRunAudioPlayThread	= false;
	if (NULL != m_hAudioPlayThread)
	{
		WaitForSingleObject(m_hAudioPlayThread, INFINITE);
	}

	if (NULL != m_hVideoQueueMutex)
	{
		CloseHandle(m_hVideoQueueMutex);
		m_hVideoQueueMutex = NULL;
	}

	if (NULL != m_hAudioQueueMutex)
	{
		CloseHandle(m_hAudioQueueMutex);
		m_hAudioQueueMutex = NULL;
	}

	if (0 < m_VideoQueue.size())
	{
		FrameQueueIt	It;

		for (It = m_VideoQueue.begin(); It != m_VideoQueue.end(); ++It)
		{
			FRAME_DATA* pFrameData = (FRAME_DATA*)*It;
			if (NULL != pFrameData)
			{
                unsigned char* pBuffer = (unsigned char*)pFrameData;
                SAFE_DELETE_ARRAY(pBuffer);
			}
		}

		m_VideoQueue.clear();
	}

	if (0 < m_AudioQueue.size())
	{
		FrameQueueIt	It;

		for (It = m_AudioQueue.begin(); It != m_AudioQueue.end(); ++It)
		{
			FRAME_DATA* pFrameData = (FRAME_DATA*)*It;
			if (NULL != pFrameData)
			{
                unsigned char* pBuffer = (unsigned char*)pFrameData;
                SAFE_DELETE_ARRAY(pBuffer);
			}
		}

		m_AudioQueue.clear();
	}

    if (NULL != m_pRenderClient)
    {
        m_pRenderClient->ReleaseBuffer(m_uiBufferSize, m_dwAudioClientFlags);
    }

	SAFE_RELEASE(m_pEnumerator);
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pAudioClient);
	SAFE_RELEASE(m_pRenderClient);
}

void CScreenWnd::VideoRender(UINT uiDataSize, void* pData)
{
	if ((0 == uiDataSize) || (NULL == pData))
	{
		return;
	}

	/* Use double buffering*/

	CRect rcScreen;
	GetClientRect(rcScreen);

	int			nWidth			= m_nScreenWidth;
	int			nHeight			= m_nScreenHeight;

	BITMAPINFO	Bmi				= { 0, };
	Bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	Bmi.bmiHeader.biBitCount	= 32;	// RGB32
	Bmi.bmiHeader.biWidth		= nWidth;
	Bmi.bmiHeader.biHeight		= -nHeight;
	Bmi.bmiHeader.biPlanes		= 1;

	HDC hDC = ::GetDC(GetSafeHwnd());
	HDC hMemDC = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hDC, rcScreen.right, rcScreen.bottom);
	SetDIBits(hDC, hBitmap, 0, nHeight, pData, &Bmi, DIB_RGB_COLORS);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	SetStretchBltMode(hDC, COLORONCOLOR);
	::StretchBlt(hDC, 0, 0, rcScreen.right, rcScreen.bottom, hMemDC, 0, 0, nWidth, nHeight, SRCCOPY);

	SelectObject(hMemDC, hOldBitmap);
	::DeleteObject(hBitmap);
	::DeleteDC(hMemDC);
	::ReleaseDC(GetSafeHwnd(), hDC);
}

HRESULT CScreenWnd::AudioPlay(UINT uiDataSize, void* pData)
{
	if ((0 == uiDataSize) || (NULL == pData))
	{
		return S_FALSE;
	}

	if ((NULL == m_pAudioClient) || (NULL == m_pRenderClient))
	{
		return E_FAIL;
	}

    HRESULT     hr                      = S_OK;
    UINT        uiCopySize              = 0;

    if (0 == m_uiBufferIndex)
    {
        // Grab the entire buffer for the initial fill operation.
	    hr = m_pRenderClient->GetBuffer(m_uiBufferLengthOut, &m_pAudioBuffer);
	    EXIT_ON_ERROR(hr);

        uiCopySize  = uiDataSize;
    }
    else
    {
        if (m_uiBufferSize < (m_uiBufferIndex + uiDataSize))
        {
            uiCopySize  = m_uiBufferSize - m_uiBufferIndex;
        }
        else
        {
            uiCopySize  = uiDataSize;
        }
    }

	// Load the data into the shared buffer.
	memcpy(m_pAudioBuffer + m_uiBufferIndex, pData, uiCopySize);
    m_uiBufferIndex += uiCopySize;

    if (m_uiBufferIndex < m_uiBufferSize)
    {
        EXIT_ON_ERROR(E_FAIL);
    }

    m_uiBufferIndex = 0;

    hr = m_pRenderClient->ReleaseBuffer(m_uiBufferLengthOut, 0);
    EXIT_ON_ERROR(hr)

	hr = m_pAudioClient->Start();  // Start playing.
	EXIT_ON_ERROR(hr);

    // if exist not copied data
    if (0 < (uiDataSize - uiCopySize))
    {
        UINT32      uiNumFramesAvailable	= 0;
        UINT32      uiNumFramesPadding	    = 0;

        // See how much buffer space is available.
		hr = m_pAudioClient->GetCurrentPadding(&uiNumFramesPadding);
		EXIT_ON_ERROR(hr);

        uiNumFramesAvailable = m_uiBufferLengthOut - uiNumFramesPadding;

        if (0 < uiNumFramesAvailable)
        {
            // Get pointer to next space in render buffer.
            hr = m_pRenderClient->GetBuffer(uiNumFramesAvailable, &m_pAudioBuffer);
            EXIT_ON_ERROR(hr)

            // Copy data from remain input data into rendering buffer.
            memcpy(m_pAudioBuffer, (char*)pData + uiCopySize, uiDataSize - uiCopySize);
            m_uiBufferIndex += uiDataSize - uiCopySize;

            m_uiBufferLengthOut = uiNumFramesAvailable;
            m_uiBufferSize      = m_uiBufferLengthOut * m_nFrameSize;

            hr = m_pRenderClient->ReleaseBuffer(uiNumFramesAvailable, 0);
            EXIT_ON_ERROR(hr)
        }
    }

	// Wait for last data in buffer to play before stopping.
	Sleep((DWORD)(m_hnsActualDuration / REFTIMES_PER_MILLISEC));

EXIT :
	hr = m_pAudioClient->Stop();  // Stop playing.
	return hr;
}

unsigned int __stdcall CScreenWnd::VideoRenderThread(void* pParam)
{
	CScreenWnd* pThis		= (CScreenWnd*)pParam;
	if (NULL == pThis)
	{
		return -1;
	}

    unsigned char*  pBuffer     = NULL;
	FRAME_DATA*     pFrameData	= NULL;

	while (true == pThis->m_bRunVideoRenderThread)
	{
		WaitForSingleObject(pThis->m_hVideoQueueMutex, INFINITE);

		if (0 >= pThis->m_VideoQueue.size())
		{
			ReleaseMutex(pThis->m_hVideoQueueMutex);
			Sleep(10);
			continue;
		}

		pFrameData = (FRAME_DATA*)pThis->m_VideoQueue.front();
		pThis->m_VideoQueue.pop_front();
		
		ReleaseMutex(pThis->m_hVideoQueueMutex);

		if (NULL == pFrameData)
		{
			continue;
		}

        if (pFrameData->uiFrameNum > pThis->m_llVideoFrameNum)
        {
            //pThis->VideoRender(pFrameData->uiFrameSize, pFrameData->pData);
			pThis->m_llVideoFrameNum = pFrameData->uiFrameNum;
        }

		pBuffer = (unsigned char*)pFrameData;
        SAFE_DELETE_ARRAY(pBuffer);
	}

	CloseHandle(pThis->m_hVideoRenderThread);
	pThis->m_hVideoRenderThread = NULL;

	return 0;
}

unsigned int __stdcall CScreenWnd::AudioPlayThread(void* pParam)
{
	CScreenWnd* pThis		= (CScreenWnd*)pParam;
	if (NULL == pThis)
	{
		return -1;
	}

	unsigned char*  pBuffer     = NULL;
	FRAME_DATA*     pFrameData	= NULL;

	while (true == pThis->m_bRunAudioPlayThread)
	{
		WaitForSingleObject(pThis->m_hAudioQueueMutex, INFINITE);

		if (0 >= pThis->m_AudioQueue.size())
		{
			ReleaseMutex(pThis->m_hAudioQueueMutex);
			Sleep(10);
			continue;
		}

		pFrameData = (FRAME_DATA*)pThis->m_AudioQueue.front();
		pThis->m_AudioQueue.pop_front();

		ReleaseMutex(pThis->m_hAudioQueueMutex);

		if (NULL == pFrameData)
		{
			continue;
		}

        if (pFrameData->uiFrameNum > pThis->m_llVideoFrameNum)
        {
		    pThis->AudioPlay(pFrameData->uiFrameSize, pFrameData->pData);
			pThis->m_llAudioFrameNum = pFrameData->uiFrameNum;
        }

        pBuffer = (unsigned char*)pFrameData;
        SAFE_DELETE_ARRAY(pBuffer);
	}

	CloseHandle(pThis->m_hAudioPlayThread);
	pThis->m_hAudioPlayThread = NULL;

	return 0;
}