#include "stdafx.h"
#include "ListenSocket.h"

CListenSocket::CListenSocket()
{
	m_hListenSocket		= INVALID_SOCKET;
	m_hAcceptThread		= NULL;
	m_bRunAcceptThread	= false;

	memset(&m_fdReads,		0,	sizeof(fd_set));
	memset(&m_fdCpyReads,	0,	sizeof(fd_set));

	m_SessionThreadMap.clear();
}

CListenSocket::~CListenSocket()
{
#ifdef _WINDOWS
	if (NULL != m_hAcceptThread)
	{
		m_bRunAcceptThread = false;
		//while (NULL == m_hAcceptThread)
		//{
		//	Sleep(100);
		//}
		WaitForSingleObject(m_hAcceptThread, INFINITE);
	}

	closesocket(m_hListenSocket);
#else
	//pthread_join(m_hAcceptThread, NULL);
	close(m_hListenSocket);
#endif

	m_hListenSocket		= INVALID_SOCKET;

	if (0 < m_SessionThreadMap.size())
	{
		for (m_SessionThreadMapIt = m_SessionThreadMap.begin(); m_SessionThreadMapIt != m_SessionThreadMap.end(); ++m_SessionThreadMapIt)
		{
			CSessionThread* pSessionThread = m_SessionThreadMapIt->second;
			SAFE_DELETE(pSessionThread);
		}

		m_SessionThreadMap.clear();
	}

#ifdef _WINDOWS
	WSACleanup();
#endif
}

bool CListenSocket::Init(int nPort)
{
#ifdef _WINDOWS
	WSADATA		wsaData			= {0, };
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		TraceLog("WSAStartup() error!");
		return false;
	}
#endif

	if (INVALID_SOCKET == m_hListenSocket)
	{
#ifdef _WINDOWS
		closesocket(m_hListenSocket);
#else
		close(m_hListenSocket);
#endif
	}

	m_hListenSocket	= socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == m_hListenSocket)
	{
		TraceLog("socket() error!");
		return false;
	}

	SOCKADDR_IN	ServAddr		= {0, };
	ServAddr.sin_family			= AF_INET;
	ServAddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	ServAddr.sin_port			= htons(nPort);

	if (SOCKET_ERROR == bind(m_hListenSocket, (SOCKADDR*)&ServAddr, sizeof(ServAddr)))
	{
		TraceLog("bind() error!");
		return false;
	}

	return true;
}

