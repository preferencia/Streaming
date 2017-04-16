#pragma once

#include "Mux.h"

class CDemuxer : public CMux
{
public:
	CDemuxer();
	virtual ~CDemuxer();

	virtual int		FileOpenProc(char* pszFileName, AVFormatContext** ppFmtCtx);
	virtual int		OpenCodecContext(int* pStreamIndex, AVCodecContext** ppCodecCtx, 
									 AVFormatContext* pInFmtCtx, AVFormatContext* pOutFmtCtx, enum AVMediaType Type);
	virtual int		GetFormatFromSampleFmt(const char** pszFmt, enum AVSampleFormat SampleFmt);
};

