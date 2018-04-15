// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//

#pragma once

#include "targetver.h"


// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <tchar.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <list>
#include <map>
#include <time.h>

#include "CommonDef.h"
#include "Common_Protocol.h"

using namespace std;

#if defined(_WIN32) && !defined(snprintf)
#define snprintf _snprintf
#endif

#define _USE_FILTER_GRAPH

extern int 	g_nHwPixFmt;
extern int 	g_nHwDevType;
extern bool 	g_bCheckHwPixFmt;
extern bool 	g_bDecodeOnlyHwPixFmt;

extern void 	OpenLogFile(char* pszProcessName);
extern void 	CloseLogFile();
extern void 	TraceLog(const char* pszFmt, ...);
