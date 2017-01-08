#pragma once

#include "Common_Protocol.h"

class CConnectSocket
{
public:
    CConnectSocket(void);
    virtual ~CConnectSocket(void);

	SOCKET				GetConSocket()			{	return m_hConnectSocket;	}
	void				Attach(SOCKET hSocket)	{	m_hConnectSocket = hSocket;	}

	void				SetSocketThread(void* pSocketThread)	{	m_pSocketThread = pSocketThread;	}

	virtual bool		Send(char* pData, int nDataLen);
	virtual UINT		ProcessReceive(char* lpBuf, int nDataLen)	= 0;	// define in subclasses

protected:
	void*				m_pSocketThread;
	SOCKET				m_hConnectSocket;
};
