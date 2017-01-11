//////////////////////////////////////////////////////////////////////////
// 2008.10.7 blue2net
/********************************************************************
created:	2009/02/26
created:	26:2:2009   14:50
filename: 	Project Afreeca\Include\TraceLog.h
file path:	Project Afreeca\Include
file base:	TraceLog
file ext:	h
author:		blue2net

purpose:	debug helper Ŭ����.
����	:	
1.	GETSETTINGVALUE(�±װ�, ��ȯ�� �������, ����ũ��);
GETSETTINGVALUE(�±װ�, UINT �� ����);
- �������Ͽ��� �ش� �±װ��� �о� ���ۿ� �����Ѵ�. ��ȯ���� FALSE�� ��� ����

2. TRACELOG(outlevel, ����� �ؽ�Ʈ)
- outlevel�� ����Ʈ�� ���. �ܰ�� �ϴ� enum�� ����.

3. ReLoadingConfigFile();
- ���� �߿� ini ������ �ٽ� �о� �鿩�� ���� �߰��ϰų� ���� �Ͽ� �޼������� ���� ���� ��� ȣ��. ��, ini ������ �ٽ� �о� ���δ�. 



INI ���� ����. �����̸��� afcsetting.ini �� ����.
�ش� ���� 1�� ��쿡�� ����. 0�̸� ������� �ʴ´�.
�� ������ ���μ����� ����Ǵ� ������ �־�־���.
service_name	cinemaServer		; ���� �̸�, ���� ���ٸ� ���������̸����� ���
log_file    	c:\logfile.txt		; �α�����, ���� ��쿣 ���� ���������� logfile.txt �� ����.

showfunctionin		1					; LEVEL_IN  �Լ� ���� ���
showfunctionout		1					; LEVEL_OUT �Լ� ���� ���
showdebugstring		1					; LEVEL_DBG �Ϲ� ����� �޼��� ���
writelogfile		1					; LEVEL_LOG �̰��� �����ϸ� LEVEL_LOG�� �ش��ϴ� �α׵��� ���Ͽ� ����.
debugview			1					; �޼������� outputdebugstring ���� ����ʹٸ� ����
log_backup_hour		3					; �α����� ��� �ֱ�. ���������� �ð��� ���..
*********************************************************************/

#pragma  once

#include <stdio.h>
#include <process.h>
#include "ConfigFile.h"
#include <conio.h>

#ifndef __FUNCTION__
	#define __FUNCTION__ __FILE__
#endif

// �����ڵ� ó��
#ifdef _UNICODE

	#define __toWideChar(str) L##str
	#define _toWideChar(str) __toWideChar(str)
	#define __TFUNC__ _toWideChar(__FUNCTION__)
	#define __TTIME__ _toWideChar(__TIME__)

#else

	#define __TFUNC__ __FUNCTION__
	#define __TTIME__ __TIME__

#endif

// Interface ��ũ�� �Լ� ����
#ifndef _TIME_CHECK

	#define TRACELOG  CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).Log													// �α� ���
	#define GETVALUESTRING(GetValueString, OutValue, nbuffsize) CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).GetValue(GetValueString, OutValue,nbuffsize)
	#define GETVALUEINT(GetValueString, OutValue) CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).GetValue(GetValueString, OutValue)

#else

	#define TRACELOG  CDebugHelper::GetStaticClass(__TFUNC__, __TTIME__, __LINE__).Log													// �α� ���
	#define GETVALUESTRING(GetValueString, OutValue, nbuffsize) CDebugHelper::GetStaticClass(__TFUNC__, __TTIME__, __LINE__).GetValue(GetValueString, OutValue,nbuffsize)
	#define GETVALUEINT(GetValueString, OutValue) CDebugHelper::GetStaticClass(__TFUNC__, __TTIME__, __LINE__).GetValue(GetValueString, OutValue)

#endif

#ifdef _UNICODE
#define ReLoadingConfigFile() CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).ReadConfig(_T("afcsettingU.ini"), FALSE)
#else
#define ReLoadingConfigFile() CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).ReadConfig(_T("afcsetting.ini"), FALSE)
#endif


