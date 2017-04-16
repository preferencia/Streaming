#include "stdafx.h"
#include "Encoder.h"

#ifdef _WIN32
#pragma warning(disable:4996)
#endif


CEncoder::CEncoder()
{
	m_nCodecType				= OBJECT_TYPE_ENCODER;

	m_pVideoFrame				= NULL;
	m_pAudioFrame				= NULL;

	m_AVSrcPixFmt				= AV_PIX_FMT_NONE;
	m_nSrcWidth					= 0;
	m_nSrcHeight				= 0;

	m_pObject					= NULL;
	m_pWriteEncFrameCallback	= NULL;
}


CEncoder::~CEncoder()
{
	if (NULL != m_pVideoFrame)
	{
		av_frame_free(&m_pVideoFrame);
		m_pVideoFrame = NULL;
	}

	if (NULL != m_pAudioFrame)
	{
		av_frame_free(&m_pAudioFrame);
		m_pAudioFrame = NULL;
	}
}


void CEncoder::SetFramePtsData(int nStreamIndex, 
                               int64_t llPts, int64_t llPktPts, int64_t llPktDts)
{
    AVFrame* pFrame = NULL;

    switch (nStreamIndex)
    {
    case AVMEDIA_TYPE_VIDEO:
        pFrame = m_pVideoFrame;
        break;

    case AVMEDIA_TYPE_AUDIO:
        pFrame = m_pAudioFrame;
        break;

    default:
        break;
    }

    if (NULL == pFrame)
    {
        return;
    }

    pFrame->pts       = llPts;
    pFrame->pkt_pts   = llPktPts;
    pFrame->pkt_dts   = llPktDts;
}


void CEncoder::GetFramePtsData(int nStreamIndex, 
                               int64_t& llPts, int64_t& llPktPts, int64_t& llPktDts)
{
    AVFrame* pFrame = NULL;

    switch (nStreamIndex)
    {
    case AVMEDIA_TYPE_VIDEO:
        pFrame = m_pVideoFrame;
        break;

    case AVMEDIA_TYPE_AUDIO:
        pFrame = m_pAudioFrame;
        break;

    default:
        break;
    }

    if (NULL == pFrame)
    {
        return;
    }

    llPts       = pFrame->pts;
    llPktPts    = pFrame->pkt_pts;
    llPktDts    = pFrame->pkt_dts;
}


void CEncoder::SetCallbackProc(void* pObject, WriteEncFrameCallback pWriteEncFrameCallback)
{
	m_pObject					= pObject;
	m_pWriteEncFrameCallback	= pWriteEncFrameCallback;
}


void CEncoder::SetVideoSrcInfo(AVPixelFormat AVSrcPixFmt, int nSrcWidth, int nSrcHeight)
{
	m_AVSrcPixFmt				= AVSrcPixFmt;
	m_nSrcWidth					= nSrcWidth;
	m_nSrcHeight				= nSrcHeight;
}


int64_t CEncoder::Encode(int nStreamIndex,
						 unsigned char* pSrcData, unsigned int uiSrcDataSize,
						 unsigned char** ppEncData, unsigned int& uiEncDataSize)
{
	switch (nStreamIndex)
	{
	case AVMEDIA_TYPE_VIDEO:
		{
			return EncodeVideo(pSrcData, uiSrcDataSize, ppEncData, uiEncDataSize);
		}
		break;

	case AVMEDIA_TYPE_AUDIO:
		{
			return EncodeAudio(pSrcData, uiSrcDataSize, ppEncData, uiEncDataSize);
		}
		break;

	case AVMEDIA_TYPE_NB + AVMEDIA_TYPE_VIDEO:
	case AVMEDIA_TYPE_NB + AVMEDIA_TYPE_AUDIO:
		{
			return EncodeDelayedFrame(nStreamIndex - AVMEDIA_TYPE_NB, ppEncData, uiEncDataSize);
		}
		break;

	default:
		{
		}
		break;
	}

	return 0;
}

