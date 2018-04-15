#pragma once

class CConnectSocket
{
public:
    CConnectSocket(void);
    virtual ~CConnectSocket(void);

	SOCKET				GetSocket()				{	return m_hSocket;		}
	void				Attach(SOCKET hSocket)	{	m_hSocket = hSocket;	}

	void				SetSocketThread(void* pSocketThread)	{	m_pSocketThread = pSocketThread;	}

	virtual UINT		Send(char* pData, int nDataLen);
	virtual UINT		ProcessReceive(char* lpBuf, UINT uiDataLen);
    virtual int		    ProcSvcData(int nSvcCode, int nSvcDataLen, char* lpBuf) = 0;

protected:
	void*				m_pSocketThread;
	SOCKET				m_hSocket;
};