//////////////////////////////////////////////////////////////////////////
// ��� ���� ����.
// ���ϴ� ��� ���� ���� �߰�, ���� ����. 
enum 
{
	LEVEL_IN		= 1,					// �Լ� IN 
	LEVEL_OUT		= 2,					// �Լ� OUT
	LEVEL_DBG		= 4,					// �Ϲ� ����� �޼���
	LEVEL_ERROR		= 16,					// ���� �޼���.		
	LEVEL_LOG		= 32,					// �޼����� ���Ͽ� ����
};

class CDebugHelper
{
	CConfigFile*		m_pConf;						// �������� ����
	FILE*       		m_hLogFile;						// �α����� �ڵ�
	TCHAR      			m_szProcessName[_MAX_FNAME];	// �����̸� ����
	SYSTEMTIME  		m_tm;
	TCHAR       		m_ptcTimeBuf[32];
	TCHAR*      		m_ptcFileName;
	int         		m_iLastBackupDay;
	int         		m_iDailyBackupHour;
	BOOL				m_bSettingLoadded;				// ���� ������ �о��°�?
	BOOL				m_bViewDebugMessage;			// outputdebugstring ���� �޼����� ���� �ʹٸ� debugview ����

	CRITICAL_SECTION	m_cr;						// ����ȭ

	TCHAR				s_szMsgBuf[8192];
	LPTSTR				m_szFunc;
	LPTSTR				m_szTime;
	int					m_nLine;	
	UINT				m_nTraceLevel;				// trace ��� ���� ����.

public:

	CDebugHelper() 
	{
		m_hLogFile			= NULL;
		m_ptcFileName		= NULL;
		m_iLastBackupDay	= -1;
		m_iDailyBackupHour	= -1;
		m_nTraceLevel		= LEVEL_ERROR;			// �⺻������ ���� �޼����� �������
		m_pConf				= new CConfigFile;		
		InitializeCriticalSection(&m_cr);
#ifdef _UNICODE
        m_bSettingLoadded	= ReadConfig(_T("afcsettingU.ini"), TRUE);					// ���� �ε��ϴ°� ����. 
#else
        m_bSettingLoadded	= ReadConfig(_T("afcsetting.ini"), TRUE);					// ���� �ε��ϴ°� ����. 
#endif
	}

	~CDebugHelper() 
	{
		if (m_hLogFile != NULL)
		{
			fflush(m_hLogFile);
			fclose(m_hLogFile);			
		}	

		if (m_ptcFileName)
		{
			free(m_ptcFileName);
		}

		if(m_pConf)
		{
			delete m_pConf;
		}
	}

	static CDebugHelper& GetStaticClass(LPCTSTR lpszFunc, int nLine= 0) 
	{
		static CDebugHelper _Log;					// ������ ��ü

		_Log.m_szFunc	= (LPTSTR)lpszFunc;
		_Log.m_nLine	= nLine;

		return _Log; 
	}

	// _TIME_CHECK ���� �� ���
	static CDebugHelper& GetStaticClass(LPCTSTR lpszFunc, LPCTSTR lpszTime, int nLine= 0) 
	{
		static CDebugHelper _Log;					// ������ ��ü

		_Log.m_szFunc	= (LPTSTR)lpszFunc;
		_Log.m_szTime	= (LPTSTR)lpszTime;
		_Log.m_nLine	= nLine;

		return _Log;
	}

	void BackUpCheck()
	{
		::GetLocalTime(&m_tm);

		if (m_iDailyBackupHour != -1)
		{
			if (m_iLastBackupDay != m_tm.wDay && m_iDailyBackupHour <= m_tm.wHour)
			{
				fclose(m_hLogFile);
				m_hLogFile = NULL;

				TCHAR ptcNewFile[MAX_PATH + 1];
				TCHAR ptcBuf[64];

				_stprintf(ptcBuf, _T("_%d%02d%02d_logbackup"), m_tm.wYear, m_tm.wMonth, m_tm.wDay);

				_tcscpy(ptcNewFile, m_ptcFileName);
				_tcscat(ptcNewFile, ptcBuf);

				if (_trename(m_ptcFileName, ptcNewFile) == 0)
					m_iLastBackupDay  = m_tm.wDay;

				OpenLogfile(NULL);
			}
		}
	}