int64_t CEncoder::Encode(int nStreamIndex, AVFrame* pFrame,
						 unsigned char** ppEncData, unsigned int& uiEncDataSize)
{
	if (NULL == pFrame)
	{
		return -1;
	}

	if ((NULL == ppEncData) || (NULL != *ppEncData))
	{
		return -2;
	}

	uint8_t**		pOrgData = NULL;
	int				pVideoDstLineSize[_DEC_PLANE_SIZE] = { 0, };
	uint8_t*		pVideoDstData[_DEC_PLANE_SIZE] = { 0, };
	unsigned int	uiDataSize = 0;
	int				nGotOutput = 0;

	if (nStreamIndex == AVMEDIA_TYPE_VIDEO)
	{
		pOrgData = (uint8_t**)pFrame->data;

		uiDataSize = av_image_alloc(pVideoDstData, pVideoDstLineSize, m_nSrcWidth, m_nSrcHeight, m_AVSrcPixFmt, 1);

		av_image_copy(pVideoDstData, pVideoDstLineSize, (const uint8_t**)(pFrame->data), pFrame->linesize,
					  m_AVSrcPixFmt, m_nSrcWidth, m_nSrcHeight);

		pFrame->data[0] = pVideoDstData[0];
	}
	else if (nStreamIndex == AVMEDIA_TYPE_AUDIO)
	{
		uiDataSize = pFrame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)pFrame->format);
	}

	if (0 < uiDataSize)
	{		
		// initalize packet to remove prev data
		InitPacket();

		switch (nStreamIndex)
		{
		case AVMEDIA_TYPE_VIDEO:
		{
			// scaling
			unsigned char*	pScalingData = NULL;
			int				nDstWidth = m_pVideoCtx->width;
			int				nDstHeight = m_pVideoCtx->height;
			AVPixelFormat	AVDstPixFmt = m_pVideoCtx->pix_fmt;
			int64_t			llScalingDataSize = ScalingVideo(m_nSrcWidth, m_nSrcHeight, m_AVSrcPixFmt, pFrame->data[0],
				nDstWidth, nDstHeight, AVDstPixFmt, &pScalingData);

			if ((0 >= llScalingDataSize) || (NULL == pScalingData))
			{
				return -3;
			}

			pFrame->data[0] = pScalingData;

			avcodec_encode_video2(m_pVideoCtx, &m_Pkt, pFrame, &nGotOutput);

			av_freep(&pScalingData);
			pScalingData = NULL;
		}
		break;

		case AVMEDIA_TYPE_AUDIO:
		{
			avcodec_encode_audio2(m_pVideoCtx, &m_Pkt, pFrame, &nGotOutput);
		}
		break;

		default:
			break;
		}

		uiEncDataSize = PacketToBuffer(nStreamIndex, nGotOutput, ppEncData);
	}

	if (nStreamIndex == AVMEDIA_TYPE_VIDEO)
	{
		pFrame->data[0] = *pOrgData;
	}

	if (NULL != pVideoDstData[0])
	{
		av_free(pVideoDstData[0]);
	}

	return 0;
}


int64_t CEncoder::EncodeVideo(unsigned char* pSrcData, unsigned int uiSrcDataSize,
							  unsigned char** ppEncData, unsigned int& uiEncDataSize)
{
	if ((NULL == pSrcData) || (0 == uiSrcDataSize))
	{
		return -1;
	}

	if ((NULL == ppEncData) || (NULL != *ppEncData))
	{
		return -2;
	}

	// scaling
	unsigned char*	pScalingData		= NULL;
	int				nDstWidth			= m_pVideoCtx->width;
	int				nDstHeight			= m_pVideoCtx->height;
	AVPixelFormat	AVDstPixFmt			= m_pVideoCtx->pix_fmt;
	int64_t			llScalingDataSize	= ScalingVideo(m_nSrcWidth, m_nSrcHeight, m_AVSrcPixFmt, pSrcData,
													   nDstWidth, nDstHeight, AVDstPixFmt, &pScalingData);

	if ((0 >= llScalingDataSize) || (NULL == pScalingData))
	{
		return -3;
	}

	int				nGotOutput			= 0;
	int64_t			llRet				= 0;

	m_pVideoFrame->format				= m_pVideoCtx->pix_fmt;
	m_pVideoFrame->width				= m_pVideoCtx->width;
	m_pVideoFrame->height				= m_pVideoCtx->height;

	/* the image can be allocated by any means and av_image_alloc() is
	* just the most convenient way if av_malloc() is to be used */
	//llRet = av_image_alloc(m_pVideoFrame->data, m_pVideoFrame->linesize, m_pVideoCtx->width, m_pVideoCtx->height, m_pVideoCtx->pix_fmt, 32);
	llRet = av_image_alloc(m_pVideoFrame->data, m_pVideoFrame->linesize, nDstWidth, nDstHeight, AVDstPixFmt, 32);
	//if ((0 > llRet) || (llRet != uiSrcDataSize))
	if ((0 > llRet) || (llRet != llScalingDataSize))
	{
		llRet = -4;
		goto $REMOVE_SCALE_DATA;
	}

	//memcpy(m_pVideoFrame->data[0], pSrcData, uiSrcDataSize);
	memcpy(m_pVideoFrame->data[0], pScalingData, llScalingDataSize);

	// initalize packet to remove prev data
	InitPacket();

	llRet = avcodec_encode_video2(m_pVideoCtx, &m_Pkt, m_pVideoFrame, &nGotOutput);
	if (0 > llRet)
	{
		llRet = -5;
		goto $REMOVE_FRAME_DATA;
	}

	uiEncDataSize = PacketToBuffer(AVMEDIA_TYPE_VIDEO, nGotOutput, ppEncData);

	++m_pVideoFrame->pts;

$REMOVE_FRAME_DATA:
	av_freep(&m_pVideoFrame->data[0]);

$REMOVE_SCALE_DATA:
	av_freep(&pScalingData);
	pScalingData = NULL;

	return llRet;
}


