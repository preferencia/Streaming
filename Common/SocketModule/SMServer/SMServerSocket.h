#pragma once

#include "ConnectSocket.h"

class CSMServerSocket : public CConnectSocket
{
public:
    CSMServerSocket(void);
    virtual ~CSMServerSocket(void);

protected:
    virtual void    OnClose(int nErrorCode);
    virtual int     ProcessReceive(char* lpBuf, int nDataLen); // define in subclasses
    void            DoSend(char* pBuf, int nBufLen);
};
