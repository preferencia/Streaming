#pragma once

#include "Codec.h"

class CDecoder : public CCodec
{
public:
	CDecoder();
	virtual ~CDecoder();

	virtual void					SetCallbackProc(void* pObject, DataProcCallback pDataProcCallback);
	virtual void					SetStreamIndex(int nVideoStreamIndex, int nAudioStreamIndex);
	virtual void					SetAudioSrcInfo(AVSampleFormat AVSrcSampleFmt,
													int nSrcChannelLayout, int nSrcChannels,
													unsigned int uiSrcSampleRate, unsigned int uiSrcSamples);
	virtual void					SetPause(bool bPause)	{ m_bPauseDecodeThread = bPause; }
	virtual bool					GetPauseStatus()		{ return m_bPauseDecodeThread; }

	virtual int						RunDecodeThread();
	virtual void					StopDecodeThread();

	virtual int64_t					Decode(int nStreamIndex,
										   unsigned char* pSrcData, unsigned int uiSrcDataSize,
										   unsigned char** ppEncData, unsigned int& uiEncDataSize);

private:
	virtual int64_t					DecodeVideo(unsigned char* pSrcData, unsigned int uiSrcDataSize,
												unsigned char** ppDstData, unsigned int& uiDstDataSize);
	virtual int64_t					DecodeAudio(unsigned char* pSrcData, unsigned int uiSrcDataSize,
												unsigned char** ppDstData, unsigned int& uiDstDataSize);	
	virtual int						DecodePacket(int *pGotFrame, int nCached);

	virtual int						ProcDecodeData(int nDataType, unsigned int uiDataSize, unsigned char* pData, int nPictType = 0);

protected:
	virtual int						AllocVideoBuffer();
	virtual int						InitFrame();

#ifdef _WINDOWS
	static unsigned int __stdcall	DecodeThread(void* lpParam);
#else
	static void*					DecodeThread(void* lpParam);
#endif

private:
	AVFrame*			m_pFrame;

	// Audio Src Info (for resampling)
	AVSampleFormat		m_AVSrcSampleFmt;
	int					m_nSrcChannelLayout;
	int					m_nSrcChannels;
	unsigned int		m_uiSrcSampleRate;
	unsigned int		m_uiSrcSamples;

	/* Enable or disable frame reference counting. ou are not supposed to support
	* both paths in your application but pick the one most appropriate to your
	* needs. Look for the use of refcount in this example to see what are the
	* differences of API usage between them. */
	int					m_nRefCount;

	int					m_nVideoStreamIndex;
	int					m_nAudioStreamIndex;

	int					m_nVideoFrameCount;
	int					m_nAudioFrameCount;

	int					m_nVideoDstBufSize;
	int					m_VideoDstLineSize[_DEC_PLANE_SIZE];
	uint8_t*			m_pVideoDstData[_DEC_PLANE_SIZE];

	unsigned int		m_uiDecAudioSize;

#ifdef _WINDOWS
	HANDLE				m_hDecodeThread;
	HANDLE				m_hDecodeThreadMutex;
#else
	pthread_t			m_hDecodeThread;
	pthread_mutex_t		m_hDecodeThreadMutex;
#endif

	bool                m_bRunDecodeThread;
	bool				m_bPauseDecodeThread;

	void*				m_pObject;
	DataProcCallback	m_pDataProcCallback;
};

