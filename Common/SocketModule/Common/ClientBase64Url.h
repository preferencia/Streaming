#pragma once

#ifndef _WIN32

#include <stdint.h>

#else   // _WIN32

#ifndef uint8_t
#define uint8_t  unsigned char
#endif

typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;

#endif  // _WIN32

static char g_Encoding_Table[] = {
                                    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                    'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                    '4', '5', '6', '7', '8', '9', '+', '/'
                                 };
static char *g_Decoding_Table = NULL;
static int g_Mod_Table[] = {0, 2, 1};


#ifdef __cplusplus
extern "C" { 
#endif

    //int base64_encode(char *text, int numBytes, char **encodedText);
    //int base64_decode(char *text, unsigned char *dst, int numBytes );

    char *base64_encode(const unsigned char *data,
        size_t input_length,
        size_t *output_length);

    unsigned char *base64_decode(const char *data,
        size_t input_length,
        size_t *output_length);

    void build_decoding_table();

    void base64_cleanup();

#ifdef __cplusplus
}
#endif
