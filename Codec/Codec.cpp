#include "stdafx.h"
#include "Codec.h"


CCodec::CCodec()
{
	m_pFmtCtx				= NULL;
	m_pVideoCtx				= NULL;
	m_pAudioCtx				= NULL;

	m_pSwsCtx				= NULL;
	m_pSwrCtx				= NULL;

	m_pVideoStream			= NULL;
	m_pAudioStream			= NULL;

	m_PixelFmt				= AV_PIX_FMT_NONE;

	m_nWidth				= 0;
	m_nHeight				= 0;

	m_nCodecType			= OBJECT_TYPE_NONE;

	m_bInitFrame			= false;
	m_bInitPacket			= false;

	m_bNeedCtxCleanUp		= true;
}


CCodec::~CCodec()
{
	if (true == m_bNeedCtxCleanUp)
	{
		CtxCleanUp();
	}

	if (NULL != m_pSwsCtx)
	{
		sws_freeContext(m_pSwsCtx);
		m_pSwsCtx = NULL;
	}

	if (NULL != m_pSwrCtx)
	{
		swr_free(&m_pSwrCtx);
		m_pSwrCtx = NULL;
	}
}


int CCodec::InitFromContext(AVFormatContext* pFmtCtx, AVCodecContext* pVideoCtx, AVCodecContext* pAudioCtx)
{
	if ((NULL == pFmtCtx) || (NULL == pVideoCtx) || (NULL == pAudioCtx))
	{
		return -1;
	}

	m_pFmtCtx	= pFmtCtx;
	m_pVideoCtx = pVideoCtx;
	m_pAudioCtx = pAudioCtx;

	/* allocate image where the decoded image will be put */
	m_nWidth	= m_pVideoCtx->width;
	m_nHeight	= m_pVideoCtx->height;
	m_PixelFmt	= m_pVideoCtx->pix_fmt;

	if (false == m_bInitFrame)
	{
		if (0 > InitFrame())
		{
			return -2;
		}
	}

	if (false == m_bInitPacket)
	{
		InitPacket();
	}

#ifndef _USE_FILTER_GRAPH
	if (OBJECT_TYPE_DECODER == m_nCodecType)
	{
		if (0 > AllocVideoBuffer())
		{
			return -3;
		}
	}
#endif

	m_bNeedCtxCleanUp = false;

	return 0;
}


