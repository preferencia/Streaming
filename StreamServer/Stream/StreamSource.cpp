#include "stdafx.h"
#include "StreamSource.h"

#ifdef _WINDOWS
#pragma warning(disable:4996)
#endif


CStreamSource::CStreamSource()
{
	m_pszSrcFileName		= NULL;

	m_pDemuxer				= NULL;
	m_pCodecManager			= NULL;
	m_pDecoder				= NULL;
	m_pDecoder2				= NULL;
	m_pEncoder				= NULL;

	m_pFmtCtx				= NULL;
	m_pVideoDecCtx			= NULL;
	m_pAudioDecCtx			= NULL;
	m_pVideoStream			= NULL;
	m_pAudioStream			= NULL;

	m_pParent				= NULL;
	m_pStreamCallbackFunc	= NULL;

	m_nRefCount				= 0;

	m_nVideoStreamIndex		= ~0;
	m_nAudioStreamIndex		= ~0;

	m_nEncVideoWidth		= 0;
	m_nEncVideoHeight		= 0;

	m_llEncVideoBitRate		= 0LL;
	m_llEncAudioBitRate		= 0LL;

	m_hSteramCallbackMutex	= NULL;
}


CStreamSource::~CStreamSource()
{
	CodecCleanUp();

	SAFE_DELETE(m_pDemuxer);
	SAFE_DELETE_ARRAY(m_pszSrcFileName);

	avcodec_free_context(&m_pVideoDecCtx);
	avcodec_free_context(&m_pAudioDecCtx);
	avformat_close_input(&m_pFmtCtx);

#ifdef _WINDOWS
	if (NULL != m_hSteramCallbackMutex)
	{
		CloseHandle(m_hSteramCallbackMutex);
		m_hSteramCallbackMutex = NULL;
	}
#else
	pthread_mutex_destroy(&m_hSteramCallbackMutex);
#endif
}


int CStreamSource::Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszSrcFileName)
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

	if (NULL == pszSrcFileName)
	{
		TraceLog("File Name is NULL");
		return -3;
	}

	if (NULL != m_pszSrcFileName)
	{
		delete[] m_pszSrcFileName;
		m_pszSrcFileName = NULL;
	}

	m_pParent				= pParent;
	m_pStreamCallbackFunc	= pStreamCallbackFunc;

	int nSrcFileNameLen = strlen(pszSrcFileName);
	m_pszSrcFileName = new char[nSrcFileNameLen + 1];
	strncpy(m_pszSrcFileName, pszSrcFileName, nSrcFileNameLen);
	m_pszSrcFileName[nSrcFileNameLen] = NULL;

#ifdef _WINDOWS
	m_hSteramCallbackMutex = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&m_hSteramCallbackMutex, NULL);
#endif

	/* register all formats and codecs */
	av_register_all();

	// Set Demuxer
	m_pDemuxer = new CDemuxer;
	if (NULL == m_pDemuxer)
	{
		return -1;
	}

	if (NULL != m_pszSrcFileName)
	{
		if (0 > m_pDemuxer->SrcFileOpenProc(m_pszSrcFileName, &m_pFmtCtx))
		{
			return -1;
		}

		if (0 > m_pDemuxer->OpenCodecContext(&m_nVideoStreamIndex, &m_pVideoDecCtx, m_pFmtCtx, AVMEDIA_TYPE_VIDEO))
		{
			return -2;
		}
		m_pVideoStream = m_pFmtCtx->streams[m_nVideoStreamIndex];

		if (0 > m_pDemuxer->OpenCodecContext(&m_nAudioStreamIndex, &m_pAudioDecCtx, m_pFmtCtx, AVMEDIA_TYPE_AUDIO))
		{
			return -3;
		}
		m_pAudioStream = m_pFmtCtx->streams[m_nAudioStreamIndex];
	}

	if ((NULL == m_pFmtCtx) || (NULL == m_pVideoDecCtx) || (NULL == m_pAudioDecCtx))
	{
		return -4;
	}

	// Set Video and Audio Bitrate for Encoder
	m_llEncVideoBitRate = (_DEC_VIDEO_ENC_BITRATE < m_pVideoDecCtx->bit_rate) ? _DEC_VIDEO_ENC_BITRATE : m_pVideoDecCtx->bit_rate;
	m_llEncAudioBitRate = (_DEC_AUDIO_ENC_BITRATE < m_pAudioDecCtx->bit_rate) ? _DEC_AUDIO_ENC_BITRATE : m_pAudioDecCtx->bit_rate;

	ProcFileOpen(m_pVideoDecCtx->pix_fmt, m_pVideoDecCtx->width, m_pVideoDecCtx->height,
				 m_pAudioDecCtx->sample_fmt, m_pAudioDecCtx->channel_layout, m_pAudioDecCtx->channels, m_pAudioDecCtx->sample_rate, m_pAudioDecCtx->frame_size);

	return 0;
}


