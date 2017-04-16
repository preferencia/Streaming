#include "stdafx.h"
#include "StreamSource.h"

#ifdef _WIN32
#pragma warning(disable:4996)
#endif


CStreamSource::CStreamSource()
{
	memset(m_szSrcFileName,	 0, MAX_PATH * sizeof(char));
	memset(m_szTrscFileName, 0, MAX_PATH * sizeof(char));
	m_pszInputFileName		= NULL;
	
	m_pObjectManager		= NULL;
	m_pDecoder				= NULL;
	m_pEncoder				= NULL;
    m_pDemuxer				= NULL;
	m_pMuxer				= NULL;

	m_pInFmtCtx				= NULL;
    m_pDecCodecCtx          = NULL;
    m_pDecStream            = NULL;

    m_pOutFmtCtx			= NULL;
    m_pTrscCodecCtx         = NULL;
    m_pTrscStream           = NULL;

    m_nDecCodecCtxSize      = 0;
    m_nDecStreamSize        = 0;

    m_nTrscCodecCtxSize     = 0;
    m_nTrscStreamSize       = 0;

#ifdef _USE_FILTER_GRAPH
	m_pFilterCtx			= NULL;
#endif

	m_pParent				= NULL;
	m_pStreamCallbackFunc	= NULL;

	m_nRefCount				= 0;

	m_nVideoStreamIndex		= ~0;
	m_nAudioStreamIndex		= ~0;

	m_nEncVideoWidth		= 0;
	m_nEncVideoHeight		= 0;

	m_llEncVideoBitRate		= 0LL;
	m_llEncAudioBitRate		= 0LL;

	m_llVideoFrameNum		= 0LL;
	m_llAudioFrameNum		= 0LL;

    m_bResetEncoder         = false;
	m_bCreateTrscVideoFile	= false;

#ifdef _WIN32
    m_hSteramCallbackMutex	= NULL;
	m_hDecodeThreadMutex	= NULL;
	m_hDecodeThread			= NULL;
#endif

	m_bRunDecodeThread		= false;
    m_bRunEncodeThread		= false;
	m_bPauseDecodeThread	= false;
}


CStreamSource::~CStreamSource()
{
	ObjectCleanUp();

	SAFE_DELETE_ARRAY(m_pszInputFileName);

#ifdef _USE_FILTER_GRAPH
	for (int nIndex = 0; nIndex < m_pInFmtCtx->nb_streams; ++nIndex)
	{
		avcodec_close(m_pInFmtCtx->streams[nIndex]->codec);
		if ( (NULL != m_pOutFmtCtx) 
			&& (m_pOutFmtCtx->nb_streams > nIndex)
			&& (NULL != m_pOutFmtCtx->streams[nIndex]) 
			&& (NULL != m_pOutFmtCtx->streams[nIndex]->codec) )
		{
			avcodec_close(m_pOutFmtCtx->streams[nIndex]->codec);
		}

		if ((NULL != m_pFilterCtx) && (NULL != m_pFilterCtx[nIndex].pFilterGraph))
			avfilter_graph_free(&m_pFilterCtx[nIndex].pFilterGraph);
	}

	av_free(m_pFilterCtx);
#endif

    CtxAndStreamCleanUp(0);    

    if (true == m_bCreateTrscVideoFile)
    {
        CtxAndStreamCleanUp(1);
    }    

#ifdef _WIN32
	if (NULL != m_hSteramCallbackMutex)
	{
		CloseHandle(m_hSteramCallbackMutex);
		m_hSteramCallbackMutex = NULL;
	}

    if (NULL != m_hDecodeThreadMutex)
	{
		CloseHandle(m_hDecodeThreadMutex);
		m_hDecodeThreadMutex = NULL;
	}
#else
	pthread_mutex_destroy(&m_hSteramCallbackMutex);
    pthread_mutex_destroy(&m_hDecodeThreadMutex);
#endif
}


int CStreamSource::Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszSrcFileName, bool bCreateTrscVideoFile /* = false */)
{
	if (NULL == pParent)
	{
		TraceLog("Parent Object is NULL.");
		return -1;
	}

	if (NULL == pStreamCallbackFunc)
	{
		TraceLog("Callback Function is NULL");
		return -2;
	}

	if ((NULL == pszSrcFileName) || (0 == strlen(pszSrcFileName)))
	{
		TraceLog("File Name is NULL");
		return -3;
	}

	m_pParent					= pParent;
	m_pStreamCallbackFunc		= pStreamCallbackFunc;

	if (NULL != pszSrcFileName)
	{
		int nSrcFileNameLen		= strlen(pszSrcFileName) + 1;
		m_pszInputFileName		= new char[nSrcFileNameLen];
		memset(m_pszInputFileName, 0, nSrcFileNameLen * sizeof(char));
		strcpy(m_pszInputFileName, pszSrcFileName);

		snprintf(m_szSrcFileName,	MAX_PATH - 1, "./Video/%s", m_pszInputFileName);
		snprintf(m_szTrscFileName,	MAX_PATH - 1, m_pszInputFileName);

		m_bCreateTrscVideoFile	= bCreateTrscVideoFile;
	}

#ifdef _WIN32
	m_hSteramCallbackMutex		= CreateMutex(NULL, FALSE, NULL);
    m_hDecodeThreadMutex		= CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&m_hSteramCallbackMutex, NULL);
    pthread_mutex_init(&m_hDecodeThreadMutex, NULL);
#endif

    return 0;
}

