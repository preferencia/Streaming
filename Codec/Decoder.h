#pragma once

#include "Codec.h"

class CDecoder : public CCodec
{
public:
	CDecoder();
	virtual ~CDecoder();

    virtual void                SetFramePtsData(int nStreamIndex, 
                                                int64_t llPts, int64_t llPktPts, int64_t llPktDts);
    virtual void                GetFramePtsData(int nStreamIndex, 
                                                int64_t& llPts, int64_t& llPktPts, int64_t& llPktDts);

	virtual void				SetCallbackProc(void* pObject, DataProcCallback pDataProcCallback);
	virtual void				SetStreamIndex(int nVideoStreamIndex, int nAudioStreamIndex);
	virtual void				SetAudioSrcInfo(AVSampleFormat AVSrcSampleFmt,
												int nSrcChannelLayout, int nSrcChannels,
												unsigned int uiSrcSampleRate, unsigned int uiSrcSamples);

    virtual int                 ReadFrameData();
	virtual int64_t				Decode(int nStreamIndex,
									   unsigned char* pSrcData, unsigned int uiSrcDataSize,
									   unsigned char** ppEncData, unsigned int& uiEncDataSize);

private:
	virtual int64_t				DecodeVideo(unsigned char* pSrcData, unsigned int uiSrcDataSize,
											unsigned char** ppDstData, unsigned int& uiDstDataSize);
	virtual int64_t				DecodeAudio(unsigned char* pSrcData, unsigned int uiSrcDataSize,
											unsigned char** ppDstData, unsigned int& uiDstDataSize);	
    virtual int64_t             DecodeCachedData(int nCached);
	virtual int					DecodePacket(int *pGotFrame, int nCached);

#ifdef _USE_FILTER_GRAPH
	virtual int					ProcDecodeData(int nMediaType, int nPicType, AVFrame* pFilterFrame);
#else
	virtual int					ProcDecodeData(int nDataType, int nPictType, unsigned int uiDataSize, unsigned char* pData);
#endif

protected:
	virtual int					AllocVideoBuffer();
	virtual int					InitFrame();

private:
	AVFrame*			        		m_pFrame;
	AVFrame*							m_pFilterFrames[2];

	// Audio Src Info (for resampling)
	AVSampleFormat		        m_AVSrcSampleFmt;
	int					        			m_nSrcChannelLayout;
	int					        			m_nSrcChannels;
	unsigned int		        		m_uiSrcSampleRate;
	unsigned int		        		m_uiSrcSamples;

	/* Enable or disable frame reference counting. ou are not supposed to support
	* both paths in your application but pick the one most appropriate to your
	* needs. Look for the use of refcount in this example to see what are the
	* differences of API usage between them. */
	int					        			m_nRefCount;

	int					        			m_nVideoStreamIndex;
	int					        			m_nAudioStreamIndex;

	int					        			m_nVideoFrameCount;
	int					        			m_nAudioFrameCount;
	
	bool 									m_bAllocVideoBuffer;
	int64_t								m_llVideoDstBufSize;

	int					        			m_nVideoDstBufSize;
	int					        			m_nVideoDstLineSizeArray[_DEC_PLANE_SIZE];
	uint8_t*			        		m_pVideoDstData[_DEC_PLANE_SIZE];

	unsigned int		        		m_uiDecAudioSize;

	void*				        			m_pObject;
	DataProcCallback	        m_pDataProcCallback;
};

