#pragma once

#include "SocketThread.h"
#include "ClientSocket.h"
#include <map>

typedef int (*ProcCallback)(void*, unsigned int, unsigned int, char*);

typedef SendDataQueue	ProcSvcDataQueue;
typedef SendDataQueueIt	ProcSvcDataQueueIt;

class CClientThread : public CSocketThread
{
public:
    CClientThread();
    virtual ~CClientThread(void);

	void					SetServerInfo(char* pszServerIP, int nServerPort);
	void					SetProcCallbakcInfo(void* pObject, ProcCallback ProcCallbackFunc);

    virtual bool            InitSocketThread();
	virtual void			StopSocketThread();
	virtual void			PushSvcData(int nSvcCode, UINT uiSvcDataLen, char* pSvcData);
	virtual int			    ActiveConnectSocket(SOCKET hConnectSocket, SOCKADDR_IN* pConnectAdr);
	virtual CConnectSocket* GetConSocket(SOCKET hConnectSocket);
    virtual CConnectSocket* GetDefaultConSocket() { return m_pClientSocket; }

private:
#ifdef _WINDOWS
	static unsigned int __stdcall	ProcSvcDataThread(void* lpParam);
#else
	static void*					ProcSvcDataThread(void* lpParam);
#endif

private:
	char					m_szServerIP[_DEC_SERVER_ADDR_LEN];
	int						m_nServerPort;
	CClientSocket*			m_pClientSocket;

#ifdef _WINDOWS
	HANDLE					m_hProcSvcDataThread;
	HANDLE					m_hProcSvcDataQueueMutex;
#else
	pthread_t				m_hProcSvcDataThread;
	pthread_mutex_t			m_hProcSvcDataQueueMutex;
#endif

	bool					m_bRunProcSvcDataThread;

	ProcSvcDataQueue*		m_pProcSvcDataQueue;
	ProcSvcDataQueueIt		m_ProcSvcDataQueueIt;

	void*					m_pObject;
	ProcCallback			m_ProcCallbackFunc;	
};
