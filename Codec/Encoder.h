#pragma once

#include "Codec.h"

class CEncoder : public CCodec
{
public:
	CEncoder();
	virtual ~CEncoder();

    virtual void                SetFramePtsData(int nStreamIndex, 
                                                int64_t llPts, int64_t llPktPts, int64_t llPktDts);
    virtual void                GetFramePtsData(int nStreamIndex, 
                                                int64_t& llPts, int64_t& llPktPts, int64_t& llPktDts);

	virtual void				SetCallbackProc(void* pObject, WriteEncFrameCallback pWriteEncFrameCallback);
	virtual void				SetVideoSrcInfo(AVBufferRef* pHwFramesCtx, AVPixelFormat AVSrcPixFmt, int nSrcWidth, int nSrcHeight);
	virtual int64_t				Encode(int nStreamIndex,
									   unsigned char* pSrcData, unsigned int uiSrcDataSize,
									   unsigned char** ppEncData, unsigned int& uiEncDataSize);
	virtual int64_t				Encode(int nStreamIndex, AVFrame* pFrame,
									   unsigned char** ppEncData, unsigned int& uiEncDataSize);

private:
	virtual int64_t				EncodeVideo(unsigned char* pSrcData, unsigned int uiSrcDataSize,
											unsigned char** ppEncData, unsigned int& uiEncDataSize);
	virtual int64_t				EncodeAudio(unsigned char* pSrcData, unsigned int uiSrcDataSize,
											unsigned char** ppEncData, unsigned int& uiEncDataSize);
	virtual int64_t				EncodeDelayedFrame(int nStreamIndex, unsigned char** ppEncData, unsigned int& uiEncDataSize);

	virtual int64_t 			PacketToBuffer(int nStreamIndex, int nGotOutput, unsigned char** ppEncData);

protected:
	virtual int					InitFrame();

private:
	AVFrame*								m_pVideoFrame;
	AVFrame*								m_pAudioFrame;

	// Video Src Info (for scaling)
	AVPixelFormat						m_AVSrcPixFmt; 
	int											m_nSrcWidth; 
	int											m_nSrcHeight;

	void*										m_pObject;
	WriteEncFrameCallback		m_pWriteEncFrameCallback;
};

