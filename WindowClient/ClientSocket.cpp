#include "stdafx.h"
#include "ClientSocket.h"
#include "SocketThread.h"

CClientSocket::CClientSocket()
{
	m_ProcCallbackFunc	= NULL;
	m_pObject			= NULL;
	m_bIsConnected      = false;
}

CClientSocket::~CClientSocket()
{
#ifdef _WINDOWS
	closesocket(m_hConnectSocket);
#else
	close(m_hConnectSocket);
#endif

	m_hConnectSocket	= INVALID_SOCKET;
	m_ProcCallbackFunc	= NULL;
	m_pObject			= NULL;
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

	m_hConnectSocket	= socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == m_hConnectSocket)
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

	if (SOCKET_ERROR == connect(m_hConnectSocket, (SOCKADDR*)&ServAddr, nServAddrSize))
	{
		TraceLog("connect() error!");
		return false;
	}

	m_bIsConnected = true;

	return true;
}

void CClientSocket::SetProcCallbakcInfo(void* pObject, ProcCallback ProcCallbackFunc)
{	
	m_pObject			= pObject;
	m_ProcCallbackFunc	= ProcCallbackFunc;	
}

bool CClientSocket::Send(char* pData, int nDataLen)
{
	if ((NULL == pData) || (0 >= nDataLen))
	{
		TraceLog("Data is wrong!");
		return false;
	}

	if (INVALID_SOCKET == m_hConnectSocket)
	{
		TraceLog("Client socket is invalid!");
		return false;
	}

	return CConnectSocket::Send(pData, nDataLen);
}

UINT CClientSocket::ProcessReceive(char* lpBuf, int nDataLen)
{
	if ((NULL == lpBuf) || (0 >= nDataLen))
	{
		TraceLog("Data is wrong!");
		return false;
	}

	if (INVALID_SOCKET == m_hConnectSocket)
	{
		TraceLog("Client socket is invalid!");
		return false;
	}

	PVS_HEADER pHeader      = (PVS_HEADER)lpBuf;
	TraceLog("Header Info - SVC = %d, Err = %d, Data Len = %d, Checksum = %d", pHeader->uiSvcCode, pHeader->uiErrCode, pHeader->uiDataLen, pHeader->uiChecksum);

    UINT uiChecksum = pHeader->uiSvcCode ^ pHeader->uiErrCode ^ pHeader->uiDataLen;
    if (pHeader->uiChecksum != uiChecksum)
    {
        TraceLog("Checksum is invalid - Calc = %d, recv = %d", uiChecksum, pHeader->uiChecksum);
		return false;
    }

    unsigned int uiRecvSvcCode      = pHeader->uiSvcCode;
    unsigned int uiRecvErrCode      = pHeader->uiErrCode;
    unsigned int uiRecvDataLen      = pHeader->uiDataLen;
    unsigned int uiHeaderSize		= sizeof(VS_HEADER);
    if ((0 < uiRecvDataLen) && (nDataLen < (uiHeaderSize + uiRecvDataLen)))
    {
		TraceLog("Need More Data - Total Data Len = %d, Receive Data Len = %d", uiHeaderSize + uiRecvDataLen, nDataLen);
        return (uiHeaderSize + uiRecvDataLen) - nDataLen;
    }

	if (0 > uiRecvErrCode)
	{
		return -1;
	}

	char*   pProcData = NULL;
	BOOL    bDataLenOver = FALSE;
	int     nInputDataLen = nDataLen;
	int     nOverDataLen = 0;

	// exist extra data
	if (nInputDataLen > (uiHeaderSize + uiRecvDataLen))
	{
		bDataLenOver = TRUE;
		nOverDataLen = nInputDataLen - (uiHeaderSize + uiRecvDataLen);
		pProcData = new char[(uiHeaderSize + uiRecvDataLen)];
		memcpy(pProcData, lpBuf, (uiHeaderSize + uiRecvDataLen));
	}
	else
	{
		pProcData = lpBuf;
	}

	if ((NULL != m_pObject) && (NULL != m_ProcCallbackFunc))
	{
		m_ProcCallbackFunc(m_pObject, uiRecvSvcCode, uiRecvDataLen, pProcData + uiHeaderSize);
	}

	if (TRUE == bDataLenOver)
	{
		SAFE_DELETE(pProcData);
		lpBuf = (char*)(lpBuf + (uiHeaderSize + uiRecvDataLen));
		return ProcessReceive(lpBuf, nOverDataLen);
	}

	return 0;
}