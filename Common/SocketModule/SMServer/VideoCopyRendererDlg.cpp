
// VideoCopyRendererDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "VideoCopyRenderer.h"
#include "VideoCopyRendererDlg.h"

#include "DXUT.h"

#include <process.h>
#include <tlhelp32.h>

#ifdef _USE_GDIPLUS
#include <Gdiplus.h>
#include <GdiplusBase.h>
#include <GdiplusBitmap.h>
#include <GdiplusColor.h>
#include <GdiplusPen.h>
#include <GdiplusBrush.h>
#include <GdiplusPath.h>
#include <Gdiplusgraphics.h>

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")
#endif

#ifdef _DEBUG
#ifndef _USE_GDIPLUS
#define new DEBUG_NEW
#endif
#endif

#define MAX_BUF_SIZE                    4096
#define MAX_DEFAULT_TIME_OUT            5000

#define _DEC_WINDOW_WIDTH               640
#define _DEC_WINDOW_HEIGHT              480

#define TM_ELLAPSE_CHECK_FRAME_REC      500
#define TMID_CHECK_FRAME_RECV           (WM_USER + 3000)

#define IDS_VIDEO_PLYAER_EXE			_T("VideoPlayer.exe")
#define IDS_VIDEO_STREAM_PIPE_NAME      _T("\\\\.\\pipe\\VideoStreamPipe")

// A structure for our custom vertex type
struct Vertex
{
    Vertex(){}
    Vertex(
        float x, float y, float z,
        float nx, float ny, float nz,
        float u, float v)
    {
        _x  = x;  _y  = y;  _z  = z;
        _nx = nx; _ny = ny; _nz = nz;
        _u  = u;  _v  = v;
    }
    float _x, _y, _z;
    float _nx, _ny, _nz;
    float _u, _v; // texture coordinates

    static const DWORD FVF;
};
const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

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


// CVideoCopyRendererDlg 대화 상자




CVideoCopyRendererDlg::CVideoCopyRendererDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVideoCopyRendererDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pD3D								= NULL;
    m_pD3DDevice						= NULL;
    m_pVB								= NULL;
	m_pTexture							= NULL;
	m_pFrontSurface						= NULL;

	m_bInitComplete						= FALSE;

	m_hVideoStreamPipe					= NULL;
    m_hVideoStreamPipeThread			= NULL;
    m_hVideoPlayerMonitorThread			= NULL;
    m_bStopVideoStreamPipeThread		= FALSE;
    m_bStopVideoPlayerMonitorThread		= FALSE;
    m_uiVideoStreamBufferOffset			= 0;
    m_pVideoStreamBuffer				= NULL;

	m_pScreenWnd						= NULL;

    m_dwPipeProcTime					= 0;
}

CVideoCopyRendererDlg::~CVideoCopyRendererDlg()
{
	Cleanup();

    if (NULL != m_pScreenWnd)
    {
        if (NULL != m_pScreenWnd->GetSafeHwnd())
        {
            m_pScreenWnd->DestroyWindow();
        }

        delete m_pScreenWnd;
        m_pScreenWnd = NULL;
    }

#ifdef _USE_GDIPLUS
	::GdiplusShutdown(m_gdiplusToken);
#endif
}

void CVideoCopyRendererDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVideoCopyRendererDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()

	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CVideoCopyRendererDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CVideoCopyRendererDlg::OnBnClickedButtonPlay)
END_MESSAGE_MAP()


// CVideoCopyRendererDlg 메시지 처리기

BOOL CVideoCopyRendererDlg::OnInitDialog()
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
#ifdef _USE_GDIPLUS
	GdiplusStartupInput gdiplusStartupInput;
    ::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