int CCodec::InitVideoCtx(AVPixelFormat PixFmt, AVCodecID VideoCodecID, AVRational TimeBase, int nWidth, int nHeight, int nGopSize, unsigned int uiBitrate)
{
	int         nErr        = 0;
    AVCodec*    pVideoCodec = NULL;

	if (NULL == m_pFmtCtx)
	{
		/** Create a new format context for the output container format. */
		m_pFmtCtx = avformat_alloc_context();
		if (NULL == m_pFmtCtx)
		{
			nErr = -1;
			goto $ERROR_END;
		}
	}

	if (NULL != m_pVideoCtx)
	{
		nErr = -2;
		goto $ERROR_END;
	}

	/** Find the encoder/decoder to be used by its name. */
	switch (m_nCodecType)
	{
	case OBJECT_TYPE_ENCODER:
		{
			pVideoCodec = avcodec_find_encoder(VideoCodecID);
		}
		break;

	case OBJECT_TYPE_DECODER:
		{
			pVideoCodec = avcodec_find_decoder(VideoCodecID);
		}
		break;

	default:
		{

		}
		break;
	}

	if (NULL == pVideoCodec)
	{
		nErr = -3;
		goto $ERROR_END;
	}

	/** Create a new video stream in the output file container. */
	if (NULL == (m_pVideoStream = avformat_new_stream(m_pFmtCtx, NULL)))
	{
		nErr = -4;
		goto $ERROR_END;
	}

	//m_pVideoCtx = avcodec_alloc_context3(pVideoCodec);
    m_pVideoCtx = m_pVideoStream->codec;
	if (NULL == m_pVideoCtx)
	{
		nErr = -5;
		goto $ERROR_END;
	}

	/**
	* Set the basic encoder parameters.
	* The input file's sample rate is used to avoid a sample rate conversion.
	*/
	m_pVideoCtx->pix_fmt			            = PixFmt;
	m_pVideoCtx->time_base			            = TimeBase;
	m_pVideoCtx->width				            = nWidth;
	m_pVideoCtx->height				            = nHeight;
	
	/* emit one intra frame every ten frames
	* check frame pict_type before passing frame
	* to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	* then gop_size is ignored and the output of encoder
	* will always be I frame irrespective to gop_size
	*/
	m_pVideoCtx->gop_size			            = nGopSize;
	m_pVideoCtx->bit_rate			            = uiBitrate;
	m_pVideoCtx->bit_rate_tolerance             = uiBitrate;

    m_pVideoCtx->me_method			            = 5;
    m_pVideoCtx->me_cmp			                = 0;
    m_pVideoCtx->me_subpel_quality	            = 8;
    m_pVideoCtx->me_range			            = 0;

    m_pVideoCtx->b_quant_factor	                = 1.25;
	m_pVideoCtx->b_frame_strategy	            = 0;
	m_pVideoCtx->i_quant_factor		            = -0.8;

    m_pVideoCtx->keyint_min			            = 25;
    m_pVideoCtx->refs                           = 1;

    m_pVideoCtx->qcompress			            = 0.6;
    m_pVideoCtx->qblur                          = 0.5;
	m_pVideoCtx->qmin				            = 3;
	m_pVideoCtx->qmax				            = 35;
	m_pVideoCtx->max_qdiff			            = 4;

    m_pVideoCtx->rc_initial_buffer_occupancy	= 0;
    m_pVideoCtx->coder_type                     = 0;
	m_pVideoCtx->min_prediction_order			= -1;
	m_pVideoCtx->max_prediction_order			= -1;
	
    m_pVideoCtx->thread_count			        = 1;
    m_pVideoCtx->thread_type			        = 3;

    m_pVideoCtx->sub_text_format	            = 1;

	m_nWidth						            = m_pVideoCtx->width;
	m_nHeight						            = m_pVideoCtx->height;
	m_PixelFmt						            = m_pVideoCtx->pix_fmt;
	
	/**
	* Some container formats (like MP4) require global headers to be present
	* Mark the encoder so that it behaves accordingly.
	*/
	m_pFmtCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	/** Open the encoder for the audio stream to use it later. */
	nErr = avcodec_open2(m_pVideoCtx, pVideoCodec, NULL);
	if (0 > nErr)
	{
		char szErrLog[AV_ERROR_MAX_STRING_SIZE] = { 0, };
		TraceLog("Could not open output codec (error '%s')",
			    av_strerror(nErr, szErrLog, AV_ERROR_MAX_STRING_SIZE));
		nErr = -6;
		goto $ERROR_END;
	}

	nErr = avcodec_parameters_from_context(m_pVideoStream->codecpar, m_pVideoCtx);
	if (0 > nErr)
	{
		TraceLog("Could not initialize stream parameters");
		nErr = -7;
		goto $ERROR_END;
	}

	if (false == m_bInitFrame)
	{
		if (0 > InitFrame())
		{
			nErr = -2;
			goto $ERROR_END;
		}
	}

	if (false == m_bInitPacket)
	{
		InitPacket();
	}

#ifndef _USE_FILTER_GRAPH
	if (OBJECT_TYPE_DECODER == m_nCodecType)
	{
		if (0 > AllocVideoBuffer())
		{
			nErr = -3;
			goto $ERROR_END;
		}
	}
#endif

	return 0;

$ERROR_END:
	avcodec_free_context(&m_pVideoCtx);
	avformat_free_context(m_pFmtCtx);

	m_pVideoCtx = NULL;
	m_pFmtCtx = NULL;

	return nErr;
}


