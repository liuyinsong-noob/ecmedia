//
//  silk_interface.cc
//  libsilk
//
//  Created by hubin  on 13-4-22.
//  Copyright (c) 2013å¹?ronglian. All rights reserved.
//

#include "silk_interface.h"
//#include "trace.h"
#include "string.h"
#include <stdlib.h>
#include <stdio.h>

#include "SKP_Silk_control.h"
#include "SKP_Silk_errors.h"
#include "SKP_Silk_SDK_API.h"
#include "SKP_Silk_typedef.h"
#ifndef _WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif


struct silk_enc_struct {
	SKP_SILK_SDK_EncControlStruct control;
	void* psEnc;
	uint32_t ts;
	unsigned char ptime;
	unsigned char max_ptime;
	unsigned int max_network_bitrate;
};

struct silk_dec_struct {
    SKP_SILK_SDK_DecControlStruct control;
	void  *psDec;
};


#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

WebRtc_Word16 WebRtcSilk_EncoderCreate(silk_enc_struct **silk_encinst) {
    
    *silk_encinst = (silk_enc_struct *)malloc(sizeof(silk_enc_struct));
    if ( !(*silk_encinst) ) {
        return -1;
    }
    
    SKP_int32 encSizeBytes;
    SKP_int ret = SKP_Silk_SDK_Get_Encoder_Size(&encSizeBytes);
    if(ret < 0) {
        free(*silk_encinst);
        *silk_encinst = NULL;
        return -1;
    }
    
    (*silk_encinst)->psEnc = malloc(encSizeBytes);
    if( !((*silk_encinst)->psEnc) ) {
        free(*silk_encinst);
        *silk_encinst = NULL;
        return -1;
    }

    return 0;
}

WebRtc_Word16 WebRtcSilk_DecoderCreate(silk_dec_struct **silk_decinst) {
    
    *silk_decinst = (silk_dec_struct *)malloc(sizeof(silk_dec_struct));
    if ( !(*silk_decinst) ) {
        return -1;
    }
    
    SKP_int32 decSizeBytes;
    SKP_int ret = SKP_Silk_SDK_Get_Decoder_Size(&decSizeBytes);
    if(ret < 0) {
        free(*silk_decinst);
        *silk_decinst = NULL;
        return -1;
    }
    
    (*silk_decinst)->psDec = malloc(decSizeBytes);
    if( !((*silk_decinst)->psDec) ) {
        free(*silk_decinst);
        *silk_decinst = NULL;
        return -1;
    }
    
    return 0;
}

WebRtc_Word16 WebRtcSilk_EncoderFree(silk_enc_struct *silk_encinst) {

    free(silk_encinst->psEnc);
    free(silk_encinst);
    return 0;
}

WebRtc_Word16 WebRtcSilk_DecoderFree(silk_dec_struct *silk_decinst) {
    
    free(silk_decinst->psDec);
    free(silk_decinst);
    return 0;
}

WebRtc_Word16 WebRtcSilk_EncoderInit(silk_enc_struct *silk_encinst, WebRtc_Word16 sampleRate, WebRtc_Word16 pacsize, WebRtc_Word16 bitRate, bool enableDTX) {
    
    SKP_int ret = SKP_Silk_SDK_InitEncoder(silk_encinst->psEnc, &silk_encinst->control);
    if(ret < 0)
    {
        return ret;
    }
    
    /* Set Encoder parameters */
    //silk_encinst->control.API_sampleRate        = sampleRate;
    //silk_encinst->control.maxInternalSampleRate = sampleRate;
    //silk_encinst->control.packetSize            = pacsize;
    //silk_encinst->control.packetLossPercentage  = 2;
    //silk_encinst->control.useInBandFEC          = 1;
    //silk_encinst->control.useDTX                = 1;
    //silk_encinst->control.complexity            = 1;
    //silk_encinst->control.bitRate               = bitRate;

	silk_encinst->control.API_sampleRate        = sampleRate;
	silk_encinst->control.maxInternalSampleRate = sampleRate;
	silk_encinst->control.packetSize            = pacsize;
	silk_encinst->control.packetLossPercentage  = 0;
	silk_encinst->control.useInBandFEC          = 1;
	silk_encinst->control.useDTX                = 0;
	silk_encinst->control.complexity            = 2;
	silk_encinst->control.bitRate               = bitRate;
    
    return 0;
}

