#ifndef _BASE64_H
#define _BASE64_H

#define  MAXLINELEN  80

#ifdef __cplusplus
#include <string>
	std::string base64_encode(const char * , unsigned int len);
	std::string base64_decode(std::string const& s);
extern "C" {
#endif
    char *base64_encode_line( const char *s,char* out);
	char *base64_encode_string(const char *src, unsigned int len,char * out);
    char* base64_decode_string(const char* pSrc,char* out,int* outlen);
#ifdef __cplusplus
}
#endif

#endif