	void Log(int level, LPCTSTR lpszFmt=NULL, ...) 
	{
		if(!m_bSettingLoadded)
			return;

		EnterCriticalSection(&m_cr);
		BackUpCheck();

		if(m_nTraceLevel & level)
		{

			_stprintf(m_ptcTimeBuf, _T("%02d:%02d:%02d"), m_tm.wHour, m_tm.wMinute, m_tm.wSecond);		

			LPTSTR cxtBuf = s_szMsgBuf + 
				_stprintf(s_szMsgBuf, (lpszFmt != NULL)  ? _T("%s [%d] %s(%d) - ") : _T("%s [%d] %s(%d)"),
				m_ptcTimeBuf, level, m_szFunc, m_nLine);

			if(lpszFmt != NULL)
			{
				va_list arg;
				va_start(arg, lpszFmt);
				_vstprintf(cxtBuf, lpszFmt, arg);
				va_end(arg);			
			}

			if(m_bViewDebugMessage)
			{
				OutputDebugString(s_szMsgBuf);
				OutputDebugString(_T("\n"));
			}

			// LEVEL_LOG �Ǵ� LEVEL_ERROR�� ��쿡 ���Ͽ� ����.
			if( m_hLogFile && (level == LEVEL_ERROR || level == LEVEL_LOG) )
			{
                _stprintf(m_ptcTimeBuf, _T("%d-%02d-%02d "),
                    m_tm.wYear, m_tm.wMonth, m_tm.wDay);			

                if (_ftprintf(m_hLogFile, _T("%s "), m_ptcTimeBuf) < 0)
                    goto BAIL;

                if (_ftprintf(m_hLogFile, s_szMsgBuf) < 0)
                    goto BAIL;

                if (_ftprintf(m_hLogFile, _T("\r\n")) < 0)
                    goto BAIL;

				fflush(m_hLogFile);
			}
		}

		LeaveCriticalSection(&m_cr);										// Unlock
		return;
BAIL:
		OpenLogfile(NULL);
		return;
	}
	BOOL OpenLogfile(LPCTSTR pszFilename)
	{		
		if (pszFilename)
		{
			if (m_ptcFileName != NULL)
			{
				free(m_ptcFileName);
				m_ptcFileName = NULL;
			}

			m_ptcFileName = _tcsdup(pszFilename);
		}

		if (m_ptcFileName == NULL)
			return FALSE;

		if (m_hLogFile != NULL)
		{
			fclose(m_hLogFile);
			m_hLogFile = NULL;
		}

		m_hLogFile = _tfopen(m_ptcFileName, _T("ab"));

		if(m_hLogFile == NULL)
		{
			TCHAR tmp[100] = {0,};
			_tcscpy(tmp, m_ptcFileName);
			_tcscat(tmp, _T(" File Open Fail_afcsetting.ini ������ Ÿ�� ����"));

			OutputDebugString(tmp);
			return FALSE;
		}

#ifdef _UNICODE
		fseek(m_hLogFile, 0, SEEK_END);

		if (ftell(m_hLogFile) == 0)
			fprintf(m_hLogFile, "%c%c", 0xff, 0xfe);
#endif

		return TRUE;
	};

	void SetDailyBackupTime(int iHour)
	{
		if (iHour < 0 || iHour > 23)
		{
			this->m_iLastBackupDay = -1;
			iHour = -1;
		}
		else
		{
			::GetLocalTime(&this->m_tm);
			this->m_iLastBackupDay = this->m_tm.wDay;
		}

		this->m_iDailyBackupHour = iHour;

		return;

	};

