#include "stdafx.h"
#include "ClientSocket.h"
#include "SocketThread.h"

CClientSocket::CClientSocket()
{
	m_bIsConnected      = false;
}

CClientSocket::~CClientSocket()
{
#ifdef _WINDOWS
	closesocket(m_hSocket);
#else
	close(m_hSocket);
#endif

	m_hSocket			= INVALID_SOCKET;
	m_bIsConnected      = false;

#ifdef _WINDOWS
	WSACleanup();
#endif
}

bool CClientSocket::Init(char* pszServerIP, int nServerPort)
{
	if ((NULL == pszServerIP) || (0 >= nServerPort))
	{
		TraceLog("Server info is wrong.");
		return false;
	}

#ifdef _WINDOWS
	WSADATA		wsaData			= {0, };

	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		TraceLog("WSAStartup() error!");
		return false;
	}
#endif

	m_hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == m_hSocket)
	{
		TraceLog("socket() error!");
		return false;
	}

	return Connect(pszServerIP, nServerPort);
}

bool CClientSocket::Connect(char* pszServerIP, int nServerPort)
{
	SOCKADDR_IN	ServAddr		= {0, };
	int			nServAddrSize	= sizeof(ServAddr);

	ServAddr.sin_family			= AF_INET;
	ServAddr.sin_addr.s_addr	= inet_addr(pszServerIP);
	ServAddr.sin_port			= htons(nServerPort);

	if (SOCKET_ERROR == connect(m_hSocket, (SOCKADDR*)&ServAddr, nServAddrSize))
	{
		TraceLog("connect() error!");
		return false;
	}	

	if (0 > ((CSocketThread*)m_pSocketThread)->ActiveConnectSocket(m_hSocket, &ServAddr))
	{
		return false;
	}

	m_bIsConnected = true;
	return m_bIsConnected;
}

int CClientSocket::ProcSvcData(int nSvcCode, int nSvcDataLen, char* lpBuf)
{
	int nRet = 0;

	if (0 > (nRet = CConnectSocket::ProcSvcData(nSvcCode, nSvcDataLen, lpBuf)))
	{
		TraceLog("Error Param checked - err = [%d]", nRet);
		return nRet;
	}

	if (NULL != m_pSocketThread)
	{
		((CSocketThread*)m_pSocketThread)->PushSvcData(nSvcCode, nSvcDataLen, lpBuf);
	}

	return 0;
}