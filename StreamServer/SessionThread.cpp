#include "stdAfx.h"
#include "SessionThread.h"

CSessionThread::CSessionThread()
{
	m_hSocket			= INVALID_SOCKET;
	m_pSessionSocket	= NULL;
}

CSessionThread::~CSessionThread(void)
{
	StopSocketThread();
	SAFE_DELETE(m_pSessionSocket);
}

bool CSessionThread::InitSocketThread() 
{
	if (INVALID_SOCKET == m_hSocket)
	{
		return false;
	}

	SAFE_DELETE(m_pSessionSocket);
	m_pSessionSocket = new CSessionSocket();
	if (NULL == m_pSessionSocket)
	{
		return false;
	}
	
	m_pSessionSocket->Attach(m_hSocket);
	m_pSessionSocket->SetSocketThread(this);

    return CSocketThread::InitSocketThread();
}