int CStreamSource::Open()
{
    if (NULL == m_pObjectManager)
	{
		m_pObjectManager = new CObjectManager;
	}
	
	if (NULL == m_pObjectManager)
	{
		return -1;
	}

    if (NULL != m_pDemuxer)
    {
        m_pObjectManager->DestroyObject((void**)&m_pDemuxer, OBJECT_TYPE_DEMUXER);
    }
    
    // Set Demuxer
	if (0 > m_pObjectManager->CreateObject((void**)&m_pDemuxer, OBJECT_TYPE_DEMUXER))
	{
		return -2;
	}

    if (NULL == m_pDemuxer)
    {
        return -3;
    }

	float	fFps            = 0.0f;
	UINT	uiFps           = 0;
    int     nStreamIndex    = -1;

	if (0 < strlen(m_szSrcFileName))
	{
        CtxAndStreamCleanUp(0);

		if (0 > m_pDemuxer->FileOpenProc(m_szSrcFileName, &m_pInFmtCtx))
		{
			return -4;
		}

        m_nDecCodecCtxSize  = m_pInFmtCtx->nb_streams;
        m_nDecStreamSize    = m_pInFmtCtx->nb_streams;

        m_pDecCodecCtx      = (AVCodecContext**)av_malloc_array(m_pInFmtCtx->nb_streams, sizeof(*m_pDecCodecCtx));
        m_pDecStream        = (AVStream**)av_malloc_array(m_pInFmtCtx->nb_streams, sizeof(*m_pDecStream));

        for (int nIndex = 0; nIndex < m_nDecCodecCtxSize; ++nIndex)
        {
            m_pDecCodecCtx[nIndex]  = NULL;
            m_pDecStream[nIndex]    = NULL;

            if (0 > m_pDemuxer->OpenCodecContext(&nStreamIndex, &m_pDecCodecCtx[nIndex], m_pInFmtCtx, NULL, (AVMediaType)(AVMEDIA_TYPE_VIDEO + nIndex)))
		    {
			    return -5;
		    }

            if (nStreamIndex != (int)(AVMEDIA_TYPE_VIDEO + nIndex))
            {
                return -6;
            }

		    m_pDecStream[nIndex] = m_pInFmtCtx->streams[nStreamIndex];		
        }
	}

    if ( (NULL == m_pInFmtCtx) 
        || (NULL == m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]) 
        || (NULL == m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]) )
	{
		return -7;
	}

	// Set Video and Audio Bitrate for Encoder
	m_llEncVideoBitRate = (_DEC_VIDEO_ENC_BITRATE < m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->bit_rate) ? _DEC_VIDEO_ENC_BITRATE : m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->bit_rate;
	m_llEncAudioBitRate = (_DEC_AUDIO_ENC_BITRATE < m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->bit_rate) ? _DEC_AUDIO_ENC_BITRATE : m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->bit_rate;

	// Calc fps
	fFps				= (float)m_pDecStream[AVMEDIA_TYPE_VIDEO]->r_frame_rate.num / (float)m_pDecStream[AVMEDIA_TYPE_VIDEO]->r_frame_rate.den;
	uiFps				= ((UINT)fFps >= fFps) ? (UINT)fFps : (UINT)fFps + 1;

	ProcFileOpen(m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->pix_fmt, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->width, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->height, uiFps,
				 m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->sample_fmt, m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->channel_layout, m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->channels, 
                 m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->sample_rate, m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->frame_size);

	return 0;
}


void CStreamSource::SetResolution(int nWidth, int nHeight, int nResetResolution)
{
	m_nEncVideoWidth	= nWidth;
	m_nEncVideoHeight	= nHeight;

    if ((1 == nResetResolution) && (NULL != m_pObjectManager) && (NULL != m_pEncoder))
    {
        m_bResetEncoder = true;
    }
}


int CStreamSource::Start()
{
    if (NULL == m_pObjectManager)
    {
        return -1;
    }

    if (NULL != m_pDecoder)
    {
        m_pObjectManager->DestroyObject((void**)&m_pDecoder, OBJECT_TYPE_DECODER);
    }

	// Decoder Run
	if (0 > m_pObjectManager->CreateObject((void**)&m_pDecoder, OBJECT_TYPE_DECODER))
	{
		return -2;
	}

	if (NULL == m_pDecoder)
	{
		return -3;
	}

	if (0 > m_pDecoder->InitFromContext(m_pInFmtCtx, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO], m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]))
	{
		return -4;
	}

	//m_pDecoder->SetStreamIndex(m_nVideoStreamIndex, m_nAudioStreamIndex);
    m_pDecoder->SetStreamIndex(AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO);
	m_pDecoder->SetCallbackProc(this, DataProcCallback);

    // Start video file decode thread
#ifdef _WIN32
	m_hDecodeThreadMutex = CreateMutex(NULL, FALSE, NULL);

	m_hDecodeThread = (HANDLE)_beginthreadex(NULL, 0, DecodeThread, this, 0, NULL);
	if (NULL != m_hDecodeThread)
	{
		m_bRunDecodeThread = true;
	}
#else
	pthread_mutex_init(&m_hDecodeThreadMutex, NULL);

	int nRet = pthread_create(&m_hDecodeThread, NULL, DecodeThread, this);
	if (0 == nRet)
	{
		m_bRunDecodeThread = true;
	}

	pthread_detach(m_hDecodeThread);
#endif

	return 0;
}


bool CStreamSource::Pause()
{
    if (NULL != m_hDecodeThread)
    {
        m_bPauseDecodeThread = !m_bPauseDecodeThread;
        return m_bPauseDecodeThread;
    }

	return false;
}


