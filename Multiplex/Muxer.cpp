#include "stdafx.h"
#include "Muxer.h"

#ifdef _WIN32
#pragma warning(disable:4996)
#endif

CMuxer::CMuxer()
{
	m_nWidth			= 0;
	m_nHeight			= 0;
	m_llVideoBitrate	= 0LL;
	m_llAudioBitrate	= 0LL;
}

CMuxer::~CMuxer()
{
	m_pszFileName    = NULL;
}

int CMuxer::FileOpenProc(char* pszFileName, AVFormatContext** ppFmtCtx)
{
	if ((NULL == pszFileName) || (0 >= strlen(pszFileName)))
	{
		return -1;
	}

	if ((NULL == ppFmtCtx) || (NULL != *ppFmtCtx))
	{
		return -3;
	}

	m_pszFileName = pszFileName;

	int nRet = avformat_alloc_output_context2(ppFmtCtx, NULL, NULL, m_pszFileName);
	if (NULL == *ppFmtCtx)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
		return AVERROR_UNKNOWN;
	}

    if (!((*ppFmtCtx)->oformat->flags & AVFMT_NOFILE))
	{
		nRet = avio_open(&(*ppFmtCtx)->pb, m_pszFileName, AVIO_FLAG_WRITE);
		if (0 > nRet)
		{
			TraceLog("Could not open output file '%s'", m_pszFileName);
			return nRet;
		}
	}

	//(*ppFmtCtx)->oformat->flags |= AVFMT_NOFILE;

	return 0;
}

int CMuxer::OpenCodecContext(int* pStreamIndex, AVCodecContext** ppCodecCtx,
							 AVFormatContext* pInFmtCtx, AVFormatContext* pOutFmtCtx, enum AVMediaType Type)
{
	if ((NULL == ppCodecCtx) || (NULL != *ppCodecCtx))
	{
		return -1;
	}

	if ((NULL == pInFmtCtx) || (NULL == pOutFmtCtx))
	{
		return -2;
	}

	int				nRet			= 0;
	int				nStreamIndex	= (int)Type;

	AVStream*		pOutStream		= NULL;
	AVStream*		pInStream		= NULL;
	AVCodecContext* pDecCtx			= NULL; 
	AVCodecContext* pEncCtx			= NULL;
	AVCodec*		pEncoder		= NULL;

	pOutStream = avformat_new_stream(pOutFmtCtx, NULL);
	if (!pOutStream)
	{
		TraceLog("Failed allocating output stream");
		return AVERROR_UNKNOWN;
	}

	pInStream	= pInFmtCtx->streams[nStreamIndex];
	pDecCtx		= pInStream->codec;
	pEncCtx		= pOutStream->codec;

	if ((AVMEDIA_TYPE_VIDEO == pDecCtx->codec_type)
		|| (AVMEDIA_TYPE_AUDIO == pDecCtx->codec_type))
	{
		/* in this example, we choose transcoding to same codec */
		if (AVMEDIA_TYPE_VIDEO == pDecCtx->codec_type)
		{
			pEncoder = avcodec_find_encoder(AV_CODEC_ID_H264);
		}
		else
		{
			pEncoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
		}

		if (!pEncoder)
		{
			TraceLog("Necessary encoder not found");
			return AVERROR_INVALIDDATA;
		}

		/* In this example, we transcode to same properties (picture size,
		* sample rate etc.). These properties can be changed for output
		* streams easily using filters */
		if (AVMEDIA_TYPE_VIDEO == pDecCtx->codec_type)
		{
			pEncCtx->width								= m_nWidth;
			pEncCtx->height							= m_nHeight;
			pEncCtx->sample_aspect_ratio	= pDecCtx->sample_aspect_ratio;
			pEncCtx->pix_fmt							= AV_PIX_FMT_YUV420P;

			/* video time_base can be set to whatever is handy and supported by encoder */
			pEncCtx->time_base						= pDecCtx->time_base;

			pEncCtx->bit_rate						= m_llVideoBitrate;

			pEncCtx->qcompress						= 0.6;
			pEncCtx->qmin								= 3;
			pEncCtx->qmax								= 35;
			pEncCtx->max_qdiff						= 4;
		}
		else
		{
			pEncCtx->sample_rate					= pDecCtx->sample_rate;
			pEncCtx->channel_layout			= AV_CH_LAYOUT_STEREO;
			pEncCtx->channels						= av_get_channel_layout_nb_channels(pEncCtx->channel_layout);

			/* take first format from list of supported formats */
			pEncCtx->sample_fmt					= AV_SAMPLE_FMT_FLTP;
			pEncCtx->time_base.num				= 1;
			pEncCtx->time_base.den				= pEncCtx->sample_rate;

			pEncCtx->bit_rate						= m_llAudioBitrate;
		}

		/* Third parameter can be used to pass settings to encoder */
		nRet = avcodec_open2(pEncCtx, pEncoder, NULL);
		if (0 > nRet)
		{
			TraceLog("Cannot open video encoder for stream #%u", nStreamIndex);
			return nRet;
		}
	}
	else if (AVMEDIA_TYPE_UNKNOWN == pDecCtx->codec_type)
	{
		TraceLog("Elementary stream #%d is of unknown type, cannot proceed", nStreamIndex);
		return AVERROR_INVALIDDATA;
	}
	else
	{
		/* if this stream must be remuxed */
		nRet = avcodec_copy_context(pOutFmtCtx->streams[nStreamIndex]->codec, pInFmtCtx->streams[nStreamIndex]->codec);
		if (0 > nRet)
		{
			TraceLog("Copying stream context failed");
			return nRet;
		}
	}

    if (pOutFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
		pEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	*pStreamIndex	= nStreamIndex;
	*ppCodecCtx		= pEncCtx;

	return nRet;
}

void CMuxer::SetMuxInfo(int nWidth, int nHeight, int64_t llVideoBitrate, int64_t llAudioBitrate)
{
	m_nWidth			= nWidth;
	m_nHeight			= nHeight;
	m_llVideoBitrate	= llVideoBitrate;
	m_llAudioBitrate	= llAudioBitrate;
}

int CMuxer::WriteFileHeader(AVFormatContext* pFmtCtx)
{
    if (NULL == pFmtCtx)
    {
        return -1;
    }

    /* init muxer, write output file header */
    int nRet = avformat_write_header(pFmtCtx, NULL);
    if (0 > nRet) 
    {
        TraceLog("Error occurred when opening %s", m_pszFileName);
        return nRet;
    }

    return 0;
}

int CMuxer::WriteEncFrame(AVFormatContext* pFmtCtx, AVPacket* pPacket)
{
	if ((NULL == pFmtCtx) || (NULL == pPacket))
	{
		return -1;
	}

    /* prepare packet for muxing */
    av_packet_rescale_ts(pPacket,
											pFmtCtx->streams[pPacket->stream_index]->codec->time_base,
											pFmtCtx->streams[pPacket->stream_index]->time_base);

	int nRet = av_interleaved_write_frame(pFmtCtx, pPacket);
	return nRet;
}
