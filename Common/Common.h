#pragma once

extern "C"
{
	#include <libavutil/imgutils.h>	
	#include <libavutil/samplefmt.h>
	#include <libavutil/timestamp.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/hwcontext.h>
	#include <libavutil/opt.h>
	#include <libavutil/channel_layout.h>
	#include <libavformat/avformat.h>
	#include <libavfilter/avfiltergraph.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
	#include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}

#ifdef _WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#endif

#define _DEC_PLANE_SIZE			(4)

#define _DEC_VIDEO_ENC_BITRATE	(4000000)	// 4000 kbps
#define _DEC_AUDIO_ENC_BITRATE	(128000)	// 128 kbps

enum
{
	OBJECT_TYPE_NONE = -1,
	OBJECT_TYPE_ENCODER,
	OBJECT_TYPE_DECODER,
    OBJECT_TYPE_MUXER,
    OBJECT_TYPE_DEMUXER,
};

enum
{
	DATA_TYPE_VIDEO = 0,
	DATA_TYPE_AUDIO,
	DATA_TYPE_FLUSH,
};

enum
{
	STREAM_PROC_OPENED_FILE = 0,
	STREAM_PROC_TRANSCODING_INFO,
	STREAM_PROC_FRAME_DATA,
	STREAM_PROC_COMPLETE,
	STREAM_PROC_PAUSE,
	STREAM_PROC_STOP,
};

typedef struct FilterContext
{
    AVFilterContext*	pBufferSrcCtx;
	AVFilterContext*	pBufferSinkCtx;
	AVFilterGraph*		pFilterGraph;
} FilterContext;

typedef struct _OPENED_FILE_INFO
{
	// opened video info for resampling
	UINT		uiPixFmt;
	UINT		uiWidth;
	UINT		uiHeight;
	UINT		uiFps;
	// opened audio info for resampling
	UINT		uiSampleFmt;
	UINT		uiChannelLayout;
	UINT		uiChannels;
	UINT		uiSampleRate;
	UINT		uiFrameSize;
} OPENED_FILE_INFO, *POPENED_FILE_INFO;

typedef struct _TRANSCODING_INFO
{
	// decode video info
	UINT		uiPixFmt;
	UINT		uiVideoCodecID;
	UINT		uiNum;			// time base numerator
	UINT		uiDen;			// time base denominator
	UINT		uiWidth;
	UINT		uiHeight;
	UINT		uiGopSize;
	UINT		uiVideoBitrate;
	// decode audio info
	UINT		uiSampleFmt;
	UINT		uiAudioCodecID;
	UINT		uiChannelLayout;
	UINT		uiChannels;
	UINT		uiSampleRate;
	UINT		uiFrameSize;
	UINT		uiAudioBitrate;
} TRANSCODING_INFO, *PTRANSCODING_INFO;

typedef struct FRAME_DATA
{
	UINT		uiFrameType;
	UINT		uiFrameNum;
	UINT		uiFrameSize;
	char		pData[0];
} FRAME_DATA, *PFRAME_DATA;

typedef struct FILTER_DESCR_DATA
{
    // Scale
    UINT        uiWidth;
    UINT        uiHeight;
} FILTER_DESCR_DATA, *PFILTER_DESCR_DATA;