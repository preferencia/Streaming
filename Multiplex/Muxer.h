#pragma once

#include "Mux.h"

class CMuxer : public CMux
{
public:
	CMuxer();
	virtual ~CMuxer();

	virtual int		FileOpenProc(char* pszDstFileName, AVFormatContext** ppFmtCtx);
	virtual int		OpenCodecContext(int* pStreamIndex, AVCodecContext** ppCodecCtx, 
									 AVFormatContext* pInFmtCtx, AVFormatContext* pOutFmtCtx, enum AVMediaType Type);

	virtual void	SetMuxInfo(int nWidth, int nHeight, int64_t llVideoBitrate, int64_t llAudioBitrate);
    virtual int		WriteFileHeader(AVFormatContext* pFmtCtx);
	virtual int		WriteEncFrame(AVFormatContext* pFmtCtx, AVPacket* pPacket);

private:
	int		m_nWidth;
	int		m_nHeight;
	int64_t m_llVideoBitrate;
	int64_t m_llAudioBitrate;
};

