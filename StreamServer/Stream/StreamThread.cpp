#include "stdafx.h"
#include "StreamThread.h"
#include "StreamSource.h"

CStreamThread::CStreamThread()
{
	m_pStreamSource = NULL;
}

CStreamThread::~CStreamThread()
{
}

int CStreamThread::Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszVideoFileName)
{
	CStreamSource* pStreamSource = new CStreamSource;
	if (NULL == pStreamSource)
	{
		return -1;
	}

	int nErr = pStreamSource->Init(pParent, pStreamCallbackFunc, pszVideoFileName);
	if (0 > nErr)
	{
		TraceLog("Stream Source Initialize Failed. Error = %d", nErr);
		return -2;
	}

	m_pStreamSource = pStreamSource;

	return 0;
}

void CStreamThread::SetResolution(int nWidth, int nHeight)
{
	CStreamSource* pStreamSource = (CStreamSource*)m_pStreamSource;
	if (NULL == pStreamSource)
	{
		return;
	}

	pStreamSource->SetResolution(nWidth, nHeight);
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
