#ifndef __VM_CRYPTO_H__
#define __VM_CRYPTO_H__

typedef unsigned char uint8_t;

typedef struct {
	uint8_t iv[8];
	uint8_t rk[8];
} clop_crypto_quick_key_t;

/*≥ı ºªØº”Ω‚√‹µƒKEY*/
int clop_crypto_quick_init_key(clop_crypto_quick_key_t* key, const uint8_t* userkey, int len);

/*º”√‹¥¶¿Ì*/
int clop_crypto_quick_enc(clop_crypto_quick_key_t* key, uint8_t* dst, const uint8_t* src, int len);

/*Ω‚√‹¥¶¿Ì*/
int clop_crypto_quick_dec(clop_crypto_quick_key_t* key, uint8_t* dst, const uint8_t* src, int len);


//
int CLOP_Encrypt_1(const unsigned char *in,int inlen,unsigned char *out,const unsigned char *key);
int CLOP_Decrypt_1(const unsigned char *in, int inlen, unsigned char *out,const unsigned char *key);

#endif	//__VM_CRYPTO_H__


@interface CLOPEncrypt : NSObject
//加密
+ (NSString *)clop_encodedData:(NSString *)srcData andPrivateKey:(NSString*) theKey;
//解密
+ (NSString *)clop_decodeData:(NSString *)srcData andPrivateKey:(NSString*) theKey;
@end