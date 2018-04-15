// StreamSourceTest.cpp
//

#include "stdafx.h"
#include "Common.h"
#include "StreamSource.h"

static int StreamCallbackFunc(void*, int, UINT, void*);

int main(int argc, char* argv[])
{
	OpenLogFile("StreamServer");
	
	if (4 > argc) 
    {
        TraceLog("Need More Argument - Usage: %s <cpu|vaapi|vdpau|dxva2|d3d11va> <input file[in ./Video file]> <true|false[create transcoding file]>", argv[0]);
        CloseLogFile();
        return -1;
    }
	
	TraceLog("Dev type = %s", argv[1]);
	TraceLog("Input file = %s", argv[2]);
	TraceLog("Create transcoding file = %s", argv[3]);

	/* register all formats and codecs */
	av_register_all();
	/* register all filters */
	avfilter_register_all();
	
	char* 	pszDevType 			= argv[1];	
	char* 	pszInputFile 		= argv[2];
	bool	bCreateTrscFile 	= (0 == strcmp(argv[3], "true")) ? true : false;
	
	if ( (NULL != pszDevType) && (0 != strcmp(pszDevType, "cpu")) )
	{
		g_nHwDevType = (int)av_hwdevice_find_type_by_name(argv[1]);
		g_nHwPixFmt = (int)HWAccel::FindFmtByHwType((const enum AVHWDeviceType)g_nHwDevType);
		TraceLog("Hw dev type = %d, pixel format = %s[%d]", g_nHwDevType, av_get_pix_fmt_name((AVPixelFormat)g_nHwPixFmt), g_nHwPixFmt);
	}

	CStreamSource* pStreamSource = new CStreamSource;
	pStreamSource->Init(pStreamSource, StreamCallbackFunc, pszInputFile, bCreateTrscFile);
	pStreamSource->Open();

	while (true)
	{
#ifdef _WIN32
		Sleep(1000);
#else
		usleep(1000000);
#endif		
	}
	
	CloseLogFile();

	return 0;
}

int StreamCallbackFunc(void* pObject, int nOpCode, UINT uiDataSize, void* pData)
{
	CStreamSource* pStreamSource = (CStreamSource*)pObject;
	if (NULL == pStreamSource)
	{
		return -1;
	}

	switch (nOpCode)
	{
	case STREAM_PROC_OPENED_FILE :
	{
		POPENED_FILE_INFO pInfo = (POPENED_FILE_INFO)pData;

		TraceLog("Pix Fmt = %d, Width = %d, Height = %d, FPS = %d, "
						"Sample Fmt = %d, Channel Layout = %d, Channes = %d, Sample Rate = %d, Frmae Size = %d",
						pInfo->uiPixFmt, pInfo->uiWidth, pInfo->uiHeight, pInfo->uiFps, 
						pInfo->uiSampleFmt, pInfo->uiChannelLayout, pInfo->uiChannels, pInfo->uiSampleRate, pInfo->uiFrameSize);

		pStreamSource->SetResolution(pInfo->uiWidth / 2, pInfo->uiHeight / 2, 0);
		pStreamSource->Start();
	}
	break;

	case STREAM_PROC_TRANSCODING_INFO :
	{		
		
	}
	break;

	case STREAM_PROC_FRAME_DATA :
	{

	}
	break;

	default:
		break;
	}

	return 0;
}