void CStreamSource::Stop()
{
	m_bRunDecodeThread = false;
    m_bRunEncodeThread = false;

#ifdef _WIN32
	if (NULL != m_hDecodeThread)
	{
        WaitForSingleObject(m_hDecodeThread, INFINITE);
	}

	if (NULL != m_hDecodeThreadMutex)
	{
		CloseHandle(m_hDecodeThreadMutex);
		m_hDecodeThreadMutex = NULL;
	}
#else
	pthread_mutex_destroy(&m_hDecodeThreadMutex);
#endif

	m_pParent				= NULL;
	m_pStreamCallbackFunc	= NULL;
}


#ifdef _USE_FILTER_GRAPH
int CStreamSource::DataProcCallback(void* pObject, int nMediaType, AVFrame* pFrame)
#else
int CStreamSource::DataProcCallback(void* pObject, int nProcDataType, int nPictType, unsigned int uiDataSize, unsigned char* pData)
#endif
{
	CStreamSource* pThis = (CStreamSource*)pObject;
	if (NULL == pThis)
	{
		return -1;
	}

	if ((true == pThis->m_bCreateTrscVideoFile) && (NULL == pThis->m_pMuxer))
	{
		pThis->SetMuxer();
	}

	if (NULL == pThis->m_pEncoder)
	{
		pThis->SetEncoder();
	}

#ifdef _USE_FILTER_GRAPH
    if (NULL == pThis->m_pFilterCtx)
    {
		pThis->InitFilters();
    }

	if (NULL == pFrame)
	{
		return -2;
	}

	if (NULL != pThis->m_pFilterCtx[nMediaType].pBufferSrcCtx)
	{
		/* push the decoded frame into the filtergraph */
		int nRet = av_buffersrc_add_frame_flags(pThis->m_pFilterCtx[nMediaType].pBufferSrcCtx, pFrame, 0);

		if (0 > nRet)
		{
			TraceLog("Error while feeding the filtergraph");
			return nRet;
		}

		nRet = pThis->EncodeFilterFrame(nMediaType);
	}
#else
	if ((0 >= uiDataSize) || (NULL == pData))
	{
		return -2;
	}

	switch (nProcDataType)
	{
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_AUDIO:
		{
			pThis->EncodeProc(nProcDataType, nPictType, uiDataSize, pData);
		}
		break;

	default:
		{
			if (NULL != pThis->m_pMuxer)
			{
				/* remux this frame without reencoding */
				av_packet_rescale_ts(pThis->m_pDecoder->GetCodecPkt(),
					pThis->m_pInFmtCtx->streams[nProcDataType]->time_base,
					pThis->m_pOutFmtCtx->streams[nProcDataType]->time_base);

				pThis->m_pMuxer->WriteEncFrame(pThis->m_pOutFmtCtx, pThis->m_pDecoder->GetCodecPkt());
			}
		
		}
		break;
	}
#endif // _USE_FILTER_GRAPH

    if (true == pThis->m_bResetEncoder)
    {
		pThis->Pause();

		if (true == pThis->m_bCreateTrscVideoFile)
		{
			pThis->SetMuxer();
		}
		
        pThis->SetEncoder();
		pThis->Pause();
        pThis->m_bResetEncoder = false;
    }

	return 0;
}


int CStreamSource::WriteEncFrameCallback(void* pObject, AVPacket* pPacket)
{
	CStreamSource* pThis = (CStreamSource*)pObject;
	if (NULL == pThis)
	{
		return -1;
	}

	if (NULL == pThis->m_pMuxer)
	{
		return -2;
	}

	int nRet = pThis->m_pMuxer->WriteEncFrame(pThis->m_pOutFmtCtx, pPacket);
	return nRet;
}


int	CStreamSource::ProcFileOpen(UINT uiPixFmt, UINT uiWidth, UINT uiHeight, UINT uiFps,
								UINT uiSampleFmt, UINT uiChannelLayout, UINT uiChannels, UINT uiSampleRate, UINT uiFrameSize)
{
	if ((NULL != m_pParent) && (NULL != m_pStreamCallbackFunc))
	{
		OPENED_FILE_INFO Info	= { 0, };
		Info.uiPixFmt			= uiPixFmt;
		Info.uiWidth			= uiWidth;
		Info.uiHeight			= uiHeight;
		Info.uiFps				= uiFps;
		Info.uiSampleFmt		= uiSampleFmt;
		Info.uiChannelLayout	= uiChannelLayout;
		Info.uiChannels			= uiChannels;
		Info.uiSampleRate		= uiSampleRate;
		Info.uiFrameSize		= uiFrameSize;

#ifdef _WIN32
		WaitForSingleObject(m_hSteramCallbackMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hSteramCallbackMutex);
#endif

		m_pStreamCallbackFunc(m_pParent, STREAM_PROC_OPENED_FILE, sizeof(Info), &Info);

#ifdef _WIN32
		ReleaseMutex(m_hSteramCallbackMutex);
#else
		pthread_mutex_unlock(&m_hSteramCallbackMutex);
#endif

		return 1;
	}

	return 0;
}


