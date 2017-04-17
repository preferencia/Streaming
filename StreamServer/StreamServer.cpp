// SimpleServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Common.h"
#include "ListenSocket.h"

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

	CListenSocket* pListenSocket = new CListenSocket();
	if (NULL != pListenSocket)
	{
		if (true == pListenSocket->Init(atoi(argv[1])))
		{
			if (true == pListenSocket->Listen())
			{
				TraceLog("If want to quit, press q or Q and Enter : ");

				while (1)
				{
					int nInput = getchar();
					if (('q' == nInput) || ('Q' == nInput))
					{
						TraceLog("Enter quit signal[%c]", nInput);

						pListenSocket->Close();
#ifdef _WIN32
					    Sleep(10);
#else
					    usleep(10000);
#endif
						break;
					}
#ifdef _WIN32
					Sleep(10);
#else
					usleep(10000);
#endif
				}
			}			
		}

		SAFE_DELETE(pListenSocket);
	}

	CloseLogFile();

	return 0;
}