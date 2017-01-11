#include "stdafx.h"
#include "CommonDef.h"
#include "FSCryption.h"

void SetKeySrcValue(char* pszSrc, int nSrcLen)
{
    if ((NULL == pszSrc) || (0 >= nSrcLen))
    {
        return;
    }

    srand(time(NULL));

    for (int nIndex = 0; nIndex < nSrcLen; ++nIndex)
    {
        *(pszSrc + nIndex) = g_Encoding_Table[rand() % sizeof(g_Encoding_Table)];
    }
}

int MakeCryptionKey(char* pszIn, int nInputLen, char** pszOut)
{
    if ((NULL == pszIn) || (0 >= nInputLen))
    {
        return NULL;
    }

    if (0 == strlen(pszIn))
    {
        SetKeySrcValue(pszIn, nInputLen);
    }

    SAFE_DELETE_ARRAY(*pszOut);

    int             nOutLen     = 0;
    char*           pszEncData  = base64_encode((unsigned char*)pszIn, nInputLen, (size_t*)&nOutLen);

    if ((NULL == pszEncData) || (0 == nOutLen))
    {
        return 0;
    }

    *pszOut = new char[nOutLen + 1];
    memset(*pszOut, 0, nOutLen + 1);
    memcpy(*pszOut, pszEncData, nOutLen);

    SAFE_DELETE_ARRAY(pszEncData);

    return nOutLen;
}

int DecodeCryptionKey(char* pszIn, int nInputLen, char** pszOut)
{
	SAFE_DELETE_ARRAY(*pszOut);

    int             nOutLen     = 0;
    char*           pszDecData  = (char*)base64_decode(pszIn, nInputLen, (size_t*)&nOutLen);

    if ((NULL == pszDecData) || (0 == nOutLen))
    {
        return 0;
    }

    *pszOut = new char[nOutLen + 1];
    memset(*pszOut, 0, nOutLen + 1);
    memcpy(*pszOut, pszDecData, nOutLen);

    base64_cleanup();
    SAFE_DELETE_ARRAY(pszDecData);

    return nOutLen;
}