#pragma once

#include "SessionThread.h"
#include <map>

typedef map<SOCKET, CSessionThread*>			SessionThreadMap;
typedef map<SOCKET, CSessionThread*>::iterator	SessionThreadMapIt;

class CWinListenSocket
{
public:
	CWinListenSocket();
	virtual ~CWinListenSocket();

	bool	Init(int nPort);
	bool	Listen();

private:
	static unsigned int __stdcall	AcceptThread(void* pParam);

private:
	SOCKET				m_hListenSocket;
	HANDLE				m_hAcceptThread;

	bool				m_bRunAcceptThread;

	fd_set				m_fdReads;
	fd_set				m_fdCpyReads;

	SessionThreadMap	m_SessionThreadMap;
	SessionThreadMapIt	m_SessionThreadMapIt;
};