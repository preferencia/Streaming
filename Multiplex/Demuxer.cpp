#include "stdafx.h"
#include "Demuxer.h"

#ifdef _WIN32
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
	m_pHwDeviceCtx 	= NULL;
    m_pHwFramesCtx  = NULL;
}

CDemuxer::~CDemuxer()
{
	av_buffer_unref(&m_pHwDeviceCtx);
    av_buffer_unref(&m_pHwFramesCtx);
	m_pszFileName    	= NULL;	
}

int CDemuxer::FileOpenProc(char* pszFileName, AVFormatContext** ppFmtCtx)
{
	if ((NULL == pszFileName) || (0 >= strlen(pszFileName)))
	{
		return -1;
	}

	m_pszFileName = pszFileName;

	int nRet = 0;

	/* open input file, and allocate format context */
	nRet = avformat_open_input(ppFmtCtx, m_pszFileName, NULL, NULL);
	if (0 > nRet)
	{
        TraceLog("Could not open source file [result = %d]", nRet);
		return -2;
	}

	/* retrieve stream information */
	nRet = avformat_find_stream_info(*ppFmtCtx, NULL);
	if (0 > nRet)
	{
		TraceLog("Could not find stream information");
		return -3;
	}

	return nRet;
}

int CDemuxer::OpenCodecContext(int* pStreamIndex, AVCodecContext** ppCodecCtx, 
							   AVFormatContext* pInFmtCtx, AVFormatContext* pOutFmtCtx, enum AVMediaType Type)
{
	if ((NULL == ppCodecCtx) || (NULL != *ppCodecCtx))
	{
		return -1;
	}

	if (NULL == pInFmtCtx)
	{
		return -2;
	}

	int nRet			= -1; 
	int nStreamIndex	= -1;
	AVStream* pSt		= NULL;
	AVCodec* pDec		= NULL;
	AVDictionary* pOpts = NULL;

	nRet = av_find_best_stream(pInFmtCtx, Type, -1, -1, NULL, 0);
	if (0 > nRet)
	{
		TraceLog("Could not find %s stream in input file '%s'", av_get_media_type_string(Type), m_pszFileName);
		return nRet;
	}
	else
	{
		nStreamIndex = nRet;
		pSt = pInFmtCtx->streams[nStreamIndex];

		/* find decoder for the stream */
		pDec = avcodec_find_decoder(pSt->codecpar->codec_id);
		if (!pDec)
		{
			TraceLog("Failed to find %s codec", av_get_media_type_string(Type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*ppCodecCtx = avcodec_alloc_context3(pDec);
		if (!*ppCodecCtx)
		{
			TraceLog("Failed to allocate the %s codec context", av_get_media_type_string(Type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		nRet = avcodec_parameters_to_context(*ppCodecCtx, pSt->codecpar);
		if (0 > nRet)
		{
			TraceLog("Failed to copy %s codec parameters to decoder context", av_get_media_type_string(Type));
			return nRet;
		}

		TraceLog("CDemuxer::OpenCodecContext - Media Type[%d] - avcodec_parameters_to_context result = %d", Type, nRet);

		/* Init the decoders, with or without reference counting */
		av_dict_set(&pOpts, "refcounted_frames", "1", 0);
		
		/* Init video hw accel */
		if ( (-1 < g_nHwDevType) && (AVMEDIA_TYPE_VIDEO == Type) )
		{
			TraceLog("CDemuxer::OpenCodecContext - Try init video hw accel codec");
#ifdef _WIN32
            if (true == g_bCheckHwPixFmt)
#endif
            {
                (*ppCodecCtx)->get_format  = HWAccel::GetHwFormat;
            }
            
			nRet = HWAccel::InitHwCodec(*ppCodecCtx, (const enum AVHWDeviceType)g_nHwDevType, &m_pHwDeviceCtx, &m_pHwFramesCtx);
            TraceLog("CDemuxer::OpenCodecContext - Init video hw accel codec result = %d, hw device ctx = 0x%08X, hw frames ctx = 0x%08X", nRet, m_pHwDeviceCtx, m_pHwFramesCtx);
		}
		
		nRet = avcodec_open2(*ppCodecCtx, pDec, &pOpts);
		if (0 > nRet)
		{
			TraceLog("Failed to open %s codec", av_get_media_type_string(Type));
			return nRet;
		}

		TraceLog("CDemuxer::OpenCodecContext - Media Type[%d] - avcodec_open2 result = %d", Type, nRet);

		*pStreamIndex = nStreamIndex;
	}

	return 0;
}

int CDemuxer::GetFormatFromSampleFmt(const char** pszFmt, enum AVSampleFormat SampleFmt)
{
	SampleFmtEntry sample_fmt_entries[] =
	{
		{ AV_SAMPLE_FMT_U8,  "u8",    		"u8" 		},
		{ AV_SAMPLE_FMT_S16, "s16be", 	"s16le" 	},
		{ AV_SAMPLE_FMT_S32, "s32be", 	"s32le" 	},
		{ AV_SAMPLE_FMT_FLT, "f32be", 	"f32le" 	},
		{ AV_SAMPLE_FMT_DBL, "f64be", 	"f64le" 	},
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
