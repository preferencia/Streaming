#pragma once

#include "SocketThread.h"
#include "SessionSocket.h"

class CSessionThread : public CSocketThread
{
public:
    CSessionThread();
    virtual ~CSessionThread(void);

    virtual bool            InitSocketThread();
	virtual int			    ActiveConnectSocket(SOCKET hConnectSocket, SOCKADDR_IN* pConnectAdr);
    virtual CConnectSocket* GetConSocket(SOCKET hConnectSocket);

	void					SetSessionThradNum(int nNum)	{	m_nSessionThreadNum = nNum; }
#ifdef _WIN32
	void					SetCPObject(HANDLE hComPort)	{	m_hComPort = hComPort;		}
#endif
protected:

private:	
};
