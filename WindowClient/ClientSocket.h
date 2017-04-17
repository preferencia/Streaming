#pragma once

#include "ConnectSocket.h"

class CClientSocket : public CConnectSocket
{
public:
	CClientSocket();
	virtual ~CClientSocket();

	bool			Init(char* pszServerIP, int nServerPort);
	bool			Connect(char* pszServerIP, int nServerPort);
    bool			IsConnected()			{   return m_bIsConnected;		}

protected:
	virtual int		ProcSvcData(int nSvcCode, int nSvcDataLen, char* lpBuf);

private:
	bool			m_bIsConnected;
};