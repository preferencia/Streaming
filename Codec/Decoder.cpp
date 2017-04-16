#include "stdafx.h"
#include "Decoder.h"

#ifdef _WIN32
#pragma warning(disable:4996)
#endif

CDecoder::CDecoder()
{
	m_nCodecType			= OBJECT_TYPE_DECODER;

	m_pFrame				= NULL;

	m_AVSrcSampleFmt		= AV_SAMPLE_FMT_NONE;
	m_nSrcChannelLayout		= AV_CH_LAYOUT_MONO;
	m_nSrcChannels			= 0;
	m_uiSrcSampleRate		= 0;
	m_uiSrcSamples			= 0;

	m_nRefCount				= 0;
	
	m_nVideoStreamIndex		= -1;
	m_nAudioStreamIndex		= -1;

	m_nVideoFrameCount		= 0;
	m_nAudioFrameCount		= 0;

	m_nVideoDstBufSize		= 0;

	memset(m_nVideoDstLineSizeArray,	0,	_DEC_PLANE_SIZE * sizeof(int));
	memset(m_pVideoDstData,		        0,	_DEC_PLANE_SIZE * sizeof(uint8_t*));

	m_uiDecAudioSize		= 0;

	m_pObject				= NULL;
	m_pDataProcCallback		= NULL;
}


CDecoder::~CDecoder()
{
	if (NULL != m_pVideoDstData[0])
	{
		av_free(m_pVideoDstData[0]);
	}

	if (NULL != m_pFrame)
	{
		av_frame_free(&m_pFrame);
		m_pFrame = NULL;
	}
}


void CDecoder::SetFramePtsData(int nStreamIndex, 
                               int64_t llPts, int64_t llPktPts, int64_t llPktDts)
{
    if (NULL == m_pFrame)
    {
        return;
    }

    m_pFrame->pts       = llPts;
    m_pFrame->pkt_pts   = llPktPts;
    m_pFrame->pkt_dts   = llPktDts;
}


void CDecoder::GetFramePtsData(int nStreamIndex, 
                               int64_t& llPts, int64_t& llPktPts, int64_t& llPktDts)
{
    if (NULL == m_pFrame)
    {
        return;
    }

    llPts       = m_pFrame->pts;
    llPktPts    = m_pFrame->pkt_pts;
    llPktDts    = m_pFrame->pkt_dts;
}


void CDecoder::SetCallbackProc(void* pObject, DataProcCallback pDataProcCallback)
{
	m_pObject			= pObject;
	m_pDataProcCallback = pDataProcCallback;
}


void CDecoder::SetStreamIndex(int nVideoStreamIndex, int nAudioStreamIndex)
{
	m_nVideoStreamIndex = nVideoStreamIndex;
	m_nAudioStreamIndex = nAudioStreamIndex;
}


void CDecoder::SetAudioSrcInfo(AVSampleFormat AVSrcSampleFmt, 
							   int nSrcChannelLayout, int nSrcChannels,
							   unsigned int uiSrcSampleRate, unsigned int uiSrcSamples)
{
	m_AVSrcSampleFmt	= AVSrcSampleFmt;
	m_nSrcChannelLayout = nSrcChannelLayout;
	m_nSrcChannels		= nSrcChannels;
	m_uiSrcSampleRate	= uiSrcSampleRate;
	m_uiSrcSamples		= uiSrcSamples;
}


int CDecoder::ReadFrameData()
{
    if (0 > av_read_frame(m_pFmtCtx, &m_Pkt))
    {
        return -1;
    }

    return m_Pkt.stream_index;
}


int64_t CDecoder::Decode(int nStreamIndex,
						 unsigned char* pSrcData, unsigned int uiSrcDataSize,
						 unsigned char** ppEncData, unsigned int& uiEncDataSize)
{
	switch (nStreamIndex)
	{
	case AVMEDIA_TYPE_VIDEO:
		{
			return DecodeVideo(pSrcData, uiSrcDataSize, ppEncData, uiEncDataSize);
		}
		break;

	case AVMEDIA_TYPE_AUDIO:
		{
			return DecodeAudio(pSrcData, uiSrcDataSize, ppEncData, uiEncDataSize);
		}
        break;

    case AVMEDIA_TYPE_NB + 1:
    case AVMEDIA_TYPE_NB + 2:
        {
            return DecodeCachedData(nStreamIndex - (AVMEDIA_TYPE_NB + 1));
        }
        break;

	default:
		break;
	}

	return 0;
}


