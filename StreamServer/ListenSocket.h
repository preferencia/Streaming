#pragma once

#include "SessionThread.h"
#include <map>

typedef map<SOCKET, CSessionThread*>				SessionThreadMap;
typedef map<SOCKET, CSessionThread*>::iterator	SessionThreadMapIt;

class CListenSocket
{
public:
	CListenSocket();
	virtual ~CListenSocket();

	bool	Init(int nPort);
	bool	Listen();

private:
#ifdef _WINDOWS
	static unsigned int __stdcall	AcceptThread(void* pParam);
#else
	static void*					AcceptThread(void* pParam);
#endif

private:
	SOCKET				m_hListenSocket;
#ifdef _WINDOWS
	HANDLE				m_hAcceptThread;
#else
	pthread_t			m_hAcceptThread;
#endif

	bool				m_bRunAcceptThread;

	fd_set				m_fdReads;
	fd_set				m_fdCpyReads;

	SessionThreadMap		m_SessionThreadMap;
	SessionThreadMapIt	m_SessionThreadMapIt;
};