int CCodec::InitAudioCtx(AVSampleFormat SampleFmt, AVCodecID AudioCodecID,
						 int nChannelLayout, int nChannels, unsigned int uiSampleRate, unsigned int uiSamples, unsigned int uiBitRate)
{
	int         nErr        = 0;
    AVCodec*    pAuidoCodec = NULL;

	if (NULL == m_pFmtCtx)
	{
		/** Create a new format context for the output container format. */
		m_pFmtCtx = avformat_alloc_context();
		if (NULL == m_pFmtCtx)
		{
			nErr = -1;
			goto $ERROR_END;
		}
	}

	if (NULL != m_pAudioCtx)
	{
		nErr = -2;
		goto $ERROR_END;
	}

	/** Find the encoder/decoder to be used by its name. */
	switch (m_nCodecType)
	{
	case OBJECT_TYPE_ENCODER:
		{
			pAuidoCodec = avcodec_find_encoder(AudioCodecID);
		}
		break;

	case OBJECT_TYPE_DECODER:
		{
			pAuidoCodec = avcodec_find_decoder(AudioCodecID);
		}
		break;

	default:
		{

		}
		break;
	}

	if (NULL == pAuidoCodec)
	{
		nErr = -3;
		goto $ERROR_END;
	}

	/** Create a new audio stream in the output file container. */
	if (NULL == (m_pAudioStream = avformat_new_stream(m_pFmtCtx, NULL)))
	{
		nErr = -4;
		goto $ERROR_END;
	}

	//m_pAudioCtx = avcodec_alloc_context3(pAuidoCodec);
    m_pAudioCtx = m_pAudioStream->codec;
	if (NULL == m_pAudioCtx)
	{
		nErr = -5;
		goto $ERROR_END;
	}

	/**
	* Set the basic encoder parameters.
	* The input file's sample rate is used to avoid a sample rate conversion.
	*/
	m_pAudioCtx->sample_fmt = SampleFmt;
	m_pAudioCtx->channel_layout = nChannelLayout;
	m_pAudioCtx->channels = nChannels;
	m_pAudioCtx->sample_rate = uiSampleRate;
	m_pAudioCtx->frame_size = uiSamples;
	m_pAudioCtx->bit_rate = uiBitRate;

	/** Allow the use of the experimental AAC encoder */
	m_pAudioCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	/** Set the sample rate for the container. */
	m_pAudioStream->time_base.den = uiSampleRate;
	m_pAudioStream->time_base.num = 1;

	/**
	* Some container formats (like MP4) require global headers to be present
	* Mark the encoder so that it behaves accordingly.
	*/
	m_pFmtCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	/** Open the encoder for the audio stream to use it later. */
	nErr = avcodec_open2(m_pAudioCtx, pAuidoCodec, NULL);
	if (0 > nErr)
	{
		char szErrLog[AV_ERROR_MAX_STRING_SIZE] = { 0, };
		TraceLog("Could not open output codec (error '%s')",
			    av_strerror(nErr, szErrLog, AV_ERROR_MAX_STRING_SIZE));
		nErr = -6;
		goto $ERROR_END;
	}

	nErr = avcodec_parameters_from_context(m_pAudioStream->codecpar, m_pAudioCtx);
	if (0 > nErr)
	{
		TraceLog("Could not initialize stream parameters\n");
		nErr = -7;
		goto $ERROR_END;
	}

	if (false == m_bInitFrame)
	{
		if (0 > InitFrame())
		{
			nErr = -10;
			goto $ERROR_END;
		}
	}

	if (false == m_bInitPacket)
	{
		InitPacket();
	}

	return 0;

$ERROR_END:
	avcodec_free_context(&m_pAudioCtx);
	avformat_free_context(m_pFmtCtx);

	m_pAudioCtx = NULL;
	m_pFmtCtx = NULL;

	return nErr;
}


AVCodecContext* CCodec::GetCodecCtx(enum AVMediaType Type)
{
    AVCodecContext* pCodecCtx = NULL;

    switch (Type)
    {
    case AVMEDIA_TYPE_VIDEO:
        {
            pCodecCtx = m_pVideoCtx;
        }
        break;

    case AVMEDIA_TYPE_AUDIO:
        {
            pCodecCtx = m_pAudioCtx;
        }
        break;

    default:
        break;
    }

    return pCodecCtx;
}


