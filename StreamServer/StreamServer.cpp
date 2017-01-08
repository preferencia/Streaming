// SimpleServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "ListenSocket.h"

static bool ErrorHandling(const char* pszFmt, ...);

int main(int argc, char* argv[])
{
	if (2 > argc)
	{
		ErrorHandling("Need More Argument!");
		return -1;
	}

	CListenSocket* pListenSocket = new CListenSocket();
	if (NULL != pListenSocket)
	{
		if (true == pListenSocket->Init(atoi(argv[1])))
		{
			if (true == pListenSocket->Listen())
			{
				cout << "If want to quit, press q or Q and Enter : ";
				while (1)
				{
					int nInput = getchar();
					if (('q' == nInput) || ('Q' == nInput))
					{
						break;
					}
#ifdef _WINDOWS
					Sleep(1);
#else
					usleep(1000);
#endif
				}
			}			
		}

		SAFE_DELETE(pListenSocket);
	}

/*
#ifdef _WINDOWS
	WSADATA		wsaData			= {0, };
	SOCKET		hServSock		= NULL;
	SOCKET		hClientSock		= NULL;
#else
	int			hServSock		= 0;
	int			hClientSock		= 0;
#endif

	SOCKADDR_IN	ServAddr		= {0, };
	SOCKADDR_IN	ClientAddr		= {0, };
	socklen_t	nClientAddrSize	= sizeof(ClientAddr);

	char		SendBuf[_DEC_MAX_BUF_SIZE]	= {0, };
	char*		pszSendMsg					= (char*)"Hello World!";
	memcpy(SendBuf, pszSendMsg, strlen(pszSendMsg));

#ifdef _WINDOWS
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		ErrorHandling("WSAStartup() error!");
	}
#endif

	hServSock	= socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == hServSock)
	{
		ErrorHandling("socket() error!");
	}

	ServAddr.sin_family				= AF_INET;
	ServAddr.sin_addr.s_addr		= htonl(INADDR_ANY);
	ServAddr.sin_port				= htons(atoi(argv[1]));

	if (SOCKET_ERROR == bind(hServSock, (SOCKADDR*)&ServAddr, sizeof(ServAddr)))
	{
		ErrorHandling("bind() error!");
	}

	if (SOCKET_ERROR == listen(hServSock, 5))
	{
		ErrorHandling("listen() error!");
	}

	hClientSock = accept(hServSock, (SOCKADDR*)&ClientAddr, &nClientAddrSize);
	if (INVALID_SOCKET == hClientSock)
	{
		ErrorHandling("accept() error!");
	}

#ifdef _WINDOWS
	send(hClientSock, SendBuf, sizeof(SendBuf), 0);
	closesocket(hClientSock);
	closesocket(hServSock);
#else
	write(hClientSock, SendBuf, sizeof(SendBuf));
	close(hClientSock);
	close(hServSock);
#endif

#ifdef _WINDOWS
	WSACleanup();
#endif
*/
	return 0;
}

static bool ErrorHandling(const char* pszFmt, ...)
{
	if (NULL != pszFmt)
	{
		char szLog[_DEC_MAX_BUF_SIZE] = {0, };

		va_list args;
		va_start(args, pszFmt);
		vsprintf(szLog, pszFmt, args);
		va_end(args);

#ifdef _WINDOWS
		OutputDebugString(szLog);
		OutputDebugString("\n");
#endif
		cout << szLog << endl;
	}

	return false;
}
