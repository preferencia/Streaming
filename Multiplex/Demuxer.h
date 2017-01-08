#pragma once

#include "Common.h"

class CDemuxer
{
public:
	CDemuxer();
	virtual ~CDemuxer();

	int SrcFileOpenProc(char* pszFileName, AVFormatContext** pFmtCtx);
	int OpenCodecContext(int* pStreamIndex, AVCodecContext** ppDecCtx, AVFormatContext* pFmtCtx, enum AVMediaType Type);
	int GetFormatFromSampleFmt(const char** pszFmt, enum AVSampleFormat SampleFmt);

private:
	char*				m_pszSrcFileName;
};