void CCodec::CtxCleanUp()
{
    avcodec_close(m_pVideoCtx);
    avcodec_close(m_pAudioCtx);

	//avcodec_free_context(&m_pVideoCtx);
	//avcodec_free_context(&m_pAudioCtx);

    avformat_close_input(&m_pFmtCtx);
}


void CCodec::InitPacket()
{
	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&m_Pkt);
	m_Pkt.data		= NULL;
	m_Pkt.size		= 0;
	m_bInitPacket	= true;
}


int64_t	CCodec::ScalingVideo(int nSrcWidth, int nSrcHeight, AVPixelFormat AvSrcPixFmt, unsigned char* pSrcData,
							 int nDstWidth, int nDstHeight, AVPixelFormat AvDstPixFmt, unsigned char** ppDstData)
{
	if (NULL == pSrcData)
	{
		return -1;
	}

	if ((0 >= nSrcWidth) || (0 >= nSrcHeight) || (AV_PIX_FMT_NONE == AvSrcPixFmt) || (AV_PIX_FMT_NONE == AvDstPixFmt))
	{
		return -2;
	}

	if ((0 >= nDstWidth) || (0 >= nDstHeight))
	{
		return -3;
	}

	uint8_t* ppInData[_DEC_PLANE_SIZE] = { 0, };
	uint8_t* ppOutData[_DEC_PLANE_SIZE] = { 0, };

	int		nSwsScaleRet = 0;
	int		nSrcLineSize = av_image_get_linesize(AvSrcPixFmt, nSrcWidth, 1);
	int		SrcLineSize[_DEC_PLANE_SIZE] = { nSrcWidth, nSrcLineSize, nSrcLineSize, 0 };
	int		DstLineSize[_DEC_PLANE_SIZE] = { 0, };
	int64_t llRet = 0;

	/* create scaling context */
	if (NULL == m_pSwsCtx)
	{
		m_pSwsCtx = sws_getContext(nSrcWidth, nSrcHeight, AvSrcPixFmt,
								   nDstWidth, nDstHeight, AvDstPixFmt,
								   SWS_BILINEAR, NULL, NULL, NULL);
	}

	if (NULL == m_pSwsCtx)
	{
		return -5;
	}

	llRet = av_image_alloc(ppInData, SrcLineSize, nSrcWidth, nSrcHeight, AvSrcPixFmt, 1);
	if (0 > llRet)
	{
		return -6;
	}
	memcpy(ppInData[0], pSrcData, llRet);

	// If src and dst's data are same,
	if ((nSrcWidth == nDstWidth) && (nSrcHeight == nDstHeight) && (AvSrcPixFmt == AvDstPixFmt))
	{
		*ppDstData = ppInData[0];
		return llRet;
	}

	llRet = av_image_alloc(ppOutData, DstLineSize, nDstWidth, nDstHeight, AvDstPixFmt, 1);
	if (0 > llRet)
	{
		return -6;
	}

	/* convert to destination format */
	if (nDstHeight != (nSwsScaleRet = sws_scale(m_pSwsCtx, ppInData, SrcLineSize, 0, nSrcHeight, ppOutData, DstLineSize)))
	{
		av_freep(ppOutData);
		return -7;
	}

	*ppDstData = ppOutData[0];

	av_freep(&ppInData[0]);

	return llRet;
}


