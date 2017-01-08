#pragma once

enum 
{
    SVC_CODE_START  = 1000,

	// for list
	SVC_GET_LIST	= 1100,

	// for file open and close, data
	SVC_FILE_OPEN	= 1200,
	SVC_SELECT_RESOLUTION,
	SVC_FRAME_DATA,
	SVC_FILE_CLOSE,

	// for file play
	SVC_SET_PLAY_STATUS,

    SVC_CODE_END,
};

#pragma pack( push, 1 )

typedef struct _VS_HEADER
{
    UINT    uiSvcCode;                          // 서비스 코드
    UINT    uiErrCode;                          // 에러 코드(SM은 0으로 Setting)
    UINT    uiDataLen;                          // 데이터 길이
    UINT    uiChecksum;                         // 헤더 검증용 체크섬
} VS_HEADER, *PVS_HEADER;

#define MAKE_VS_HEADER(header, svc, err, len) \
{\
    (header)->uiSvcCode     = svc;\
    (header)->uiErrCode     = err;\
    (header)->uiDataLen     = len;\
    (header)->uiChecksum    = svc ^ err ^ len;\
}

// Video List
typedef struct _VideoListData
{
	int nFileNameLen;
	char pszFileName[0];
} VideoListData, *PVideoListData;

typedef struct VS_LIST_C2S
{
} VS_LIST_REQ, *PVS_LIST_REQ;

typedef struct VS_LIST_S2C
{
	int			nCnt;
	char		pVideoListData[0];
} VS_LIST_REP, *PVS_LIST_REP;

typedef struct VS_FILE_OPEN_C2S
{
	char		pszFileName[0];
} VS_FILE_OPEN_REQ, *PVS_FILE_OPEN_REQ;

typedef struct VS_FILE_OPEN_S2C
{
	// opened video info for resampling
	UINT		uiPixFmt;
	UINT		uiWidth;
	UINT		uiHeight;
	// opened audio info for resampling
	UINT		uiSampleFmt;
	UINT		uiChannelLayout;
	UINT		uiChannels;
	UINT		uiSampleRate;
	UINT		uiFrameSize;
} VS_FILE_OPEN_REP, *PVS_FILE_OPEN_REP;

typedef struct VS_SELECT_RESOLUTION_C2S
{
	UINT		uiWidth;
	UINT		uiHeight;
} VS_SELECT_RESOLUTION_REQ, *PVS_SELECT_RESOLUTION_REQ;

typedef struct VS_SELECT_RESOLUTION_S2C
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
} VS_SELECT_RESOLUTION_REP, *PVS_SELECT_RESOLUTION_REP;

typedef struct VS_FRAME_DATA
{
	UINT		uiFrameType;
	UINT		uiFrameNum;
	UINT		uiFrameSize;
	char		pFrameData[0];
} VS_FRAME_DATA, *PVS_FRAME_DATA;

typedef struct VS_SET_PLAY_STATUS
{
	UINT		uiPlayStatus;	// 0 : Play, 1 : Pause, 2 : Stop
} VS_SET_PLAY_STATUS, *PVS_SET_PLAY_STATUS;