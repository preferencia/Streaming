#pragma once

#include "Common.h"

class CMux
{
public:
    CMux() : m_nMuxType(OBJECT_TYPE_NONE), 
			 m_pszFileName(NULL)
    {

    }
	virtual ~CMux() {}

	virtual int		FileOpenProc(char* pszFileName, AVFormatContext** ppFmtCtx)											{ return 0; }
	virtual int		OpenCodecContext(int* pStreamIndex, AVCodecContext** ppCodecCtx, 
									 AVFormatContext* pInFmtCtx, AVFormatContext* pOutFmtCtx, enum AVMediaType Type)	{ return 0; }
    virtual int		GetFormatFromSampleFmt(const char** pszFmt, enum AVSampleFormat SampleFmt)							{ return 0; }

    // for muxing
	virtual void	SetMuxInfo(int nWidth, int nHeight, int64_t llVideoBitrate, int64_t llAudioBitrate)					{			}
    virtual int		WriteFileHeader(AVFormatContext* pFmtCtx)											                { return 0; }
	virtual int		WriteEncFrame(AVFormatContext* pFmtCtx, AVPacket* pPacket)											{ return 0; }

protected:
    int         m_nMuxType;
	char*		m_pszFileName;
};

