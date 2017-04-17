#include "stdafx.h"
#include "ListenSocket.h"

CListenSocket::CListenSocket()
{
	m_hListenSocket		= INVALID_SOCKET;

	m_hAcceptThread		= NULL;

	m_bRunAcceptThread	= false;

    m_nThreashold       = 0;

	memset(&m_ServAddr, 0, sizeof(SOCKADDR_IN));

	m_SessionThreadMap.clear();
}

CListenSocket::~CListenSocket()
{
	m_hListenSocket		= INVALID_SOCKET;

	if (0 < m_SessionThreadMap.size())
	{
		m_SessionThreadMapIt = m_SessionThreadMap.begin();
		while (m_SessionThreadMapIt != m_SessionThreadMap.end())
		{
			CSessionThread* pSessionThread = m_SessionThreadMapIt->second;
			SAFE_DELETE(pSessionThread);
			m_SessionThreadMap.erase(m_SessionThreadMapIt++);
		}
	}

#ifdef _WIN32
	WSACleanup();
#endif
}

bool CListenSocket::Init(int nPort)
{
#ifdef _WIN32
	WSADATA		wsaData			= {0, };
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		TraceLog("WSAStartup() error!");
		return false;
	}
#endif

	if (INVALID_SOCKET == m_hListenSocket)
	{
#ifdef _WIN32
		closesocket(m_hListenSocket);
#else
        close(m_hListenSocket);
#endif
	}

	m_hListenSocket	= socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == m_hListenSocket)
	{
		TraceLog("socket() error!");
		return false;
	}

	m_ServAddr.sin_family		= AF_INET;
	m_ServAddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	m_ServAddr.sin_port			= htons(nPort);

	if (SOCKET_ERROR == bind(m_hListenSocket, (SOCKADDR*)&m_ServAddr, sizeof(m_ServAddr)))
	{
		TraceLog("bind() error!");
		return false;
	}

	return true;
}

void CListenSocket::Close()
{
	m_bRunAcceptThread = false;

#ifdef _WIN32
	closesocket(m_hListenSocket);

	if (NULL != m_hAcceptThread)
	{
		WaitForSingleObject(m_hAcceptThread, INFINITE);
	}
#else
    SOCKET hDummySock = socket(PF_INET, SOCK_STREAM, 0);

	connect(hDummySock, (struct sockaddr*)&m_ServAddr, sizeof(m_ServAddr));
	close(hDummySock);

	close(m_hListenSocket);
#endif
}

bool CListenSocket::Listen()
{
	if (INVALID_SOCKET == m_hListenSocket)
	{
		TraceLog("listen socket is invalid!");
		return false;
	}

	if (SOCKET_ERROR == listen(m_hListenSocket, 5))
	{
		TraceLog("listen() error!");
		return false;
	}

    m_nThreashold = CreateSessionThread();

    if (0 >= m_nThreashold)
    {
        TraceLog("CreateSessionThread() error!");
		return false;
    }

#ifdef _WIN32
	if (NULL == m_hAcceptThread)
	{
		m_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
		if (NULL != m_hAcceptThread)
		{
			m_bRunAcceptThread = true;
		}
	}
#else
    int nRet = pthread_create(&m_hAcceptThread, NULL, AcceptThread, this);
	if (0 == nRet)
	{
		m_bRunAcceptThread = true;
	}

	pthread_detach(m_hAcceptThread);
#endif	

	return true;
}

int CListenSocket::CreateSessionThread()
{
    int             nThreashold = 0;

#ifdef _WIN32
	SYSTEM_INFO	    SysInfo		= { 0, };
	HANDLE			hComPort	= NULL;

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == hComPort)
	{
		TraceLog("Create IOCP Object Failed.");
        return -1;
	}

	GetSystemInfo(&SysInfo);
	nThreashold = SysInfo.dwNumberOfProcessors;    
#else
	FILE*				fpCpuInfo	= fopen("/proc/cpuinfo", "r");

	if (NULL != fpCpuInfo)
	{
		char szLine[1024] = { 0, };
		while (NULL != fgets(szLine, 1023, fpCpuInfo))
		{
			if (NULL != strstr(szLine, "processor"))
			{
				++nThreashold;
			}
		}

		fclose(fpCpuInfo);
	}
#endif

	if (0 >= nThreashold)
	{
		TraceLog("Get Number of Process Failed");
        return -2;
	}

	for (int nIndex = 0; nIndex < nThreashold; ++nIndex)
	{
		CSessionThread* pSessionThread = new CSessionThread;
		if (NULL == pSessionThread)
		{
			TraceLog("Create Session Thread Failed");
			continue;
		}

		pSessionThread->SetSessionThradNum(nIndex);
#ifdef _WIN32
		pSessionThread->SetCPObject(hComPort);
#endif

		if (false == pSessionThread->InitSocketThread())
		{
			TraceLog("Session Thread Init Failed");
			continue;
		}
		        
        m_SessionThreadMapIt = m_SessionThreadMap.find(nIndex);
        if (m_SessionThreadMapIt == m_SessionThreadMap.end())
        {            
            m_SessionThreadMap[nIndex] = pSessionThread;
        }
	}

    return m_SessionThreadMap.size();
}

#ifdef _WIN32
unsigned int __stdcall  CListenSocket::AcceptThread(void* lpParam)
#else
void*                   CListenSocket::AcceptThread(void* lpParam)
#endif
{
	CListenSocket* pThis	= (CListenSocket*)lpParam;
	if (NULL == pThis)
	{
		TraceLog("Parameter Error!");
        return 0;
	}

    CSessionThread*     pSessionThread      = NULL;
	LONGLONG            llThreadNum         = 0;
    int					nRet		        = 0;

	while (true == pThis->m_bRunAcceptThread)
	{
		SOCKET          hSessionSocket  = INVALID_SOCKET;
		SOCKADDR_IN     SessionAdr      = { 0, };
		socklen_t       nSessionAdrSize = sizeof(SessionAdr);

        hSessionSocket = accept(pThis->m_hListenSocket, (SOCKADDR*)&SessionAdr, &nSessionAdrSize);
		if (INVALID_SOCKET == hSessionSocket)
		{
			TraceLog("accept() error!");
			continue;
		}

        if (0 > pThis->m_SessionThreadMap[llThreadNum % pThis->m_nThreashold]->ActiveConnectSocket(hSessionSocket, &SessionAdr))
        {
            TraceLog("Active Socket Failed.");
            continue;
        }

        ++llThreadNum;
	}

$END:
#ifdef _WIN32
	CloseHandle(pThis->m_hAcceptThread);
	pThis->m_hAcceptThread	= NULL;

    return nRet;
#else
    return NULL;
#endif
}