int	CStreamSource::ProcTranscodingInfo(UINT uiPixFmt, UINT uiVideoCodecID, UINT uiNum, UINT uiDen, 
									   UINT uiWidth, UINT uiHeight, UINT uiGopSize, UINT uiVideoBitrate,
									   UINT uiSampleFmt, UINT uiAudioCodecID, UINT uiChannelLayout, UINT uiChannels, 
									   UINT uiSampleRate, UINT uiFrameSize, UINT uiAudioBitrate)
{
	if ((NULL != m_pParent) && (NULL != m_pStreamCallbackFunc))
	{
		TRANSCODING_INFO Info	= { 0, };
		
		// Video
		Info.uiPixFmt			= uiPixFmt;
		Info.uiVideoCodecID		= uiVideoCodecID;
		Info.uiNum				= uiNum;
		Info.uiDen				= uiDen;
		Info.uiWidth			= uiWidth;
		Info.uiHeight			= uiHeight;
		Info.uiGopSize			= uiGopSize;
		Info.uiVideoBitrate		= uiVideoBitrate;
		
		// Audio
		Info.uiSampleFmt		= uiSampleFmt;
		Info.uiAudioCodecID		= uiAudioCodecID;
		Info.uiChannelLayout	= uiChannelLayout;
		Info.uiChannels			= uiChannels;
		Info.uiSampleRate		= uiSampleRate;
		Info.uiFrameSize		= uiFrameSize;
		Info.uiAudioBitrate		= uiAudioBitrate;

#ifdef _WIN32
		WaitForSingleObject(m_hSteramCallbackMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hSteramCallbackMutex);
#endif

		m_pStreamCallbackFunc(m_pParent, STREAM_PROC_TRANSCODING_INFO, sizeof(Info), &Info);

#ifdef _WIN32
		ReleaseMutex(m_hSteramCallbackMutex);
#else
		pthread_mutex_unlock(&m_hSteramCallbackMutex);
#endif

		return 1;
	}

	return 0;
}


int	CStreamSource::ProcFrameData(char cFrameType, UINT uiFrameNum, UINT uiFrameSize, unsigned char* pData)
{
	if ((NULL != m_pParent) && (NULL != m_pStreamCallbackFunc))
	{
		int		nIndex			= 0;
		
		char*	pFrameDataBuf	= new char[sizeof(FRAME_DATA) + uiFrameSize];
		memset(pFrameDataBuf, 0, sizeof(FRAME_DATA) + uiFrameSize);
		
		PFRAME_DATA pFrameData	= (PFRAME_DATA)pFrameDataBuf;
		pFrameData->uiFrameType	= cFrameType;
		nIndex += sizeof(UINT);
		pFrameData->uiFrameNum	= uiFrameNum;
		nIndex += sizeof(UINT);
		pFrameData->uiFrameSize = uiFrameSize;
		nIndex += sizeof(UINT);
		memcpy(pFrameDataBuf + nIndex, pData, uiFrameSize);

#ifdef _WIN32
		WaitForSingleObject(m_hSteramCallbackMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hSteramCallbackMutex);
#endif

		m_pStreamCallbackFunc(m_pParent, STREAM_PROC_FRAME_DATA, sizeof(FRAME_DATA) + uiFrameSize, pFrameDataBuf);

#ifdef _WIN32
		ReleaseMutex(m_hSteramCallbackMutex);
#else
		pthread_mutex_unlock(&m_hSteramCallbackMutex);
#endif
		
		SAFE_DELETE_ARRAY(pFrameDataBuf);

		return 1;
	}

	return 0;
}


#ifdef _USE_FILTER_GRAPH
int CStreamSource::EncodeFilterFrame(int nStreamIndex)
{
	if (NULL == m_pFilterCtx[nStreamIndex].pBufferSinkCtx)
	{
		return -1;
	}

	int				nRet = 0;

	while (1)
	{
		AVFrame*		pFilterFrame = av_frame_alloc();

		if (NULL == pFilterFrame)
		{
			nRet = AVERROR(ENOMEM);
			break;
		}

		av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
		nRet = av_buffersink_get_frame(m_pFilterCtx[nStreamIndex].pBufferSinkCtx, pFilterFrame);
		if (0 > nRet)
		{
			/* if no more frames for output - returns AVERROR(EAGAIN)
			* if flushed and no more frames for output - returns AVERROR_EOF
			* rewrite retcode to 0 to show it as normal procedure completion
			*/
			if ((AVERROR(EAGAIN) == nRet) || (AVERROR_EOF == nRet))
			{
				nRet = 0;
			}

			av_frame_free(&pFilterFrame);
			break;
		}

		//pFilterFrame->pict_type = AV_PICTURE_TYPE_NONE;
		nRet = EncodeProc(nStreamIndex, pFilterFrame->pict_type, pFilterFrame);

		if (NULL != pFilterFrame)
		{
			av_frame_free(&pFilterFrame);
		}

		if (0 > nRet)
			break;
	}

	return nRet;
}