int64_t CDecoder::DecodeCachedData(int nCached)
{
    int64_t llRet		= 0;
    int     nGotFrame   = -1;

    AVPacket OrgPkt = m_Pkt;
	do
	{
		llRet = DecodePacket(&nGotFrame, nCached);
		if (0 > llRet)
		{
			break;
		}

        if (0 == nCached)
        {
            m_Pkt.data += llRet;
    		m_Pkt.size -= llRet;
        }		
    } while ((0 == nCached) ? (m_Pkt.size > 0) : nGotFrame);

	if (1 == nGotFrame)
	{
		av_packet_unref(&OrgPkt);
	}

    return llRet;
}

int64_t CDecoder::DecodeVideo(unsigned char* pSrcData, unsigned int uiSrcDataSize,
							  unsigned char** ppDstData, unsigned int& uiDstDataSize)
{
	if ((NULL == pSrcData) || (0 == uiSrcDataSize))
	{
		return -1;
	}

	if ((NULL == ppDstData) || (NULL != *ppDstData))
	{
		return -2;
	}

	int		nGotFrame	= 0;
	int64_t llRet		= 0;

	m_Pkt.data			= pSrcData;
	m_Pkt.size			= uiSrcDataSize;
	m_Pkt.stream_index	= m_nVideoStreamIndex;

	AVPacket OrgPkt		= m_Pkt;
	do
	{
		llRet = DecodePacket(&nGotFrame, 0);
		if (0 > llRet)
		{
			break;
		}

		m_Pkt.data += llRet;
		m_Pkt.size -= llRet;
	} while (m_Pkt.size > 0);

	if (1 == nGotFrame)
	{
		av_packet_unref(&OrgPkt);
	}

	// YUV420 to RGB32 (client)
	unsigned char*	pScalingData		= NULL;
	int64_t			llScalingDataSize	= ScalingVideo(m_pVideoCtx->width, m_pVideoCtx->height, m_pVideoCtx->pix_fmt, m_pVideoDstData[0],
													 m_pVideoCtx->width, m_pVideoCtx->height, AV_PIX_FMT_RGB32, &pScalingData);

	if ((0 >= llScalingDataSize) || (NULL == pScalingData))
	{
		return -3;
	}

	*ppDstData		= (uint8_t*)pScalingData;
	uiDstDataSize	= llScalingDataSize;

	//*ppDstData		= m_pVideoDstData[0];
	//uiDstDataSize	= m_nVideoDstBufSize;

	return llRet;
}


