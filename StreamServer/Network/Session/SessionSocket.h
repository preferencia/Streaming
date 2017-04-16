#pragma once

#include "ConnectSocket.h"
#include "StreamThread.h"

class CSessionSocket : public CConnectSocket
{
public:
	CSessionSocket();
	virtual ~CSessionSocket();

protected:
	virtual int						ProcSvcData(int nSvcCode, int nSvcDataLen, char* lpBuf);

private:
	virtual int						ProcVideoList(); 
	virtual int						ProcFileOpen(char* pszFileName, int nFileNameLen);
	virtual int						ProcSelectResolution(UINT uiWidth, UINT uiHeight, UINT uiResetResolution);
	virtual int						ProcSetPlayStatus(UINT uiPlayStatus);

private:
	static int						StreamCallback(void* pObject, int nOpCode, UINT uiDataSize, void* pData);

#ifdef _WIN32
	static unsigned int __stdcall	StreamStopCheckThread(void* lpParam);
#else
	static void*					StreamStopCheckThread(void* lpParam);
#endif

private:
	CStreamThread*					m_pStreamThread;

#ifdef _WIN32
	HANDLE							m_hStreamStopCheckThread;
#else
	pthread_t						m_hStreamStopCheckThread;
#endif

	bool							m_bRunStreamStopCheckThread;
	bool							m_bRecvStreamStopSignal;
};