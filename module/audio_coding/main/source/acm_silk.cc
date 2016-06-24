//
//  acm_silk.cc
//  audio_coding_module
//
//  Created by hubin  on 13-4-22.
//  Copyright (c) 2013å¹?hisunsray. All rights reserved.
//

#include "acm_silk.h"

#include "acm_common_defs.h"

//#include "acm_neteq.h"
#include "trace.h"
//#include "webrtc_neteq.h"
//#include "webrtc_neteq_help_macros.h"

#ifdef WEBRTC_CODEC_SILK
#include "silk_interface.h"
#endif

namespace cloopenwebrtc
{
namespace acm2{
#ifndef WEBRTC_CODEC_SILK
    
    ACMSILK::ACMSILK(WebRtc_Word16 /* codecID */)
    : _encoderInstPtr(NULL),
    _decoderInstPtr(NULL) {
        return;
    }
    
    
    ACMSILK::~ACMSILK()
    {
        return;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalEncode(
                            WebRtc_UWord8* /* bitStream        */,
                            WebRtc_Word16* /* bitStreamLenByte */)
    {
        return -1;
    }
    
    
    WebRtc_Word16
    ACMSILK::DecodeSafe(
                        WebRtc_UWord8* /* bitStream        */,
                        WebRtc_Word16  /* bitStreamLenByte */,
                        WebRtc_Word16* /* audio            */,
                        WebRtc_Word16* /* audioSamples     */,
                        WebRtc_Word8*  /* speechType       */)
    {
        return -1;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalInitEncoder(
                                 WebRtcACMCodecParams* /* codecParams */)
    {
        return -1;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalInitDecoder(
                                 WebRtcACMCodecParams* /* codecParams */)
    {
        return -1;
    }
    
    
    WebRtc_Word32
    ACMSILK::CodecDef(
                      WebRtcNetEQ_CodecDef& /* codecDef  */,
                      const CodecInst&      /* codecInst */)
    {
        return -1;
    }
    
    
    ACMGenericCodec*
    ACMSILK::CreateInstance(void)
    {
        return NULL;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalCreateEncoder()
    {
        return -1;
    }
    
    
    void
    ACMSILK::DestructEncoderSafe()
    {
        return;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalCreateDecoder()
    {
        return -1;
    }
    
    
    void
    ACMSILK::DestructDecoderSafe()
    {
        return;
    }
    
    
    void
    ACMSILK::InternalDestructEncoderInst(
                                         void* /* ptrInst */)
    {
        return;
    }
    
    WebRtc_Word16
    ACMSILK::SetBitRateSafe(const WebRtc_Word32 /* rate */)
    {
        return -1;
    }
    
#else     //===================== Actual Implementation =======================
    
    
    ACMSILK::ACMSILK(
                     WebRtc_Word16 codecID):
    _encoderInstPtr(NULL),
    _decoderInstPtr(NULL)
    {
        _codecID = codecID;
        return;
    }
    
    
    ACMSILK::~ACMSILK()
    {
        if(_encoderInstPtr != NULL)
        {
            WebRtcSilk_EncoderFree((silk_enc_struct *)_encoderInstPtr);
            _encoderInstPtr = NULL;
        }
        if(_decoderInstPtr != NULL)
        {
            WebRtcSilk_DecoderFree((silk_dec_struct *)_decoderInstPtr);
            _decoderInstPtr = NULL;
        }
        return;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalEncode(
                            WebRtc_UWord8* bitStream,
                            WebRtc_Word16* bitStreamLenByte)
    {
        *bitStreamLenByte = WebRtcSilk_Encode((silk_enc_struct *)_encoderInstPtr,
                                                 &_inAudio[_inAudioIxRead], _frameLenSmpl, (WebRtc_Word16*)bitStream);
        
        if (*bitStreamLenByte < 0)
        {
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                         "InternalEncode: error in encode for SILK %d", *bitStreamLenByte);
            return -1;
        }
        // increment the read index this tell the caller that how far
        // we have gone forward in reading the audio buffer
        _inAudioIxRead += _frameLenSmpl;
        return *bitStreamLenByte;
    }
    
    
    WebRtc_Word16
    ACMSILK::DecodeSafe(
                        WebRtc_UWord8* /* bitStream        */,
                        WebRtc_Word16  /* bitStreamLenByte */,
                        WebRtc_Word16* /* audio            */,
                        WebRtc_Word16* /* audioSamples     */,
                        WebRtc_Word8*  /* speechType       */)
    {
        return 0;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalInitEncoder(
                                 WebRtcACMCodecParams* codecParams)
    {
        WebRtc_Word16 ret = WebRtcSilk_EncoderInit((silk_enc_struct *)_encoderInstPtr,
                                                codecParams->codecInstant.plfreq,
                                                codecParams->codecInstant.pacsize,
                                                codecParams->codecInstant.rate,
                                                codecParams->enableDTX);
        
        WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                     "InternalInitEncoder: Silk InitEncoder freq=%d pacsize=%d rate=%d",
                     codecParams->codecInstant.plfreq,
                     codecParams->codecInstant.pacsize,
                     codecParams->codecInstant.rate);
        if(ret < 0)
        {
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                         "InternalInitEncoder: Silk InitEncoder error!");
            return ret;
        }
        return 0;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalInitDecoder(
                                 WebRtcACMCodecParams* codecParams)
    {
        
        WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                     "InternalInitDecoder: Silk InitDecoder freq=%d",
                     codecParams->codecInstant.plfreq);
        WebRtcSetDecSampleRate((silk_dec_struct*)_decoderInstPtr, codecParams->codecInstant.plfreq);
        /* Reset decoder */
        WebRtc_Word16 ret = WebRtcSilk_DecoderInit((silk_dec_struct*)_decoderInstPtr);
        if( ret ) {
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                         "InternalInitDecoder: Silk InitDecoder error!");
            return -1;
        }
        return 0;
    }
    
    
    WebRtc_Word32
    ACMSILK::CodecDef(
                      WebRtcNetEQ_CodecDef& codecDef,
                      const CodecInst&      codecInst)
    {
        if (!_decoderInitialized)
        {
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                         "CodeDef: decoder not initialized for SILK");
            return -1;
        }
        // Fill up the structure by calling
        // "SET_CODEC_PAR" & "SET_SILK_FUNCTION."
        // Then return the structure back to NetEQ to add the codec to it's
        // database.
        
        if(codecInst.plfreq == 8000) {
            SET_CODEC_PAR((codecDef), kDecoderSILK8K, codecInst.pltype,
                      _decoderInstPtr, codecInst.plfreq);
        }
        else if(codecInst.plfreq == 12000) {
            SET_CODEC_PAR((codecDef), kDecoderSILK12K, codecInst.pltype,
                          _decoderInstPtr, codecInst.plfreq);
        }
        else if(codecInst.plfreq == 16000) {
            SET_CODEC_PAR((codecDef), kDecoderSILK16K, codecInst.pltype,
                          _decoderInstPtr, codecInst.plfreq);
        }

        
        SET_SILK_FUNCTIONS((codecDef));
        return 0;
    }    
    
    ACMGenericCodec*
    ACMSILK::CreateInstance(void)
    {
        return NULL;
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalCreateEncoder()
    {
        WebRtc_Word16 ret = WebRtcSilk_EncoderCreate((silk_enc_struct **)&_encoderInstPtr);
        if(ret < 0){
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                         "InternalCreateEncoder: cannot create instance for SILK encoder");
            return -1;
        }
        return 0;
    }
    
    
    void
    ACMSILK::DestructEncoderSafe()
    {
        _encoderInitialized = false;
        _encoderExist = false;
        if(_encoderInstPtr != NULL)
        {
            free(_encoderInstPtr);
            _encoderInstPtr = NULL;
        }
    }
    
    
    WebRtc_Word16
    ACMSILK::InternalCreateDecoder()
    {
        WebRtc_Word16 ret = WebRtcSilk_DecoderCreate((silk_dec_struct **)&_decoderInstPtr);
        if(ret < 0){
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                         "InternalCreateDecoder: cannot create instance for SILK decoder");
            return -1;
        }
        return 0;
    }    
    
    void
    ACMSILK::DestructDecoderSafe()
    {
        _decoderInitialized = false;
        _decoderExist = false;
        if(_decoderInstPtr != NULL)
        {
            WebRtcSilk_DecoderFree((silk_dec_struct*)_decoderInstPtr);
            _decoderInstPtr = NULL;
        }
    }
    
    
    void
    ACMSILK::InternalDestructEncoderInst(
                                         void* ptrInst)
    {
        if(ptrInst != NULL)
        {
            WebRtcSilk_EncoderFree((silk_enc_struct*)ptrInst);
        }
        return;
    }
    
    WebRtc_Word16
    ACMSILK::SetBitRateSafe(const WebRtc_Word32 rate)
    {
        WebRtc_Word16 retRate = WebRtcSetEncRate((silk_enc_struct*)_encoderInstPtr, rate);
                
        if(retRate != rate){
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                         "Silk enc unsupported codec bitrate [%i], normalizing",rate);
        }
        
        WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceAudioCoding, _uniqueID,
                     "MSSilkEnc: Setting silk codec birate to [%i]",retRate);
        _encoderParams.codecInstant.rate = retRate;
        return 0;
    }
    
#endif
   } //acm2 
} // namespace cloopenwebrtc