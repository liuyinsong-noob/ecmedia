//  silk_interface.h
//  libsilk
//
//  Created by hubin  on 13-4-22.
//  Copyright (c) 2013年 ronglian. All rights reserved.
//

#ifndef libsilk_silk_interface_h
#define libsilk_silk_interface_h

#include "typedefs.h"


struct silk_enc_struct;
struct silk_dec_struct;
#ifdef __cplusplus
extern "C" {
#endif
    
WebRtc_Word16 WebRtcSilk_EncoderCreate(silk_enc_struct **silk_encinst);
WebRtc_Word16 WebRtcSilk_DecoderCreate(silk_dec_struct **silk_decinst);

WebRtc_Word16 WebRtcSilk_EncoderFree(silk_enc_struct *silk_encinst);
WebRtc_Word16 WebRtcSilk_DecoderFree(silk_dec_struct *silk_decinst);

WebRtc_Word16 WebRtcSilk_EncoderInit(silk_enc_struct *silk_encinst, WebRtc_Word16 sampleRate, WebRtc_Word16 pacsize, WebRtc_Word16 bitRate, bool enableDTX);

WebRtc_Word16 WebRtcSilk_Encode(silk_enc_struct *silk_encinst,
                                   WebRtc_Word16 *speechIn,
                                   WebRtc_Word16 len,
                                   WebRtc_Word16 *encoded);

WebRtc_Word16 WebRtcSilk_DecoderInit(silk_dec_struct *silk_decinst);

WebRtc_Word16 WebRtcSilk_Decode(silk_dec_struct *silk_decinst,
                                   WebRtc_Word16* encoded,
                                   WebRtc_Word16 len,
                                   WebRtc_Word16 *decoded,
                                   WebRtc_Word16 *speechType);
    
WebRtc_Word16 WebRtcSilk_DecodePLC(silk_dec_struct *silk_decinst,
                                       WebRtc_Word16 *decoded,
                                       WebRtc_Word16 frames);
    
WebRtc_Word16 WebRtcSetEncRate(silk_enc_struct *silk_encinst, WebRtc_Word16 rate);
WebRtc_Word16 WebRtcSetDecSampleRate(silk_dec_struct *silk_decinst, WebRtc_Word16 sampleRate);
void WebRtcSilk_version(char *version, int len);
    
#ifdef __cplusplus
}
#endif

#endif
