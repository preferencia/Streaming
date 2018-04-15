#include "stdafx.h"
#include "Codec.h"
#include "HWAccel.h"

CCodec::CCodec()
{
	m_pFmtCtx					= NULL;
	m_pVideoCtx				= NULL;
	m_pAudioCtx				= NULL;
	m_pHwDeviceCtx		= NULL;
    m_pHwFramesCtx      = NULL;

    //m_pBuffersinkCtx    = NULL;
    //m_pBuffersrcCtx     = NULL;
    //m_pFilterGraph      = NULL;

    m_pVideoFilterCtx   = NULL;
    m_pAudioFilterCtx   = NULL;

	m_pSwsCtx					= NULL;
	m_pSwrCtx					= NULL;

	m_pVideoStream		= NULL;
	m_pAudioStream		= NULL;

	m_nWidth					= 0;
	m_nHeight					= 0;

	m_nCodecType			= OBJECT_TYPE_NONE;

	m_bInitFrame			= false;
	m_bInitPacket			= false;

	m_bNeedCtxCleanUp	= true;

    memset(m_bInitFilters, 0, sizeof(m_bInitFilters));

    memset(m_szVideoFiltersDescr, 0, sizeof(char) * 512);
    memset(m_szAudioFiltersDescr, 0, sizeof(char) * 512);
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

	m_pFmtCtx		= pFmtCtx;
	m_pVideoCtx 	= pVideoCtx;
	m_pAudioCtx 	= pAudioCtx;

	/* allocate image where the decoded image will be put */
	m_nWidth		= m_pVideoCtx->width;
	m_nHeight		= m_pVideoCtx->height;
	m_PixelFmt	    = m_pVideoCtx->pix_fmt;
	m_pHwDeviceCtx = m_pVideoCtx->hw_device_ctx;
	m_pHwFramesCtx = m_pVideoCtx->hw_frames_ctx;

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

//#ifndef _USE_FILTER_GRAPH
//	if (OBJECT_TYPE_DECODER == m_nCodecType)
//	{
//		if (0 > AllocVideoBuffer())
//		{
//			return -3;
//		}
//	}
//#endif

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
			if ((-1 < g_nHwPixFmt) && (AV_PIX_FMT_YUV420P != PixFmt))
			{
				char szHwCodecName[256] = {0, };
				nErr = GetHwCodecName(szHwCodecName, sizeof(szHwCodecName) - 1, VideoCodecID);
				TraceLog("GetHwCodecName result = %d, codec name = %s", nErr, szHwCodecName);
				
				pVideoCodec = avcodec_find_encoder_by_name(szHwCodecName);
			}
			else
			{
				pVideoCodec = avcodec_find_encoder(VideoCodecID);
			}
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
	m_pVideoCtx->pix_fmt			            	= PixFmt;
    m_pVideoCtx->time_base			            = TimeBase;
	m_pVideoCtx->width				            	= nWidth;
	m_pVideoCtx->height				            = nHeight;
	
	/* emit one intra frame every ten frames
	* check frame pict_type before passing frame
	* to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	* then gop_size is ignored and the output of encoder
	* will always be I frame irrespective to gop_size
	*/
	m_pVideoCtx->gop_size			            = nGopSize;
	m_pVideoCtx->bit_rate			            = uiBitrate;
	m_pVideoCtx->bit_rate_tolerance   = uiBitrate;

    m_pVideoCtx->me_method			            = 5;
    m_pVideoCtx->me_cmp			                = 0;
    m_pVideoCtx->me_subpel_quality	    = 8;
    m_pVideoCtx->me_range			            = 0;

    m_pVideoCtx->b_quant_factor	        = 1.25;
	m_pVideoCtx->b_frame_strategy	    = 0;
	m_pVideoCtx->i_quant_factor		    = -0.8;

    m_pVideoCtx->keyint_min			        = 25;
    m_pVideoCtx->refs                           	= 1;

    m_pVideoCtx->qcompress			            = 0.6;
    m_pVideoCtx->qblur                          	= 0.5;
	m_pVideoCtx->qmin				            	= 3;
	m_pVideoCtx->qmax				            	= 35;
	m_pVideoCtx->max_qdiff			            = 4;

    m_pVideoCtx->rc_initial_buffer_occupancy	= 0;
    m_pVideoCtx->coder_type                     				= 0;
	m_pVideoCtx->min_prediction_order				= -1;
	m_pVideoCtx->max_prediction_order				= -1;
	
    m_pVideoCtx->thread_count			        			= 1;
    m_pVideoCtx->thread_type			       					= 3;

    m_pVideoCtx->sub_text_format	            			= 1;

	m_nWidth						            							= m_pVideoCtx->width;
	m_nHeight						            							= m_pVideoCtx->height;
	m_PixelFmt						            						= m_pVideoCtx->pix_fmt;
	
	/**
	* Some container formats (like MP4) require global headers to be present
	* Mark the encoder so that it behaves accordingly.
	*/
	m_pFmtCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        /** Init video hw accel */
	if ( -1 < g_nHwDevType)
	{
		TraceLog("CCodec::InitVideoCtx - Try init video hw accel codec");
        bool bInitHwFramesCtx = false;

        if (OBJECT_TYPE_ENCODER == m_nCodecType)
        {
            if ( (NULL == m_pHwFramesCtx)
#ifdef _WIN32
                && (false == g_bDecodeOnlyHwPixFmt)
#endif
                )
            {
                bInitHwFramesCtx = true;
            }
            else
            {
				m_pVideoCtx->hw_frames_ctx = m_pHwFramesCtx;
			}
        }
        else if (OBJECT_TYPE_DECODER == m_nCodecType)
        {
            m_pVideoCtx->get_format = HWAccel::GetHwFormat;

            if (NULL == m_pHwFramesCtx)
            {
                bInitHwFramesCtx = true;
            }
        }
		
        nErr = HWAccel::InitHwCodec(m_pVideoCtx, (const enum AVHWDeviceType)g_nHwDevType, &m_pHwDeviceCtx, (true == bInitHwFramesCtx) ? &m_pHwFramesCtx : NULL);
		TraceLog("CCodec::InitVideoCtx - Init video hw accel codec result = %d, hw device ctx = 0x%08X, hw frames ctx = 0x%08X", nErr, m_pHwDeviceCtx, m_pHwFramesCtx);
	}

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
			nErr = -8;
			goto $ERROR_END;
		}
	}

	if (false == m_bInitPacket)
	{
		InitPacket();
	}

