#pragma once

#include "SocketThread.h"
#include "ClientSocket.h"
#include <map>

class CClientThread : public CSocketThread
{
public:
    CClientThread();
    virtual ~CClientThread(void);

	void					SetServerInfo(char* pszServerIP, int nServerPort);
	void					SetProcCallbakcInfo(void* pObject, ProcCallback ProcCallbackFunc);

    virtual bool            InitSocketThread();
    virtual CConnectSocket* GetSocket() { return m_pClientSocket; }

private:
	char					m_szServerIP[_DEC_SERVER_ADDR_LEN];
	int						m_nServerPort;
	CClientSocket*			m_pClientSocket;
};
