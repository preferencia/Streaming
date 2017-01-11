#pragma once

#include "ClientBase64Url.h"

void    SetKeySrcValue(char* pszSrc, int nSrcLen);
int     MakeCryptionKey(char* pszIn, int nInputLen, char** pszOut);
int     DecodeCryptionKey(char* pszIn, int nInputLen, char** pszOut);