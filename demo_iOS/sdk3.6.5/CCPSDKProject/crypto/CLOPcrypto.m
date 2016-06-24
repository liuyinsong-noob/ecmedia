#include "CLOPcrypto.h"
#include <string.h>

//#define CLOPFULL_UNROLL

/*øÏÀŸº”√‹À„∑®¬Î±Ì£¨«ÎŒ–ﬁ∏ƒ*/
static uint8_t rev[16] = { 0x0b, 0x04, 0x0f, 0x06, 0x01, 0x0a, 0x03, 0x09, 
						   0x0d, 0x07, 0x05, 0x00, 0x0e, 0x08, 0x0c, 0x02 };
static uint8_t sk[16]  = { 0xd7, 0x6a, 0xa4, 0x78, 0xf5, 0x7c, 0x42, 0xab,
						   0xa4, 0x52, 0xf6, 0x76, 0x3b, 0x4d, 0x61, 0xce };

/*≥ı ºªØº”Ω‚√‹µƒKEY*/
int clop_crypto_quick_init_key(clop_crypto_quick_key_t* key, const uint8_t* userkey, int len)
{
	static int __smask[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	uint8_t ukey[8] = { 0 };
	int i, j;

	/*∞¥8bits’πø™*/
	memset(key->rk, 0, 8);
	memset(key->iv, 0, 8);
	
	for (i=0; *userkey != '\0'; i++) 
		ukey[i&0x7] ^= *userkey++;

	for (i=0; i<8; i++)	for (j=0; j<8; j++)
		key->rk[i] |= ukey[(i+j)&0x07] & __smask[(i+j)&0x07];
	return 0;
}

/*øÏÀŸº”√‹À„∑®£¨ ‰≥ˆŒ™8±∂’˚ ˝*/
/* userkey∞¥64bits’πø™ */
/* src[0] => div(8) => ^iv[0]=> rev(16) => ^key[0] => ^sk[0] => dst[0] (dst[0] -> iv[next]) */
int clop_crypto_quick_enc(clop_crypto_quick_key_t* key, uint8_t* dst, const uint8_t* src, int len)
{
	uint8_t* base = dst;
	uint8_t* iv = key->iv;
	uint8_t* rk = key->rk;

	while (len > 0)
	{
#ifdef CLOPFULL_UNROLL
		iv[0] ^= src[7];
		iv[1] ^= src[6];
		iv[2] ^= src[5];
		iv[3] ^= src[4];
		iv[4] ^= src[3];
		iv[5] ^= src[2];
		iv[6] ^= src[1];
		iv[7] ^= src[0];

		dst[0] = (rev[iv[0]>>4] + (rev[iv[0]&0xF]<<4)) ^ rk[0] ^ sk[0];
		dst[1] = (rev[iv[1]>>4] + (rev[iv[1]&0xF]<<4)) ^ rk[1] ^ sk[1];
		dst[2] = (rev[iv[2]>>4] + (rev[iv[2]&0xF]<<4)) ^ rk[2] ^ sk[2];
		dst[3] = (rev[iv[3]>>4] + (rev[iv[3]&0xF]<<4)) ^ rk[3] ^ sk[3];
		dst[4] = (rev[iv[4]>>4] + (rev[iv[4]&0xF]<<4)) ^ rk[4] ^ sk[4];
		dst[5] = (rev[iv[5]>>4] + (rev[iv[5]&0xF]<<4)) ^ rk[5] ^ sk[5];
		dst[6] = (rev[iv[6]>>4] + (rev[iv[6]&0xF]<<4)) ^ rk[6] ^ sk[6];
		dst[7] = (rev[iv[7]>>4] + (rev[iv[7]&0xF]<<4)) ^ rk[7] ^ sk[7];
		
		memcpy(iv, dst, 8);
		
#else //CLOPFULL_UNROLL
		int n;
		for (n=0; n<8; n++)
		{
			iv[n] ^= src[7-n];
			iv[n] = (rev[iv[n]>>4] + (rev[iv[n]&0xF]<<4)) ^ rk[n] ^ sk[n];
			dst[n] = iv[n];
            //NSLog(@"AEScode.cpp vm_crypto_quick_enc,n=[%d],src[7-n]=[%d],dst=[%d],len=%d",n,src[7-n],dst[n],len);
		}

#endif //CLOPFULL_UNROLL
		
		len -= 8;
		src += 8;
		dst += 8;
	}
	return (dst - base);

}

/*øÏÀŸΩ‚√‹À„∑®£¨ ‰≥ˆŒ™8±∂’˚ ˝*/
/* src[0] => ^sk[0]=> ^key[0] => rev(16) => ^iv[0] => div(8) => dst[0] / (src[0]->iv[next]) */
int clop_crypto_quick_dec(clop_crypto_quick_key_t* key, uint8_t* dst, const uint8_t* src, int len)
{
	uint8_t* base = dst;
	uint8_t* iv = key->iv;
	uint8_t* rk = key->rk;
	uint8_t  r[8];
	
	while (len > 0) 
	{	
#ifdef CLOPFULL_UNROLL
		r[0] = src[0] ^ rk[0] ^ sk[0];
		r[1] = src[1] ^ rk[1] ^ sk[1];
		r[2] = src[2] ^ rk[2] ^ sk[2];
		r[3] = src[3] ^ rk[3] ^ sk[3];
		r[4] = src[4] ^ rk[4] ^ sk[4];
		r[5] = src[5] ^ rk[5] ^ sk[5];
		r[6] = src[6] ^ rk[6] ^ sk[6];
		r[7] = src[7] ^ rk[7] ^ sk[7];
		
		dst[7] = (rev[r[0]>>4] + (rev[r[0]&0xF]<<4)) ^ iv[0];
		dst[6] = (rev[r[1]>>4] + (rev[r[1]&0xF]<<4)) ^ iv[1];
		dst[5] = (rev[r[2]>>4] + (rev[r[2]&0xF]<<4)) ^ iv[2];
		dst[4] = (rev[r[3]>>4] + (rev[r[3]&0xF]<<4)) ^ iv[3];
		dst[3] = (rev[r[4]>>4] + (rev[r[4]&0xF]<<4)) ^ iv[4];
		dst[2] = (rev[r[5]>>4] + (rev[r[5]&0xF]<<4)) ^ iv[5];
		dst[1] = (rev[r[6]>>4] + (rev[r[6]&0xF]<<4)) ^ iv[6];
		dst[0] = (rev[r[7]>>4] + (rev[r[7]&0xF]<<4)) ^ iv[7];

		memcpy(iv, src, 8);
		
#else //CLOPFULL_UNROLL
		int n;
		for (n=0; n<8; n++)
		{
			r[n] = src[n] ^ rk[n] ^ sk[n];
			dst[7-n] = (rev[r[n]>>4] + (rev[r[n]&0xF]<<4)) ^ iv[n];
			iv[n] = src[n];
		}

#endif //CLOPFULL_UNROLL
		
		len -= 8;
		src += 8;
		dst += 8;
	}
	return (dst - base);
}


int CLOP_Encrypt_1(const unsigned char *in,int inlen,unsigned char *out,const unsigned char *key)
{
	clop_crypto_quick_key_t ekey;
	clop_crypto_quick_init_key(&ekey, key, 8);
	return clop_crypto_quick_enc(&ekey, out, in, inlen);
}

int CLOP_Decrypt_1(const unsigned char *in, int inlen, unsigned char *out,const unsigned char *key)
{
	clop_crypto_quick_key_t dkey;
	clop_crypto_quick_init_key(&dkey, key, 8);
	return clop_crypto_quick_dec(&dkey, out, in, inlen);
}

@implementation CLOPEncrypt

//加密
+ (NSString *)clop_encodedData:(NSString *)srcData andPrivateKey:(NSString*) theKey
{
    
    if ( !srcData || srcData.length == 0 )
    {
        return nil;
    }
    
    const char *cstrOriBody = [srcData cStringUsingEncoding:NSUTF8StringEncoding];
    int cstrOriBodyLen = (int)strlen(cstrOriBody);
    
    char *chEncodeBody = (char*)malloc(cstrOriBodyLen+8);
    memset(chEncodeBody, 0, cstrOriBodyLen+8);
    
    int encodelen = 0;
    
    int len8=cstrOriBodyLen%8;
    int myOriLen = cstrOriBodyLen;
	if(len8 > 0)
	{
		myOriLen=cstrOriBodyLen-len8+8;
	}
    
    char *myStrOriBody = (char*)malloc(myOriLen);
    memset(myStrOriBody, 0, myOriLen);
    memcpy(myStrOriBody,cstrOriBody,cstrOriBodyLen);
    
    encodelen = CLOP_Encrypt_1((const unsigned char*)myStrOriBody,
                              cstrOriBodyLen,
                              (unsigned char*)chEncodeBody,
                              (const unsigned char*)[theKey UTF8String]);
    
    NSData *encodedData = [[NSData alloc] initWithBytes:chEncodeBody length:encodelen];
    NSString *base64 = [self cloopenBase64:encodedData];
    free(chEncodeBody);
    free(myStrOriBody);
    [encodedData release];
    
    return base64;
}
+(NSString*) cloopenBase64:(NSData*)theData
{
    const uint8_t* input = (const uint8_t*)[theData bytes];
	NSInteger length = [theData length];
	
    static char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	
    NSMutableData* data = [NSMutableData dataWithLength:((length + 2) / 3) * 4];
    uint8_t* output = (uint8_t*)data.mutableBytes;
	
	NSInteger i,i2;
    for (i=0; i < length; i += 3) {
        NSInteger value = 0;
		for (i2=0; i2<3; i2++) {
            value <<= 8;
            if (i+i2 < length) {
                value |= (0xFF & input[i+i2]);
            }
        }
		
        NSInteger theIndex = (i / 3) * 4;
        output[theIndex + 0] =                    table[(value >> 18) & 0x3F];
        output[theIndex + 1] =                    table[(value >> 12) & 0x3F];
        output[theIndex + 2] = (i + 1) < length ? table[(value >> 6)  & 0x3F] : '=';
        output[theIndex + 3] = (i + 2) < length ? table[(value >> 0)  & 0x3F] : '=';
    }
	
    return [[[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding] autorelease];

}
+ (NSData*) clop_Base64EncodedString:(NSString *) string {
    NSMutableData *mutableData = nil;
    
    if( string ) {
        unsigned long ixtext = 0;
        unsigned long lentext = 0;
        unsigned char ch = 0;
        unsigned char inbuf[4], outbuf[3];
        short i = 0, ixinbuf = 0;
        BOOL flignore = NO;
        BOOL flendtext = NO;
        NSData *base64Data = nil;
        const unsigned char *base64Bytes = nil;
        
        base64Data = [string dataUsingEncoding:NSASCIIStringEncoding];
        base64Bytes = [base64Data bytes];
        mutableData = [NSMutableData dataWithCapacity:[base64Data length]];
        lentext = [base64Data length];
        
        while( YES ) {
            if( ixtext >= lentext ) break;
            ch = base64Bytes[ixtext++];
            flignore = NO;
            
            if( ( ch >= 'A' ) && ( ch <= 'Z' ) ) ch = ch - 'A';
            else if( ( ch >= 'a' ) && ( ch <= 'z' ) ) ch = ch - 'a' + 26;
            else if( ( ch >= '0' ) && ( ch <= '9' ) ) ch = ch - '0' + 52;
            else if( ch == '+' ) ch = 62;
            else if( ch == '=' ) flendtext = YES;
            else if( ch == '/' ) ch = 63;
            else flignore = YES;
            
            if( ! flignore ) {
                short ctcharsinbuf = 3;
                BOOL flbreak = NO;
                
                if( flendtext ) {
                    if( ! ixinbuf ) break;
                    if( ( ixinbuf == 1 ) || ( ixinbuf == 2 ) ) ctcharsinbuf = 1;
                    else ctcharsinbuf = 2;
                    ixinbuf = 3;
                    flbreak = YES;
                }
                
                inbuf [ixinbuf++] = ch;
                
                if( ixinbuf == 4 ) {
                    ixinbuf = 0;
                    outbuf [0] = ( inbuf[0] << 2 ) | ( ( inbuf[1] & 0x30) >> 4 );
                    outbuf [1] = ( ( inbuf[1] & 0x0F ) << 4 ) | ( ( inbuf[2] & 0x3C ) >> 2 );
                    outbuf [2] = ( ( inbuf[2] & 0x03 ) << 6 ) | ( inbuf[3] & 0x3F );
                    
                    for( i = 0; i < ctcharsinbuf; i++ )
                        [mutableData appendBytes:&outbuf[i] length:1];
                }
                
                if( flbreak )  break;
            }
        }
    }
    NSData* data = [[[NSData alloc] initWithData:mutableData] autorelease];
    return data;
}
//解密
+ (NSString *)clop_decodeData:(NSString *)srcData andPrivateKey:(NSString*) theKey
{
    if ( !srcData || srcData.length == 0 )
    {
        return nil;
    }
    NSData *encodedData = [[[NSData alloc] initWithData:[srcData dataUsingEncoding:NSUTF8StringEncoding]] autorelease];
    //解密包体
    char *chDecodeBody = (char*)malloc([encodedData length]*2);
    memset(chDecodeBody, 0, [encodedData length]*2);
    NSString *base64Body = [[NSString alloc] initWithData:encodedData encoding:NSUTF8StringEncoding];
    NSData *encodeData = [self clop_Base64EncodedString:base64Body];
    [base64Body release];
    CLOP_Decrypt_1((unsigned char*)[encodeData bytes], encodeData.length, (unsigned char*)chDecodeBody, (const unsigned char*)[theKey UTF8String]);
    CFDataRef bodyData = CFDataCreate(NULL, (UInt8 *)chDecodeBody, strlen(chDecodeBody));
    NSString *bodyMsg = [[NSString alloc] initWithData:(NSData *)bodyData encoding:NSUTF8StringEncoding];
    free(chDecodeBody);
    CFRelease(bodyData);
    return [bodyMsg autorelease];;
}

@end

//#endif
