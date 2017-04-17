#pragma once

#include <list>
#include <Mmdeviceapi.h>
#include <Audioclient.h>

// CScreenWnd dialog

#define UM_SCREEN_CREATE_MSG	(WM_USER + 0x1000)
#define UM_SCREEN_CLOSE_MSG		(WM_USER + 0x1001)

typedef list<void*>				FrameQueue;
typedef list<void*>::iterator	FrameQueueIt;

class CScreenWnd : public CDialog
{
	DECLARE_DYNAMIC(CScreenWnd)

public:
	CScreenWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CScreenWnd();

// Dialog Data
	enum { IDD = IDD_SCREEN_WND };

protected:
	virtual void    DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
    virtual BOOL    PreTranslateMessage(MSG* pMsg);
    virtual BOOL    OnInitDialog();
    afx_msg void    OnClose();
    afx_msg void    OnDestroy();
	virtual void    PostNcDestroy();

public:
    void			SetScreenSize(int nWidth, int nHeight);
	void			SetWfx(int nChannels, int nSamplePerSec);
    void			Stop();
	void			PushFrameData(int nFrameType, UINT uiFrameNum, UINT uiFrameSize, void* pData);
    void            TestAudio(char* pszPCMFileName);

private:
	HRESULT			InitAudioClient();
	HRESULT			ValidateWaveFormatEx(WAVEFORMATEX *pWfx);
	void			CleanUp();
	void			VideoRender(UINT uiDataSize, void* pData);
	HRESULT			AudioPlay(UINT uiDataSize, void* pData);

	static unsigned int __stdcall	VideoRenderThread(void* pParam);
	static unsigned int __stdcall	AudioPlayThread(void* pParam);

private:
    int						m_nScreenWidth;
	int						m_nScreenHeight;

    bool					m_bRunVideoRenderThread;
	bool					m_bRunAudioPlayThread;

	HANDLE					m_hVideoRenderThread;
	HANDLE					m_hAudioPlayThread;

	HANDLE					m_hVideoQueueMutex;
	HANDLE					m_hAudioQueueMutex;

	FrameQueue				m_VideoQueue;
	FrameQueue				m_AudioQueue;

    LONGLONG				m_llVideoFrameNum;
    LONGLONG				m_llAudioFrameNum;

	// for audio rendering
	IMMDeviceEnumerator*	m_pEnumerator;
	IMMDevice*				m_pDevice;
	IAudioClient*			m_pAudioClient;
	IAudioRenderClient*		m_pRenderClient;
	BYTE*					m_pAudioBuffer;

	REFERENCE_TIME			m_hnsActualDuration;
	UINT32					m_uiBufferSize;
    UINT32					m_uiBufferLengthOut;
    UINT32                  m_uiBufferIndex;
    int                     m_nFrameSize;
	DWORD					m_dwAudioClientFlags;
	WAVEFORMATEXTENSIBLE	m_Wfx;
};
