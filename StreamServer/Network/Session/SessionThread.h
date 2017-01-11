#pragma once

#include "SocketThread.h"
#include "SessionSocket.h"
#include <map>

class CSessionThread : public CSocketThread
{
public:
    CSessionThread();
    virtual ~CSessionThread(void);

    virtual bool            InitSocketThread(bool bRunSendThread = true, bool bRunRecvThread = true);
	virtual void			SetSocket(SOCKET hSocket)	{	m_hSocket = hSocket;		}
    virtual CConnectSocket* GetSocket()					{	return m_pSessionSocket;	}

private:
	SOCKET					m_hSocket;
	CSessionSocket*			m_pSessionSocket;
};
