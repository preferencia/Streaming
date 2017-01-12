#include "stdafx.h"
#include "WinListenSocket.h"

CWinListenSocket::CWinListenSocket()
{
	m_hListenSocket		= INVALID_SOCKET;
	m_hAcceptThread		= NULL;
	m_bRunAcceptThread	= false;

	memset(&m_fdReads,		0,	sizeof(fd_set));
	memset(&m_fdCpyReads,	0,	sizeof(fd_set));

	m_SessionThreadMap.clear();
}

CWinListenSocket::~CWinListenSocket()
{
	if (NULL != m_hAcceptThread)
	{
		m_bRunAcceptThread = false;
		WaitForSingleObject(m_hAcceptThread, INFINITE);
	}

	closesocket(m_hListenSocket);

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

	WSACleanup();
}

bool CWinListenSocket::Init(int nPort)
{
	WSADATA		wsaData			= {0, };
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		TraceLog("WSAStartup() error!");
		return false;
	}

	if (INVALID_SOCKET == m_hListenSocket)
	{
		closesocket(m_hListenSocket);
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

bool CWinListenSocket::Listen()
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

	if (NULL == m_hAcceptThread)
	{
		m_hAcceptThread			= (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
		if (NULL != m_hAcceptThread)
		{
			m_bRunAcceptThread	= true;
		}
	}

	return true;
}

unsigned int __stdcall CWinListenSocket::AcceptThread(void* pParam)
{
	CWinListenSocket* pThis	= (CWinListenSocket*)pParam;
	if (NULL == pThis)
	{
		TraceLog("Parameter Error!");
		return 0;
	}

	char szSendBuf[_DEC_MAX_BUF_SIZE]	= {0, };
	char szRecvBuf[_DEC_MAX_BUF_SIZE]	= {0, };
	int  nFdNum							= 0;

	while (true == pThis->m_bRunAcceptThread)
	{
		SOCKET		hClientSock			= INVALID_SOCKET;
		SOCKADDR_IN	ClientAddr			= {0, };
		socklen_t	nClientAddrSize		= sizeof(ClientAddr);

		TIMEVAL		TimeOut				= {	5, 5000 };

		pThis->m_fdCpyReads				= pThis->m_fdReads;

		if (SOCKET_ERROR == (nFdNum = select(0, &pThis->m_fdCpyReads, NULL, NULL, NULL)))
		{
			break;
		}

		if (0 == nFdNum)
		{
			continue;
		}

		for (int nIndex = 0; nIndex < pThis->m_fdReads.fd_count; ++nIndex)
		{
			if (0 < FD_ISSET(pThis->m_fdReads.fd_array[nIndex], &pThis->m_fdCpyReads))
			{
				if (pThis->m_hListenSocket == pThis->m_fdReads.fd_array[nIndex])	// connection request
				{
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

						TraceLog("[%d] %d Session Connect", pThis->m_SessionThreadMap.size(), hClientSock);

						FD_SET(pSessionThread->GetSocket()->GetConSocket(), &pThis->m_fdReads);
					}
				}
				else
				{
					SOCKET	ProcSocket			= pThis->m_fdReads.fd_array[nIndex];
					pThis->m_SessionThreadMapIt	= pThis->m_SessionThreadMap.find(ProcSocket);

					int		nRecvLen			= recv(ProcSocket, szRecvBuf, _DEC_MAX_BUF_SIZE, 0);
					if (0 >= nRecvLen)	// close client connect
					{
						TraceLog("%d Session Disconnect", ProcSocket);

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

	CloseHandle(pThis->m_hAcceptThread);
	pThis->m_hAcceptThread	= NULL;

	return 0;
}