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

purpose:	debug helper 클래스.
사용법	:	
1.	GETSETTINGVALUE(태그값, 반환값 저장버퍼, 버퍼크기);
GETSETTINGVALUE(태그값, UINT 형 변수);
- 설정파일에서 해당 태그값을 읽어 버퍼에 저장한다. 반환값이 FALSE일 경우 실패

2. TRACELOG(outlevel, 출력할 텍스트)
- outlevel로 텏스트를 출력. 단계는 하단 enum값 참조.

3. ReLoadingConfigFile();
- 실행 중에 ini 파일을 다시 읽어 들여서 값을 추가하거나 제거 하여 메세지들을 보고 싶을 경우 호출. 즉, ini 파일을 다시 읽어 들인다. 



INI 파일 내용. 파일이름은 afcsetting.ini 로 고정.
해당 값이 1일 경우에만 노출. 0이면 노출되지 않는다.
이 파일은 프로세스가 실행되는 폴더에 넣어둬야함.
service_name	cinemaServer		; 서비스 이름, 값이 없다면 실행파일이름으로 출력
log_file    	c:\logfile.txt		; 로그파일, 없을 경우엔 현재 실행폴더에 logfile.txt 로 생성.

showfunctionin		1					; LEVEL_IN  함수 진입 출력
showfunctionout		1					; LEVEL_OUT 함수 종료 출력
showdebugstring		1					; LEVEL_DBG 일반 디버그 메세지 출력
writelogfile		1					; LEVEL_LOG 이것을 선언하면 LEVEL_LOG에 해당하는 로그들은 파일에 쓴다.
debugview			1					; 메세지들을 outputdebugstring 으로 보고싶다면 선언
log_backup_hour		3					; 로그파일 백업 주기. 매일지정된 시간에 백업..
*********************************************************************/

#pragma  once

#include <stdio.h>
#include <process.h>
#include "ConfigFile.h"
#include <conio.h>

#ifndef __FUNCTION__
	#define __FUNCTION__ __FILE__
#endif

// 유니코드 처리
#ifdef _UNICODE

	#define __toWideChar(str) L##str
	#define _toWideChar(str) __toWideChar(str)
	#define __TFUNC__ _toWideChar(__FUNCTION__)
	#define __TTIME__ _toWideChar(__TIME__)

#else

	#define __TFUNC__ __FUNCTION__
	#define __TTIME__ __TIME__

#endif

// Interface 매크로 함수 정의
#ifndef _TIME_CHECK

	#define TRACELOG  CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).Log													// 로그 출력
	#define GETVALUESTRING(GetValueString, OutValue, nbuffsize) CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).GetValue(GetValueString, OutValue,nbuffsize)
	#define GETVALUEINT(GetValueString, OutValue) CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).GetValue(GetValueString, OutValue)

#else

	#define TRACELOG  CDebugHelper::GetStaticClass(__TFUNC__, __TTIME__, __LINE__).Log													// 로그 출력
	#define GETVALUESTRING(GetValueString, OutValue, nbuffsize) CDebugHelper::GetStaticClass(__TFUNC__, __TTIME__, __LINE__).GetValue(GetValueString, OutValue,nbuffsize)
	#define GETVALUEINT(GetValueString, OutValue) CDebugHelper::GetStaticClass(__TFUNC__, __TTIME__, __LINE__).GetValue(GetValueString, OutValue)

#endif

#ifdef _UNICODE
#define ReLoadingConfigFile() CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).ReadConfig(_T("afcsettingU.ini"), FALSE)
#else
#define ReLoadingConfigFile() CDebugHelper::GetStaticClass(__TFUNC__, __LINE__).ReadConfig(_T("afcsetting.ini"), FALSE)
#endif


//////////////////////////////////////////////////////////////////////////
// 출력 레벨 설정.
// 원하는 대로 때에 따라 추가, 삭제 가능. 
enum 
{
	LEVEL_IN		= 1,					// 함수 IN 
	LEVEL_OUT		= 2,					// 함수 OUT
	LEVEL_DBG		= 4,					// 일반 디버깅 메세지
	LEVEL_ERROR		= 16,					// 에러 메세지.		
	LEVEL_LOG		= 32,					// 메세지를 파일에 쓰기
};

