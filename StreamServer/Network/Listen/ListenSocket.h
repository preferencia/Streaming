#pragma once

#ifdef _WINDOWS
#include "WinListenSocket.h"
typedef CWinListenSocket CListenSocket;
#else
#include "LnxListenSocket.h"
typedef CLnxListenSocket CListenSocket;
#endif