#endif

	SetWindowPos(NULL, 0, 0, _DEC_WINDOW_WIDTH, _DEC_WINDOW_HEIGHT, SWP_NOMOVE);

    m_pScreenWnd = new CWnd;
    if (NULL != m_pScreenWnd)
    {
        CRect rcClient;
        GetClientRect(&rcClient);

        int nExtraHeight    = _DEC_WINDOW_HEIGHT - rcClient.Height();
        int nNewLeft        = 0;

        CRect rcEdit;
        CRect rcBtn;        

        GetDlgItem(IDC_EDIT_FILE_PATH)->GetWindowRect(&rcEdit);
        //GetDlgItem(IDC_EDIT_FILE_PATH)->SetWindowPos(NULL, rcEdit.left, rcEdit.top - nExtraHeight, rcEdit.right, rcEdit.bottom - nExtraHeight - 10, 0);
		GetDlgItem(IDC_EDIT_FILE_PATH)->SetWindowPos(NULL, 0, 0, rcEdit.right, rcEdit.bottom - nExtraHeight - 10, SWP_NOMOVE);

        nNewLeft = rcEdit.left + rcEdit.right + 4;

        GetDlgItem(IDC_BUTTON_OPEN)->GetWindowRect(&rcBtn);
        GetDlgItem(IDC_BUTTON_OPEN)->SetWindowPos(NULL, nNewLeft, rcBtn.top - nExtraHeight + 2, 0, 0, SWP_NOSIZE);

        nNewLeft += rcBtn.Width() + 4; 

        GetDlgItem(IDC_BUTTON_PLAY)->GetWindowRect(&rcBtn);
        GetDlgItem(IDC_BUTTON_PLAY)->SetWindowPos(NULL, nNewLeft, rcBtn.top - nExtraHeight + 2, 0, 0, SWP_NOSIZE);

        int nCoorY          = rcEdit.bottom - rcEdit.top + 18;
        int nScreenWidth    = rcClient.Width();
		int nScreenHeight   = rcClient.Height();

        if (FALSE == m_pScreenWnd->CreateEx(WS_EX_TOOLWINDOW, NULL, _T("ScreenWnd"), WS_VISIBLE | WS_CHILD, CRect(0, nCoorY, nScreenWidth, nScreenHeight), this, NULL))
        {
            return FALSE;
        }
    }

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

BOOL CVideoCopyRendererDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
    if (pMsg->message == WM_KEYDOWN)
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

void CVideoCopyRendererDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CVideoCopyRendererDlg::OnPaint()
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
HCURSOR CVideoCopyRendererDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CVideoCopyRendererDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
    switch (nIDEvent)
    {
    case TMID_CHECK_FRAME_RECV:
        {
            if (0 != m_dwPipeProcTime)
            {
                DWORD dwCurTime = ::timeGetTime();
                if (dwCurTime - m_dwPipeProcTime > TM_ELLAPSE_CHECK_FRAME_REC)
                {
                    TRACE(_T("Time Out ! - Pre Proc Time = %u, Cur Time = %u, Gap = %u\n"), m_dwPipeProcTime, dwCurTime, dwCurTime - m_dwPipeProcTime);
                    ClosePipes();
                    Cleanup();
                    ProcProcessByName(1, IDS_VIDEO_PLYAER_EXE);
                    KillTimer(nIDEvent);
                    GetDlgItem(IDC_BUTTON_PLAY)->EnableWindow(TRUE);
                }
            }
        }
        break;

    default:
        break;
    }

    CDialog::OnTimer(nIDEvent);
}

