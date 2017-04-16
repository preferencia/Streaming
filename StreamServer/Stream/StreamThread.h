#pragma once

typedef int (*StreamCallback)(void*, int, UINT, void*);

class CStreamThread
{
public:
	CStreamThread();
	~CStreamThread();

	int								Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszVideoFileName, bool bCreateTrscVideoFile = false);
    int                             Open();
	void							SetResolution(int nWidth, int nHeight, int nResetResolution);
	int								Start();
	bool							Pause();
	void							Stop();

private:
	void*							m_pStreamSource;	
};

