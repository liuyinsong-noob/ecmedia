//
//  amrnb_api.c
//  amrnb
//
//  Created by hubin on 13-6-6.
//  Copyright (c) 2013年 hisunsray. All rights reserved.
//

#include <stdio.h>
#include "amrnb_api.h"
#include "amr_interface.h"
#include "mode.h"
//#ifndef AMRNB_WRAPPER_INTERNAL
///* Copied from enc/src/gsmamr_enc.h */
//enum Mode {
//	MR475 = 0,/* 4.75 kbps */
//	MR515,    /* 5.15 kbps */
//	MR59,     /* 5.90 kbps */
//	MR67,     /* 6.70 kbps */
//	MR74,     /* 7.40 kbps */
//	MR795,    /* 7.95 kbps */
//	MR102,    /* 10.2 kbps */
//	MR122,    /* 12.2 kbps */
//	MRDTX,    /* DTX       */
//	N_MODES   /* Not Used  */
//};
//#endif


#ifdef __cplusplus
extern "C" {
#endif
    
void* Decoder_Interface_init(void);    
void Decoder_Interface_exit(void* state);    
void Decoder_Interface_Decode(void* state, const unsigned char* in, short* out, int bfi);

struct encoder_state;
void* Encoder_Interface_init(int dtx);    
void Encoder_Interface_exit(void* s);    
int Encoder_Interface_Encode(void* s, enum Mode mode, const short* speech, unsigned char* out, int forceSpeech);
    
#ifdef __cplusplus
}
#endif

void *encInst = NULL;
void *decInst = NULL;

int AmrNBCreateEnc()
{
    if(encInst)
        return 0;
    int ret = WebRtcAmr_CreateEnc(&encInst);
    return ret;
}
int AmrNBCreateDec()
{
    if(decInst)
        return 0;
    int ret = WebRtcAmr_CreateDec(&decInst);
    return ret;
}

int AmrNBFreeEnc()
{
    if(!encInst)
        return 0;
    int ret = WebRtcAmr_FreeEnc(encInst);
    encInst = NULL;
    return ret;
}

int AmrNBFreeDec()
{
    if(!decInst)
        return 0;
    int ret = WebRtcAmr_FreeDec(decInst);
    decInst = NULL;
    return ret;
}
int AmrNBEncode(short* input,
                 short len,
                 short*output,
                 short mode)
{
    if(!encInst)
        return -1;
    

    int ret = WebRtcAmr_Encode(encInst, input, len, output, mode);
    return ret;
}

int AmrNBEncoderInit(short dtxMode)
{
    if(!encInst)
        return -1;
    int ret = WebRtcAmr_EncoderInit(encInst, dtxMode);
    return ret;
}


int AmrNBDecode(short* encoded,
                 int len, short* decoded)
{
    if(!decInst)
        return -1;
    short speechType;
    int ret = WebRtcAmr_Decode(decInst, encoded, len, decoded, &speechType);
    return ret;
}

int AmrNBVersion(char *versionStr, short len)
{
    return WebRtcAmr_Version(versionStr, len);
}