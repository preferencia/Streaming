#include "stdafx.h"
#include "SessionSocket.h"
#include "SocketThread.h"

typedef list<VideoListData*>			VideoListDataQueue;
typedef list<VideoListData*>::iterator	VideoListDataQueueIt;

#define _DEC_NEW_LINE           (10)
#define _DEC_CARRIAGE_RETURN    (13)

CSessionSocket::CSessionSocket()
{
	m_pStreamThread				= NULL;

#ifdef _WIN32
	m_hStreamStopCheckThread	= NULL;
#endif

	m_bRunStreamStopCheckThread = false;
	m_bRecvStreamStopSignal		= false;
}

CSessionSocket::~CSessionSocket()
{
	m_bRunStreamStopCheckThread = false;

#ifdef _WIN32
	if (NULL != m_hStreamStopCheckThread)
	{
		WaitForSingleObject(m_hStreamStopCheckThread, INFINITE);
		CloseHandle(m_hStreamStopCheckThread);
		m_hStreamStopCheckThread = NULL;
	}
#endif // _WIN32

	if (NULL != m_pStreamThread)
	{
		m_pStreamThread->Stop();
	}
	SAFE_DELETE(m_pStreamThread);
}

int CSessionSocket::ProcSvcData(int nSvcCode, int nSvcDataLen, char* lpBuf)
{
	int nRet = 0;

	if (0 > (nRet = CConnectSocket::ProcSvcData(nSvcCode, nSvcDataLen, lpBuf)))
	{
		TraceLog("Error Param checked - err = [%d]", nRet);
		return nRet;
	}

	switch (nSvcCode)
	{
	case SVC_GET_LIST:
		{
			ProcVideoList();
		}
		break;

	case SVC_FILE_OPEN:
		{
			PVS_FILE_OPEN_REQ pReqData				= (PVS_FILE_OPEN_REQ)lpBuf;
			if (NULL != pReqData)
			{
				ProcFileOpen(pReqData->pszFileName, nSvcDataLen);
			}
		}
		break;

	case SVC_SELECT_RESOLUTION:
		{
			PVS_SELECT_RESOLUTION_REQ pReqData		= (PVS_SELECT_RESOLUTION_REQ)lpBuf;
			if (NULL != pReqData)
			{
				ProcSelectResolution(pReqData->uiWidth, pReqData->uiHeight, pReqData->uiResetResolution);
			}
		}
		break;

	case SVC_SET_PLAY_STATUS:
		{
			PVS_SET_PLAY_STATUS pReqData = (PVS_SET_PLAY_STATUS)lpBuf;
			if (NULL != pReqData)
			{
				ProcSetPlayStatus(pReqData->uiPlayStatus);
			}
		}
		break;

	default:
		break;
	}

	return 0;
}

