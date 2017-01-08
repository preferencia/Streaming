
// stdafx.cpp : ǥ�� ���� ���ϸ� ��� �ִ� �ҽ� �����Դϴ�.
// WindowClient.pch�� �̸� �����ϵ� ����� �˴ϴ�.
// stdafx.obj���� �̸� �����ϵ� ���� ������ ���Ե˴ϴ�.

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