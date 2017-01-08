#pragma once

#include "ConnectSocket.h"
#include <list>
using namespace std;

typedef list<void*>				SendDataQueue;
typedef list<void*>::iterator	SendDataQueueIt;

class CSocketThread
{
public:
    CSocketThread();
    virtual ~CSocketThread(void);

    virtual bool    InitSocketThread();
	virtual void	StopSocketThread();
	virtual void    Send(char* pData, int nDataLen);

	virtual CConnectSocket* GetSocket()	{ return NULL; }

private:
#ifdef _WINDOWS
    static unsigned int __stdcall	SendThread(void* lpParam);
	static unsigned int __stdcall	RecvThread(void* lpParam);
#else
	static void*					SendThread(void* lpParam);
	static void*					RecvThread(void* lpParam);
#endif

protected:
#ifdef _WINDOWS
	HANDLE				m_hSendThread;
	HANDLE				m_hRecvThread;
	HANDLE				m_hSendDataQueueMutex;
#else
	pthread_t			m_hSendThread;
	pthread_t			m_hRecvThread;
	pthread_mutex_t		m_hSendDataQueueMutex;
#endif

    bool                m_bRunSendThread;
	bool                m_bRunRecvThread;

    SendDataQueue*      m_pSendDataQueue;
    SendDataQueueIt     m_SendDataQueueIt;
};
