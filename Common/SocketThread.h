#pragma once

#include "ConnectSocket.h"

using namespace std;

#ifdef _WIN32
#define _DEC_MODE_READ  0
#define _DEC_MODE_WRITE 1

typedef struct _SOCKET_INFO_DATA        // socket info
{
	SOCKET      hSessionSocket;
	SOCKADDR_IN SessionAddr;
} SOCKET_INFO_DATA, *PSOCKET_INFO_DATA;

typedef struct _OVERLAPPED_IO_DATA
{
	OVERLAPPED      Ovlp;
	WSABUF          wsaBuf;
    WSABUF          wsaSavedBuf;
	int             nRWMode;            // read or write
} OVERLAPPED_IO_DATA, *POVERLAPPED_IO_DATA;
#endif

typedef list<void*>									SendDataQueue;
typedef list<void*>::iterator						SendDataQueueIt;

typedef map<SOCKET, CConnectSocket*>			    ConnectSocketMap;
typedef map<SOCKET, CConnectSocket*>::iterator	    ConnectSocketMapIt;

class CSocketThread
{
public:
    CSocketThread();
    virtual ~CSocketThread(void);

    virtual bool			InitSocketThread();
	virtual void			StopSocketThread();
	virtual void			Send(SOCKET hSocket, int nDataLen, char* pData);
	virtual void			PushSvcData(int nSvcCode, UINT uiSvcDataLen, char* pSvcData) {}

	virtual int			    ActiveConnectSocket(SOCKET hConnectSocket, SOCKADDR_IN* pConnectAdr) { return 0; }
	virtual CConnectSocket* GetConSocket(SOCKET hConnectSocket)		{ return NULL; }
	virtual CConnectSocket* GetDefaultConSocket()					{ return NULL; }

protected:
#ifdef _WIN32
    static unsigned int __stdcall	SendThread(void* lpParam);
	static unsigned int __stdcall	RecvThread(void* lpParam);
#else
	static void*					SendThread(void* lpParam);
	static void*					RecvThread(void* lpParam);
#endif

protected:
	int					m_nSessionThreadNum;
#ifdef _WIN32
	HANDLE				m_hComPort;

	HANDLE				m_hSendThread;
	HANDLE				m_hRecvThread;
	HANDLE				m_hSendDataQueueMutex;
#else
	epoll_event*		m_pEpollEvents;
	int					m_nEpollFd;

	pthread_t			m_hSendThread;
	pthread_t			m_hRecvThread;
	pthread_mutex_t		m_hSendDataQueueMutex;
#endif

    bool                m_bRunSendThread;
	bool                m_bRunRecvThread;

    SendDataQueue       m_SendDataQueue;
    SendDataQueueIt     m_SendDataQueueIt;

	ConnectSocketMap	m_ConnectSocketMap;
	ConnectSocketMapIt	m_ConnectSocketMapIt;
};