int CStreamSource::EncodeProc(int nMediaType, int nPictType, AVFrame* pFrame)
{
#else
int CStreamSource::EncodeProc(int nMediaType, int nPictType, unsigned int uiDataSize, unsigned char* pData)
{
#endif

	unsigned char*	pEncData	= NULL;
	unsigned int	uiEncSize	= 0;
	char			cFrameType	= 0;
	
	switch (nPictType)
	{
	case AV_PICTURE_TYPE_NONE:
		cFrameType = 'A';
		break;

	case AV_PICTURE_TYPE_I:
		cFrameType = 'I';
		break;

	case AV_PICTURE_TYPE_P:
		cFrameType = 'P';
		break;

	case AV_PICTURE_TYPE_B:
		cFrameType = 'B';
		break;

	case AV_PICTURE_TYPE_S:
		cFrameType = 'S';
		break;

	case AV_PICTURE_TYPE_SI:
		cFrameType = 'i';
		break;

	case AV_PICTURE_TYPE_SP:
		cFrameType = 'p';
		break;

	case AV_PICTURE_TYPE_BI:
		cFrameType = 'b';
		break;

	default:
		break;
	}

    // Get decoded frame pts and set encode frame pts
    if (true == m_bCreateTrscVideoFile)
    {
        int64_t llPts       = 0;
        int64_t llPktPts    = 0;
        int64_t llPktDts    = 0;

        m_pDecoder->GetFramePtsData(nMediaType, llPts, llPktPts, llPktDts);
        m_pEncoder->SetFramePtsData(nMediaType, llPts, llPktPts, llPktDts);
    }

	// transcoding
#ifdef _USE_FILTER_GRAPH
	if ( (0 <= m_pEncoder->Encode(nMediaType, pFrame, &pEncData, uiEncSize))
#else
	if ( (0 <= m_pEncoder->Encode(nMediaType, pData, uiDataSize, &pEncData, uiEncSize))
#endif
		&& ((NULL != pEncData) && (0 < uiEncSize)) )
	{	
		ProcFrameData(cFrameType,
					  (AVMEDIA_TYPE_VIDEO == nMediaType) ? m_llVideoFrameNum++ : m_llAudioFrameNum++,
				      uiEncSize, pEncData);
		SAFE_DELETE(pEncData);
	}
	else
	{
		TraceLog("CStreamSource::EncodeThread - %s Encode Failed", ('A' == cFrameType) ? "Audio" : "Video");
		return -2;
	}

	return 0;
}


int64_t CStreamSource::EncodeDelayedFrame(int nStreamIndex)
{
	int64_t			llRet = -1;

	if (NULL == m_pEncoder)
	{
		return llRet;
	}

	unsigned char*	pEncData = NULL;
	unsigned int	uiEncSize = 0;

	// transcoding
	if ((0 <= m_pEncoder->Encode(nStreamIndex, NULL, 0, &pEncData, uiEncSize)) && ((NULL != pEncData) && (0 < uiEncSize)))
	{
		llRet = 0;
	}

	return llRet;
}


int CStreamSource::SetMuxer()
{
	if (false == m_bCreateTrscVideoFile)
	{
		return -1;
	}

	if (NULL == m_pObjectManager)
	{
		return -2;
	}

	if (NULL != m_pMuxer)
	{
		m_pObjectManager->DestroyObject((void**)&m_pMuxer, OBJECT_TYPE_MUXER);
	}

    CtxAndStreamCleanUp(1);
    
	// Set Muxer
	if (0 > m_pObjectManager->CreateObject((void**)&m_pMuxer, OBJECT_TYPE_MUXER))
	{
		return -3;
	}

	if (NULL == m_pMuxer)
	{
        return -4;
    }
		
	int nStreamIndex = -1;

	// Make transcoded video file path
	snprintf(m_szTrscFileName, MAX_PATH - 1, "./TrscVideo/[Trsc][%dx%d]%s", m_nEncVideoWidth, m_nEncVideoHeight, m_pszInputFileName);

	if (0 > m_pMuxer->FileOpenProc(m_szTrscFileName, &m_pOutFmtCtx))
	{
		return -5;
	}

    m_nTrscCodecCtxSize = m_pInFmtCtx->nb_streams;
    m_nTrscStreamSize   = m_pInFmtCtx->nb_streams;

    m_pTrscCodecCtx     = (AVCodecContext**)av_malloc_array(m_nTrscCodecCtxSize, sizeof(*m_pTrscCodecCtx));
    m_pTrscStream       = (AVStream**)av_malloc_array(m_nTrscStreamSize, sizeof(*m_pTrscStream));

	m_pMuxer->SetMuxInfo(m_nEncVideoWidth, m_nEncVideoHeight, m_llEncVideoBitRate, m_llEncAudioBitRate);

    for (int nIndex = 0; nIndex < m_nTrscCodecCtxSize; ++nIndex)
    {
        m_pTrscCodecCtx[nIndex] = NULL;
        m_pTrscStream[nIndex]   = NULL;
        
        if (0 > m_pMuxer->OpenCodecContext(&nStreamIndex, &m_pTrscCodecCtx[nIndex], m_pInFmtCtx, m_pOutFmtCtx, (AVMediaType)(AVMEDIA_TYPE_VIDEO + nIndex)))
        {
            return -5;
        }
        
        if (nStreamIndex != (int)(AVMEDIA_TYPE_VIDEO + nIndex))
        {
            return -6;
        }
        
        m_pTrscStream[nIndex] = m_pOutFmtCtx->streams[nStreamIndex];		
    }

    if ((NULL == m_pOutFmtCtx) || (NULL == m_pTrscCodecCtx[AVMEDIA_TYPE_VIDEO]) || (NULL == m_pTrscCodecCtx[AVMEDIA_TYPE_AUDIO]))
	{
		return -8;
	}

    if (0 > m_pMuxer->WriteFileHeader(m_pOutFmtCtx))
    {
        return -9;
    }

	return 0;
}


int CStreamSource::SetEncoder()
{
    if (NULL == m_pObjectManager)
    {
        return -1;
    }

	if ((0 >= m_nEncVideoWidth) || (0 >= m_nEncVideoHeight))
	{
		return -2;
	}

	if (NULL != m_pEncoder)
	{
		m_pObjectManager->DestroyObject((void**)&m_pEncoder, OBJECT_TYPE_ENCODER);
	}

	// Init Encoder
	if ((0 > m_pObjectManager->CreateObject((void**)&m_pEncoder, OBJECT_TYPE_ENCODER)) || (NULL == m_pEncoder))
	{
		return -3;
	}

	// for Rescaling Video Data
	m_pEncoder->SetVideoSrcInfo(m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->pix_fmt, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->width, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->height);

	if (0 > m_pEncoder->InitVideoCtx(AV_PIX_FMT_YUV420P, AV_CODEC_ID_H264, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->time_base,
									 m_nEncVideoWidth, m_nEncVideoHeight, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->gop_size, m_llEncVideoBitRate))
	{
		return -4;
	}

	if (0 > m_pEncoder->InitAudioCtx(AV_SAMPLE_FMT_FLTP, AV_CODEC_ID_AAC, AV_CH_LAYOUT_STEREO, 2,
									 m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->sample_rate, m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->frame_size, m_llEncAudioBitRate))
	{
		return -5;
	}

	if (true == m_bCreateTrscVideoFile)
	{
		m_pEncoder->SetCallbackProc(this, WriteEncFrameCallback);
	}

	ProcTranscodingInfo(AV_PIX_FMT_YUV420P, AV_CODEC_ID_H264, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->time_base.num, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->time_base.den,
						m_nEncVideoWidth, m_nEncVideoHeight, m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->gop_size, m_llEncVideoBitRate,
						AV_SAMPLE_FMT_FLTP, AV_CODEC_ID_AAC, AV_CH_LAYOUT_STEREO, 2,
						m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->sample_rate, m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->frame_size, m_llEncAudioBitRate);

	return 0;
}

#ifdef _USE_FILTER_GRAPH
int CStreamSource::InitFilters()
{
    const char* pszFilterSpec   = NULL;
    int         nRet            = 0;

    if (NULL != m_pFilterCtx)
    {
        av_free(m_pFilterCtx);
    }

    m_pFilterCtx = (FilteringContext*)av_malloc_array(m_pInFmtCtx->nb_streams, sizeof(*m_pFilterCtx));
    if (!m_pFilterCtx)
    {
        return AVERROR(ENOMEM);
    }

    for (int nIndex = 0; nIndex < m_pInFmtCtx->nb_streams; ++nIndex) 
    {        
        m_pFilterCtx[nIndex].pBufferSrcCtx  = NULL;
        m_pFilterCtx[nIndex].pBufferSinkCtx = NULL;        
        m_pFilterCtx[nIndex].pFilterGraph   = NULL;

        if (!( (AVMEDIA_TYPE_AUDIO == m_pInFmtCtx->streams[nIndex]->codec->codec_type)
                || (AVMEDIA_TYPE_VIDEO == m_pInFmtCtx->streams[nIndex]->codec->codec_type)) )
            continue;
        
        if (AVMEDIA_TYPE_VIDEO == m_pInFmtCtx->streams[nIndex]->codec->codec_type)
        {
            pszFilterSpec = "null"; /* passthrough (dummy) filter for video */
        }
        else
        {
            pszFilterSpec = "anull"; /* passthrough (dummy) filter for audio */
        }
        
        nRet = InitFilter(&m_pFilterCtx[nIndex], 
                          m_pInFmtCtx->streams[nIndex]->codec,
                          m_pEncoder->GetCodecCtx(m_pInFmtCtx->streams[nIndex]->codec->codec_type), 
                          pszFilterSpec);
        
        if (0 < nRet)
        {
            return nRet;
        }
    }

    return 0;
}


int CStreamSource::InitFilter(FilteringContext* pFilterCtx, AVCodecContext* pDecCtx, AVCodecContext* pEncCtx, const char *pszFilterSpec)
{
    if ((NULL == pFilterCtx) || (NULL == pDecCtx) || (NULL == pEncCtx) || (NULL == pszFilterSpec))
    {
        return -1;
    }

    char                szArgs[512]     = {0, };
    int                 nRet            = 0;

    AVFilter*           pBufferSrc      = NULL;
    AVFilter*           pBufferSink     = NULL;
    
    AVFilterContext*    pBufferSinkCtx  = NULL;
    AVFilterContext*    pBufferSrcCtx   = NULL;    
    
    AVFilterInOut*      pOutputs        = avfilter_inout_alloc();
    AVFilterInOut*      pInputs         = avfilter_inout_alloc();
    AVFilterGraph*      pFilterGraph    = avfilter_graph_alloc();

    if ((NULL == pOutputs) || (NULL == pInputs) || (NULL == pFilterGraph)) 
    {
        nRet = AVERROR(ENOMEM);
        goto $END;
    }

    if (pDecCtx->codec_type == AVMEDIA_TYPE_VIDEO) 
    {
        pBufferSrc  = avfilter_get_by_name("buffer");
        pBufferSink = avfilter_get_by_name("buffersink");

        if ((NULL == pBufferSrc) || (NULL == pBufferSink)) 
        {
            TraceLog("filtering source or sink element not found");
            nRet = AVERROR_UNKNOWN;
            goto $END;
        }

        _snprintf(szArgs, sizeof(szArgs),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                 pDecCtx->width, pDecCtx->height, pDecCtx->pix_fmt,
                 pDecCtx->time_base.num, pDecCtx->time_base.den,
                 pDecCtx->sample_aspect_ratio.num,
                 pDecCtx->sample_aspect_ratio.den);

        nRet = avfilter_graph_create_filter(&pBufferSrcCtx, pBufferSrc, "in",
                                            szArgs, NULL, pFilterGraph);
        if (0 > nRet) 
        {
			TraceLog("Cannot create buffer source");
            goto $END;
        }

        nRet = avfilter_graph_create_filter(&pBufferSinkCtx, pBufferSink, "out",
                                            NULL, NULL, pFilterGraph);
        if (0 > nRet) 
        {
			TraceLog("Cannot create buffer sink");
            goto $END;
        }

        nRet = av_opt_set_bin(pBufferSinkCtx, "pix_fmts",
                              (uint8_t*)&pEncCtx->pix_fmt, sizeof(pEncCtx->pix_fmt),
                              AV_OPT_SEARCH_CHILDREN);
        if (0 > nRet) 
        {
			TraceLog("Cannot set output pixel format");
            goto $END;
        }
    } 
    else if (pDecCtx->codec_type == AVMEDIA_TYPE_AUDIO) 
    {
        pBufferSrc  = avfilter_get_by_name("abuffer");
        pBufferSink = avfilter_get_by_name("abuffersink");

        if ((NULL == pBufferSrc) || (NULL == pBufferSink))
        {
			TraceLog("filtering source or sink element not found");
            nRet = AVERROR_UNKNOWN;
            goto $END;
        }

        if (0 == pDecCtx->channel_layout)
        {
            pDecCtx->channel_layout = av_get_default_channel_layout(pDecCtx->channels);
        }

        _snprintf(szArgs, sizeof(szArgs),
                  "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%llx",
                   pDecCtx->time_base.num, pDecCtx->time_base.den, pDecCtx->sample_rate,
                   av_get_sample_fmt_name(pDecCtx->sample_fmt),
                   pDecCtx->channel_layout);

        nRet = avfilter_graph_create_filter(&pBufferSrcCtx, pBufferSrc, "in",
                                            szArgs, NULL, pFilterGraph);
        if (0 > nRet) 
        {
			TraceLog("Cannot create audio buffer source");
            goto $END;
        }

        nRet = avfilter_graph_create_filter(&pBufferSinkCtx, pBufferSink, "out",
                                            NULL, NULL, pFilterGraph);
        if (0 > nRet)  
        {
			TraceLog("Cannot create audio buffer sink");
            goto $END;
        }

        nRet = av_opt_set_bin(pBufferSinkCtx, "sample_fmts",
                              (uint8_t*)&pEncCtx->sample_fmt, sizeof(pEncCtx->sample_fmt),
                              AV_OPT_SEARCH_CHILDREN);
        if (0 > nRet) 
        {
			TraceLog("Cannot set output sample format");
            goto $END;
        }

        nRet = av_opt_set_bin(pBufferSinkCtx, "channel_layouts",
                              (uint8_t*)&pEncCtx->channel_layout,
                              sizeof(pEncCtx->channel_layout), AV_OPT_SEARCH_CHILDREN);
        if (0 > nRet) 
        {
			TraceLog("Cannot set output channel layout");
            goto $END;
        }

        nRet = av_opt_set_bin(pBufferSinkCtx, "sample_rates",
                              (uint8_t*)&pEncCtx->sample_rate, sizeof(pEncCtx->sample_rate),
                              AV_OPT_SEARCH_CHILDREN);
        if (0 > nRet) 
        {
			TraceLog("Cannot set output sample rate");
            goto $END;
        }
    } 
    else 
    {
        nRet = AVERROR_UNKNOWN;
        goto $END;
    }

    /* Endpoints for the filter graph. */
    pOutputs->name          = av_strdup("in");
    pOutputs->filter_ctx    = pBufferSrcCtx;
    pOutputs->pad_idx       = 0;
    pOutputs->next          = NULL;

    pInputs->name           = av_strdup("out");
    pInputs->filter_ctx     = pBufferSinkCtx;
    pInputs->pad_idx        = 0;
    pInputs->next           = NULL;

    if ((NULL == pOutputs->name) || (NULL == pInputs->name)) 
    {
        nRet = AVERROR(ENOMEM);
        goto $END;
    }

    nRet = avfilter_graph_parse_ptr(pFilterGraph, pszFilterSpec, &pInputs, &pOutputs, NULL);
    if (0 > nRet)
    {
        goto $END;
    }

    nRet = avfilter_graph_config(pFilterGraph, NULL);
    if (0 > nRet)
    {
        goto $END;
    }

    /* Fill FilteringContext */
    pFilterCtx->pBufferSrcCtx   = pBufferSrcCtx;
    pFilterCtx->pBufferSinkCtx  = pBufferSinkCtx;
    pFilterCtx->pFilterGraph    = pFilterGraph;

$END:
    avfilter_inout_free(&pInputs);
    avfilter_inout_free(&pOutputs);

    return nRet;
}
#endif


void CStreamSource::Flush()
{
	/* get the delayed frames */
#ifndef _USE_FILTER_GRAPH
	while (0 <= EncodeProc(AVMEDIA_TYPE_NB + AVMEDIA_TYPE_VIDEO, AV_PICTURE_TYPE_I, 0, NULL))
	{

	}

	while (0 <= EncodeProc(AVMEDIA_TYPE_NB + AVMEDIA_TYPE_AUDIO, AV_PICTURE_TYPE_NONE, 0, NULL))
	{

	}
#endif

    // close transcoded video file
    if (true == m_bCreateTrscVideoFile)
    {
        av_write_trailer(m_pOutFmtCtx);
    }

	if ((NULL != m_pDecoder) && (NULL != m_pDemuxer))
	{
        if (NULL != m_pDecStream[AVMEDIA_TYPE_VIDEO])
		{
			printf("Play the output video file with the command:\n"
				   "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d\n",
				   av_get_pix_fmt_name(m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->pix_fmt),
				   m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->width,
				   m_pDecCodecCtx[AVMEDIA_TYPE_VIDEO]->height);
		}

        if (NULL != m_pDecStream[AVMEDIA_TYPE_AUDIO])
		{
			enum AVSampleFormat SampleFmt	= m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->sample_fmt;
			int					nChannels	= m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->channels;
			const char*			pszFmt		= NULL;

			if (av_sample_fmt_is_planar(SampleFmt))
			{
				const char *pszPacked = av_get_sample_fmt_name(SampleFmt);
				printf("Warning: the sample format the decoder produced is planar "
					"(%s). This example will output the first channel only.\n",
					pszPacked ? pszPacked : "?");
				SampleFmt = av_get_packed_sample_fmt(SampleFmt);
				nChannels = 1;
			}

			if (0 > m_pDemuxer->GetFormatFromSampleFmt(&pszFmt, SampleFmt))
			{
				return;
			}

			printf("Play the output audio file with the command:\n"
				"ffplay -f %s -ac %d -ar %d\n",
				pszFmt, nChannels, m_pDecCodecCtx[AVMEDIA_TYPE_AUDIO]->sample_rate);
		}
	}

	if ((NULL != m_pParent) && (NULL != m_pStreamCallbackFunc))
	{
#ifdef _WIN32
		WaitForSingleObject(m_hSteramCallbackMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hSteramCallbackMutex);
#endif
		m_pStreamCallbackFunc(m_pParent, STREAM_PROC_COMPLETE, 0, NULL);

#ifdef _WIN32
		ReleaseMutex(m_hSteramCallbackMutex);
#else
		pthread_mutex_unlock(&m_hSteramCallbackMutex);
#endif
	}
}


void CStreamSource::ObjectCleanUp()
{
	if (NULL != m_pObjectManager)
	{
		m_pObjectManager->DestroyObject((void**)&m_pMuxer,		OBJECT_TYPE_MUXER);
		m_pObjectManager->DestroyObject((void**)&m_pDemuxer,	OBJECT_TYPE_DEMUXER);
		m_pObjectManager->DestroyObject((void**)&m_pEncoder,	OBJECT_TYPE_ENCODER);
        m_pObjectManager->DestroyObject((void**)&m_pDecoder,	OBJECT_TYPE_DECODER);

		SAFE_DELETE(m_pObjectManager);
	}
}


void CStreamSource::CtxAndStreamCleanUp(int nTarget)
{
    switch (nTarget)
    {
    case 0:
        {
            if (NULL != m_pDecCodecCtx)
            {
                for (int nIndex = 0; nIndex < m_nDecCodecCtxSize; ++nIndex)
                {
                    avcodec_close(m_pDecCodecCtx[nIndex]);
                    avcodec_free_context(&m_pDecCodecCtx[nIndex]);
                }
        
                av_free(m_pDecCodecCtx);
                m_pDecCodecCtx  = NULL;
            }

            if (NULL != m_pDecStream)
            {
                av_free(m_pDecStream);
                m_pDecStream    = NULL;
            }

            avformat_close_input(&m_pInFmtCtx);
            m_pInFmtCtx         = NULL;
        }
        break;

    case 1:
        {
            if (NULL != m_pTrscCodecCtx)
            {
                for (int nIndex = 0; nIndex < m_nTrscCodecCtxSize; ++nIndex)
                {
                    avcodec_close(m_pTrscCodecCtx[nIndex]);
                }
        
                av_free(m_pTrscCodecCtx);
                m_pTrscCodecCtx = NULL;
            }

            if (NULL != m_pTrscStream)
            {
                av_free(m_pTrscStream);
                m_pTrscStream   = NULL;
            } 

            if ((NULL != m_pOutFmtCtx) && !(m_pOutFmtCtx->oformat->flags & AVFMT_NOFILE))
	        {
    		    avio_closep(&m_pOutFmtCtx->pb);		        
    	    }

            avformat_free_context(m_pOutFmtCtx);
            m_pOutFmtCtx    = NULL;
        }
        break;

    default:
        break;
    }
}


#ifdef _WIN32
unsigned int __stdcall  CStreamSource::DecodeThread(void* lpParam)
#else
void*                   CStreamSource::DecodeThread(void* lpParam)
#endif
{
	CStreamSource* pThis = (CStreamSource*)lpParam;
	if (NULL == pThis)
	{
		return 0;
	}

	int nRet		    = 0;
	int nGotFrame	    = -1;
    int nStreamIndex    = -1;

    unsigned int    uiDecSize       = 0;

	/* read frames from the file */
	while ( (true == pThis->m_bRunDecodeThread) 
        && (0 <= (nStreamIndex = pThis->m_pDecoder->ReadFrameData())) )
	{
        pThis->m_pDecoder->Decode(AVMEDIA_TYPE_NB + 1, NULL, 0, NULL, uiDecSize);

		// if pause, wait
		while (true == pThis->m_bPauseDecodeThread)
		{
#ifdef _WIN32
			Sleep(10);
#else
			usleep(10);
#endif
		}
	}

    // Proc cached data
	pThis->m_pDecoder->Decode(AVMEDIA_TYPE_NB + 2, NULL, 0, NULL, uiDecSize);

    // End
	pThis->Flush();

#ifdef _WIN32
	CloseHandle(pThis->m_hDecodeThread);
	pThis->m_hDecodeThread = NULL;

    return 1;
#else
	return NULL;
#endif 
}