BOOL CVideoCopyRendererDlg::Init( HWND hWnd, int nWidth, int nHeight )
{
    if ( S_FALSE == InitD3D(hWnd))
    {
        MessageBox(_T("Fail Init Direct3D"));
        return FALSE;
    }

    if (S_FALSE == InitVB())
    {
        return FALSE;
    }

    if (S_FALSE == InitTexture(nWidth, nHeight))
    {
        return FALSE;
    }

    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);
    m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE);
    m_pD3DDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE);

    // Turn off D3D lighting
    m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT CVideoCopyRendererDlg::InitD3D( HWND hWnd )
{
    // Create the D3D object, which is needed to create the D3DDevice.
    m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
    if( NULL == m_pD3D )
    {
        return E_FAIL;
    }

    D3DDISPLAYMODE	ddm;
    if (FAILED(m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm)))
    {
        return E_FAIL;
    }

    // Set up the structure used to create the D3DDevice. Most parameters are
    // zeroed out. We set Windowed to TRUE, since we want to do D3D in a
    // window, and then set the SwapEffect to "discard", which is the most
    // efficient method of presenting the back buffer to the display.  And 
    // we request a back buffer format that matches the current desktop display 
    // format.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed                      = TRUE;
    d3dpp.Flags                         = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    d3dpp.SwapEffect                    = D3DSWAPEFFECT_DISCARD;

    d3dpp.BackBufferFormat              = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferHeight              = ddm.Height;
    d3dpp.BackBufferWidth               = ddm.Width;
    d3dpp.BackBufferCount               = 1;
    d3dpp.MultiSampleType               = D3DMULTISAMPLE_NONE;
    d3dpp.PresentationInterval          = D3DPRESENT_INTERVAL_IMMEDIATE;

    // Create the Direct3D device. Here we are using the default adapter (most
    // systems only have one, unless they have multiple graphics hardware cards
    // installed) and requesting the HAL (which is saying we want the hardware
    // device rather than a software one). Software vertex processing is 
    // specified since we know it will work on all cards. On cards that support 
    // hardware vertex processing, though, we would see a big performance gain 
    // by specifying hardware vertex processing.
    if( FAILED( m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp, &m_pD3DDevice ) ) )
    {
        return E_FAIL;
    }

    if( FAILED(m_pD3DDevice->CreateOffscreenPlainSurface(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 
        D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_pFrontSurface, NULL)) )
    {
        return E_FAIL;
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: InitVB()
// Desc: Creates a vertex buffer and fills it with our vertices. The vertex
//       buffer is basically just a chuck of memory that holds vertices. After
//       creating it, we must Lock()/Unlock() it to fill it. For indices, D3D
//       also uses index buffers. The special thing about vertex and index
//       buffers is that they can be created in device memory, allowing some
//       cards to process them in hardware, resulting in a dramatic
//       performance gain.
//-----------------------------------------------------------------------------
HRESULT CVideoCopyRendererDlg::InitVB()
{
    // Initialize three vertices for rendering a triangle

    // Create the vertex buffer. Here we are allocating enough memory
    // (from the default pool) to hold all our 3 custom vertices. We also
    // specify the FVF, so the vertex buffer knows what data it contains.
    if( FAILED( m_pD3DDevice->CreateVertexBuffer( 6 * sizeof( Vertex ),
        D3DUSAGE_WRITEONLY, Vertex::FVF, D3DPOOL_MANAGED, &m_pVB, NULL ) ) )
    {
        return S_FALSE;
    }

    // Now we fill the vertex buffer. To do this, we need to Lock() the VB to
    // gain access to the vertices. This mechanism is required becuase vertex
    // buffers may be in device memory.
    Vertex* pVertices;
    if( FAILED( m_pVB->Lock( 0, 0, ( void** )&pVertices, 0 ) ) )
    {
        return S_FALSE;
    }

    //memcpy( pVertices, vertices, sizeof( vertices ) );

    // quad built from two triangles, note texture coordinates:
    
    pVertices[0] = Vertex(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    pVertices[1] = Vertex(-1.0f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    pVertices[2] = Vertex( 1.0f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

    pVertices[3] = Vertex(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    pVertices[4] = Vertex( 1.0f,  1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
    pVertices[5] = Vertex( 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
    
    m_pVB->Unlock();

    //
    // Set the projection matrix.
    //
    D3DXMatrixPerspectiveFovLH(&m_Proj, D3DX_PI * 0.5f /* 90 - degree */, 1.0f /* 화면에 꽉 채움 */, 1.0f, 1000.0f);
    m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &m_Proj);

    return S_OK;
}

HRESULT CVideoCopyRendererDlg::InitTexture( int nWidth, int nHeight )
{
    const D3DSURFACE_DESC* pBackBufferDesc = DXUTGetD3D9BackBufferSurfaceDesc();

    // texture 적용

    // Setup our texture. Using textures introduces the texture stage states,
    // which govern how textures get blended together (in the case of multiple
    // textures) and lighting information. In this case, we are modulating
    // (blending) our texture with the diffuse color of the vertices.
    if (NULL == m_pTexture)
    {
        //D3DRTYPE_TEXTURE
        if( FAILED( D3DXCreateTexture( m_pD3DDevice, nWidth, nHeight/*GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)*/, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pTexture ) ) )
        {
            return E_FAIL;
        }
    }

    m_pD3DDevice->SetTexture( 0, m_pTexture );
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
void CVideoCopyRendererDlg::Cleanup()
{
    if (NULL != m_pFrontSurface)
    {
        SAFE_RELEASE(m_pFrontSurface);
    }

    if( NULL != m_pTexture )
    {
        SAFE_RELEASE(m_pTexture);
    }

    if( NULL != m_pVB )
    {
        SAFE_RELEASE(m_pVB);
    }

    if( NULL != m_pD3DDevice )
    {
        SAFE_RELEASE(m_pD3DDevice);
    }

    if( NULL != m_pD3D )
    {
        SAFE_RELEASE(m_pD3D);
    }

	m_bInitComplete		= FALSE;

	m_dwPipeProcTime	= 0;
}

//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
void CVideoCopyRendererDlg::Render()
{
    if( NULL == m_pD3DDevice )
    {
        return;
    }

    // Clear the backbuffer to a blue color
    m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( m_pD3DDevice->BeginScene() ) )
    {
        // Rendering of scene objects can happen here
        // Draw the triangles in the vertex buffer. This is broken into a few
        // steps. We are passing the vertices down a "stream", so first we need
        // to specify the source of that stream, which is our vertex buffer. Then
        // we need to let D3D know what vertex shader to use. Full, custom vertex
        // shaders are an advanced topic, but in most cases the vertex shader is
        // just the FVF, so that D3D knows what type of vertices we are dealing
        // with. Finally, we call DrawPrimitive() which does the actual rendering
        // of our geometry (in this case, just one triangle).
        // Setup tone mapping technique

        m_pD3DDevice->SetFVF( Vertex::FVF );
        m_pD3DDevice->SetStreamSource( 0, m_pVB, 0, sizeof( Vertex ) );
        m_pD3DDevice->SetTexture( 0, m_pTexture );
        m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );

        // End the scene
        m_pD3DDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    m_pD3DDevice->Present( NULL, NULL, NULL, NULL );
}

void CVideoCopyRendererDlg::DoubleBuffering(void* pvData, UINT uiDataSize)
{
    if ((NULL == pvData) || (0 == uiDataSize))
    {
        return;
    }

	CRect rcScreen;
    m_pScreenWnd->GetClientRect(rcScreen);

    BITMAPINFO* pBmi			= (BITMAPINFO*)pvData;
    unsigned char* pData		= (unsigned char*)pvData + sizeof(BITMAPINFO);

	int nWidth					= pBmi->bmiHeader.biWidth;
	int nHeight					= -pBmi->bmiHeader.biHeight;

    HDC hDC						= ::GetDC(m_pScreenWnd->GetSafeHwnd());
    HDC hMemDC					= CreateCompatibleDC(hDC);
	HBITMAP hBitmap				= NULL;

	// buffer to hbitmap
#ifdef _USE_GDIPLUS
	Gdiplus::Bitmap* pBitmap	= new Gdiplus::Bitmap(pBmi, (void*)pData);
	if (NULL != pBitmap)
	{
		pBitmap->GetHBITMAP(Color(0,0,0), &hBitmap);
	}
#else
	hBitmap						= CreateCompatibleBitmap(hDC, rcScreen.right, rcScreen.bottom);
	SetDIBits(hDC, hBitmap, 0, nHeight, pData, pBmi, DIB_RGB_COLORS);
#endif

    HBITMAP hOldBitmap          = (HBITMAP)SelectObject(hMemDC, hBitmap);

    SetStretchBltMode(hDC, COLORONCOLOR);
    ::StretchBlt(hDC, 0, 0, rcScreen.right, rcScreen.bottom, hMemDC, 0, 0, nWidth, nHeight, SRCCOPY);

#ifdef _USE_GDIPLUS
	if (NULL != pBitmap)
	{
		delete pBitmap;
		pBitmap = NULL;
	}
#endif

    SelectObject(hMemDC, hOldBitmap);
	::DeleteObject(hBitmap);
    ::DeleteDC(hMemDC);
    ::ReleaseDC(m_pScreenWnd->GetSafeHwnd(), hDC);
}

BOOL CVideoCopyRendererDlg::StartVideoPlayer(TCHAR* pszFilePath)
{
	if ((NULL == pszFilePath) || (0 >= _tcslen(pszFilePath)))
	{
		return FALSE;
	}

    m_hVideoStreamPipeThread			= (HANDLE)_beginthreadex(NULL, 0, VideoStreamPipeThread, this, 0, NULL);
    m_hVideoPlayerMonitorThread			= (HANDLE)_beginthreadex(NULL, 0, VideoPlayerMonitorThread, this, 0, NULL);

    TCHAR szCmd[4096]					= {0, };
	TCHAR szCurDirPath[MAX_PATH]		= {0, };

    STARTUPINFO			oStartupInfo	= {0, };
    PROCESS_INFORMATION oProcInfo		= {0, };

	GetModuleFileName(NULL, szCurDirPath, MAX_PATH - 1);
	for (int Index = _tcslen(szCurDirPath); Index >= 0; --Index)
	{
		if ('\\' == szCurDirPath[Index])
		{
			szCurDirPath[Index] = NULL;
			break;
		}
	}

    _stprintf(szCmd, _T("%s\\%s \"%s\""), szCurDirPath, IDS_VIDEO_PLYAER_EXE, pszFilePath);
    
	oStartupInfo.cb				= sizeof(oStartupInfo);
    oStartupInfo.dwFlags		= STARTF_USESHOWWINDOW;
    oStartupInfo.wShowWindow	= SW_HIDE;

    BOOL bResult = CreateProcess(NULL, szCmd, NULL, NULL, FALSE, /*CREATE_NEW_CONSOLE |*/ CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL, &oStartupInfo, &oProcInfo);

    return bResult;
}

void CVideoCopyRendererDlg::ClosePipes()
{
    // 유틸리티를 이용한 강제종료 또는 VideoPlayer.exe 자체 비정상 종료가 생길 경우 연결된 파이프들을 닫아주어야 함
    if ((NULL != m_hVideoStreamPipeThread) && (NULL != m_hVideoStreamPipe) && (INVALID_HANDLE_VALUE != m_hVideoStreamPipe))
    {
        HANDLE hCloseVideoStreamPipe = CreateFile(IDS_VIDEO_STREAM_PIPE_NAME, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if ((NULL != hCloseVideoStreamPipe) && (INVALID_HANDLE_VALUE != hCloseVideoStreamPipe))
        {
            UINT uiWriteOffset          = 0;
            DWORD nNumOfBytesWritten    = 0;
            BITMAPINFO Bmi              = {0, };

            WriteFile(hCloseVideoStreamPipe, &Bmi, sizeof(BITMAPINFO), &nNumOfBytesWritten, NULL);

            CloseHandle(hCloseVideoStreamPipe);
            hCloseVideoStreamPipe = NULL;
        }
    }
}

int CVideoCopyRendererDlg::ProcProcessByName(int nCommand, LPCTSTR pszProcName)
{
    //Get the snapshot of the system
    HANDLE hSnapShot;
    PROCESSENTRY32 pEntry;
    int nCount = 0;

    hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL,NULL);

    pEntry.dwSize =sizeof(pEntry);

    //Get first process
    Process32First(hSnapShot, &pEntry);

    //Iterate thru all processes
    while (Process32Next(hSnapShot, &pEntry))
    {
        TCHAR szProcessName[MAX_PATH] = {0, };
        _tcscpy(szProcessName, pEntry.szExeFile);
        //strlwr(szProcessName);

        if (0 == _tcscmp(szProcessName, pszProcName))
        {
            if (1 == nCommand)
            {
                HANDLE hTemp = OpenProcess(PROCESS_TERMINATE, FALSE, pEntry.th32ProcessID);
                if(hTemp)
                    TerminateProcess(hTemp, -1);
            }
            nCount++;
        }
    }

    CloseHandle(hSnapShot);
    return nCount;
}

void CVideoCopyRendererDlg::VideoPlayerCallback(void* pUserData, void* pvData, UINT uiDataSize)
{
	CVideoCopyRendererDlg* pThis = (CVideoCopyRendererDlg*)pUserData;
    if (NULL == pThis)
    {
        return;
    }

    if ((NULL != pUserData) && (NULL != pvData) && (0 < uiDataSize))
    {
        BITMAPINFO* pBmi            = (BITMAPINFO*)pvData;
        unsigned char* pData        = (unsigned char*)pvData + sizeof(BITMAPINFO);

        __try
        {
            D3DSURFACE_DESC Desc;
            D3DLOCKED_RECT LockRect;

            pThis->m_pTexture->GetLevelDesc(0, &Desc);
            pThis->m_pTexture->LockRect(0, &LockRect, NULL, D3DLOCK_DISCARD);
            memcpy(LockRect.pBits, pData, pBmi->bmiHeader.biSizeImage);
            pThis->m_pTexture->UnlockRect(0);

			pThis->Render();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
			TRACE(_T("D3D Render Exception! Use GDI Double Buffering!\n"));
            pThis->DoubleBuffering(pvData, uiDataSize);
        }        
    }
}

unsigned __stdcall CVideoCopyRendererDlg::VideoStreamPipeThread(void* lpParameter)
{
    CVideoCopyRendererDlg* pThis = (CVideoCopyRendererDlg*)lpParameter;
    if (NULL == pThis)
    {
        return -1;
    }

    BOOL bConnected                 = FALSE;
    BOOL bSuccess                   = FALSE;
    BOOL bRecvCloseData             = FALSE;
    DWORD cbBytesRead               = 0;
    char chBufRead[MAX_BUF_SIZE]    = {0, };    // 버퍼 사이즈를 1MB로 잡으면 Stack Overflow가 발생하므로 ReadFile을 반복시켜서 전체 데이터를 읽어야 한다.
    int nRecvData                   = 0;
    UINT uiTotalDataSize            = 0;

    pThis->m_hVideoStreamPipe = CreateNamedPipe(IDS_VIDEO_STREAM_PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, MAX_BUF_SIZE, 1, MAX_DEFAULT_TIME_OUT, NULL);

    if (INVALID_HANDLE_VALUE == pThis->m_hVideoStreamPipe)
    {
        return 0;
    }

    while (FALSE == pThis->m_bStopVideoStreamPipeThread)
    {
		pThis->m_dwPipeProcTime = ::timeGetTime();

        bConnected = ConnectNamedPipe(pThis->m_hVideoStreamPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (TRUE == bConnected)
        {
            // 넘어오는 데이터 사이즈를 정확히 모르므로 무한 루프로 처리
            while (1)
            {
                bSuccess = ReadFile(pThis->m_hVideoStreamPipe, chBufRead, MAX_BUF_SIZE, &cbBytesRead, NULL);

                //if ((FALSE == bSuccess) || (0 == cbBytesRead))
                //{
                //    TRACE(_T("Error! Read Success = %d, Read Bytes = %d\n"), bSuccess, cbBytesRead);
                //    //CloseHandle(pThis->m_hVideoStreamPipe);
                //    //pThis->m_hVideoStreamPipe = NULL;
                //    continue;
                //}
                
                if (0 == pThis->m_uiVideoStreamBufferOffset)
                {
                    BITMAPINFO* pBmi = (BITMAPINFO*)chBufRead;
                    if (NULL != pBmi)
                    {
                        if ( (0 == pBmi->bmiHeader.biWidth) && (0 == pBmi->bmiHeader.biHeight) && (0 == pBmi->bmiHeader.biSizeImage) )
                        {
                            // 종료시 Vidoe Stream Pipe의 ConnectNamedPipe 탈출을 위해 빈데이터 보냄.
                            bRecvCloseData = TRUE;
                            break;
                        }
                        else
                        {
                            if (FALSE == pThis->m_bInitComplete)
                            {
								TRACE(_T("Frame Width = %d, Height = %d\n"), pBmi->bmiHeader.biWidth, -pBmi->bmiHeader.biHeight);
                                pThis->m_bInitComplete = pThis->Init(pThis->m_pScreenWnd->GetSafeHwnd(), pBmi->bmiHeader.biWidth, -pBmi->bmiHeader.biHeight);
                            }

                            uiTotalDataSize = pBmi->bmiHeader.biSizeImage + sizeof(BITMAPINFO);
                        }

                        if (0 >= uiTotalDataSize)
                        {
                            pThis->m_bStopVideoStreamPipeThread = TRUE;
                            break;
                        }

                        if (NULL == pThis->m_pVideoStreamBuffer)
                        {
							//TRACE(_T("Video Data Size = %u\n"), uiTotalDataSize);
                            pThis->m_pVideoStreamBuffer = new unsigned char[uiTotalDataSize];
                            memset(pThis->m_pVideoStreamBuffer, uiTotalDataSize, 0);
                        }
                    }
                }

				//TRACE(_T("Cur Buffer Index = %u, Read Bytes = %u\n"), pThis->m_uiVideoStreamBufferOffset, cbBytesRead);

                if (NULL != pThis->m_pVideoStreamBuffer)
                {
                    memcpy(pThis->m_pVideoStreamBuffer + pThis->m_uiVideoStreamBufferOffset, chBufRead, cbBytesRead);
                    pThis->m_uiVideoStreamBufferOffset += cbBytesRead;
                }                

                if (pThis->m_uiVideoStreamBufferOffset >= uiTotalDataSize)
                {
                    //TRACE(_T("Total Read Bytes = %u\n"), pThis->m_uiVideoStreamBufferOffset);
                    pThis->m_uiVideoStreamBufferOffset = 0;
                    break;
                }
            }   
            
            if ((FALSE == bRecvCloseData) && (TRUE == pThis->m_bInitComplete))
            {
                pThis->VideoPlayerCallback(pThis, pThis->m_pVideoStreamBuffer, uiTotalDataSize);
            }            
        }
        else
        {
            continue;
        }

        FlushFileBuffers(pThis->m_hVideoStreamPipe);
        DisconnectNamedPipe(pThis->m_hVideoStreamPipe);

        if (TRUE == bRecvCloseData)
        {
            // 재생 완료됨
            pThis->m_bStopVideoStreamPipeThread = TRUE;
            break;
        }
    }

    if (NULL != pThis->m_pVideoStreamBuffer)
    {
        delete [] pThis->m_pVideoStreamBuffer;
        pThis->m_pVideoStreamBuffer = NULL;
    }

    CloseHandle(pThis->m_hVideoStreamPipe);
    pThis->m_hVideoStreamPipe = NULL;

    CloseHandle(pThis->m_hVideoStreamPipeThread);
    pThis->m_hVideoStreamPipeThread = NULL;

    pThis->m_bStopVideoStreamPipeThread = FALSE;

    pThis->Invalidate();

    return 1;
}

unsigned __stdcall CVideoCopyRendererDlg::VideoPlayerMonitorThread(void* lpParameter)
{
    CVideoCopyRendererDlg* pThis = (CVideoCopyRendererDlg*)lpParameter;
    if (NULL == pThis)
    {
        return -1;
    }

    /* 
        VideoPlayer.exe가 실행되고 있는지 감시.
        프로세스를 찾지 못했을 경우 비정상 종료일 수도 있으므로 ClosePipes() 실행함.
    */
    while (FALSE == pThis->m_bStopVideoPlayerMonitorThread)
    {
        if (0 >= pThis->ProcProcessByName(0, IDS_VIDEO_PLYAER_EXE))
        {
            pThis->m_bStopVideoPlayerMonitorThread = TRUE;
            pThis->ClosePipes();
            break;
        }
        Sleep(1000);
    }

    pThis->m_bStopVideoPlayerMonitorThread = FALSE;
    
    CloseHandle(pThis->m_hVideoPlayerMonitorThread);
    pThis->m_hVideoPlayerMonitorThread = NULL;

    return 1;
}

void CVideoCopyRendererDlg::OnBnClickedButtonOpen()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	TCHAR	szBuf[MAX_PATH] = { NULL, };
    CString strPathName;
    CString szFilter = _T("Video File(*.*)|*.*||");

    CFileDialog dlg(TRUE, _T(""), NULL, OFN_HIDEREADONLY, szFilter, this);	

    dlg.m_ofn.Flags |= /*OFN_ALLOWMULTISELECT |*/ OFN_EXPLORER | OFN_ENABLESIZING;
    dlg.m_ofn.lpstrTitle	= _T("Select A Video File");
    dlg.m_ofn.lpstrFile		= szBuf;
    dlg.m_ofn.nMaxFile		= MAX_PATH;

    if( dlg.DoModal() == IDOK)
    {
        POSITION pos = dlg.GetStartPosition();
        CString StrPath = dlg.GetNextPathName(pos);
        GetDlgItem(IDC_EDIT_FILE_PATH)->SetWindowText(StrPath);
    }
}

void CVideoCopyRendererDlg::OnBnClickedButtonPlay()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	// Clean up d3d object for previous file
	Cleanup();

	TCHAR szFilePath[MAX_PATH]			= {0, };
	GetDlgItem(IDC_EDIT_FILE_PATH)->GetWindowText(szFilePath, MAX_PATH - 1);
	if (0 >= _tcslen(szFilePath))
	{
		AfxMessageBox(_T("Select A Video File!"));
		return;
	}

	if (TRUE == StartVideoPlayer(szFilePath))
	{
		// ffplay에 파일 재생 완료 후 종료 처리 코드가 없어서 강제 종료시키도록 타이머 설정
		SetTimer(TMID_CHECK_FRAME_RECV, CLK_TCK, NULL);
        GetDlgItem(IDC_BUTTON_PLAY)->EnableWindow(FALSE);
	}	
}