void CStreamSource::SetResolution(int nWidth, int nHeight)
{
	m_nEncVideoWidth	= nWidth;
	m_nEncVideoHeight	= nHeight;
}


int CStreamSource::Start()
{
	if (NULL == m_pCodecManager)
	{
		m_pCodecManager = new CCodecManager;
	}
	
	if (NULL == m_pCodecManager)
	{
		return -1;
	}

	// Decoder Run
	if (0 > m_pCodecManager->CreateCodec(&m_pDecoder, CODEC_TYPE_DECODER))
	{
		return -2;
	}

	if (NULL == m_pDecoder)
	{
		return -3;
	}

	if (0 > m_pDecoder->InitFromContext(m_pFmtCtx, m_pVideoDecCtx, m_pAudioDecCtx))
	{
		return -4;
	}

	m_pDecoder->SetStreamIndex(m_nVideoStreamIndex, m_nAudioStreamIndex);
	m_pDecoder->SetCallbackProc(this, DataProcCallback);

	if (0 > m_pDecoder->RunDecodeThread())
	{
		return -5;
	}

	return 0;
}


bool CStreamSource::Pause()
{
	if (NULL != m_pDecoder)
	{
		bool bPauseStatus = m_pDecoder->GetPauseStatus();
		m_pDecoder->SetPause(!bPauseStatus);

		return !bPauseStatus;
	}

	return false;
}


void CStreamSource::Stop()
{
	if (NULL != m_pDecoder)
	{
		m_pDecoder->StopDecodeThread();
	}
}


