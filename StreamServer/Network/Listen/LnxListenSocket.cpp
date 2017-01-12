#include "stdafx.h"
#include "LnxListenSocket.h"

CLnxListenSocket::CLnxListenSocket()
{
	m_hListenSocket		= INVALID_SOCKET;
	m_hAcceptThread		= NULL;
	m_bRunAcceptThread	= false;

	memset(&m_fdReads,		0,	sizeof(fd_set));
	memset(&m_fdCpyReads,	0,	sizeof(fd_set));

	m_SessionThreadMap.clear();
}

CLnxListenSocket::~CLnxListenSocket()
{
	//pthread_join(m_hAcceptThread, NULL);
	close(m_hListenSocket);

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
}

bool CLnxListenSocket::Init(int nPort)
{
	if (INVALID_SOCKET == m_hListenSocket)
	{
		close(m_hListenSocket);
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

bool CLnxListenSocket::Listen()
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
		int nRet				= pthread_create(&m_hAcceptThread, NULL, AcceptThread, this);
		if (0 == nRet)
		{
			m_bRunAcceptThread	= true;
		}
	}

	pthread_detach(m_hAcceptThread);

	return true;
}

void* CLnxListenSocket::AcceptThread(void* pParam)
{
	CLnxListenSocket* pThis	= (CLnxListenSocket*)pParam;
	if (NULL == pThis)
	{
		TraceLog("Parameter Error!");
		return 0;
	}

	char szSendBuf[_DEC_MAX_BUF_SIZE]	= {0, };
	char szRecvBuf[_DEC_MAX_BUF_SIZE]	= {0, };
	int  nFdMax							= 0;
	int  nFdNum							= 0;

	while (true == pThis->m_bRunAcceptThread)
	{
		int			hClientSock			= INVALID_SOCKET;
		SOCKADDR_IN	ClientAddr			= {0, };
		socklen_t	nClientAddrSize		= sizeof(ClientAddr);

		TIMEVAL		TimeOut				= {	5, 5000 };

		pThis->m_fdCpyReads				= pThis->m_fdReads;

		if (SOCKET_ERROR == (nFdNum = select(nFdMax + 1, &pThis->m_fdCpyReads, NULL, NULL, NULL)))
		{
			break;
		}

		if (0 == nFdNum)
		{
			continue;
		}

		for (int nIndex = 0; nIndex < nFdMax + 1; ++nIndex)
		{
			if (0 < FD_ISSET(nIndex, &pThis->m_fdCpyReads))
			{
				if (pThis->m_hListenSocket == nIndex)	// connection request
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
						
						if (nFdMax < hClientSock)
						{
							nFdMax = hClientSock;
						}
					}
				}
				else
				{
					SOCKET	ProcSocket			= pThis->m_fdReads.fd_array[nIndex];
					pThis->m_SessionThreadMapIt	= pThis->m_SessionThreadMap.find(ProcSocket);

					int		nReadLen			= read(ProcSocket, szRecvBuf, _DEC_MAX_BUF_SIZE);
					if (0 >= nReadLen)	// close client connect
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

	return NULL;
}