	BOOL ReadConfig(TCHAR *ptcFileName, BOOL bInit=FALSE)
	{
		TCHAR *ptcValue;		
		TCHAR	szFullFilepath[512] = {0};
		TCHAR	Drive[_MAX_DRIVE]={0};
		TCHAR	Path[_MAX_PATH]={0};

		HMODULE hModule = ::GetModuleHandle(NULL);								//handle of current module
		::GetModuleFileName(hModule, szFullFilepath, 512);						// Get Full Path		
		_tsplitpath(szFullFilepath, Drive, Path, m_szProcessName, NULL);		// parse path data
		ZeroMemory(szFullFilepath, sizeof(szFullFilepath));
		_stprintf(szFullFilepath, _T("%s%s%s"), Drive, Path, ptcFileName);
		m_nTraceLevel		= LEVEL_ERROR;

		if (m_pConf == NULL)
		{
			_tprintf(_T("�޸� ����\n"));
			return FALSE;
		}

		if (m_pConf->Open(szFullFilepath) < 0)
		{
			_tprintf(_T("%s\n���������� �� �� ���ų� �������� ���� ����\n"), ptcFileName);
			return FALSE;
		}

		if ((ptcValue = m_pConf->GetValue(_T("log_file"), NULL)) != NULL)						// �α�����
		{
			OpenLogfile(ptcValue);
			free(ptcValue);
		}
		else
		{
			ZeroMemory(szFullFilepath, sizeof(szFullFilepath));
			_stprintf(szFullFilepath, _T("%s%slogfile.txt"), Drive, Path);
			OpenLogfile(szFullFilepath);
		}

		if(bInit)
		{
			if ((ptcValue = m_pConf->GetValue(_T("log_backup_hour"), NULL)) != NULL)				// �α����� ��� �ֱ�
			{
				SetDailyBackupTime(_ttoi(ptcValue));
				free(ptcValue);
			}
		}


		if ((ptcValue = m_pConf->GetValue(_T("showfunctionin"), NULL)) != NULL)					// �Լ� ���� �޼��� ���� ����
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel = LEVEL_IN;
			free(ptcValue);
		}


		if ((ptcValue = m_pConf->GetValue(_T("showfunctionout"), NULL)) != NULL)				// �Լ� ��ȯ �� �޼��� ���� ����
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel |= LEVEL_OUT;
			free(ptcValue);
		}


		if ((ptcValue = m_pConf->GetValue(_T("showdebugstring"), NULL)) != NULL)				// ����� �޼��� ���� ����
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel |= LEVEL_DBG;
			free(ptcValue);
		}

		if ((ptcValue = m_pConf->GetValue(_T("writelogfile"), NULL)) != NULL)					// �α����� ��� ����
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel |= LEVEL_LOG;
			free(ptcValue);
		}
		if ((ptcValue = m_pConf->GetValue(_T("debugview"), NULL)) != NULL)						// outputdebugstring �� ��¿���
		{
			m_bViewDebugMessage = _ttoi(ptcValue);													
			free(ptcValue);
		}
		
		return TRUE;
	};

	BOOL GetValue(TCHAR* pszGetValue, OUT TCHAR* pszOutString, IN UINT nbuffersize)
	{
		TCHAR	*ptcValue;
		size_t	nLen = 0;
		if ((ptcValue = m_pConf->GetValue(pszGetValue, NULL)) == NULL)						//
			return FALSE;
		nLen = _tcslen(ptcValue);
		if(nLen > nbuffersize)								// ���� �˻�
			return FALSE;

		_tcsncpy(pszOutString, ptcValue, nLen);
		free(ptcValue);
		return TRUE;
	}

	BOOL GetValue(TCHAR* pszGetValue, OUT INT& pOutValue, IN UINT nbuffersize=0)
	{
		TCHAR *ptcValue;
		UINT	nLen = 0;
		if ((ptcValue = m_pConf->GetValue(pszGetValue, NULL)) == NULL)						//
			return FALSE;

		pOutValue = _ttoi(ptcValue);

		free(ptcValue);
		return TRUE;
	}
};