#include "stdafx.h"
#include "ConnectSocket.h"

CConnectSocket::CConnectSocket(void)
{
	m_pSocketThread	= NULL;
	m_hSocket		= INVALID_SOCKET;
}

CConnectSocket::~CConnectSocket(void)
{
#ifdef _WIN32
	closesocket(m_hSocket);
#else
	close(m_hSocket);
#endif

	m_hSocket = INVALID_SOCKET;
}

UINT CConnectSocket::Send(char* pData, int nDataLen)
{
	if ((NULL == pData) || (0 >= nDataLen))
	{
		TraceLog("Data is wrong!");
		return false;
	}

	if (INVALID_SOCKET == m_hSocket)
	{
		TraceLog("Session socket is invalid!");
		return false;
	}

	int nSendBytes = 0;

#ifdef _WIN32
	nSendBytes = send(m_hSocket, pData, nDataLen, 0);
#else
	nSendBytes = write(m_hSocket, pData, nDataLen);
#endif

	return nSendBytes;
}

UINT CConnectSocket::ProcessReceive(char* lpBuf, UINT uiDataLen)
{
	if ((NULL == lpBuf) || (0 >= uiDataLen))
	{
		TraceLog("Data is wrong!");
		return -1;
	}

	PVS_HEADER pHeader = (PVS_HEADER)lpBuf;
	TraceLog("CConnectSocket::ProcessReceive - Header Info - SVC = %d, Err = %d, Data Len = %d, Checksum = %d", 
             pHeader->uiSvcCode, pHeader->uiErrCode, pHeader->uiDataLen, pHeader->uiChecksum);

	UINT uiChecksum = pHeader->uiSvcCode ^ pHeader->uiErrCode ^ pHeader->uiDataLen;
	if (pHeader->uiChecksum != uiChecksum)
	{
		TraceLog("Checksum is invalid - Calc = %ld, recv = %ld", uiChecksum, pHeader->uiChecksum);
		return -2;
	}

	unsigned int uiRecvSvcCode = pHeader->uiSvcCode;
	unsigned int uiRecvErrCode = pHeader->uiErrCode;
	unsigned int uiRecvDataLen = pHeader->uiDataLen;
	unsigned int uiHeaderSize = sizeof(VS_HEADER);

	if (0 > uiRecvErrCode)
	{
		return -3;
	}

	if ((0 < uiRecvDataLen) && (uiDataLen < (uiHeaderSize + uiRecvDataLen)))
	{
		TraceLog("Need More Data - Total Data Len = %ld, Receive Data Len = %ld", uiHeaderSize + uiRecvDataLen, uiDataLen);
		return (uiHeaderSize + uiRecvDataLen) - uiDataLen;
	}

	char*   pProcData = NULL;
	bool    bDataLenOver = false;
	UINT    uiInputDataLen = uiDataLen;
	UINT    uiOverDataLen = 0;

	// exist extra data
	if (uiInputDataLen > (uiHeaderSize + uiRecvDataLen))
	{
		bDataLenOver = true;
		uiOverDataLen = uiInputDataLen - (uiHeaderSize + uiRecvDataLen);
		pProcData = new char[(uiHeaderSize + uiRecvDataLen)];
		memcpy(pProcData, lpBuf, (uiHeaderSize + uiRecvDataLen));
	}
	else
	{
		pProcData = lpBuf;
	}

	if (0 > ProcSvcData(uiRecvSvcCode, uiRecvDataLen, pProcData + sizeof(VS_HEADER)))
	{
		if (true == bDataLenOver)
		{
			SAFE_DELETE(pProcData);
		}

		TraceLog("Process svc data failed.");
		return -4;
	}

	if (true == bDataLenOver)
	{
		SAFE_DELETE(pProcData);
		lpBuf = (char*)(lpBuf + (uiHeaderSize + uiRecvDataLen));
		return ProcessReceive(lpBuf, uiOverDataLen);
	}

	return 0;
}

int CConnectSocket::ProcSvcData(int nSvcCode, int nSvcDataLen, char* lpBuf)
{
	if ((SVC_CODE_START >= nSvcCode) || (SVC_CODE_END <= nSvcCode))
	{
		TraceLog("Svc code is wrong!");
		return -1;
	}

	return 0;
}