int CSessionSocket::ProcVideoList()
{
	if (NULL != m_pSocketThread)
	{
		FILE* fpList = fopen("./Video/VideoList.ini", "r");
		if (NULL != fpList)
		{
			unsigned int	uiSendBufLen			= 0;
			unsigned int	uiTotalDataSize			= 0;
			unsigned int	uiBufIndex				= 0;
			char*			pSendBuf				= NULL;

			VideoListDataQueue		VideoListDataQ;
			VideoListDataQueueIt	It;

			while (!feof(fpList))
			{                
				char    szFileName[1024]    = {0, };
				fgets(szFileName, sizeof(szFileName), fpList);
                int     nFileNameLen        = strlen(szFileName);

                // except new line and carriage return
                for (int nIndex = 0; nIndex < strlen(szFileName); ++nIndex)
                {
                    if ( (_DEC_NEW_LINE == szFileName[nIndex]) 
                        || (_DEC_CARRIAGE_RETURN == szFileName[nIndex]) )
                    {
                        --nFileNameLen;
                    }
                }                
				
				if (0 < strlen(szFileName))
				{
					int nVideoListDataSize			= sizeof(VideoListData) + nFileNameLen + 1;
					char* pData						= new char[nVideoListDataSize];
					memset(pData, 0, nVideoListDataSize);

					PVideoListData pVideoListData	= (PVideoListData)pData;
					pVideoListData->nFileNameLen	= nFileNameLen;
					memcpy(pData + sizeof(int), szFileName, pVideoListData->nFileNameLen);

					VideoListDataQ.push_back(pVideoListData);

					uiTotalDataSize += sizeof(int) + (pVideoListData->nFileNameLen + 1) * sizeof(char);
				}
			}

			fclose(fpList);
			fpList = NULL;

			if (0 < uiTotalDataSize)
			{
				uiSendBufLen			= sizeof(VS_HEADER) + sizeof(int) + uiTotalDataSize;
				pSendBuf				= new char[uiSendBufLen];
				memset(pSendBuf,	0, uiSendBufLen);

				PVS_HEADER	 pHeader		= (PVS_HEADER)pSendBuf;
				MAKE_VS_HEADER(pHeader, SVC_GET_LIST, 0, sizeof(int) + uiTotalDataSize);

				PVS_LIST_REP pVsListS2C		= (PVS_LIST_REP)(pSendBuf + sizeof(VS_HEADER));
				pVsListS2C->nCnt = VideoListDataQ.size();

				uiBufIndex = sizeof(VS_HEADER) + sizeof(int);

				for (It = VideoListDataQ.begin(); It != VideoListDataQ.end(); ++It)
				{
					VideoListData* pVideoListData = *It;
					if (NULL != pVideoListData)
					{
						memcpy(pSendBuf + uiBufIndex, &pVideoListData->nFileNameLen, sizeof(int));
						uiBufIndex += sizeof(int);
						memcpy(pSendBuf + uiBufIndex, pVideoListData->pszFileName, pVideoListData->nFileNameLen);
						uiBufIndex += pVideoListData->nFileNameLen + 1;

						SAFE_DELETE(pVideoListData);
					}
				}

				VideoListDataQ.clear();
			}
			else
			{
				uiSendBufLen			= sizeof(VS_HEADER);
				pSendBuf				= new char[uiSendBufLen];
				memset(pSendBuf,	0, uiSendBufLen);

				PVS_HEADER	 pHeader		= (PVS_HEADER)pSendBuf;
				MAKE_VS_HEADER(pHeader, SVC_GET_LIST, -1, 0);
			}

			((CSocketThread*)m_pSocketThread)->Send(m_hSocket, uiSendBufLen, pSendBuf);
		}
	}
	return 0;
}

int CSessionSocket::ProcFileOpen(char* pszFileName, int nFileNameLen)
{
	if (NULL == pszFileName)
	{
		return -1;
	}

    TraceLog("Delete StreamThread 2");
	SAFE_DELETE(m_pStreamThread);

	m_pStreamThread = new CStreamThread;
	if (NULL == m_pStreamThread)
	{
		return -2;
	}

	if (0 > m_pStreamThread->Init(this, StreamCallback, pszFileName, true))
	{
		return -3;
	}

    if (0 > m_pStreamThread->Open())
    {
        return -4;
    }

	// Start stream stop check thread
#ifdef _WIN32
	m_hStreamStopCheckThread = (HANDLE)_beginthreadex(NULL, 0, StreamStopCheckThread, this, 0, NULL);
	if (NULL != m_hStreamStopCheckThread)
	{
		m_bRunStreamStopCheckThread = true;
	}
#else
	int nRet = pthread_create(&m_hStreamStopCheckThread, NULL, StreamStopCheckThread, this);
	if (0 == nRet)
	{
		m_bRunStreamStopCheckThread = true;
	}

	pthread_detach(m_hStreamStopCheckThread);
#endif

	return 0;
}

int CSessionSocket::ProcSelectResolution(UINT uiWidth, UINT uiHeight, UINT uiResetResolution)
{
	if (NULL == m_pStreamThread)
	{
		return -1;
	}

	m_pStreamThread->SetResolution(uiWidth, uiHeight, uiResetResolution);

    return (0 == uiResetResolution) ? m_pStreamThread->Start() : 0;
}

