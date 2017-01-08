#pragma once

#ifdef _WINDOWS

#ifndef socklen_t
typedef int					socklen_t;
#endif

#else

#ifndef SOCKET
typedef int					SOCKET;
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR		(-1)
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET		(~0)
#endif

#ifndef SOCKADDR
typedef struct sockaddr		SOCKADDR;
#endif

#ifndef SOCKADDR_IN
typedef struct sockaddr_in	SOCKADDR_IN;
#endif

#endif

#define SAFE_DELETE(p)			{if (NULL != p) delete p; p = NULL; }
#define SAFE_DELETE_ARRAY(p)	{if (NULL != p) delete [] p; p = NULL; }

#define _DEC_MAX_BUF_SIZE		(4096)
#define _DEC_SERVER_ADDR_LEN	(16)