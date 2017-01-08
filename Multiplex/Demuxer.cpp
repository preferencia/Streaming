#include "stdafx.h"
#include "Demuxer.h"

#ifdef _WINDOWS
#pragma warning(disable:4996)
#endif


typedef struct sample_fmt_entry
{
	enum AVSampleFormat SampleFmt;
	const char* pszFmtBe;
	const char* pszFmtLe;
} SampleFmtEntry;


CDemuxer::CDemuxer()
{
	m_pszSrcFileName	= NULL;
}


CDemuxer::~CDemuxer()
{
	m_pszSrcFileName = NULL;
}


int CDemuxer::SrcFileOpenProc(char* pszSrcFileName, AVFormatContext** ppFmtCtx)
{
	if ((NULL == pszSrcFileName) || (0 >= strlen(pszSrcFileName)))
	{
		return -1;
	}

	m_pszSrcFileName = pszSrcFileName;

	/* open input file, and allocate format context */
	if (0 > avformat_open_input(ppFmtCtx, m_pszSrcFileName, NULL, NULL))
	{
		TraceLog("Could not open source file %s", m_pszSrcFileName);
		return -2;
	}

	/* retrieve stream information */
	if (0 > avformat_find_stream_info(*ppFmtCtx, NULL))
	{
		TraceLog("Could not find stream information");
		return -3;
	}

	return 0;
}

int CDemuxer::OpenCodecContext(int* pStreamIndex, AVCodecContext** ppDecCtx, AVFormatContext* pFmtCtx, enum AVMediaType Type)
{
	int nRet			= -1; 
	int nStreamIndex	= -1;
	AVStream* pSt		= NULL;
	AVCodec* pDec		= NULL;
	AVDictionary* pOpts = NULL;

	nRet = av_find_best_stream(pFmtCtx, Type, -1, -1, NULL, 0);
	if (0 > nRet)
	{
		TraceLog("Could not find %s stream in input file '%s'",
			av_get_media_type_string(Type), m_pszSrcFileName);
		return nRet;
	}
	else
	{
		nStreamIndex = nRet;
		pSt = pFmtCtx->streams[nStreamIndex];

		/* find decoder for the stream */
		pDec = avcodec_find_decoder(pSt->codecpar->codec_id);
		if (!pDec)
		{
			TraceLog("Failed to find %s codec",
				av_get_media_type_string(Type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*ppDecCtx = avcodec_alloc_context3(pDec);
		if (!*ppDecCtx)
		{
			TraceLog("Failed to allocate the %s codec context",
				av_get_media_type_string(Type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if (0 > (nRet = avcodec_parameters_to_context(*ppDecCtx, pSt->codecpar)))
		{
			TraceLog("Failed to copy %s codec parameters to decoder context",
				av_get_media_type_string(Type));
			return nRet;
		}

		/* Init the decoders, with or without reference counting */
		av_dict_set(&pOpts, "refcounted_frames", "1", 0);
		if (0 > (nRet = avcodec_open2(*ppDecCtx, pDec, &pOpts)))
		{
			TraceLog("Failed to open %s codec",
				av_get_media_type_string(Type));
			return nRet;
		}

		*pStreamIndex = nStreamIndex;
	}

	return 0;
}

int CDemuxer::GetFormatFromSampleFmt(const char** pszFmt, enum AVSampleFormat SampleFmt)
{
	SampleFmtEntry sample_fmt_entries[] =
	{
		{ AV_SAMPLE_FMT_U8,  "u8",    "u8" },
		{ AV_SAMPLE_FMT_S16, "s16be", "s16le" },
		{ AV_SAMPLE_FMT_S32, "s32be", "s32le" },
		{ AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
		{ AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
	};
	
	*pszFmt = NULL;

	for (int nIndex = 0; nIndex < FF_ARRAY_ELEMS(sample_fmt_entries); ++nIndex)
	{
		struct sample_fmt_entry *entry = &sample_fmt_entries[nIndex];
		if (SampleFmt == entry->SampleFmt)
		{
			*pszFmt = AV_NE(entry->pszFmtBe, entry->pszFmtLe);
			return 0;
		}
	}

	fprintf(stderr,
		"sample format %s is not supported as output format\n",
		av_get_sample_fmt_name(SampleFmt));

	return -1;
}