int CStreamSource::DataProcCallback(void* pObject, int nProcDataType, unsigned int uiDataSize, unsigned char* pData, int nPictType)
{
	CStreamSource* pThis = (CStreamSource*)pObject;
	if (NULL == pThis)
	{
		return -1;
	}

	if (NULL == pThis->m_pEncoder)
	{
		pThis->SetEncoder();
	}

	unsigned char*	pEncData			= NULL;
	unsigned char*	pDecData			= NULL;
	unsigned int	uiEncSize			= 0;
	unsigned int	uiDecSize			= 0;

	int64_t			llVideoFrameNum		= 0LL;
	int64_t			llAudioFrameNum		= 0LL;

	switch (nProcDataType)
	{
	case DATA_TYPE_VIDEO:
		{
			char cFrameType = 0;
			switch (nPictType)
			{
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

			// transcoding				
			if ((0 <= pThis->m_pEncoder->Encode(AVMEDIA_TYPE_VIDEO, pData, uiDataSize, &pEncData, uiEncSize)) && ((NULL != pEncData) && (0 < uiEncSize)))
			{
				pThis->ProcFrameData(cFrameType, llVideoFrameNum, uiEncSize, pEncData);
				++llVideoFrameNum;
			}
		}
		break;

	case DATA_TYPE_AUDIO:
		{
			// transcoding
			if ((0 <= pThis->m_pEncoder->Encode(AVMEDIA_TYPE_AUDIO, pData, uiDataSize, &pEncData, uiEncSize)) && ((NULL != pEncData) && (0 < uiEncSize)))
			{
				pThis->ProcFrameData('A', llAudioFrameNum, uiEncSize, pEncData);
				++llAudioFrameNum;
			}
		}
		break;

	case DATA_TYPE_FLUSH:
		{
			/* get the delayed frames */
			while (0 <= pThis->EncodeDelayedFrame(DATA_TYPE_VIDEO))
			{

			}

			while (0 <= pThis->EncodeDelayedFrame(DATA_TYPE_AUDIO))
			{

			}

			if ((NULL != pThis->m_pDecoder) && (NULL != pThis->m_pDemuxer))
			{
				if (NULL != pThis->m_pVideoStream)
				{
					printf("Play the output video file with the command:\n"
						   "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d\n",
						   av_get_pix_fmt_name(pThis->m_pVideoDecCtx->pix_fmt),
						   pThis->m_pVideoDecCtx->width,
						   pThis->m_pVideoDecCtx->height);
				}

				if (NULL != pThis->m_pAudioStream)
				{
					enum AVSampleFormat SampleFmt = pThis->m_pAudioDecCtx->sample_fmt;
					int nChannels = pThis->m_pAudioDecCtx->channels;
					const char *pszFmt = NULL;

					if (av_sample_fmt_is_planar(SampleFmt))
					{
						const char *pszPacked = av_get_sample_fmt_name(SampleFmt);
						printf("Warning: the sample format the decoder produced is planar "
							"(%s). This example will output the first channel only.\n",
							pszPacked ? pszPacked : "?");
						SampleFmt = av_get_packed_sample_fmt(SampleFmt);
						nChannels = 1;
					}

					if (0 > pThis->m_pDemuxer->GetFormatFromSampleFmt(&pszFmt, SampleFmt))
					{
						break;
					}

					printf("Play the output audio file with the command:\n"
						"ffplay -f %s -ac %d -ar %d\n",
						pszFmt, nChannels, pThis->m_pAudioDecCtx->sample_rate);
				}
			}

			if ((NULL != pThis->m_pParent) && (NULL != pThis->m_pStreamCallbackFunc))
			{
#ifdef _WINDOWS
				WaitForSingleObject(pThis->m_hSteramCallbackMutex, INFINITE);
#else
				pthread_mutex_lock(&pThis->m_hSteramCallbackMutex);
#endif
				pThis->m_pStreamCallbackFunc(pThis->m_pParent, STREAM_PROC_COMPLETE, 0, NULL);

#ifdef _WINDOWS
				ReleaseMutex(pThis->m_hSteramCallbackMutex);
#else
				pthread_mutex_unlock(&pThis->m_hSteramCallbackMutex);
#endif
			}
		}
		break;

	default:
		break;
	}

	return 0;
}


int	CStreamSource::ProcFileOpen(UINT uiPixFmt, UINT uiWidth, UINT uiHeight,
								UINT uiSampleFmt, UINT uiChannelLayout, UINT uiChannels, UINT uiSampleRate, UINT uiFrameSize)
{
	if ((NULL != m_pParent) && (NULL != m_pStreamCallbackFunc))
	{
		OPENED_FILE_INFO Info	= { 0, };
		Info.uiPixFmt			= uiPixFmt;
		Info.uiWidth			= uiWidth;
		Info.uiHeight			= uiHeight;
		Info.uiSampleFmt		= uiSampleFmt;
		Info.uiChannelLayout	= uiChannelLayout;
		Info.uiChannels			= uiChannels;
		Info.uiSampleRate		= uiSampleRate;
		Info.uiFrameSize		= uiFrameSize;

#ifdef _WINDOWS
		WaitForSingleObject(m_hSteramCallbackMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hSteramCallbackMutex);
#endif

		m_pStreamCallbackFunc(m_pParent, STREAM_PROC_OPENED_FILE, sizeof(Info), &Info);

#ifdef _WINDOWS
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

#ifdef _WINDOWS
		WaitForSingleObject(m_hSteramCallbackMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hSteramCallbackMutex);
#endif

		m_pStreamCallbackFunc(m_pParent, STREAM_PROC_TRANSCODING_INFO, sizeof(Info), &Info);

#ifdef _WINDOWS
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

#ifdef _WINDOWS
		WaitForSingleObject(m_hSteramCallbackMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hSteramCallbackMutex);
#endif

		m_pStreamCallbackFunc(m_pParent, STREAM_PROC_FRAME_DATA, sizeof(FRAME_DATA) + uiFrameSize, pFrameDataBuf);

#ifdef _WINDOWS
		ReleaseMutex(m_hSteramCallbackMutex);
#else
		pthread_mutex_unlock(&m_hSteramCallbackMutex);
#endif
		
		SAFE_DELETE_ARRAY(pFrameDataBuf);

		return 1;
	}

	return 0;
}


int64_t CStreamSource::EncodeDelayedFrame(int nStreamIndex)
{
	int64_t			llRet = -1;

	if ((NULL == m_pEncoder) || (NULL == m_pDecoder2))
	{
		return llRet;
	}

	unsigned char*	pEncData = NULL;
	unsigned char*	pDecData = NULL;
	unsigned int	uiEncSize = 0;
	unsigned int	uiDecSize = 0;

	// transcoding
	if ((0 <= m_pEncoder->Encode(nStreamIndex, NULL, 0, &pEncData, uiEncSize, true)) && ((NULL != pEncData) && (0 < uiEncSize)))
	{
		llRet = 0;
	}

	return llRet;
}


int CStreamSource::SetEncoder()
{
	if ((0 >= m_nEncVideoWidth) || (0 >= m_nEncVideoHeight))
	{
		return -2;
	}

	if (NULL != m_pDecoder)
	{
		m_pDecoder->SetPause(true);
	}

	if (NULL != m_pEncoder)
	{
		m_pCodecManager->DestroyCodec(m_pEncoder);
	}

	// Init Encoder
	if ((0 > m_pCodecManager->CreateCodec(&m_pEncoder, CODEC_TYPE_ENCODER)) || (NULL == m_pEncoder))
	{
		return -3;
	}

	// for Rescaling Video Data
	m_pEncoder->SetVideoSrcInfo(m_pVideoDecCtx->pix_fmt, m_pVideoDecCtx->width, m_pVideoDecCtx->height);

	if (0 > m_pEncoder->InitVideoCtx(AV_PIX_FMT_YUV420P, AV_CODEC_ID_H264, m_pVideoDecCtx->time_base,
									 m_nEncVideoWidth, m_nEncVideoHeight, m_pVideoDecCtx->gop_size, m_llEncVideoBitRate))
	{
		return -4;
	}

	if (0 > m_pEncoder->InitAudioCtx(AV_SAMPLE_FMT_FLTP, AV_CODEC_ID_AAC, AV_CH_LAYOUT_STEREO, 2,
									 m_pAudioDecCtx->sample_rate, m_pAudioDecCtx->frame_size, m_llEncAudioBitRate))
	{
		return -5;
	}

	if (NULL != m_pDecoder)
	{
		m_pDecoder->SetPause(false);
	}

	ProcTranscodingInfo(AV_PIX_FMT_YUV420P, AV_CODEC_ID_H264, m_pVideoDecCtx->time_base.num, m_pVideoDecCtx->time_base.den,
						m_nEncVideoWidth, m_nEncVideoHeight, m_pVideoDecCtx->gop_size, m_llEncVideoBitRate,
						AV_SAMPLE_FMT_FLTP, AV_CODEC_ID_AAC, AV_CH_LAYOUT_STEREO, 2,
						m_pAudioDecCtx->sample_rate, m_pAudioDecCtx->frame_size, m_llEncAudioBitRate);

	return 0;
}


void CStreamSource::CodecCleanUp()
{
	if (NULL != m_pCodecManager)
	{
		m_pCodecManager->DestroyCodec(m_pDecoder);
		m_pCodecManager->DestroyCodec(m_pDecoder2);
		m_pCodecManager->DestroyCodec(m_pEncoder);

		SAFE_DELETE(m_pCodecManager);
	}
}