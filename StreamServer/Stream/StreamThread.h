#pragma once

typedef int (*StreamCallback)(void*, int, UINT, void*);

class CStreamThread
{
public:
	CStreamThread();
	~CStreamThread();

	int								Init(void* pParent, StreamCallback pStreamCallbackFunc, char* pszVideoFileName);
	void							SetResolution(int nWidth, int nHeight);
	int								Start();
	bool							Pause();
	void							Stop();

private:
	void*							m_pStreamSource;	
};