int64_t CEncoder::EncodeAudio(unsigned char* pSrcData, unsigned int uiSrcDataSize,
							  unsigned char** ppEncData, unsigned int& uiEncDataSize)
{
	if ((NULL == pSrcData) || (0 == uiSrcDataSize))
	{
		return -1;
	}

	if ((NULL == ppEncData) || (NULL != *ppEncData))
	{
		return -2;
	}

	int			nGotOutput		= 0;
	int64_t		llRet			= 0;
	uint16_t*	pAudioData		= NULL;
	uint32_t	uiAudioDataSize = 0;

	m_pAudioFrame->nb_samples		= m_pAudioCtx->frame_size;
	m_pAudioFrame->format			= m_pAudioCtx->sample_fmt;
	m_pAudioFrame->channel_layout	= m_pAudioCtx->channel_layout;
	m_pAudioFrame->channels			= m_pAudioCtx->channels;

	/* the codec gives us the frame size, in samples,
	* we calculate the size of the samples buffer in bytes */
	uiAudioDataSize = av_samples_get_buffer_size(NULL, m_pAudioCtx->channels, m_pAudioCtx->frame_size, m_pAudioCtx->sample_fmt, 0);
	if ((0 >= uiAudioDataSize) || (uiSrcDataSize > uiAudioDataSize))
	{
		return -3;
	}

	pAudioData = (uint16_t*)av_malloc(uiAudioDataSize);
	if (NULL == pAudioData)
	{
		return -4;
	}

	/* setup the data pointers in the AVFrame */
	llRet = avcodec_fill_audio_frame(m_pAudioFrame, m_pAudioCtx->channels, m_pAudioCtx->sample_fmt,
									(const uint8_t*)pAudioData, uiAudioDataSize, 0);
	if (0 > llRet) 
	{
		TraceLog("Could not setup audio frame");
		return -5;
	}
	memset(pAudioData, 0, uiAudioDataSize);
	memcpy(pAudioData, pSrcData, uiSrcDataSize);

	// initalize packet to remove prev data
	InitPacket();

	llRet = avcodec_encode_audio2(m_pAudioCtx, &m_Pkt, m_pAudioFrame, &nGotOutput);
	if (0 > llRet)
	{
		return -6;
	}

	uiEncDataSize = PacketToBuffer(AVMEDIA_TYPE_AUDIO, nGotOutput, ppEncData);

	++m_pAudioFrame->pts;

	av_freep(&pAudioData);

	return 0;
}

int64_t	CEncoder::EncodeDelayedFrame(int nStreamIndex, unsigned char** ppEncData, unsigned int& uiEncDataSize)
{
	if ((NULL == ppEncData) || (NULL != *ppEncData))
	{
		return -1;
	}

	int			nGotOutput	= 0;
	int64_t		llRet		= 0;

	switch (nStreamIndex)
	{
	case AVMEDIA_TYPE_VIDEO:
		{
			llRet = avcodec_encode_video2(m_pVideoCtx, &m_Pkt, NULL, &nGotOutput);
		}
		break;

	case AVMEDIA_TYPE_AUDIO:
		{
			llRet = avcodec_encode_audio2(m_pAudioCtx, &m_Pkt, NULL, &nGotOutput);
		}
		break;

	default:
		break;
	}

	if (0 > llRet)
	{
		return -1;
	}

	uiEncDataSize = PacketToBuffer(nStreamIndex, nGotOutput, ppEncData);

	return 0;
}


int64_t CEncoder::PacketToBuffer(int nStreamIndex, int nGotOutput, unsigned char** ppEncData)
{
	if ((0 >= nGotOutput) || (NULL == ppEncData) || (NULL != *ppEncData))
	{
		return 0;
	}

	int64_t llRet	= m_Pkt.size;
	*ppEncData		= new unsigned char[m_Pkt.size];
	memcpy(*ppEncData, m_Pkt.data, m_Pkt.size);

	if ((NULL != m_pObject) && (NULL != m_pWriteEncFrameCallback))
	{
		/* prepare packet for muxing */
		m_Pkt.stream_index = nStreamIndex;
		m_pWriteEncFrameCallback(m_pObject, &m_Pkt);
	}

	av_packet_unref(&m_Pkt);

	return llRet;
}


int CEncoder::InitFrame()
{
	// alloc video frame
	if (NULL == m_pVideoFrame)
	{
		m_pVideoFrame = av_frame_alloc();
		if (NULL == m_pVideoFrame)
		{
			TraceLog("Could not allocate video frame");
			return -1;
		}

		m_pVideoFrame->pts		= 0;
		m_pVideoFrame->pkt_pts	= 0;
		m_pVideoFrame->pkt_dts	= 0;
	}	

	// alloc audio frame
	if (NULL == m_pAudioFrame)
	{
		m_pAudioFrame = av_frame_alloc();
		if (NULL == m_pAudioFrame)
		{
			TraceLog("Could not allocate audio frame");
			return -2;
		}

		m_pAudioFrame->pts		= 0;
		m_pAudioFrame->pkt_pts	= 0;
		m_pAudioFrame->pkt_dts	= 0;
	}	

	m_bInitFrame = true;

	return 0;
}