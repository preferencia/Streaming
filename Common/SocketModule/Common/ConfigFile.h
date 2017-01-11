#pragma once

#include <tchar.h>
#include <malloc.h>

#define CONF_COMMENT    _T(';')
#define BUFSIZE 1024

typedef struct CONFIGFILEtag CONF1, *PCONF;

struct CONFIGFILEtag
{
    TCHAR   *ptcName;
    TCHAR   *ptcValue;
    PCONF	pNext;
};


class CConfigFile
{
public:
    CConfigFile(void)
	{
		m_pConf = NULL;
	};
    ~CConfigFile(void)
	{
		if (m_pConf != NULL)
			Close();
	}

    int Open(TCHAR* ptcFileName)
	{
		FILE*   pFile;
		PCONF   pNew;
		PCONF   pPrev;
		TCHAR   ptcBuf[BUFSIZE];
		TCHAR*  ptcName;
		TCHAR*  ptcValue;
		int     iLine;

		if (!(pFile = _tfopen(ptcFileName, _T("rb"))))
			return -1;

		this->m_pConf = pPrev = NULL;

		for (iLine = 1; _fgetts(ptcBuf, BUFSIZE, pFile); iLine++)
		{
			if ((ptcName = _tcschr(ptcBuf, CONF_COMMENT)) != NULL)
				*ptcName = _T('\0');

			for (ptcName = ptcBuf; *ptcName != _T('\0') && IsSkipChar(*ptcName); ptcName++);

			if (*ptcName != _T('\0'))
			{
				for (ptcValue = ptcName + _tcslen(ptcName) - 1;
					ptcValue > ptcName && IsSkipChar(*ptcValue); ptcValue--)
					;

				if (ptcValue == ptcName)
					continue;
				else
					*(ptcValue + 1) = _T('\0');

				for (ptcValue = ptcName; *ptcValue != _T('\0') && !IsSkipChar(*ptcValue); ptcValue++);

				if (*ptcValue != _T('\0'))
				{
					*ptcValue++ = _T('\0');

					for (; *ptcValue != _T('\0') && IsSkipChar(*ptcValue); ptcValue++);

					if (*ptcValue != _T('\0'))
					{
						if (!(pNew = (CONF1 *)malloc(sizeof(CONF1))))
						{
							iLine   = -1;
							goto BAIL;
						}

						pNew->ptcName   = _tcsdup(ptcName);
						pNew->ptcValue  = _tcsdup(ptcValue);
						pNew->pNext     = NULL;

						if (!m_pConf)
							m_pConf = pNew;

						if (pPrev)
							pPrev->pNext = pNew;

						pPrev = pNew;
					}
				}
				else
				{
					iLine = -1;
					goto BAIL;
				}
			}
		}

BAIL:
		fclose(pFile);

		if (iLine < 0)
		{
			Close();
			m_pConf = NULL;
		}

		return 0;
	}
    void Close(void)
	{
		PCONF   pPrev;
		PCONF   pConf;

		pConf = m_pConf;

		for (pPrev = NULL; pConf; pConf = pConf->pNext)
		{
			if (pConf->ptcName)
				free(pConf->ptcName);

			if (pConf->ptcValue)
				free(pConf->ptcValue);

			if (pPrev)
				free(pPrev);

			pPrev = pConf;
		}

		if (pPrev)
			free(pPrev);

		return;
	}

    TCHAR* GetValue(TCHAR* ptcName, TCHAR* ptcValue)
	{
		PCONF   pTmp;

		if (ptcName == NULL)
			return NULL;

		for (pTmp = m_pConf; pTmp; pTmp = pTmp->pNext)
		{
			if (!_tcscmp(pTmp->ptcName, ptcName))
			{
				if (ptcValue)
					_tcscpy(ptcValue, pTmp->ptcValue);
				else
					ptcValue = _tcsdup(pTmp->ptcValue);

				return ptcValue;
			}
		}

		return NULL;
	}

private:

    PCONF	m_pConf;
    int IsSkipChar(TCHAR c)
    {
        return ((c == _T('\n') || c == _T('\r') || c == _T(' ') || c == _T('\t')) ? 1 : 0);
    }
};