int64_t CDecoder::DecodeAudio(unsigned char* pSrcData, unsigned int uiSrcDataSize,
							  unsigned char** ppDstData, unsigned int& uiDstDataSize)
{
	if ((NULL == pSrcData) || (0 == uiSrcDataSize))
	{
		return -1;
	}

	if ((NULL == ppDstData) || (NULL != *ppDstData))
	{
		return -2;
	}

	int		nGotFrame   = 0;
    int64_t llRet       = 0;

	m_Pkt.data          = pSrcData;
	m_Pkt.size          = uiSrcDataSize;
	m_Pkt.stream_index  = m_nAudioStreamIndex;

	AVPacket OrgPkt = m_Pkt;
	do
	{
		llRet = DecodePacket(&nGotFrame, 0);
		if (0 > llRet)
		{
			break;
		}

		m_Pkt.data += llRet;
		m_Pkt.size -= llRet;
	} while (m_Pkt.size > 0);

	if (1 == nGotFrame)
	{
		av_packet_unref(&OrgPkt);
	}

	// resampling (FLTP AAC to 16bit PCM)
	AVSampleFormat	AVSrcSampleFmt		= m_AVSrcSampleFmt;
	int				nSrcChannelLayout	= m_nSrcChannelLayout;
	int				nSrcChannels		= m_nSrcChannels;
	int				nSrcSampleRate		= m_uiSrcSampleRate;
	int				nSrcSamples			= m_uiSrcSamples;

	AVSampleFormat	AVDstSampleFmt		= AVSrcSampleFmt;
	int				nDstChannelLayout	= nSrcChannelLayout;
	int				nDstChannels		= nSrcChannels;
	int				nDstSampleRate		= nSrcSampleRate;
	int				nDstSamples			= 0;

	bool			bPlanarSampleFmt = false;

	/*
	if sample format is plannar,
	channel layout and channes adjust to mono.
	if want to play resampling audio data for stereo,
	dst sample rate transfer it's half, and channel transfer stereo.
	*/
	if (0 < av_sample_fmt_is_planar(AVSrcSampleFmt))
	{
		nSrcChannelLayout = AV_CH_LAYOUT_MONO;
		nSrcChannels = 1;

		if ((AV_CH_LAYOUT_STEREO == nDstChannelLayout) && (2 == nDstChannels))
		{
			if (AV_SAMPLE_FMT_U8P < AVSrcSampleFmt)
			{
				AVDstSampleFmt = AV_SAMPLE_FMT_S16;
			}
			else
			{
				AVDstSampleFmt = AV_SAMPLE_FMT_U8;
			}
		}

		bPlanarSampleFmt = true;
	}

	unsigned char*	pResamplingData = NULL;
	int64_t			llResampligDataSize = ResamplingAudio(AVSrcSampleFmt, nSrcChannelLayout, nSrcChannels, nSrcSampleRate, nSrcSamples,
														  m_uiDecAudioSize, m_pFrame->extended_data[0],
														  AVDstSampleFmt, nDstChannelLayout, nDstChannels, nDstSampleRate, nDstSamples, &pResamplingData);

	if ((0 >= llResampligDataSize) || (NULL == pResamplingData))
	{
		return -3;
	}

	*ppDstData		= pResamplingData;
	uiDstDataSize	= llResampligDataSize;

	//*ppDstData = m_pFrame->extended_data[0];
	//uiDstDataSize = m_uiDecAudioSize;

	return llRet;
}