//#ifndef _USE_FILTER_GRAPH
//	if (OBJECT_TYPE_DECODER == m_nCodecType)
//	{
//		if (0 > AllocVideoBuffer())
//		{
//			nErr = -3;
//			goto $ERROR_END;
//		}
//	}
//#endif

	return 0;

$ERROR_END:
	avcodec_free_context(&m_pVideoCtx);
    if (true == m_bNeedCtxCleanUp)
    {
        avformat_free_context(m_pFmtCtx);
    }	

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
	m_pAudioCtx->sample_fmt 			= SampleFmt;
	m_pAudioCtx->channel_layout 	= nChannelLayout;
	m_pAudioCtx->channels 				= nChannels;
	m_pAudioCtx->sample_rate 		= uiSampleRate;
	m_pAudioCtx->frame_size 			= uiSamples;
	m_pAudioCtx->bit_rate 				= uiBitRate;

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
    if (true == m_bNeedCtxCleanUp)
    {
        avformat_free_context(m_pFmtCtx);
    }	

	m_pAudioCtx = NULL;
	m_pFmtCtx = NULL;

	return nErr;
}

int CCodec::MakeFilterDescr(FILTER_DESCR_DATA* pFilterDescrData)
{
    if (NULL == pFilterDescrData)
    {
        TraceLog("Wrong parameter.");
        return -1;
    }

    memset(m_szVideoFiltersDescr, 0, sizeof(char) * 512);
    memset(m_szAudioFiltersDescr, 0, sizeof(char) * 512);

    if (0 > g_nHwPixFmt)
    {
#ifdef _WIN32
        _snprintf_s(m_szVideoFiltersDescr, sizeof(m_szVideoFiltersDescr) - 1,
#else
		snprintf(m_szVideoFiltersDescr, sizeof(m_szVideoFiltersDescr) - 1,
#endif
            "scale=w=%d:h=%d", pFilterDescrData->uiWidth, pFilterDescrData->uiHeight);
    }
    else
    {
        switch (g_nHwPixFmt) 
		{
		case AV_PIX_FMT_VAAPI:
#ifdef _WIN32
			_snprintf_s(m_szVideoFiltersDescr, sizeof(m_szVideoFiltersDescr) - 1,
#else
			snprintf(m_szVideoFiltersDescr, sizeof(m_szVideoFiltersDescr) - 1,
#endif
                        "scale_vaapi=w=%d:h=%d", pFilterDescrData->uiWidth, pFilterDescrData->uiHeight);
			break;
		
		case AV_PIX_FMT_DXVA2_VLD:
        case AV_PIX_FMT_D3D11:
        case AV_PIX_FMT_CUDA:
#ifdef _WIN32
			_snprintf_s(m_szVideoFiltersDescr, sizeof(m_szVideoFiltersDescr) - 1,
#else
			snprintf(m_szVideoFiltersDescr, sizeof(m_szVideoFiltersDescr) - 1,
#endif
                        "scale=w=%d:h=%d", pFilterDescrData->uiWidth, pFilterDescrData->uiHeight);
			break;
		
		case AV_PIX_FMT_VDPAU:
			break;
		
		case AV_PIX_FMT_VIDEOTOOLBOX:
			break;
		
		default:
			break;
		}
    }

    if (0 >= strlen(m_szVideoFiltersDescr))
    {
        TraceLog("Failed to make filter decsr");
        return -2;
    }

#ifdef _WIN32
    _snprintf_s(m_szAudioFiltersDescr, sizeof(m_szAudioFiltersDescr) - 1,
#else
	snprintf(m_szAudioFiltersDescr, sizeof(m_szAudioFiltersDescr) - 1,
#endif
            "anull");

    return 0;
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
    if (NULL != m_pAudioFilterCtx)
    {
        avfilter_graph_free(&m_pAudioFilterCtx->pFilterGraph);
    }
    SAFE_DELETE(m_pAudioFilterCtx);

    if (NULL != m_pVideoFilterCtx)
    {
        avfilter_graph_free(&m_pVideoFilterCtx->pFilterGraph);
    }
    SAFE_DELETE(m_pVideoFilterCtx);

    //avfilter_graph_free(&m_pFilterGraph);

    avcodec_close(m_pVideoCtx);
    avcodec_close(m_pAudioCtx);
    av_buffer_unref(&m_pHwDeviceCtx);
    av_buffer_unref(&m_pHwFramesCtx);

	//avcodec_free_context(&m_pVideoCtx);
	//avcodec_free_context(&m_pAudioCtx);

    avformat_close_input(&m_pFmtCtx);
}

void CCodec::InitPacket()
{
	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&m_Pkt);
	m_Pkt.data			= NULL;
	m_Pkt.size			= 0;
	m_bInitPacket	= true;
}

int CCodec::InitVideoFilters(const char* pszFilterDescr, AVFrame* pFrame)
{
    if ((NULL == m_pFmtCtx) || (NULL == pszFilterDescr) || (0 >= strlen(pszFilterDescr)) || (NULL == pFrame))
    {
        return -1;
    }

    char                    szArgs[512]     = {0, };
    int                     nRet            = 0;
    int                     nIndex          = 0;
    AVFilter*               pBufferSrc      = avfilter_get_by_name("buffer");
    AVFilter*               pBufferSink     = avfilter_get_by_name("buffersink");
    AVFilterInOut*          pOutputs        = NULL;
    AVFilterInOut*          pInputs         = NULL;
    AVFilterInOut*          pCur            = NULL;
    AVRational              TimeBase        = m_pFmtCtx->streams[AVMEDIA_TYPE_VIDEO]->time_base;
    enum AVPixelFormat      TargetPixFmt    = AV_PIX_FMT_NONE;
    enum AVPixelFormat      PixFmts[]       = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NV12, AV_PIX_FMT_VAAPI, AV_PIX_FMT_DXVA2_VLD, AV_PIX_FMT_CUDA, AV_PIX_FMT_NONE };

    if ((AV_PIX_FMT_DXVA2_VLD == g_nHwPixFmt) || (AV_PIX_FMT_D3D11 == g_nHwPixFmt))
    {
        TargetPixFmt = AV_PIX_FMT_NV12;
        pFrame->format = AV_PIX_FMT_NV12;
    }
    else
    {
        TargetPixFmt = (AVPixelFormat)pFrame->format;
    }

    if (NULL == m_pVideoFilterCtx)
    {
        m_pVideoFilterCtx = new FilterContext;
        memset(m_pVideoFilterCtx, 0, sizeof(FilterContext));
    }
 
    m_pVideoFilterCtx->pFilterGraph = avfilter_graph_alloc();
    if (NULL == m_pVideoFilterCtx->pFilterGraph) 
    {
		TraceLog("avfilter_graph_alloc error [%d]", nRet);
        nRet = AVERROR(ENOMEM);
        goto End;
    }
    
    TraceLog("Filter descr = %s", pszFilterDescr);
    
    //m_pVideoFilterCtx->pFilterGraph ->scale_sws_opts = av_strdup("flags=bicubic");
    //TraceLog("scale descr = %s", m_pVideoFilterCtx->pFilterGraph ->scale_sws_opts );
    
    nRet = avfilter_graph_parse2(m_pVideoFilterCtx->pFilterGraph, pszFilterDescr, &pInputs, &pOutputs);
    if (!pOutputs || !pInputs) 
    {
		TraceLog("avfilter_graph_parse2 error [%d]", nRet);
        nRet = AVERROR(ENOMEM);
        goto End;
    }
    
    TraceLog("Hw device ctx = 0x%08X", m_pHwDeviceCtx);
    if (NULL != m_pHwDeviceCtx)
    {
		for (nIndex = 0; nIndex < m_pVideoFilterCtx->pFilterGraph->nb_filters; ++nIndex)
		{
			m_pVideoFilterCtx->pFilterGraph->filters[nIndex]->hw_device_ctx = av_buffer_ref(m_pHwDeviceCtx);
			if (NULL == m_pVideoFilterCtx->pFilterGraph->filters[nIndex]->hw_device_ctx)
			{
				TraceLog("av_buffer_ref(g_pHwDeviceCtx) is NULL");
				nRet = AVERROR(ENOMEM);
				goto End;
			}
		}
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
#ifdef _WIN32
    _snprintf_s(szArgs, sizeof(szArgs) - 1,
#else
	snprintf(szArgs, sizeof(szArgs) - 1,
#endif
                "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:sws_param=flags=%d:frame_rate=%d/%d",
                m_nWidth, m_nHeight, TargetPixFmt,
                TimeBase.num, TimeBase.den, m_pVideoCtx->sample_aspect_ratio.num, m_pVideoCtx->sample_aspect_ratio.den,
                SWS_BILINEAR + ((m_pVideoCtx->flags & AV_CODEC_FLAG_BITEXACT) ? SWS_BITEXACT : 0),
                m_pVideoCtx->framerate.num, m_pVideoCtx->framerate.den);
                
	TraceLog("args = %s", szArgs);

    nIndex = 0;

    for (pCur = pInputs, nIndex = 0; pCur; pCur = pCur->next, ++nIndex)
    {
        AVBufferSrcParameters*  pParam  = av_buffersrc_parameters_alloc();
        memset(pParam, 0, sizeof(*pParam));
        pParam->format = AV_PIX_FMT_NONE;

        char szSrcFilterName[MAX_PATH]   = {0, };
#ifdef _WIN32
        _snprintf_s(szSrcFilterName, sizeof(szSrcFilterName) - 1, 
#else
		snprintf(szSrcFilterName, sizeof(szSrcFilterName) - 1,
#endif
                    "graph %d input from stream %d:%d", AVMEDIA_TYPE_VIDEO, nIndex, AVMEDIA_TYPE_VIDEO);

        nRet = avfilter_graph_create_filter(&m_pVideoFilterCtx->pBufferSrcCtx, pBufferSrc,
                                            szSrcFilterName, szArgs, NULL, m_pVideoFilterCtx->pFilterGraph);
        if (0 > nRet)
        {
			TraceLog("avfilter_graph_create_filter (Video) error [%d]", nRet);
            av_freep(&pParam);
            goto End;
        }
        
        if (NULL != pFrame->hw_frames_ctx)
        {
            pParam->hw_frames_ctx = av_buffer_ref(pFrame->hw_frames_ctx);
        }

        //if (NULL != m_pHwFramesCtx)
        //{
            //pParam->hw_frames_ctx = av_buffer_ref(m_pHwFramesCtx);
        //}
        
        TraceLog("pFrame->hw_frames_ctx = 0x%08X, pParam->hw_frames_ctx = 0x%08X", pFrame->hw_frames_ctx, pParam->hw_frames_ctx);
        
        nRet = av_buffersrc_parameters_set(m_pVideoFilterCtx->pBufferSrcCtx, pParam); 

        av_freep(&pParam);

        if (0 > nRet)
        {
			TraceLog("av_buffersrc_parameters_set (Video) error [%d]", nRet);
            goto End;
        }
        
        nRet = avfilter_link(m_pVideoFilterCtx->pBufferSrcCtx, 0, pCur->filter_ctx, pCur->pad_idx);
        if (0 > nRet)
        {
			TraceLog("avfilter_link (Video) error [%d]", nRet);
            goto End;
        }
    }
	
	for (pCur = pOutputs, nIndex = 0; pCur; pCur = pCur->next, ++nIndex)
    {
        char szSinkFilterName[MAX_PATH]   = {0, };
#ifdef _WIN32
        _snprintf_s(szSinkFilterName, sizeof(szSinkFilterName) - 1,
#else
		snprintf(szSinkFilterName, sizeof(szSinkFilterName) - 1,
#endif
                    "out_%d_%d", nIndex, AVMEDIA_TYPE_VIDEO);

        nRet = avfilter_graph_create_filter(&m_pVideoFilterCtx->pBufferSinkCtx, pBufferSink,
                                            szSinkFilterName, NULL, NULL, m_pVideoFilterCtx->pFilterGraph);
        if (0 > nRet)
        {
			TraceLog("avfilter_graph_create_filter (Video) error [%d]", nRet);
            goto End;
        }

        // format filter
        AVFilterContext* pFmtFilter = NULL;
        char* pszFmtName = (char*)av_get_pix_fmt_name(TargetPixFmt);
        TraceLog("pszFmtName = %s", pszFmtName);
        nRet = avfilter_graph_create_filter(&pFmtFilter, avfilter_get_by_name("format"),
                                            "format", pszFmtName, NULL, m_pVideoFilterCtx->pFilterGraph);
        if (0 > nRet)
        {
			TraceLog("avfilter_graph_create_filter (Video format) error [%d]", nRet);
            goto End;
        }

        nRet = avfilter_link(pCur->filter_ctx, pCur->pad_idx, pFmtFilter, 0);
        if (0 > nRet)
        {
			TraceLog("avfilter_link (Video : pOutputs->filter_ctx) error [%d]", nRet);
            goto End;
        }

        nRet = avfilter_link(pFmtFilter, 0, m_pVideoFilterCtx->pBufferSinkCtx, 0);
        if (0 > nRet)
        {
			fprintf(stderr, "avfilter_link (Video format) error [%d]\n", nRet);
            goto End;
        }
        
  //      nRet = av_opt_set_int_list(m_pVideoFilterCtx->pBufferSinkCtx, "pix_fmts", PixFmts,
  //                                  TargetPixFmt, AV_OPT_SEARCH_CHILDREN);
		//if (0 > nRet) 
		//{
		//	TraceLog("Cannot set output pixel format");
		//	goto End;
		//}    
    }
    
    nRet = avfilter_graph_config(m_pVideoFilterCtx->pFilterGraph, NULL);
    if (0 > nRet)
    {
		TraceLog("avfilter_graph_config (Video) error [%d]", nRet);
        goto End;
    }

    m_bInitFilters[AVMEDIA_TYPE_VIDEO] = true;

End:
	TraceLog("Return value = %d", nRet);
    avfilter_inout_free(&pInputs);
    avfilter_inout_free(&pOutputs);

    return nRet;
}

int CCodec::InitAudioFilters(const char* pszFilterDescr, AVFrame* pFrame)
{
    if ((NULL == m_pFmtCtx) || (NULL == pszFilterDescr) || (0 >= strlen(pszFilterDescr)) || (NULL == pFrame))
    {
        return -1;
    }

    char                    szArgs[512]     = {0, };
    int                     nRet            = 0;
    int                     nIndex          = 0;
    AVFilter*               pABufferSrc     = avfilter_get_by_name("abuffer");
    AVFilter*               pABufferSink    = avfilter_get_by_name("abuffersink");
    AVFilterInOut*          pOutputs        = NULL;
    AVFilterInOut*          pInputs         = NULL;
    AVFilterInOut*          pCur            = NULL;
    AVRational              TimeBase        = m_pFmtCtx->streams[AVMEDIA_TYPE_AUDIO]->time_base;

    if (NULL == m_pAudioFilterCtx)
    {
        m_pAudioFilterCtx = new FilterContext;
        memset(m_pAudioFilterCtx, 0, sizeof(FilterContext));
    }
 
    m_pAudioFilterCtx->pFilterGraph = avfilter_graph_alloc();
    if (NULL == m_pAudioFilterCtx->pFilterGraph) 
    {
		TraceLog("avfilter_graph_alloc error [%d]", nRet);
        nRet = AVERROR(ENOMEM);
        goto End;
    }
    
    TraceLog("Filter descr = %s", pszFilterDescr);
    
    //m_pVideoFilterCtx->pFilterGraph ->scale_sws_opts = av_strdup("flags=bicubic");
    //TraceLog("scale descr = %s", m_pVideoFilterCtx->pFilterGraph ->scale_sws_opts );
    
    nRet = avfilter_graph_parse2(m_pAudioFilterCtx->pFilterGraph, pszFilterDescr, &pInputs, &pOutputs);
    if (!pOutputs || !pInputs) 
    {
		TraceLog("avfilter_graph_parse2 error [%d]", nRet);
        nRet = AVERROR(ENOMEM);
        goto End;
    }
    
#ifdef _WIN32
    _snprintf_s(szArgs, sizeof(szArgs) - 1,
#else
	snprintf(szArgs, sizeof(szArgs) - 1,
#endif
        "time_base=%d/%d:sample_rate=%d:sample_fmt=%s", 
        TimeBase.num, TimeBase.den, pFrame->sample_rate,
        av_get_sample_fmt_name((AVSampleFormat)pFrame->format));

    if (0 < pFrame->channel_layout)
    {
        char szChannelLayoutArg[64] = {0, };
#ifdef _WIN32
        _snprintf_s(szChannelLayoutArg, sizeof(szChannelLayoutArg) - 1,
#else
	    snprintf(szChannelLayoutArg, sizeof(szChannelLayoutArg) - 1,
#endif
            ":channel_layout=0x%"PRIx64, pFrame->channel_layout);
        strcat(szArgs, szChannelLayoutArg);
    }
    else
    {
        char szChannelsArg[64] = {0, };
#ifdef _WIN32
        _snprintf_s(szChannelsArg, sizeof(szChannelsArg) - 1,
#else
	    snprintf(szChannelsArg, sizeof(szChannelsArg) - 1,
#endif
            ":channels=%d", pFrame->channels);
        strcat(szArgs, szChannelsArg);
    }

    TraceLog("args = %s", szArgs);

    for (pCur = pInputs, nIndex = 0; pCur; pCur = pCur->next, ++nIndex)
    {
        char szSrcFilterName[MAX_PATH]   = {0, };
#ifdef _WIN32
        _snprintf_s(szSrcFilterName, sizeof(szSrcFilterName) - 1, 
#else
		snprintf(szSrcFilterName, sizeof(szSrcFilterName) - 1,
#endif
                    "graph_%d_in_%d_%d", AVMEDIA_TYPE_AUDIO, nIndex, AVMEDIA_TYPE_AUDIO);

        nRet = avfilter_graph_create_filter(&m_pAudioFilterCtx->pBufferSrcCtx, pABufferSrc,
                                            szSrcFilterName, szArgs, NULL, m_pAudioFilterCtx->pFilterGraph);
        if (0 > nRet)
        {
			TraceLog("avfilter_graph_create_filter (Audio) error [%d]", nRet);
            goto End;
        }

        nRet = avfilter_link(m_pAudioFilterCtx->pBufferSrcCtx, 0, pCur->filter_ctx, pCur->pad_idx);
        if (0 > nRet)
        {
			TraceLog("avfilter_link (Audio) error [%d]", nRet);
            goto End;
        }
    }

    for (pCur = pOutputs, nIndex = 0; pCur; pCur = pCur->next, ++nIndex)
    {
        char szFilterName[MAX_PATH] = {0, };
        char szFilterArgs[MAX_PATH] = {0, };    

#ifdef _WIN32
        _snprintf_s(szFilterName, sizeof(szFilterName) - 1,
#else
		snprintf(szFilterName, sizeof(szFilterName) - 1,
#endif
                    "out_%d_%d", nIndex, AVMEDIA_TYPE_AUDIO);

        nRet = avfilter_graph_create_filter(&m_pAudioFilterCtx->pBufferSinkCtx, pABufferSink,
                                            szFilterName, NULL, NULL, m_pAudioFilterCtx->pFilterGraph);
        if (0 > nRet)
        {
			TraceLog("avfilter_graph_create_filter (Audio) error [%d]", nRet);
            goto End;
        }

        nRet = av_opt_set_int(m_pAudioFilterCtx->pBufferSinkCtx, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN);
        if (0 > nRet)
        {
			TraceLog("avfilter_graph_create_filter (Audio) error [%d]", nRet);
            goto End;
        }

        // format filter
        AVFilterContext* pFmtFilter = NULL;
#ifdef _WIN32
        _snprintf_s(szFilterName, sizeof(szFilterName) - 1,
#else
		snprintf(szFilterName, sizeof(szFilterName) - 1,
#endif
                    "format_out_%d_%d", nIndex, AVMEDIA_TYPE_AUDIO);

#ifdef _WIN32
        _snprintf_s(szFilterArgs, sizeof(szFilterArgs) - 1,
#else
		snprintf(szFilterArgs, sizeof(szFilterArgs) - 1,
#endif
            "sample_fmts=%s:sample_rates=%d:", av_get_sample_fmt_name((AVSampleFormat)pFrame->format), pFrame->sample_rate);

        TraceLog("Filter name = %s, args = %s", szFilterName, szFilterArgs);
        nRet = avfilter_graph_create_filter(&pFmtFilter, avfilter_get_by_name("aformat"),
                                            szFilterName, szFilterArgs, NULL, m_pAudioFilterCtx->pFilterGraph);
        if (0 > nRet)
        {
			TraceLog("avfilter_graph_create_filter (Audio format) error [%d]", nRet);
            goto End;
        }

        nRet = avfilter_link(pCur->filter_ctx, pCur->pad_idx, pFmtFilter, 0);
        if (0 > nRet)
        {
			TraceLog("avfilter_link (Audio : pOutputs->filter_ctx) error [%d]", nRet);
            goto End;
        }

        nRet = avfilter_link(pFmtFilter, 0, m_pAudioFilterCtx->pBufferSinkCtx, 0);
        if (0 > nRet)
        {
			fprintf(stderr, "avfilter_link (Audio format) error [%d]\n", nRet);
            goto End;
        }
    }

    nRet = avfilter_graph_config(m_pAudioFilterCtx->pFilterGraph, NULL);
    if (0 > nRet)
    {
		TraceLog("avfilter_graph_config (Audio) error [%d]", nRet);
        goto End;
    }

    m_bInitFilters[AVMEDIA_TYPE_AUDIO] = true;

End:
	TraceLog("Return value = %d", nRet);
    avfilter_inout_free(&pInputs);
    avfilter_inout_free(&pOutputs);

    return nRet;
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
		return -4;
	}

	llRet = av_image_alloc(ppInData, SrcLineSize, nSrcWidth, nSrcHeight, AvSrcPixFmt, 1);
	if (0 > llRet)
	{
		return -5;
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

	int64_t			llRet 						= 0;
	int					nConvertRet 			= 0;
	int             		nMaxDstSamples	= 0;
	int             		nSrcLineSize 		= 0;
	int             		nDstLineSize 		= 0;
	uint8_t**		ppInData 				= NULL;
	uint8_t**		ppOutData 				= NULL;

	if (NULL == m_pSwrCtx)
	{
		m_pSwrCtx = swr_alloc();
	}

	if (NULL == m_pSwrCtx)
	{
		return -4;
	}

	/* set options */
	av_opt_set_int(m_pSwrCtx,				"in_channel_layout",		nSrcChannelLayout,  	0);
	av_opt_set_int(m_pSwrCtx,				"in_sample_rate",			nSrcSampleRate,     		0);
	av_opt_set_sample_fmt(m_pSwrCtx,	"in_sample_fmt",				AvSrcSampleFmt,     		0);

	av_opt_set_int(m_pSwrCtx,				"out_channel_layout",	nDstChannelLayout,  	0);
	av_opt_set_int(m_pSwrCtx,				"out_sample_rate",			nDstSampleRate,     		0);
	av_opt_set_sample_fmt(m_pSwrCtx,	"out_sample_fmt",			AvDstSampleFmt,     		0);

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

int CCodec::FrameFilteringProc(int nStreamType, AVFrame* pFrame, AVFrame* pFilterFrame)
{
    if ((0 > nStreamType) || (NULL == pFrame) || (NULL == pFilterFrame))
    {
        return -1;
    }

    int             nRet            = 0;
    FilterContext*  pFilterCtx      = (AVMEDIA_TYPE_VIDEO == nStreamType) ? m_pVideoFilterCtx 
                                    : ( (AVMEDIA_TYPE_AUDIO == nStreamType) ? m_pAudioFilterCtx : NULL );

    if (NULL == pFilterCtx)
    {
        return -2;
    }

    /* push the decoded frame into the filtergraph */
    nRet = av_buffersrc_add_frame_flags(pFilterCtx->pBufferSrcCtx, pFrame, AV_BUFFERSRC_FLAG_KEEP_REF/*AV_BUFFERSRC_FLAG_PUSH*/);
    if (0 > nRet)
    {
        TraceLog("Error while feeding the filtergraph");
        goto END;
    }

    /* pull filtered frames from the filtergraph */
    do
    {
        nRet = av_buffersink_get_frame(pFilterCtx->pBufferSinkCtx, pFilterFrame);
        if ( (AVERROR(EAGAIN) == nRet) || ( AVERROR_EOF == nRet) ) 
        {
            nRet = 0;
            break;
        }
        else if (0 > nRet)
        {
            TraceLog("Error while pull a filtered frames from the filtergraph");
            goto END;
        }
    } while (true);

    //pFilterFrame->format = av_buffersink_get_format(pFilterCtx->pBufferSinkCtx);

    //pFilterFrame->width = av_buffersink_get_w(pFilterCtx->pBufferSinkCtx);
    //pFilterFrame->height = av_buffersink_get_h(pFilterCtx->pBufferSinkCtx);
    //pFilterFrame->sample_aspect_ratio = av_buffersink_get_sample_aspect_ratio(pFilterCtx->pBufferSinkCtx);
    //
    //pFilterFrame->sample_rate = av_buffersink_get_sample_rate(pFilterCtx->pBufferSinkCtx);
    //pFilterFrame->channels = av_buffersink_get_channels(pFilterCtx->pBufferSinkCtx);
    //pFilterFrame->channel_layout = av_buffersink_get_channel_layout(pFilterCtx->pBufferSinkCtx);
END:
    return nRet;
}

int CCodec::GetHwCodecName(char* pszHwCodecName, int nLen,  AVCodecID CodecID)
{
	if ((NULL == pszHwCodecName) || (0 >= nLen))
	{
		TraceLog("Input parameters are wrong. [0x%08X][%d]", pszHwCodecName, nLen);
		return -1;
	}
	
	if (AV_CODEC_ID_H264 !=  CodecID)
	{
		TraceLog("Input codec id [%d] is now h264 codec", CodecID);
		return -2;
	}
	
	switch (g_nHwPixFmt)
	{
	case AV_PIX_FMT_NV12:
		strncpy(pszHwCodecName, "h264", (nLen < strlen("h264")) ? nLen : strlen("h264"));
		break;
		
	case AV_PIX_FMT_VAAPI:
		strncpy(pszHwCodecName, "h264_vaapi", (nLen < strlen("h264_vaapi")) ? nLen : strlen("h264_vaapi"));
		break;
		
	default:
		break;
	}
	
	return 0;
}
