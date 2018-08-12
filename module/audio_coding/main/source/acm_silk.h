//
//  acm_silk.h
//  audio_coding_module
//
//  Created by hubin  on 13-4-22.
//  Copyright (c) 2013年 hisunsray. All rights reserved.
//

#ifndef audio_coding_module_acm_silk_h
#define audio_coding_module_acm_silk_h

#include "acm_generic_codec.h"

namespace yuntongxunwebrtc
{
 namespace acm2{   
    class ACMSILK : public ACMGenericCodec
    {
    public:
        ACMSILK(WebRtc_Word16 codecID);
        ACMSILK();
        ~ACMSILK();
        // for FEC
        ACMGenericCodec* CreateInstance(void);
        
        WebRtc_Word16 InternalEncode(
                                     WebRtc_UWord8* bitstream,
                                     WebRtc_Word16* bitStreamLenByte);
        
        WebRtc_Word16 InternalInitEncoder(
                                          WebRtcACMCodecParams *codecParams);
        
        WebRtc_Word16 InternalInitDecoder(
                                          WebRtcACMCodecParams *codecParams);
        
    protected:
        WebRtc_Word16 DecodeSafe(
                                 WebRtc_UWord8* bitStream,
                                 WebRtc_Word16  bitStreamLenByte,
                                 WebRtc_Word16* audio,
                                 WebRtc_Word16* audioSamples,
								 WebRtc_Word8*  speechType);

	/*	WebRtc_Word32 CodecDef(
			WebRtcNetEQ_CodecDef& codecDef,
			const CodecInst&      codecInst);*/
        
        
        WebRtc_Word16 SetBitRateSafe(
                                     const WebRtc_Word32 rate);
        
        void DestructEncoderSafe();
        
        void DestructDecoderSafe();
        
        WebRtc_Word16 InternalCreateEncoder();
        
        WebRtc_Word16 InternalCreateDecoder();
        
        void InternalDestructEncoderInst(
                                         void* ptrInst);
        
        void *_encoderInstPtr;
        void *_decoderInstPtr;
    };
 } //namespace acm2  
} // namespace yuntongxunwebrtc


#endif