int CDecoder::DecodePacket(int *pGotFrame, int nCached)
{
	int nRet		= 0;
	int nDecoded	= m_Pkt.size;

	*pGotFrame = 0;

	char szLog[AV_ERROR_MAX_STRING_SIZE] = { 0, };

	av_packet_rescale_ts(&m_Pkt,
						 m_pFmtCtx->streams[m_Pkt.stream_index]->time_base,
						 m_pFmtCtx->streams[m_Pkt.stream_index]->codec->time_base);

	if (m_Pkt.stream_index == m_nVideoStreamIndex)
	{
		/* decode video frame */
		nRet = avcodec_decode_video2(m_pVideoCtx, m_pFrame, pGotFrame, &m_Pkt);
		if (0 > nRet)
		{
            TraceLog("Error decoding video frame (%s)", av_make_error_string(szLog, AV_ERROR_MAX_STRING_SIZE, nRet));
			return nRet;
		}
	}
	else if (m_Pkt.stream_index == m_nAudioStreamIndex)
	{
		/* decode audio frame */
		nRet = avcodec_decode_audio4(m_pAudioCtx, m_pFrame, pGotFrame, &m_Pkt);
		if (0 > nRet)
		{
            TraceLog("Error decoding audio frame (%s)", av_make_error_string(szLog, AV_ERROR_MAX_STRING_SIZE, nRet));
			return nRet;
		}

		/* Some audio decoders decode only part of the packet, and have to be
		* called again with the remainder of the packet data.
		* Sample: fate-suite/lossless-audio/luckynight-partial.shn
		* Also, some decoders might over-read the packet. */
		nDecoded = FFMIN(nRet, m_Pkt.size);
	}

	if (*pGotFrame)
	{
		int				nPictType	= AV_PICTURE_TYPE_NONE;
		unsigned int	uiDataSize	= 0; 
		unsigned char*	pData		= NULL;

		m_pFrame->pts				= av_frame_get_best_effort_timestamp(m_pFrame);

#ifdef _USE_FILTER_GRAPH
		ProcDecodeData(m_Pkt.stream_index, m_pFrame);
#else
		if (m_Pkt.stream_index == m_nVideoStreamIndex)
		{
			if ((m_pFrame->width != m_nWidth)
				|| (m_pFrame->height != m_nHeight)
				|| (m_pFrame->format != m_PixelFmt))
			{
				/* To handle this change, one could call av_image_alloc again and
				* decode the following frames into another rawvideo file. */
				TraceLog("Error: Width, height and pixel format have to be "
						 "constant in a rawvideo file, but the width, height or "
						 "pixel format of the input video changed:\n"
						 "old: width = %d, height = %d, format = %s\n"
						 "new: width = %d, height = %d, format = %s",
						 m_nWidth, m_nHeight, av_get_pix_fmt_name(m_PixelFmt),
						 m_pFrame->width, m_pFrame->height,
						 av_get_pix_fmt_name((AVPixelFormat)m_pFrame->format));
				return -1;
			}

			TraceLog("video_frame%s n:%d coded_n:%d pts:%s",
					 nCached ? "(cached)" : "",
					 m_nVideoFrameCount++, m_pFrame->coded_picture_number,
					 av_ts_make_time_string(szLog, m_pFrame->pts, &m_pVideoCtx->time_base));

			/* copy decoded frame to destination buffer:
			* this is required since rawvideo expects non aligned data */
			av_image_copy(m_pVideoDstData, m_nVideoDstLineSizeArray,
						  (const uint8_t**)(m_pFrame->data), m_pFrame->linesize,
						  m_PixelFmt, m_nWidth, m_nHeight);

			nPictType					= (int)m_pFrame->pict_type;
			uiDataSize					= m_nVideoDstBufSize;
			pData						= m_pVideoDstData[0];
		}
		else if (m_Pkt.stream_index == m_nAudioStreamIndex)
		{
			size_t unpadded_linesize	= m_pFrame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)m_pFrame->format);
			TraceLog("audio_frame%s n:%d nb_samples:%d pts:%s",
					 nCached ? "(cached)" : "",
					 m_nAudioFrameCount++, m_pFrame->nb_samples,
					 av_ts_make_time_string(szLog, m_pFrame->pts, &m_pAudioCtx->time_base));

			m_uiDecAudioSize			= unpadded_linesize;
			uiDataSize					= unpadded_linesize;
			pData						= m_pFrame->extended_data[0];
		}

		ProcDecodeData(m_Pkt.stream_index, nPictType, uiDataSize, pData);
#endif
	}

	/* If we use frame reference counting, we own the data and need
	* to de-reference it when we don't use it anymore */
	if (*pGotFrame && m_nRefCount)
		av_frame_unref(m_pFrame);

	return nDecoded;
}


#ifdef _USE_FILTER_GRAPH
int CDecoder::ProcDecodeData(int nMediaType, AVFrame* pFrame)
#else
int CDecoder::ProcDecodeData(int nDataType, int nPictType, unsigned int uiDataSize, unsigned char* pData)
#endif
{
	if ((NULL != m_pObject) && (NULL != m_pDataProcCallback))
	{
#ifdef _USE_FILTER_GRAPH
		m_pDataProcCallback(m_pObject, nMediaType, pFrame);
#else
		m_pDataProcCallback(m_pObject, nDataType, nPictType, uiDataSize, pData);
#endif

        return 0;
	}

	return -1;
}


#ifndef _USE_FILTER_GRAPH
int CDecoder::AllocVideoBuffer()
{
	if ((0 == m_nWidth) || (0 == m_nHeight) || (AV_PIX_FMT_NONE == m_PixelFmt))
	{
		return -1;
	}

	m_nVideoDstBufSize = av_image_alloc(m_pVideoDstData, m_nVideoDstLineSizeArray, m_nWidth, m_nHeight, m_PixelFmt, 1);
	if (0 > m_nVideoDstBufSize)
	{
		TraceLog("Could not allocate raw video buffer");
		return -2;
	}

	return 0;
}
#endif


int CDecoder::InitFrame()
{
	// alloc frame
	if (NULL == m_pFrame)
	{
		m_pFrame = av_frame_alloc();
		if (NULL == m_pFrame)
		{
			TraceLog("Could not allocate frame");
			return -1;
		}
	}	

	m_bInitFrame	= true;

	return 0;
}