
// stdafx.cpp : 표준 포함 파일만 들어 있는 소스 파일입니다.
// WindowClient.pch는 미리 컴파일된 헤더가 됩니다.
// stdafx.obj에는 미리 컴파일된 형식 정보가 포함됩니다.

#include "stdafx.h"

void TraceLog(const char* pszFmt, ...)
{
	if (NULL != pszFmt)
	{
		char szLog[4096] = { 0, };

		va_list args;
		va_start(args, pszFmt);
		vsprintf(szLog, pszFmt, args);
		va_end(args);

#ifdef _WINDOWS
		OutputDebugString(szLog);
		OutputDebugString("\n");
#endif
		cout << szLog << endl;
	}
}