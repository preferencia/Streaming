#include "stdafx.h"
#include "StreamThread.h"
#include "StreamSource.h"

CStreamThread::CStreamThread()
{
	m_pStreamSource = NULL;
}

CStreamThread::~CStreamThread()
{
	CStreamSource* pStreamSource = (CStreamSource*)m_pStreamSource;
	SAFE_DELETE(pStreamSource);
}

int CStreamThread::Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszVideoFileName, bool bCreateTrscVideoFile /* = false */)
{
	CStreamSource* pStreamSource = new CStreamSource;
	if (NULL == pStreamSource)
	{
		return -1;
	}

	int nErr = pStreamSource->Init(pParent, pStreamCallbackFunc, pszVideoFileName, bCreateTrscVideoFile);
	if (0 > nErr)
	{
		TraceLog("Stream Source Initialize Failed. Error = %d", nErr);
		return -2;
	}

	m_pStreamSource = pStreamSource;

	return 0;
}

int CStreamThread::Open()
{
    CStreamSource* pStreamSource = (CStreamSource*)m_pStreamSource;
	if (NULL == pStreamSource)
	{
		return -1;
	}

    return pStreamSource->Open();
}

void CStreamThread::SetResolution(int nWidth, int nHeight, int nResetResolution)
{
	CStreamSource* pStreamSource = (CStreamSource*)m_pStreamSource;
	if (NULL == pStreamSource)
	{
		return;
	}

    pStreamSource->SetResolution(nWidth, nHeight, nResetResolution);
}

int CStreamThread::Start()
{
	CStreamSource* pStreamSource = (CStreamSource*)m_pStreamSource;
	if (NULL == pStreamSource)
	{
		return -1;
	}

	return pStreamSource->Start();
}

bool CStreamThread::Pause()
{
	CStreamSource* pStreamSource = (CStreamSource*)m_pStreamSource;
	if (NULL != pStreamSource)
	{
		return pStreamSource->Pause();
	}

	return false;
}

void CStreamThread::Stop()
{
	CStreamSource* pStreamSource = (CStreamSource*)m_pStreamSource;
	if (NULL != pStreamSource)
	{
		pStreamSource->Stop();
	}
}