bool CListenSocket::Listen()
{
	if (INVALID_SOCKET == m_hListenSocket)
	{
		TraceLog("listen socket is invalid!");
		return false;
	}

	if (SOCKET_ERROR == listen(m_hListenSocket, 5))
	{
		TraceLog("listen() error!");
		return false;
	}

	FD_ZERO(&m_fdReads);
	FD_SET(m_hListenSocket, &m_fdReads);

#ifdef _WINDOWS
	if (NULL == m_hAcceptThread)
	{
		m_hAcceptThread			= (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
		if (NULL != m_hAcceptThread)
		{
			m_bRunAcceptThread	= true;
		}
	}
#else
	if (NULL == m_hAcceptThread)
	{
		int nRet				= pthread_create(&m_hAcceptThread, NULL, AcceptThread, this);
		if (0 == nRet)
		{
			m_bRunAcceptThread	= true;
		}
	}

	pthread_detach(m_hAcceptThread);
#endif

	return true;
}

#ifdef _WINDOWS
unsigned int __stdcall	CListenSocket::AcceptThread(void* pParam)
#else
void*					CListenSocket::AcceptThread(void* pParam)
#endif
{
	CListenSocket* pThis	= (CListenSocket*)pParam;
	if (NULL == pThis)
	{
		TraceLog("Parameter Error!");
		return 0;
	}

	char szSendBuf[_DEC_MAX_BUF_SIZE]	= {0, };
	char szRecvBuf[_DEC_MAX_BUF_SIZE]	= {0, };
#ifndef _WINDOWS
	int nFdMax							= 0;
#endif
	int nFdNum							= 0;

	while (true == pThis->m_bRunAcceptThread)
	{
#ifdef _WINDOWS
		SOCKET		hClientSock			= INVALID_SOCKET;
#else
		int			hClientSock			= INVALID_SOCKET;
#endif
		SOCKADDR_IN	ClientAddr			= {0, };
		socklen_t	nClientAddrSize		= sizeof(ClientAddr);

		TIMEVAL		TimeOut				= {	5, 5000 };

		pThis->m_fdCpyReads				= pThis->m_fdReads;

#ifdef _WINDOWS
		if (SOCKET_ERROR == (nFdNum = select(0, &pThis->m_fdCpyReads, NULL, NULL, NULL)))
#else
		if (SOCKET_ERROR == (nFdNum = select(nFdMax + 1, &pThis->m_fdCpyReads, NULL, NULL, NULL)))
#endif
		{
			break;
		}

		if (0 == nFdNum)
		{
			continue;
		}

#ifdef _WINDOWS
		for (int nIndex = 0; nIndex < pThis->m_fdReads.fd_count; ++nIndex)
		{
			if (0 < FD_ISSET(pThis->m_fdReads.fd_array[nIndex], &pThis->m_fdCpyReads))
			{
				if (pThis->m_hListenSocket == pThis->m_fdReads.fd_array[nIndex])	// connection request
				{
#else
		for (int nIndex = 0; nIndex < nFdMax + 1; ++nIndex)
		{
			if (0 < FD_ISSET(nIndex, &pThis->m_fdCpyReads))
			{
				if (pThis->m_hListenSocket == nIndex)	// connection request
				{
#endif
					hClientSock = accept(pThis->m_hListenSocket, (SOCKADDR*)&ClientAddr, &nClientAddrSize);
					if (INVALID_SOCKET == hClientSock)
					{
						cout << "accept() error!" << endl;
						continue;
					}

					CSessionThread* pSessionThread = new CSessionThread();
					if (NULL != pSessionThread)
					{						
						pSessionThread->SetSocket(hClientSock);
						pSessionThread->InitSocketThread();
						pThis->m_SessionThreadMap[hClientSock] = pSessionThread;

						TraceLog("%d Session Connect\n", pThis->m_SessionThreadMap.size() + 1);

						FD_SET(pSessionThread->GetSocket()->GetConSocket(), &pThis->m_fdReads);
#ifndef _WINDOWS
						if (nFdMax < hClientSock)
						{
							nFdMax = hClientSock;
						}
#endif
					}
				}
				else
				{
					SOCKET	ProcSocket			= pThis->m_fdReads.fd_array[nIndex];
					pThis->m_SessionThreadMapIt	= pThis->m_SessionThreadMap.find(ProcSocket);

#ifdef _WINDOWS
					int		nRecvLen			= recv(ProcSocket, szRecvBuf, _DEC_MAX_BUF_SIZE, 0);
					if (0 >= nRecvLen)	// close client connect
#else
					int		nReadLen			= read(ProcSocket, szRecvBuf, _DEC_MAX_BUF_SIZE);
					if (0 >= nReadLen)	// close client connect
#endif
					{						
						FD_CLR(ProcSocket, &pThis->m_fdReads);
						
						if (pThis->m_SessionThreadMap.end() != pThis->m_SessionThreadMapIt)
						{
							CSessionThread* pSessionThread = pThis->m_SessionThreadMapIt->second;
							SAFE_DELETE(pSessionThread);
							pThis->m_SessionThreadMap.erase(pThis->m_SessionThreadMapIt);
						}
					}
					else
					{
						if (pThis->m_SessionThreadMap.end() != pThis->m_SessionThreadMapIt)
						{
							CSessionThread* pSessionThread = pThis->m_SessionThreadMapIt->second;
							if (NULL != pSessionThread)
							{
								// Process Receive
							}
						}
					}
				}				
			}
		}
	}

#ifdef _WINDOWS
	CloseHandle(pThis->m_hAcceptThread);
	pThis->m_hAcceptThread	= NULL;
#endif

	return 0;
}