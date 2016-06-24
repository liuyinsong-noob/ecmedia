
#include <string.h>
#include <stdlib.h>
#include "base64_2.h"
#include "ctype.h"

const char *EnBase64Tab="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char DeBase64Tab[] =
{
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64, 0, 1, 2, 3, 4, 5, 6,
     7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
    64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
    49,50,51,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
};

char *base64_encode_line( const char *s,char* out)
{
	return base64_encode_string( s, (unsigned int)strlen( s ) ,out);
}


char *base64_encode_string(const char *src, unsigned int len,char * out)
{
    int nDiv;
    int nMod;
    unsigned char c1, c2, c3; 
    int i;
    int nPosLine = 0;     
    int curPos=0;
    
    if(len==0)
       return NULL;
   
    nDiv = len / 3;
    nMod = len % 3;
   
    for (i = 0; i < nDiv; i ++)
    {
        c1 = *src++;
        c2 = *src++;
        c3 = *src++;
 
        out[curPos] = EnBase64Tab[c1 >> 2];curPos++;
        out[curPos] = EnBase64Tab[((c1 << 4) | (c2 >> 4)) & 0x3f];curPos++;
        out[curPos] = EnBase64Tab[((c2 << 2) | (c3 >> 6)) & 0x3f];curPos++;
        out[curPos] = EnBase64Tab[c3 & 0x3f];curPos++;
        nPosLine += 4;
 
        if (nPosLine == MAXLINELEN)
        {
            out[curPos] = '\n';curPos++;
            nPosLine=0;
        }
    }
 
    if (nMod == 1)
    {
        c1 = *src++;
        out[curPos] = EnBase64Tab[(c1 & 0xfc) >> 2];curPos++;
        out[curPos] = EnBase64Tab[((c1 & 0x03) << 4)];curPos++;
        out[curPos] = '=';curPos++;
        out[curPos] = '=';curPos++;
    }
    else if (nMod == 2)
    {
        c1 = *src++;
        c2 = *src++;
        out[curPos] = EnBase64Tab[(c1 & 0xfc) >> 2];curPos++;
        out[curPos] = EnBase64Tab[((c1 & 0x03) << 4) | ((c2 & 0xf0) >> 4)];curPos++;
        out[curPos] = EnBase64Tab[((c2 & 0x0f) << 2)];curPos++;
        out[curPos] = '=';curPos++;
    }
    out[curPos] = '\0';
    return out;
}

char* base64_decode_string(const char* pSrc,char* out,int* outlen)
{
    int  len=0;
    if(*pSrc==0)
       return NULL;
    
    *outlen=0;

    while (*pSrc)
    {
        char c1,c2,c3,c4;
        for(;isspace(*pSrc);pSrc++);
        
        c1=*pSrc;pSrc++;
        if(c1==0)
           break;
        
        c2=*pSrc;pSrc++;
        c3=*pSrc;pSrc++;
        c4=*pSrc;pSrc++;
        if(c2==0 || c3== 0 || c4==0)
        {
           return NULL;
        }
        if(DeBase64Tab[c1] ==64 || DeBase64Tab[c2] ==64 || (DeBase64Tab[c3] ==64 &&  c3 !='=') || (DeBase64Tab[c4] ==64 && c4 !='='))
        {
           return NULL;
        }

        out[len] =(DeBase64Tab[c1] <<2 & 0xFC)  | (DeBase64Tab[c2]>>4 & 0x03);
        len++;
        if (c3 != '=')
        {
            out[len] =(DeBase64Tab[c2] <<4 & 0xF0)  | (DeBase64Tab[c3]>>2 & 0x0F);
            len++;
            if (c4 != '=')
            {
                out[len] =(DeBase64Tab[c3]<<6  & 0xC0)  | (DeBase64Tab[c4] & 0x3F);
                len++;
            }
        }
    }
    out[len]=0;
    *outlen = len;
    return out;
}
#ifdef __cplusplus

#include <memory>

std::string base64_encode(const char *src , unsigned int len)
{
	char *out = new char[len*2];
	base64_encode_string(src,strlen(src),out);
	std::string result = out;
	delete[] out;
	return result;
	
}
std::string base64_decode(std::string const& src)
{
	int outlen;
	char * out = new char[src.length()];
	base64_decode_string(src.c_str(),out,&outlen);
	std::string result = out;
	delete[] out;
	return result;
}
#endif
