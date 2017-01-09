#pragma once

#include "Common.h"
#include "Demuxer.h"
#include "CodecManager.h"

typedef int (*StreamCallback)(void*, int, UINT, void*);

class CStreamSource
{
public:
	CStreamSource();
	virtual ~CStreamSource();

	int					Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszSrcFileName);
	void				SetResolution(int nWidth, int nHeight, int nResetResolution);
	int					Start();
	bool 				Pause();
	void				Stop();

private:
	static int			DataProcCallback(void* pObject, int nProcType, unsigned int uiDataSize, unsigned char* pData, int nPictType);
	
	int					ProcFileOpen(UINT uiPixFmt, UINT uiWidth, UINT uiHeight,
									 UINT uiSampleFmt, UINT uiChannelLayout, UINT uiChannels, UINT uiSampleRate, UINT uiFrameSize);
	int					ProcTranscodingInfo(UINT uiPixFmt, UINT uiVideoCodecID, UINT uiNum, UINT uiDen, 
											UINT uiWidth, UINT uiHeight, UINT uiGopSize, UINT uiVideoBitrate,
											UINT uiSampleFmt, UINT uiAudioCodecID, UINT uiChannelLayout, UINT uiChannels, 
											UINT uiSampleRate, UINT uiFrameSize, UINT uiAudioBitrate);
	int					ProcFrameData(char cFrameType, UINT uiFrameNum, UINT uiFrameSize, unsigned char* pData);

	int64_t				EncodeDelayedFrame(int nStreamIndex);

	int					SetEncoder();
	void				CodecCleanUp();

private:
	char*				m_pszSrcFileName;

	CDemuxer*			m_pDemuxer;
	CCodecManager*		m_pCodecManager;
	CCodec*				m_pDecoder;
	CCodec*				m_pEncoder;

	AVFormatContext*	m_pFmtCtx;
	AVCodecContext*		m_pVideoDecCtx;
	AVCodecContext*		m_pAudioDecCtx;
	AVStream*			m_pVideoStream;
	AVStream*			m_pAudioStream;

	void*				m_pParent;
	StreamCallback		m_pStreamCallbackFunc;

	int					m_nRefCount;

	int					m_nVideoStreamIndex;
	int					m_nAudioStreamIndex;

	int					m_nEncVideoWidth;
	int					m_nEncVideoHeight;

	int64_t				m_llEncVideoBitRate;
	int64_t				m_llEncAudioBitRate;

    bool                m_bResetEncoder;

#ifdef _WINDOWS
	HANDLE				m_hSteramCallbackMutex;
#else
	pthread_mutex_t		m_hSteramCallbackMutex;
#endif
};