WebRtc_Word16 WebRtcSilk_Encode(silk_enc_struct *silk_encinst,
                                WebRtc_Word16 *speechIn,
                                WebRtc_Word16 len,
                                WebRtc_Word16 *encoded)
{
    WebRtc_Word16 outlen = 7680*2;
    
    
//    WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceAudioCoding, 0,
//                 "InternalEncode: error in encode for SILK %d", silk_encinst->control.packetLossPercentage );
    
    SKP_int ret = SKP_Silk_SDK_Encode(silk_encinst->psEnc,
                                      &silk_encinst->control,
                                      speechIn,
                                      len,
                                      (SKP_uint8*)encoded,
                                      &outlen);
    if(ret < 0)
    {
        return ret;
    }
    return outlen;
}

WebRtc_Word16 WebRtcSilk_DecoderInit(silk_dec_struct *silk_decinst) {
    
    /* Reset decoder */
    SKP_int ret = SKP_Silk_SDK_InitDecoder(silk_decinst->psDec);
    if( ret ) {
        return -1;
    }
    /* Initialize to one frame per packet, for proper concealment before first packet arrives */
    silk_decinst->control.framesPerPacket = 1;
    return 0;
}

WebRtc_Word16 WebRtcSilk_Decode(silk_dec_struct *silk_decinst,
                                WebRtc_Word16* encoded,
                                WebRtc_Word16 len,
                                WebRtc_Word16 *decoded,
                                WebRtc_Word16 *speechType) {
    WebRtc_Word16 ret;
    WebRtc_Word16 outlen;
    WebRtc_Word16* outPtr = (WebRtc_Word16*)decoded;
    
    do {
        
        if(!encoded)
		{
//             struct timeval tv;
//             gettimeofday(&tv, NULL);            
//             printf("encoded==NULL tv_sec=%lu\r\n", tv.tv_sec);
        }
        /* Decode 20 ms */
        ret = SKP_Silk_SDK_Decode( silk_decinst->psDec, &silk_decinst->control, encoded==NULL,(SKP_uint8*)encoded, len*2, outPtr, &outlen );
        if( ret < 0 ) {
// 			struct timeval tv;
// 			gettimeofday(&tv, NULL);
// 			printf("SKP_Silk_SDK_Decode failed tv_sec=%lu\r\n", tv.tv_sec);
            return ret;
        }
        
        outPtr += outlen;
        
        /* Until last 20 ms frame of packet has been decoded */
    } while( silk_decinst->control.moreInternalDecoderFrames );
    
    return outPtr - decoded;
}

WebRtc_Word16 WebRtcSilk_DecodePLC(silk_dec_struct *silk_decinst,
                                   WebRtc_Word16 *decoded,
                                   WebRtc_Word16 frames)
{
    WebRtc_Word16 ret;
    WebRtc_Word16 outlen;
    WebRtc_Word16* outPtr = (WebRtc_Word16*)decoded;
    
    /* Decode 20 ms */
    ret = SKP_Silk_SDK_Decode( silk_decinst->psDec, &silk_decinst->control, 1, NULL, 0, outPtr, &outlen );
    if( ret < 0 ) {
        return ret;
    }
    
    return outlen;
}

WebRtc_Word16 WebRtcSetEncRate(silk_enc_struct *silk_encinst, WebRtc_Word16 rate)
{
    int normalized_cbr=rate;
    switch(silk_encinst->control.maxInternalSampleRate) {
        case 8000:
            normalized_cbr=MIN(normalized_cbr,20000);
            normalized_cbr=MAX(normalized_cbr,5000);
            break;
        case 12000:
            normalized_cbr=MIN(normalized_cbr,25000);
            normalized_cbr=MAX(normalized_cbr,7000);
            break;
        case 16000:
            normalized_cbr=MIN(normalized_cbr,32000);
            normalized_cbr=MAX(normalized_cbr,8000);
            break;
        case 24000:
            normalized_cbr=MIN(normalized_cbr,40000);
            normalized_cbr=MAX(normalized_cbr,20000);
            break;
            
    }
    silk_encinst->control.bitRate = normalized_cbr;
    return normalized_cbr;
}

WebRtc_Word16 WebRtcSetDecSampleRate(silk_dec_struct *silk_decinst, WebRtc_Word16 sampleRate)
{
    silk_decinst->control.API_sampleRate = sampleRate;
    return 0;
}

void WebRtcSilk_version(char *version, int len) {
    const char *silkversion = SKP_Silk_SDK_get_version();
    if( (int)strlen(silkversion) >= len ) {
        strncpy(version, silkversion, len);
    }
    else {
        strncpy(version, silkversion, strlen(silkversion));
    }
}


