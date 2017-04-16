#include "stdafx.h"
#include "SessionThread.h"

CSessionThread::CSessionThread()
{
}

CSessionThread::~CSessionThread(void)
{
	StopSocketThread();
}

bool CSessionThread::InitSocketThread()
{
    if (false == CSocketThread::InitSocketThread())
	{
        TraceLog("CSessionThread::InitSocketThread error");
		return false;
	}

	return true;
}

int CSessionThread::ActiveConnectSocket(SOCKET hConnectSocket, SOCKADDR_IN* pConnectAdr)
{
    if ((INVALID_SOCKET == hConnectSocket) || (NULL == pConnectAdr))
    {
        return -1;
    }

#ifdef _WIN32
	if (NULL == m_hComPort)
	{
		return -2;
	}

    DWORD				dwRecvBytes     = 0;
	DWORD				dwFlags	        = 0;
    PSOCKET_INFO_DATA   pSocketInfo     = NULL;
    POVERLAPPED_IO_DATA pOvlpInfo	    = NULL;
    CSessionSocket*     pSessionSocket  = NULL;

    pSocketInfo = new SOCKET_INFO_DATA;
	if (NULL == pSocketInfo)
	{
		closesocket(hConnectSocket);
        return -3;
	}

	memset(pSocketInfo, 0, sizeof(SOCKET_INFO_DATA));
	pSocketInfo->hSessionSocket         = hConnectSocket;
	memcpy(&pSocketInfo->SessionAddr, pConnectAdr, sizeof(SOCKADDR_IN));

	CreateIoCompletionPort((HANDLE)hConnectSocket, m_hComPort, (ULONG_PTR)pSocketInfo, 0);

    pOvlpInfo   = new OVERLAPPED_IO_DATA;
    if (NULL == pOvlpInfo)
    {
        closesocket(hConnectSocket);
        return -4;
    }

    memset(pOvlpInfo, 0, sizeof(OVERLAPPED_IO_DATA));
    pOvlpInfo->nRWMode      = _DEC_MODE_READ;
    pOvlpInfo->wsaBuf.len   = _DEC_MAX_BUF_SIZE;
    pOvlpInfo->wsaBuf.buf   = new char[pOvlpInfo->wsaBuf.len];
    memset(pOvlpInfo->wsaBuf.buf, 0, pOvlpInfo->wsaBuf.len);
    
    pSessionSocket  = new CSessionSocket();
    if (NULL == pSessionSocket)
    {
        return -5;
    }

    pSessionSocket->Attach(hConnectSocket);
    pSessionSocket->SetSocketThread(this);
	m_ConnectSocketMap[hConnectSocket] = pSessionSocket;

    WSARecv(pSocketInfo->hSessionSocket, &pOvlpInfo->wsaBuf, 1, &dwRecvBytes, &dwFlags, &pOvlpInfo->Ovlp, NULL);
#else
	if (NULL == m_pEpollEvents)
	{
		m_pEpollEvents	= new epoll_event[_DEC_EPOLL_SIZE];
		memset(m_pEpollEvents, 0, sizeof(epoll_event) * _DEC_EPOLL_SIZE);
		
		m_nEpollFd		= epoll_create(_DEC_EPOLL_SIZE);
	}

	// Connect socket set to nonblocking mode
	int					nFlag			= fcntl(hConnectSocket, F_GETFL, 0);
	fcntl(hConnectSocket, F_SETFL, nFlag | O_NONBLOCK);

	epoll_event			EpollEvent		= { 0, };
	EpollEvent.events					= EPOLLIN | EPOLLET;
	EpollEvent.data.fd					= hConnectSocket;

	CSessionSocket*     pSessionSocket	= new CSessionSocket();
	if (NULL == pSessionSocket)
	{
		return -2;
	}

	pSessionSocket->Attach(hConnectSocket);
	pSessionSocket->SetSocketThread(this);
	m_ConnectSocketMap[hConnectSocket] = pSessionSocket;

	epoll_ctl(m_nEpollFd, EPOLL_CTL_ADD, hConnectSocket, &EpollEvent);
#endif

    return 0;
}

CConnectSocket* CSessionThread::GetConSocket(SOCKET hConnectSocket)
{
    if (INVALID_SOCKET == hConnectSocket)
    {
        return NULL;
    }

	m_ConnectSocketMapIt = m_ConnectSocketMap.find(hConnectSocket);
	if (m_ConnectSocketMapIt == m_ConnectSocketMap.end())
	{
	    return NULL;
	}

	return m_ConnectSocketMapIt->second;
}