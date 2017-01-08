#pragma once

#include "ConnectSocket.h"

typedef int (*ProcCallback)(void*, unsigned int, unsigned int, char*);

class CClientSocket : public CConnectSocket
{
public:
	CClientSocket();
	virtual ~CClientSocket();

	bool			Init(char* pszServerIP, int nServerPort);
	bool			Connect(char* pszServerIP, int nServerPort);
    bool			IsConnected()			{   return m_bIsConnected;		}
	
	void			SetProcCallbakcInfo(void* pObject, ProcCallback ProcCallbackFunc);

protected:
	virtual bool	Send(char* pData, int nDataLen);
	virtual UINT	ProcessReceive(char* lpBuf, int nDataLen);

private:
	ProcCallback	m_ProcCallbackFunc;
	void*			m_pObject;
	bool			m_bIsConnected;
};