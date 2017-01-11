#pragma once

#include "ConnectSocket.h"
#include "StreamThread.h"

class CSessionSocket : public CConnectSocket
{
public:
	CSessionSocket();
	virtual ~CSessionSocket();

protected:
	virtual bool	Send(char* pData, int nDataLen);
	virtual UINT	ProcessReceive(char* lpBuf, int nDataLen);

private:
	virtual int		ProcVideoList(); 
	virtual int		ProcFileOpen(char* pszFileName, int nFileNameLen);
	virtual int		ProcSelectResolution(UINT uiWidth, UINT uiHeight, UINT uiResetResolution);
	virtual int		ProcSetPlayStatus(UINT uiPlayStatus);

private:
	static int		StreamCallback(void* pObject, int nOpCode, UINT uiDataSize, void* pData);

private:
	CStreamThread*	m_pStreamThread;
};