int64_t	CCodec::ResamplingAudio(AVSampleFormat AvSrcSampleFmt, int nSrcChannelLayout, int nSrcChannels,
								int nSrcSampleRate, int nSrcSamples, unsigned int uiSrcDataSize, unsigned char* pSrcData,
								AVSampleFormat AvDstSampleFmt, int nDstChannelLayout, int nDstChannels,
								int nDstSampleRate, int& nDstSamples, unsigned char** ppDstData)
{
	if (NULL == pSrcData)
	{
		return -1;
	}

	if ((0 >= nSrcSampleRate) || (0 >= nSrcChannels) || (0 >= nSrcSamples) || (AV_SAMPLE_FMT_NONE == AvSrcSampleFmt))
	{
		return -2;
	}

	if ((0 >= nDstSampleRate) || (0 >= nDstChannels))
	{
		return -3;
	}

	int64_t			llRet = 0;
	int				nConvertRet = 0;
	int             nMaxDstSamples = 0;
	int             nSrcLineSize = 0;
	int             nDstLineSize = 0;
	uint8_t**		ppInData = NULL;
	uint8_t**		ppOutData = NULL;


	if (NULL == m_pSwrCtx)
	{
		m_pSwrCtx = swr_alloc();
	}

	if (NULL == m_pSwrCtx)
	{
		return -4;
	}

	/* set options */
	av_opt_set_int(m_pSwrCtx,			"in_channel_layout",	nSrcChannelLayout,  0);
	av_opt_set_int(m_pSwrCtx,			"in_sample_rate",		nSrcSampleRate,     0);
	av_opt_set_sample_fmt(m_pSwrCtx,	"in_sample_fmt",		AvSrcSampleFmt,     0);

	av_opt_set_int(m_pSwrCtx,			"out_channel_layout",	nDstChannelLayout,  0);
	av_opt_set_int(m_pSwrCtx,			"out_sample_rate",		nDstSampleRate,     0);
	av_opt_set_sample_fmt(m_pSwrCtx,	"out_sample_fmt",		AvDstSampleFmt,     0);

	/* initialize the resampling context */
	llRet = swr_init(m_pSwrCtx);
	if (0 > llRet)
	{
		TraceLog("Failed to initialize the resampling context");
		llRet = -5;
		goto $END;
	}

	llRet = av_samples_alloc_array_and_samples(&ppInData, &nSrcLineSize, nSrcChannels, nSrcSamples, AvSrcSampleFmt, 0);
	if (0 > llRet)
	{
		llRet = -6;
		goto $END;
	}
	memset(ppInData[0], 0, llRet);
	memcpy(ppInData[0], pSrcData, uiSrcDataSize);

	nMaxDstSamples = av_rescale_rnd(nSrcSamples, nDstSampleRate, nSrcSampleRate, AV_ROUND_UP);
	nDstSamples = nMaxDstSamples;
	llRet = av_samples_alloc_array_and_samples(&ppOutData, &nDstLineSize, nDstChannels, nDstSamples, AvDstSampleFmt, 0);
	if (0 > llRet)
	{
		llRet = -7;
		goto $END;
	}

	/* compute destination number of samples */
	nDstSamples = av_rescale_rnd(swr_get_delay(m_pSwrCtx, nSrcSampleRate) + nSrcSamples, nDstSampleRate, nSrcSampleRate, AV_ROUND_UP);
	if (nDstSamples > nMaxDstSamples)
	{
		av_freep(&ppOutData[0]);
		llRet = av_samples_alloc(ppOutData, &nDstLineSize, nDstChannels, nDstSamples, AvDstSampleFmt, 1);
		if (llRet < 0)
		{
			llRet = -8;
			goto $END;
		}

		nMaxDstSamples = nDstSamples;
	}

	/* convert to destination format */
	nConvertRet = swr_convert(m_pSwrCtx, ppOutData, nDstSamples, (const uint8_t **)ppInData, nSrcSamples);
	if (0 > nConvertRet)
	{
		TraceLog("Error while converting");
		llRet = -9;
		goto $END;
	}

	llRet = av_samples_get_buffer_size(&nDstLineSize, nDstChannels, nConvertRet, AvDstSampleFmt, 1);
	if (0 > llRet)
	{
		TraceLog("Could not get sample buffer size");
		llRet = -10;
		goto $END;
	}

	*ppDstData = ppOutData[0];

$END:
	if (0 > llRet)
	{
		if (NULL != ppOutData)
		{
			av_freep(&ppOutData[0]);
		}
		av_freep(&ppOutData);
	}

	if (NULL != ppInData)
	{
		av_freep(&ppInData[0]);
	}
	av_freep(&ppInData);

	swr_close(m_pSwrCtx);
	
	return llRet;
}