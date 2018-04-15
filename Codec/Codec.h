#pragma once

#include "Common.h"
#include "HWAccel.h"

#ifdef _USE_FILTER_GRAPH
typedef int (*DataProcCallback)(void* pObject, int nProcDataType, int nPictType, AVFrame* pFilterFrame);
#else
typedef int (*DataProcCallback)(void* pObject, int nProcDataType, int nPictType, unsigned int uiDataSize, unsigned char* pData);
#endif
typedef int (*WriteEncFrameCallback)(void* pObject, AVPacket* pPacket);

class CCodec
{
public:
	CCodec();
	virtual ~CCodec();

	virtual int							InitFromContext(AVFormatContext* pFmtCtx, AVCodecContext* pVideoCtx, AVCodecContext* pAudioCtx);

	virtual int							InitVideoCtx(AVPixelFormat PixFmt, AVCodecID VideoCodecID, AVRational TimeBase, int nWidth, int nHeight, int nGopSize, unsigned int uiBitrate);
	virtual int							InitAudioCtx(AVSampleFormat SampleFmt, AVCodecID AudioCodecID,
																			int nChannelLayout, int nChannels, unsigned int uiSampleRate, unsigned int uiSamples, unsigned int uiBitRate);

    virtual int                         MakeFilterDescr(FILTER_DESCR_DATA* pFilterDescrData);

    virtual void                			SetFramePtsData(int nStreamIndex, 
																				int64_t llPts, int64_t llPktPts, int64_t llPktDts)              	{}
    virtual void                			GetFramePtsData(int nStreamIndex, 
																				int64_t& llPts, int64_t& llPktPts, int64_t& llPktDts)        {}

    virtual AVCodecContext*		GetCodecCtx(enum AVMediaType Type);
	virtual AVPacket*					GetCodecPkt()																	{	return &m_Pkt;				}
	virtual int								GetWidth()																			{	return m_nWidth;			}
	virtual int								GetHeight()																		{	return m_nHeight;		}

	virtual void								CtxCleanUp();

	// for encoder
	virtual void				SetCallbackProc(void* pObject, WriteEncFrameCallback pWriteEncFrameCallback)	{}
	virtual void				SetVideoSrcInfo(AVBufferRef* pHwFramesCtx, AVPixelFormat AVSrcPixFmt, int nSrcWidth, int nSrcHeight)			{}
	virtual int64_t		Encode(int nStreamIndex, 
													unsigned char* pSrcData, unsigned int uiSrcDataSize,
													unsigned char** ppEncData, unsigned int& uiEncDataSize)					{	return 0;	}
	virtual int64_t		Encode(int nStreamIndex, AVFrame* pFrame,
													unsigned char** ppEncData, unsigned int& uiEncDataSize)					{ 	return 0;	}

	// for decoder
	virtual void				SetCallbackProc(void* pObject, DataProcCallback pDataProcCallback)			{}
	virtual void				SetStreamIndex(int nVideoStreamIndex, int nAudioStreamIndex)						{}
	virtual void				SetAudioSrcInfo(AVSampleFormat AVSrcSampleFmt,
																	int nSrcChannelLayout, int nSrcChannels,
																	unsigned int uiSrcSampleRate, unsigned int uiSrcSamples)	{}

    virtual int               ReadFrameData()																											{	return 0;	}
	virtual int64_t		Decode(int nStreamIndex,
													unsigned char* pSrcData, unsigned int uiSrcDataSize,
													unsigned char** ppDecData, unsigned int& uiDecDataSize)					{	return 0;	}

protected:
	virtual int				AllocVideoBuffer()															{	return 0;	}
	virtual int				InitFrame()																		{	return 0;	}
	void							InitPacket();
    int                     			InitVideoFilters(const char* pszFilterDescr, AVFrame* pFrame);
    int                     			InitAudioFilters(const char* pszFilterDescr, AVFrame* pFrame);

	int64_t						ScalingVideo(int nSrcWidth, int nSrcHeight, AVPixelFormat AvSrcPixFmt, unsigned char* pSrcData,
																int nDstWidth, int nDstHeight, AVPixelFormat AvDstPixFmt, unsigned char** ppDstData);
	int64_t						ResamplingAudio(AVSampleFormat AvSrcSampleFmt, int nSrcChannelLayout, int nSrcChannels,
																	int nSrcSampleRate, int nSrcSamples, unsigned int uiSrcDataSize, unsigned char* pSrcData,
																	AVSampleFormat AvDstSampleFmt, int nDstChannelLayout, int nDstChannels,  
																	int nDstSampleRate, int& nDstSamples, unsigned char** ppDstData);

    int                         		FrameFilteringProc(int nStreamType, AVFrame* pFrame, AVFrame* pFilteredFrame);
    int 								GetHwCodecName(char* pszHwCodecName, int nLen, AVCodecID CodecID);

protected:
	AVFormatContext*			m_pFmtCtx;
	AVCodecContext*				m_pVideoCtx;
	AVCodecContext*				m_pAudioCtx;
	AVBufferRef* 					m_pHwDeviceCtx;
    AVBufferRef* 					m_pHwFramesCtx;

    //AVFilterContext*            m_pBuffersinkCtx;
    //AVFilterContext*            m_pBuffersrcCtx;
    //AVFilterGraph*              m_pFilterGraph;

    FilterContext*              m_pVideoFilterCtx;
    FilterContext*              m_pAudioFilterCtx;

	struct SwsContext*			m_pSwsCtx;
	struct SwrContext*			m_pSwrCtx;
	
	AVStream*							m_pVideoStream;
	AVStream*							m_pAudioStream;

	enum AVPixelFormat			m_PixelFmt;

	AVPacket							m_Pkt;

	int										m_nWidth;
	int										m_nHeight;

	int										m_nCodecType;

	bool									m_bInitFrame;
	bool									m_bInitPacket;

	bool									m_bNeedCtxCleanUp;

    bool                                    m_bInitFilters[2];

    char                        m_szVideoFiltersDescr[512];
    char                        m_szAudioFiltersDescr[512];
};

