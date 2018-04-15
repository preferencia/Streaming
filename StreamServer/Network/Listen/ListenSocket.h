#pragma once

#include "SessionThread.h"

typedef map<int, CSessionThread*>			SessionThreadMap;
typedef map<int, CSessionThread*>::iterator	SessionThreadMapIt;

class CListenSocket
{
public:
	CListenSocket();
	virtual ~CListenSocket();

	bool	Init(int nPort);
	void	Close();
	bool	Listen();   

private:
    int     CreateSessionThread();

private:
#ifdef _WIN32
	static unsigned int __stdcall	AcceptThread(void* lpParam);
#else
	static void*					AcceptThread(void* lpParam);
#endif

private:
	SOCKET				m_hListenSocket;

#ifdef _WIN32
	HANDLE				m_hAcceptThread;
#else
	pthread_t			m_hAcceptThread;
#endif

	bool				m_bRunAcceptThread;

    int                 m_nThreashold;

	SOCKADDR_IN			m_ServAddr;

	SessionThreadMap	m_SessionThreadMap;
	SessionThreadMapIt	m_SessionThreadMapIt;
};