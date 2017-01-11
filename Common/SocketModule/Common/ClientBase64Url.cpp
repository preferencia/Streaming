#include "stdafx.h"
#include "ClientBase64Url.h"

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {
						
						size_t outLen = 4 * ((input_length + 2) / 3);
                       
                        char *encoded_data = (char*)malloc(outLen);

                        int i = 0, j = 0;

						if (encoded_data == NULL) {
							// 에러코드...
							return NULL;
						}						
						memset(encoded_data, 0, outLen);
						*output_length = outLen;

                        for (i = 0, j = 0; i < input_length;) {
                            uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
                            uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
                            uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

                            uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

                            encoded_data[j++] = g_Encoding_Table[(triple >> 3 * 6) & 0x3F];
                            encoded_data[j++] = g_Encoding_Table[(triple >> 2 * 6) & 0x3F];
                            encoded_data[j++] = g_Encoding_Table[(triple >> 1 * 6) & 0x3F];
                            encoded_data[j++] = g_Encoding_Table[(triple >> 0 * 6) & 0x3F];
                        }

						int cnt = g_Mod_Table[input_length % 3];
						for (i = 0; i < cnt; i++) {
                            encoded_data[outLen - 1 - i] = '=';
						}
						
						__try
						{
							cnt = strlen(encoded_data);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							cnt = j-1;				
						}

                        for (i = 0; i < cnt; i++) {
                            __try
                            {
                                if (' ' == encoded_data[i]) {
                                    encoded_data[i] = '+';
                                }
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER)
                            {
                                delete [] encoded_data;
                                encoded_data = NULL;
                                
                                return NULL;
                            }
                        }
                        return encoded_data;
}

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {

                                 if (g_Decoding_Table == NULL) build_decoding_table();

                                 if (input_length % 4 != 0) return NULL;

                                 *output_length = input_length / 4 * 3;
                                 if (data[input_length - 1] == '=') (*output_length)--;
                                 if (data[input_length - 2] == '=') (*output_length)--;

                                 unsigned char *decoded_data = (unsigned char *)malloc(*output_length);
                                 if (decoded_data == NULL) return NULL;

                                 for (int i = 0, j = 0; i < input_length;) {

                                     uint32_t sextet_a = data[i] == '=' ? 0 & i++ : g_Decoding_Table[data[i++]];
                                     uint32_t sextet_b = data[i] == '=' ? 0 & i++ : g_Decoding_Table[data[i++]];
                                     uint32_t sextet_c = data[i] == '=' ? 0 & i++ : g_Decoding_Table[data[i++]];
                                     uint32_t sextet_d = data[i] == '=' ? 0 & i++ : g_Decoding_Table[data[i++]];

                                     uint32_t triple = (sextet_a << 3 * 6)
                                         + (sextet_b << 2 * 6)
                                         + (sextet_c << 1 * 6)
                                         + (sextet_d << 0 * 6);

                                     if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
                                     if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
                                     if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
                                 }

                                 return decoded_data;
}


void build_decoding_table() {

    g_Decoding_Table = (char*)malloc(256);

    for (int i = 0; i < 64; i++)
        g_Decoding_Table[(unsigned char) g_Encoding_Table[i]] = i;
}


void base64_cleanup() {
    free(g_Decoding_Table);
    g_Decoding_Table = NULL;
}