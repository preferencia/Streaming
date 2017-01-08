#include "StdAfx.h"
#include "ConnectSocket.h"

CConnectSocket::CConnectSocket(void)
{
	m_pSocketThread		= NULL;
	m_hConnectSocket	= INVALID_SOCKET;
}

CConnectSocket::~CConnectSocket(void)
{
#ifdef _WINDOWS
	closesocket(m_hConnectSocket);
#else
	close(m_hConnectSocket);
#endif

	m_hConnectSocket		= INVALID_SOCKET;
}

bool CConnectSocket::Send(char* pData, int nDataLen)
{
#ifdef _WINDOWS
	send(m_hConnectSocket, pData, nDataLen, 0);
#else
	write(m_hConnectSocket, pData, nDataLen);
#endif

	return true;
}
