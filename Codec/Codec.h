#pragma once

#include "Common.h"

#ifdef _USE_FILTER_GRAPH
typedef int (*DataProcCallback)(void* pObject, int nMediaType, AVFrame* pFrame);
#else
typedef int (*DataProcCallback)(void* pObject, int nProcDataType, int nPictType, unsigned int uiDataSize, unsigned char* pData);
#endif
typedef int (*WriteEncFrameCallback)(void* pObject, AVPacket* pPacket);

class CCodec
{
public:
	CCodec();
	virtual ~CCodec();

	virtual int					InitFromContext(AVFormatContext* pFmtCtx, AVCodecContext* pVideoCtx, AVCodecContext* pAudioCtx);

	virtual int					InitVideoCtx(AVPixelFormat PixFmt, AVCodecID VideoCodecID, AVRational TimeBase, int nWidth, int nHeight, int nGopSize, unsigned int uiBitrate);
	virtual int					InitAudioCtx(AVSampleFormat SampleFmt, AVCodecID AudioCodecID,
											 int nChannelLayout, int nChannels, unsigned int uiSampleRate, unsigned int uiSamples, unsigned int uiBitRate);

    virtual void                SetFramePtsData(int nStreamIndex, 
                                                int64_t llPts, int64_t llPktPts, int64_t llPktDts)              {}
    virtual void                GetFramePtsData(int nStreamIndex, 
                                                int64_t& llPts, int64_t& llPktPts, int64_t& llPktDts)           {}

    virtual AVCodecContext*     GetCodecCtx(enum AVMediaType Type);
	virtual AVPacket*			GetCodecPkt()																	{	return &m_Pkt;		}
	virtual enum AVPixelFormat	GetPixelFmt()																	{	return m_PixelFmt;	}
	virtual int					GetWidth()																		{	return m_nWidth;	}
	virtual int					GetHeight()																		{	return m_nHeight;	}

	virtual void				CtxCleanUp();

	// for encoder
	virtual void				SetCallbackProc(void* pObject, WriteEncFrameCallback pWriteEncFrameCallback)	{}
	virtual void				SetVideoSrcInfo(AVPixelFormat AVSrcPixFmt, int nSrcWidth, int nSrcHeight)		{}
	virtual int64_t				Encode(int nStreamIndex, 
									   unsigned char* pSrcData, unsigned int uiSrcDataSize,
									   unsigned char** ppEncData, unsigned int& uiEncDataSize)					{	return 0;	}
	virtual int64_t				Encode(int nStreamIndex, AVFrame* pFrame,
									   unsigned char** ppEncData, unsigned int& uiEncDataSize)					{ 	return 0;	}

	// for decoder
	virtual void				SetCallbackProc(void* pObject, DataProcCallback pDataProcCallback)				{}
	virtual void				SetStreamIndex(int nVideoStreamIndex, int nAudioStreamIndex)					{}
	virtual void				SetAudioSrcInfo(AVSampleFormat AVSrcSampleFmt,
												int nSrcChannelLayout, int nSrcChannels,
												unsigned int uiSrcSampleRate, unsigned int uiSrcSamples)		{}

    virtual int                 ReadFrameData()																	{	return 0;	}
	virtual int64_t				Decode(int nStreamIndex,
									   unsigned char* pSrcData, unsigned int uiSrcDataSize,
									   unsigned char** ppDecData, unsigned int& uiDecDataSize)					{	return 0;	}

protected:
#ifndef _USE_FILTER_GRAPH
	virtual int					AllocVideoBuffer()																{	return 0;	}
#endif
	virtual int					InitFrame()																		{	return 0;	}
	void						InitPacket();

	int64_t						ScalingVideo(int nSrcWidth, int nSrcHeight, AVPixelFormat AvSrcPixFmt, unsigned char* pSrcData,
											 int nDstWidth, int nDstHeight, AVPixelFormat AvDstPixFmt, unsigned char** ppDstData);
	int64_t						ResamplingAudio(AVSampleFormat AvSrcSampleFmt, int nSrcChannelLayout, int nSrcChannels,
												int nSrcSampleRate, int nSrcSamples, unsigned int uiSrcDataSize, unsigned char* pSrcData,
												AVSampleFormat AvDstSampleFmt, int nDstChannelLayout, int nDstChannels,  
												int nDstSampleRate, int& nDstSamples, unsigned char** ppDstData);

protected:
	AVFormatContext*			m_pFmtCtx;
	AVCodecContext*				m_pVideoCtx;
	AVCodecContext*				m_pAudioCtx;

	struct SwsContext*			m_pSwsCtx;
	struct SwrContext*			m_pSwrCtx;
	
	AVStream*					m_pVideoStream;
	AVStream*					m_pAudioStream;

	enum AVPixelFormat			m_PixelFmt;

	AVPacket					m_Pkt;

	int							m_nWidth;
	int							m_nHeight;

	int							m_nCodecType;

	bool						m_bInitFrame;
	bool						m_bInitPacket;

	bool						m_bNeedCtxCleanUp;
};