int CSessionSocket::ProcSetPlayStatus(UINT uiPlayStatus)
{
	if ((0 > uiPlayStatus) || (3 < uiPlayStatus))
	{
		return -1;
	}

	if (NULL == m_pStreamThread)
	{
		return -2;
	}

	unsigned int		uiRet			= -1;
	unsigned int		uiErr			= 0;

	unsigned int		uiSendBufLen	= sizeof(VS_HEADER) + sizeof(VS_SET_PLAY_STATUS);
	char*				pSendBuf		= new char[uiSendBufLen];

	switch (uiPlayStatus)
	{
	case 0:
	case 1:
		{
			uiRet = (true == m_pStreamThread->Pause()) ? 1 : 0;
		}
		break;

	case 2:
		{            
			if (NULL != m_pStreamThread)
	        {
		        m_pStreamThread->Stop();
	        }
            uiRet = 2;
		}
		break;

	default:
		{
			uiErr = -1;
		}
		break;
	}

	PVS_HEADER	 pHeader				= (PVS_HEADER)pSendBuf;
	MAKE_VS_HEADER(pHeader, SVC_SET_PLAY_STATUS, uiErr, sizeof(VS_SET_PLAY_STATUS));

	PVS_SET_PLAY_STATUS pRepData		= (PVS_SET_PLAY_STATUS)(pSendBuf + sizeof(VS_HEADER));
	pRepData->uiPlayStatus				= uiRet;	

	((CSocketThread*)m_pSocketThread)->Send(m_hSocket, uiSendBufLen, pSendBuf);

	return 0;
}

int CSessionSocket::StreamCallback(void* pObject, int nOpCode, UINT uiDataSize, void* pData)
{
	if ((NULL == pObject) || (0 > nOpCode))
	{
		return -1;
	}

	CSessionSocket* pThis			= (CSessionSocket*)pObject;
	if (NULL == pThis->m_pSocketThread)
	{
		return -2;
	}
	
	unsigned int	uiSendBufLen	= sizeof(VS_HEADER) + uiDataSize;
	char*			pSendBuf		= new char[uiSendBufLen];
	if (NULL == pSendBuf)
	{
		return -3;
	}

	PVS_HEADER	 pHeader		= (PVS_HEADER)pSendBuf;
	MAKE_VS_HEADER(pHeader, SVC_FILE_OPEN + nOpCode, 0, uiDataSize);

	TraceLog("CSessionSocket::StreamCallback - SVC = %d, Data Size = %u, Checksum = %u", SVC_FILE_OPEN + nOpCode, uiDataSize, pHeader->uiChecksum);

	if ((0 < uiDataSize) && (NULL != pData))
	{
		memcpy(pSendBuf + sizeof(VS_HEADER), pData, uiDataSize);
	}

	((CSocketThread*)pThis->m_pSocketThread)->Send(pThis->m_hSocket, uiSendBufLen, pSendBuf);

	// Recv play complete or stop code. Remove stream thread
	if (SVC_FILE_CLOSE == SVC_FILE_OPEN + nOpCode)
	{
		pThis->m_bRecvStreamStopSignal = true;
	}

	return 0;
}


#ifdef _WIN32
unsigned int __stdcall  CSessionSocket::StreamStopCheckThread(void* lpParam)
#else
void*                   CSessionSocket::StreamStopCheckThread(void* lpParam)
#endif
{
	CSessionSocket* pThis = (CSessionSocket*)lpParam;
	if (NULL == pThis)
	{
		return 0;
	}

	while (true == pThis->m_bRunStreamStopCheckThread)
	{
		if (true == pThis->m_bRecvStreamStopSignal)
		{
			SAFE_DELETE(pThis->m_pStreamThread);
			pThis->m_bRecvStreamStopSignal = false;
			break;
		}

#ifdef _WIN32
		Sleep(10);
#else
		usleep(10000);
#endif
	}

#ifdef _WIN32
	CloseHandle(pThis->m_hStreamStopCheckThread);
	pThis->m_hStreamStopCheckThread = NULL;

	return 1;
#else
	return NULL;
#endif 
}