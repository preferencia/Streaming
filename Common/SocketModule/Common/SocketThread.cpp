#include "StdAfx.h"
#include "SocketThread.h"
#include <process.h>

CSocketThread::CSocketThread(void)
{
    m_bRunSendThread        = FALSE;
    m_hSendThread           = NULL;
    m_Socket                = NULL;

    m_pSendDataQueue        = NULL;
    m_hSendDataQueueMutex   = NULL;

    m_nThreadType           = -1;
}

CSocketThread::~CSocketThread(void)
{
    if (NULL != m_hSendThread)
    {
        m_bRunSendThread = FALSE;
        while (NULL != m_hSendThread)
        {
            Sleep(100);
        }
    }

    if (0 < m_pSendDataQueue->size())
    {
        for (m_SendDataQueueIt = m_pSendDataQueue->begin(); m_SendDataQueueIt != m_pSendDataQueue->end(); ++m_SendDataQueueIt)
        {
            SendData* pSendData = *m_SendDataQueueIt;
            if (NULL != pSendData)
            {
                SAFE_DELETE_ARRAY(pSendData->pData);
                SAFE_DELETE(pSendData);
            }
        }
        m_pSendDataQueue->clear();
    }

    SAFE_DELETE(m_pSendDataQueue);

    if (NULL != m_hSendDataQueueMutex)
	{
		CloseHandle(m_hSendDataQueueMutex);
		m_hSendDataQueueMutex = NULL;
	}
}

BOOL CSocketThread::InitSocketThread()
{
    CConnectSocket* pConSock = GetSocket();
    if (NULL == pConSock)
    {
        return FALSE;
    }

    if (NULL != m_Socket)
    {
        pConSock->Attach(m_Socket);
    }

    m_pSendDataQueue = new SendDataQueue;
    m_pSendDataQueue->clear();

    m_hSendDataQueueMutex = CreateMutex(NULL, FALSE, NULL);
    if (NULL == m_hSendDataQueueMutex)
    {
        return FALSE;
    }

    m_hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
    if (NULL != m_hSendThread)
    {
        m_bRunSendThread = TRUE;
    }

	SaveDataLog(enumLogTypeStr, 0, "------------------[INIT_SOCKET_THREAD (%d)]----------------------", m_Socket);

    return TRUE;
}

void CSocketThread::Send(char* pData, int nDataLen)
{
    if ((NULL == pData) || (0 >= nDataLen))
    {
        TRACELOG(LEVEL_DBG, _T("Input Data is Wrong - Data = 0x%08x, Data Length = %d"), pData, nDataLen);
        return;
    }

    SendData* pSendData = new SendData;
    pSendData->nDataLen = nDataLen;
    pSendData->pData    = pData;

    WaitForSingleObject(m_hSendDataQueueMutex, INFINITE);
    m_pSendDataQueue->push_back(pSendData);
    ReleaseMutex(m_hSendDataQueueMutex);
}

unsigned int __stdcall CSocketThread::SendThread(void* lpParam)
{
	CSocketThread* pThis    = (CSocketThread*)lpParam;
	if (NULL == pThis)
	{
		return -1;
	}

	while (TRUE == pThis->m_bRunSendThread)
	{
		if (0 == pThis->m_pSendDataQueue->size())
		{
			Sleep(1);
			continue;
		}

		WaitForSingleObject(pThis->m_hSendDataQueueMutex, CLK_TCK);
		SendData* pSendData = pThis->m_pSendDataQueue->front();
		pThis->m_pSendDataQueue->pop_front();
		ReleaseMutex(pThis->m_hSendDataQueueMutex);

		if (NULL != pSendData)
		{
			if ((NULL != pSendData->pData) && (0 < pSendData->nDataLen))
			{
				int nRet = pThis->GetSocket()->DoSendData(pSendData->pData, pSendData->nDataLen);
				TRACELOG(LEVEL_DBG, _T("Data Length = %d, Send Result = %d"), pSendData->nDataLen, nRet);
                pThis->SaveDataLog(enumLogTypeStr, 0, "Send (DataLen = %d, Result= %d)", pSendData->nDataLen, nRet);
                pThis->SaveDataLog(enumLogTypeData, pSendData->nDataLen, pSendData->pData);

				SAFE_DELETE_ARRAY(pSendData->pData);
			}
			SAFE_DELETE(pSendData);
		} 
	}

	CloseHandle(pThis->m_hSendThread);
	pThis->m_hSendThread = NULL;

	return 1;
}

void CSocketThread::SaveDataLog(int nLogType, int nLen, char* pszFmt, ...)
{
	CTime CurTime = CTime::GetCurrentTime();

	char szLogFileName[MAX_PATH * 2]    = {0, };
	char szDirPath[MAX_PATH]            = {0, };
	char szThreadType[MAX_PATH]         = {0, };
	char szLogTime[MAX_PATH]            = {0, };

	GetModuleFileNameA(NULL, szDirPath, MAX_PATH);
	PathRemoveFileSpecA(szDirPath);

	if (0 == m_nThreadType)
	{
		strcpy(szThreadType, "Server");
	}
	else
	{
		strcpy(szThreadType, "Client");
		PathRemoveFileSpecA(szDirPath);
	}

	if (10 >= _snprintf(szLogFileName, (MAX_PATH * 2) - 1, "%s\\log\\SysTrd%s_%04d%02d%02d.log", 
		szDirPath, szThreadType, CurTime.GetYear(), CurTime.GetMonth(), CurTime.GetDay()))
	{
		return;
	}

	FILE* fp = fopen(szLogFileName, "a");
	if (NULL == fp)
		return;

	_snprintf(szLogTime, MAX_PATH - 1, "[%02d:%02d:%02d]", CurTime.GetHour(), CurTime.GetMinute(), CurTime.GetSecond());

	switch (nLogType)
	{
	case enumLogTypeStr:
		{
			if (NULL != pszFmt)
			{    
				char szLog[4096] = {0, };
				va_list arg;
				va_start(arg, pszFmt);
				vsprintf(szLog, pszFmt, arg);
				va_end(arg);
				fprintf(fp, "%s %s\n", szLogTime, szLog);
			}
		}
		break;

	case enumLogTypeData:
		{
			if (NULL != pszFmt)
			{
				int nPos    = sizeof(FS_HEADER);
				BOOL bPass  = FALSE;

				for (int nIndex = nPos; nIndex < nLen && nIndex < 21; (TRUE == bPass) ? nIndex += sizeof(int) : ++nIndex)
				{
					unsigned char ch = (unsigned char)*(pszFmt + nIndex);

					if (FALSE == isprint((int)ch))
					{
						nPos    += sizeof(int); 
						bPass   = TRUE;
					}
					else
					{
						bPass   = FALSE;
					}
				}

				PFS_HEADER pheader = (PFS_HEADER)pszFmt;
				// Header와 주문 관련 TR에 있는 key 값(int) 제외하고 저장
				fprintf(fp, "%s   svc,err[%4d,%5d] [H:%d] Data [", szLogTime, pheader->uiSvcCode, pheader->uiErrCode, nPos);
				if (nLen > nPos)
					fwrite(pszFmt + nPos, nLen - nPos, 1, fp);
				fprintf(fp, "]\n");
			}                    
		}
		break;

	default:
		{

		}
		break;
	}

	fclose(fp);
	fp = NULL;
}

