
// VideoCopyRendererDlg.h : 헤더 파일
//

#pragma once

#include <d3dx9.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )

// CVideoCopyRendererDlg 대화 상자
class CVideoCopyRendererDlg : public CDialog
{
// 생성입니다.
public:
	CVideoCopyRendererDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	~CVideoCopyRendererDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_VIDEOCOPYRENDERER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

	BOOL    Init( HWND hWnd, int nWidth, int nHeight );
    HRESULT InitD3D( HWND hWnd );
    HRESULT InitVB();
    HRESULT InitTexture( int nWidth, int nHeight );

	void    Cleanup();
    void    Render();
    void    DoubleBuffering(void* pvData, UINT uiDataSize);

	BOOL    StartVideoPlayer(TCHAR* pszFilePath);
    void    ClosePipes();
    int     ProcProcessByName(int nCommand, LPCTSTR pszProcName);    // 0 : Find, 1 : Kill

	static VOID					VideoPlayerCallback(void* pUserData, void* pvData, UINT uiDataSize);

	static unsigned __stdcall   VideoStreamPipeThread(void* lpParameter);
    static unsigned __stdcall   VideoPlayerMonitorThread(void* lpParameter);

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

private:
    LPDIRECT3D9             m_pD3D;         // Used to create the D3DDevice
    LPDIRECT3DDEVICE9       m_pD3DDevice;   // Our rendering device
    LPDIRECT3DVERTEXBUFFER9 m_pVB;          // Buffer to hold vertices
    LPDIRECT3DTEXTURE9      m_pTexture;     // Our texture
    LPDIRECT3DSURFACE9		m_pFrontSurface;

	D3DXMATRIX              m_Proj;

	BOOL                    m_bInitComplete;

	HANDLE                  m_hVideoStreamPipe;
    HANDLE                  m_hVideoStreamPipeThread;
    HANDLE                  m_hVideoPlayerMonitorThread;
    BOOL                    m_bStopVideoStreamPipeThread;
    BOOL                    m_bStopVideoPlayerMonitorThread;
    UINT                    m_uiVideoStreamBufferOffset;
    unsigned char*          m_pVideoStreamBuffer;

	CWnd*                   m_pScreenWnd;

    DWORD                   m_dwPipeProcTime;

	ULONG_PTR			    m_gdiplusToken;
public:
	afx_msg void OnBnClickedButtonOpen();
afx_msg void OnBnClickedButtonPlay();