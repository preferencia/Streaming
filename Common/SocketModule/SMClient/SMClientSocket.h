#pragma once

#include "ConnectSocket.h"

class CSMClientSocket : public CConnectSocket
{
public:
    CSMClientSocket(void);
    virtual ~CSMClientSocket(void);

protected:
    virtual void    OnClose(int nErrorCode);
    virtual int     ProcessReceive(char* lpBuf, int nDataLen); // define in subclasses
};
