// stdafx.cpp : ǥ�� ���� ���ϸ� ��� �ִ� �ҽ� �����Դϴ�.
// SimpleServer.pch�� �̸� �����ϵ� ����� �˴ϴ�.
// stdafx.obj���� �̸� �����ϵ� ���� ������ ���Ե˴ϴ�.

#include "stdafx.h"

// TODO: �ʿ��� �߰� �����
// �� ������ �ƴ� STDAFX.H���� �����մϴ�.

FILE* 	g_fpLog 				    			= NULL;
int 		g_nHwPixFmt		        		= -1;
int 		g_nHwDevType	        		= -1;
bool    g_bCheckHwPixFmt        	= false;
bool    g_bDecodeOnlyHwPixFmt  = false;

void OpenLogFile(char* pszProcessName)
{
	if ((NULL == pszProcessName) || (0 == strlen(pszProcessName)))
	{
		return;
	}

	time_t		_time	= time(NULL);
	struct tm*	pTm = localtime(&_time);

	if (NULL == pTm)
	{
		return;
	}

	char szLogFile[1024] = { 0, };
	sprintf(szLogFile, "./Log/StreamServer_%04d%02d%02d.log", pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday);

	g_fpLog = fopen(szLogFile, "a");
}

void CloseLogFile()
{
	if (NULL != g_fpLog)
	{
		fclose(g_fpLog);
		g_fpLog = NULL;
	}
}

void TraceLog(const char* pszFmt, ...)
{
	if (NULL != pszFmt)
	{
		char		szLog[4096] = { 0, };
		int			nPos		= 0;

		time_t		_time		= time(NULL);
		struct tm*	pTm			= localtime(&_time);

		if (NULL != pTm)
		{
			nPos = sprintf(szLog, "[%02d:%02d:%02d] ", pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
		}

		va_list args;
		va_start(args, pszFmt);
		vsprintf(szLog + nPos, pszFmt, args);
		va_end(args);

        fprintf(stderr, szLog);
		fprintf(stderr, "\n");

		if (NULL != g_fpLog)
		{
			fprintf(g_fpLog, "%s\n", szLog);
		}

#ifdef _WIN32
		OutputDebugString(szLog);
		OutputDebugString("\n");
#endif
	}
}
