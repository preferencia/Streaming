#pragma once

#include "Codec.h"

class CEncoder : public CCodec
{
public:
	CEncoder();
	virtual ~CEncoder();

	virtual void				SetVideoSrcInfo(AVPixelFormat AVSrcPixFmt, int nSrcWidth, int nSrcHeight);
	virtual int64_t				Encode(int nStreamIndex,
									   unsigned char* pSrcData, unsigned int uiSrcDataSize,
									   unsigned char** ppEncData, unsigned int& uiEncDataSize, 
									   bool bEncodeDelayedFrame = false);

private:
	virtual int64_t				EncodeVideo(unsigned char* pSrcData, unsigned int uiSrcDataSize,
											unsigned char** ppEncData, unsigned int& uiEncDataSize);
	virtual int64_t				EncodeAudio(unsigned char* pSrcData, unsigned int uiSrcDataSize,
											unsigned char** ppEncData, unsigned int& uiEncDataSize);
	virtual int64_t				EncodeDelayedFrame(int nStreamIndex, unsigned char** ppEncData, unsigned int& uiEncDataSize);

protected:
	virtual int					InitFrame();

private:
	AVFrame*					m_pVideoFrame;
	AVFrame*					m_pAudioFrame;

	// Video Src Info (for scaling)
	AVPixelFormat				m_AVSrcPixFmt; 
	int							m_nSrcWidth; 
	int							m_nSrcHeight;
	
	// for FAAC
	void*						m_pFAACEncHandle;

	unsigned long				m_ulSamples;
	unsigned long				m_ulMaxOutBytes;
};

