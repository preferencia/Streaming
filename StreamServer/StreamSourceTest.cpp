// SimpleServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Common.h"
#include "StreamSource.h"

static int StreamCallbackFunc(void*, int, UINT, void*);

int main(int argc, char* argv[])
{
	if (2 > argc)
	{
		TraceLog("Need More Argument!");
		return -1;
	}

	OpenLogFile("StreamServer");

	/* register all formats and codecs */
	av_register_all();
	/* register all filters */
	avfilter_register_all();

	CStreamSource* pStreamSource = new CStreamSource;
	pStreamSource->Init(pStreamSource, StreamCallbackFunc, "Test.mp4", true);
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

		pStreamSource->SetResolution(1280, 720, 0);
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