class CDebugHelper
{
	CConfigFile*		m_pConf;						// 설정내용 저장
	FILE*       		m_hLogFile;						// 로그파일 핸들
	TCHAR      			m_szProcessName[_MAX_FNAME];	// 파일이름 저장
	SYSTEMTIME  		m_tm;
	TCHAR       		m_ptcTimeBuf[32];
	TCHAR*      		m_ptcFileName;
	int         		m_iLastBackupDay;
	int         		m_iDailyBackupHour;
	BOOL				m_bSettingLoadded;				// 설정 파일을 읽었는가?
	BOOL				m_bViewDebugMessage;			// outputdebugstring 으로 메세지를 보고 싶다면 debugview 선언

	CRITICAL_SECTION	m_cr;						// 동기화

	TCHAR				s_szMsgBuf[8192];
	LPTSTR				m_szFunc;
	LPTSTR				m_szTime;
	int					m_nLine;	
	UINT				m_nTraceLevel;				// trace 출력 레벨 설정.

public:

	CDebugHelper() 
	{
		m_hLogFile			= NULL;
		m_ptcFileName		= NULL;
		m_iLastBackupDay	= -1;
		m_iDailyBackupHour	= -1;
		m_nTraceLevel		= LEVEL_ERROR;			// 기본적으로 에러 메세지는 강제출력
		m_pConf				= new CConfigFile;		
		InitializeCriticalSection(&m_cr);
#ifdef _UNICODE
        m_bSettingLoadded	= ReadConfig(_T("afcsettingU.ini"), TRUE);					// 파일 로딩하는거 고정. 
#else
        m_bSettingLoadded	= ReadConfig(_T("afcsetting.ini"), TRUE);					// 파일 로딩하는거 고정. 
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
		static CDebugHelper _Log;					// 유일한 객체

		_Log.m_szFunc	= (LPTSTR)lpszFunc;
		_Log.m_nLine	= nLine;

		return _Log; 
	}

	// _TIME_CHECK 정의 시 사용
	static CDebugHelper& GetStaticClass(LPCTSTR lpszFunc, LPCTSTR lpszTime, int nLine= 0) 
	{
		static CDebugHelper _Log;					// 유일한 객체

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

			// LEVEL_LOG 또는 LEVEL_ERROR일 경우에 파일에 쓴다.
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
			_tcscat(tmp, _T(" File Open Fail_afcsetting.ini 있을때 타는 로직"));

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
			_tprintf(_T("메모리 에러\n"));
			return FALSE;
		}

		if (m_pConf->Open(szFullFilepath) < 0)
		{
			_tprintf(_T("%s\n설정파일을 열 수 없거나 설정파일 문법 오류\n"), ptcFileName);
			return FALSE;
		}

		if ((ptcValue = m_pConf->GetValue(_T("log_file"), NULL)) != NULL)						// 로그파일
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
			if ((ptcValue = m_pConf->GetValue(_T("log_backup_hour"), NULL)) != NULL)				// 로그파일 백업 주기
			{
				SetDailyBackupTime(_ttoi(ptcValue));
				free(ptcValue);
			}
		}


		if ((ptcValue = m_pConf->GetValue(_T("showfunctionin"), NULL)) != NULL)					// 함수 진입 메세지 노출 여부
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel = LEVEL_IN;
			free(ptcValue);
		}


		if ((ptcValue = m_pConf->GetValue(_T("showfunctionout"), NULL)) != NULL)				// 함수 반환 전 메세지 노출 여부
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel |= LEVEL_OUT;
			free(ptcValue);
		}


		if ((ptcValue = m_pConf->GetValue(_T("showdebugstring"), NULL)) != NULL)				// 디버그 메세지 노출 여부
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel |= LEVEL_DBG;
			free(ptcValue);
		}

		if ((ptcValue = m_pConf->GetValue(_T("writelogfile"), NULL)) != NULL)					// 로그파일 기록 여부
		{
			if(_ttoi(ptcValue))
				m_nTraceLevel |= LEVEL_LOG;
			free(ptcValue);
		}
		if ((ptcValue = m_pConf->GetValue(_T("debugview"), NULL)) != NULL)						// outputdebugstring 로 출력여부
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
		if(nLen > nbuffersize)								// 버퍼 검사
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