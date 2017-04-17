#pragma once

#include "Common.h"
#include "Demuxer.h"
#include "ObjectManager.h"
#include <list>

typedef int (*StreamCallback)(void*, int, UINT, void*);

typedef list<void*>             DecodeDataList;
typedef list<void*>::iterator   DecodeDataListIt;

class CStreamSource
{
public:
	CStreamSource();
	virtual ~CStreamSource();

	int					Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszSrcFileName, bool bCreateTrscVideoFile = false);
    int                 Open();
	void				SetResolution(int nWidth, int nHeight, int nResetResolution);
	int					Start();
	bool 				Pause();
	void				Stop();

private:
#ifdef _USE_FILTER_GRAPH
	static int			DataProcCallback(void* pObject, int nMediaType, AVFrame* pFrame);
#else
	static int			DataProcCallback(void* pObject, int nProcType, int nPictType, unsigned int uiDataSize, unsigned char* pData);
#endif
	static int			WriteEncFrameCallback(void* pObject, AVPacket* pPacket);
	
	int					ProcFileOpen(UINT uiPixFmt, UINT uiWidth, UINT uiHeight, UINT uiFps,
									 UINT uiSampleFmt, UINT uiChannelLayout, UINT uiChannels, UINT uiSampleRate, UINT uiFrameSize);
	int					ProcTranscodingInfo(UINT uiPixFmt, UINT uiVideoCodecID, UINT uiNum, UINT uiDen, 
											UINT uiWidth, UINT uiHeight, UINT uiGopSize, UINT uiVideoBitrate,
											UINT uiSampleFmt, UINT uiAudioCodecID, UINT uiChannelLayout, UINT uiChannels, 
											UINT uiSampleRate, UINT uiFrameSize, UINT uiAudioBitrate);
	int					ProcFrameData(char cFrameType, UINT uiFrameNum, UINT uiFrameSize, unsigned char* pData);

#ifdef _USE_FILTER_GRAPH
	int					EncodeFilterFrame(int nStreamIndex);
	int					EncodeProc(int nMediaType, int nPictType, AVFrame* pFrame);
#else
	int					EncodeProc(int nMediaType, int nPictType, unsigned int uiDataSize, unsigned char* pData);
#endif
	int64_t				EncodeDelayedFrame(int nStreamIndex);

	int					SetMuxer();
	int					SetEncoder();

#ifdef _USE_FILTER_GRAPH
    int                 InitFilters();
    int                 InitFilter(FilteringContext* pFilterCtx, AVCodecContext* pDecCtx, AVCodecContext* pEncCtx, const char *pszFilterSpec);
#endif
	
	void				Flush();
	void				ObjectCleanUp();
    void                CtxAndStreamCleanUp(int nTarget);  // 0 : Decoder, 1 : Transcoder

#ifdef _WIN32
	static unsigned int __stdcall	DecodeThread(void* lpParam);
#else
	static void*					DecodeThread(void* lpParam);
#endif

private:
	char				m_szSrcFileName[MAX_PATH];
	char				m_szTrscFileName[MAX_PATH];
	char*				m_pszInputFileName;

	CObjectManager*		m_pObjectManager;
	CCodec*				m_pDecoder;
	CCodec*				m_pEncoder;
    CMux*			    m_pDemuxer;
	CMux*				m_pMuxer;

	AVFormatContext*	m_pInFmtCtx;
    AVCodecContext**    m_pDecCodecCtx;
    AVStream**          m_pDecStream;

    AVFormatContext*	m_pOutFmtCtx;
    AVCodecContext**    m_pTrscCodecCtx;
    AVStream**          m_pTrscStream;

    int                 m_nDecCodecCtxSize;
    int                 m_nDecStreamSize;

    int                 m_nTrscCodecCtxSize;
    int                 m_nTrscStreamSize;

#ifdef _USE_FILTER_GRAPH
	FilteringContext*	m_pFilterCtx;
#endif

	void*				m_pParent;
	StreamCallback		m_pStreamCallbackFunc;

	int					m_nRefCount;

	int					m_nVideoStreamIndex;
	int					m_nAudioStreamIndex;

	int					m_nEncVideoWidth;
	int					m_nEncVideoHeight;

	int64_t				m_llEncVideoBitRate;
	int64_t				m_llEncAudioBitRate;

	int64_t				m_llVideoFrameNum;
	int64_t				m_llAudioFrameNum;

    bool                m_bResetEncoder;
	bool				m_bCreateTrscVideoFile;

#ifdef _WIN32
    HANDLE				m_hSteramCallbackMutex;
	HANDLE				m_hDecodeThreadMutex;
	HANDLE				m_hDecodeThread;
#else
    pthread_mutex_t		m_hSteramCallbackMutex;
	pthread_mutex_t		m_hDecodeThreadMutex;
	pthread_t			m_hDecodeThread;
#endif

	bool                m_bRunDecodeThread;
    bool                m_bRunEncodeThread;
	bool				m_bPauseDecodeThread;

    DecodeDataList      m_DecodedDataList;
    DecodeDataListIt    m_DecodedDataListIt;
};

