#include "stdafx.h"
#include "Encoder.h"

#ifdef _WINDOWS
#pragma warning(disable:4996)
#endif


CEncoder::CEncoder()
{
	m_nCodecType		= CODEC_TYPE_ENCODER;

	m_pVideoFrame		= NULL;
	m_pAudioFrame		= NULL;

	m_AVSrcPixFmt		= AV_PIX_FMT_NONE;
	m_nSrcWidth			= 0;
	m_nSrcHeight		= 0;

	m_pFAACEncHandle	= NULL;

	m_ulSamples			= 0;
	m_ulMaxOutBytes		= 0;
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


void CEncoder::SetVideoSrcInfo(AVPixelFormat AVSrcPixFmt, int nSrcWidth, int nSrcHeight)
{
	m_AVSrcPixFmt		= AVSrcPixFmt;
	m_nSrcWidth			= nSrcWidth;
	m_nSrcHeight		= nSrcHeight;
}


int64_t CEncoder::Encode(int nStreamIndex,
						 unsigned char* pSrcData, unsigned int uiSrcDataSize,
						 unsigned char** ppEncData, unsigned int& uiEncDataSize,
					 	 bool bEncodeDelayedFrame /* = false */)
{
	if (true == bEncodeDelayedFrame)
	{
		return EncodeDelayedFrame(nStreamIndex, ppEncData, uiEncDataSize);
	}

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

	default:
		{

		}
		break;
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

	// swcaling
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

	if (1 == nGotOutput)
	{
		*ppEncData		= m_Pkt.data;
		uiEncDataSize	= m_Pkt.size;
		av_packet_unref(&m_Pkt);
	}

	m_pVideoFrame->pts++;

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
		fprintf(stderr, "Could not setup audio frame\n");
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

	if (1 == nGotOutput)
	{
		*ppEncData		= m_Pkt.data;
		uiEncDataSize	= m_Pkt.size;
		av_packet_unref(&m_Pkt);
	}

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

	if (1 == nGotOutput)
	{
		*ppEncData		= m_Pkt.data;
		uiEncDataSize	= m_Pkt.size;
		av_packet_unref(&m_Pkt);
	}

	return 0;
}


int CEncoder::InitFrame()
{
	// alloc video frame
	if (NULL == m_pVideoFrame)
	{
		m_pVideoFrame = av_frame_alloc();
		if (NULL == m_pVideoFrame)
		{
			fprintf(stderr, "Could not allocate video frame\n");
			return -1;
		}

		m_pVideoFrame->pts = 0;
	}	

	// alloc audio frame
	if (NULL == m_pAudioFrame)
	{
		m_pAudioFrame = av_frame_alloc();
		if (NULL == m_pAudioFrame)
		{
			fprintf(stderr, "Could not allocate audio frame\n");
			return -2;
		}
	}	

	m_bInitFrame = true;

	return 0;
}