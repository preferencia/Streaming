#include "stdafx.h"
#include "SessionSocket.h"
#include "SocketThread.h"

typedef list<VideoListData*>			VideoListDataQueue;
typedef list<VideoListData*>::iterator	VideoListDataQueueIt;

CSessionSocket::CSessionSocket()
{
	m_pStreamThread = NULL;
}

CSessionSocket::~CSessionSocket()
{
	SAFE_DELETE(m_pStreamThread);
}

bool CSessionSocket::Send(char* pData, int nDataLen)
{
	if ((NULL == pData) || (0 >= nDataLen))
	{
		TraceLog("Data is wrong!");
		return false;
	}

	if (INVALID_SOCKET == m_hConnectSocket)
	{
		TraceLog("Session socket is invalid!");
		return false;
	}

	return CConnectSocket::Send(pData, nDataLen);
}

UINT CSessionSocket::ProcessReceive(char* lpBuf, int nDataLen)
{
	if ((NULL == lpBuf) || (0 >= nDataLen))
	{
		TraceLog("Data is wrong!");
		return false;
	}

	if (INVALID_SOCKET == m_hConnectSocket)
	{
		TraceLog("Client socket is invalid!");
		return false;
	}

	PVS_HEADER pHeader      = (PVS_HEADER)lpBuf;
    unsigned int uiChecksum = pHeader->uiSvcCode ^ pHeader->uiErrCode ^ pHeader->uiDataLen;
    if (pHeader->uiChecksum != uiChecksum)
    {
        TraceLog("Checksum is invalid!");
		return false;
    }

    int nRecvSvcCode        = pHeader->uiSvcCode;
    int nRecvErrCode        = pHeader->uiErrCode;
    int nRecvDataLen        = pHeader->uiDataLen;
    int nHeaderSize			= sizeof(VS_HEADER);

	// shortage data
    if ((0 < nRecvDataLen) && (nDataLen < (nHeaderSize + nRecvDataLen)))
    {
        return (nHeaderSize + nRecvDataLen) - nDataLen;
    }

	// over data

	switch (nRecvSvcCode)
	{
	case SVC_GET_LIST:
		{
			ProcVideoList();
		}
		break;

	case SVC_FILE_OPEN:
		{
			PVS_FILE_OPEN_REQ pReqData				= (PVS_FILE_OPEN_REQ)(lpBuf + sizeof(VS_HEADER));
			if (NULL != pReqData)
			{
				ProcFileOpen(pReqData->pszFileName);
			}
		}
		break;

	case SVC_SELECT_RESOLUTION:
		{
			PVS_SELECT_RESOLUTION_REQ pReqData		= (PVS_SELECT_RESOLUTION_REQ)(lpBuf + sizeof(VS_HEADER));
			if (NULL != pReqData)
			{
				ProcSelectResolution(pReqData->uiWidth, pReqData->uiHeight, pReqData->uiResetResolution);
			}
		}
		break;

	case SVC_SET_PLAY_STATUS:
		{
			PVS_SET_PLAY_STATUS pReqData = (PVS_SET_PLAY_STATUS)(lpBuf + sizeof(VS_HEADER));
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
		FILE* fpList = fopen("Video/VideoList.ini", "r");
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
				char szFileName[1024] = {0, };
				fgets(szFileName, sizeof(szFileName), fpList);
				
				if (0 < strlen(szFileName))
				{
					int nVideoListData				= sizeof(VideoListData) + strlen(szFileName) + 1;
					char* pData						= new char[nVideoListData];
					memset(pData, 0, nVideoListData);

					PVideoListData pVideoListData	= (PVideoListData)pData;
					pVideoListData->nFileNameLen	= strlen(szFileName);
					if ('\n' == szFileName[pVideoListData->nFileNameLen - 1])	// remove carriage return
					{
						--pVideoListData->nFileNameLen;
					}

					strncpy(pVideoListData->pszFileName, szFileName, pVideoListData->nFileNameLen);
					pVideoListData->pszFileName[pVideoListData->nFileNameLen] = NULL;

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

			((CSocketThread*)m_pSocketThread)->Send(pSendBuf, uiSendBufLen);
		}
	}
	return 0;
}

int CSessionSocket::ProcFileOpen(char* pszFileName)
{
	if (NULL == pszFileName)
	{
		return -1;
	}

	char szPath[1024] = { 0, };
	snprintf(szPath, 1023, "Video/%s", pszFileName);

	SAFE_DELETE(m_pStreamThread);

	m_pStreamThread = new CStreamThread;
	if (NULL == m_pStreamThread)
	{
		return -2;
	}

	if (0 > m_pStreamThread->Init(this, StreamCallback, szPath))
	{
		return -3;
	}

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
			m_pStreamThread->Stop();
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

	((CSocketThread*)m_pSocketThread)->Send(pSendBuf, uiSendBufLen);

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

	TraceLog("SVC = %d, Data Size = %u, Checksum = %u", SVC_FILE_OPEN + nOpCode, uiDataSize, pHeader->uiChecksum);

	if ((0 < uiDataSize) && (NULL != pData))
	{
		memcpy(pSendBuf + sizeof(VS_HEADER), pData, uiDataSize);
	}

	((CSocketThread*)pThis->m_pSocketThread)->Send(pSendBuf, uiSendBufLen);

	// Remove Stream Thread
	if (SVC_FILE_CLOSE == SVC_FILE_OPEN + nOpCode)
	{
		SAFE_DELETE(pThis->m_pStreamThread);
	}

	return 0;
}