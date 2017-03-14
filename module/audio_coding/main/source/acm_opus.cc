/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "acm_opus.h"

#include <stdio.h>
#ifdef WEBRTC_CODEC_OPUS
#include "opus_interface.h"
#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "trace.h"
#endif
//#include "ogg/ogg.h"
#include "include/opus_defines.h"

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
((buf[base+2]<<16)&0xff0000)| \
((buf[base+1]<<8)&0xff00)| \
(buf[base]&0xff))
#define writeint(buf, base, val) do{ buf[base+3]=((val)>>24)&0xff; \
buf[base+2]=((val)>>16)&0xff; \
buf[base+1]=((val)>>8)&0xff; \
buf[base]=(val)&0xff; \
}while(0)

#if 0
char *file_opus = NULL;
#endif

namespace cloopenwebrtc {
    
    namespace acm2 {
        
        
        static void comment_init(char **comments, int* length, const char *vendor_string)
        {
            /*The 'vendor' field should be the actual encoding library used.*/
            int vendor_length=strlen(vendor_string);
            int user_comment_list_length=0;
            int len=8+4+vendor_length+4;
            char *p=(char*)malloc(len);
            if(p==NULL){
                fprintf(stderr, "malloc failed in comment_init()\n");
                exit(1);
            }
            memcpy(p, "OpusTags", 8);
            writeint(p, 8, vendor_length);
            memcpy(p+12, vendor_string, vendor_length);
            writeint(p, 12+vendor_length, user_comment_list_length);
            *length=len;
            *comments=p;
        }
        
        void comment_add(char **comments, int* length, char *tag, char *val)
        {
            char* p=*comments;
            int vendor_length=readint(p, 8);
            int user_comment_list_length=readint(p, 8+4+vendor_length);
            int tag_len=(tag?strlen(tag)+1:0);
            int val_len=strlen(val);
            int len=(*length)+4+tag_len+val_len;
            
            p=(char*)realloc(p, len);
            if(p==NULL){
                fprintf(stderr, "realloc failed in comment_add()\n");
                exit(1);
            }
            
            writeint(p, *length, tag_len+val_len);      /* length of comment */
            if(tag){
                memcpy(p+*length+4, tag, tag_len);        /* comment tag */
                (p+*length+4)[tag_len-1] = '=';           /* separator */
            }
            memcpy(p+*length+4+tag_len, val, val_len);  /* comment */
            writeint(p, 8+4+vendor_length, user_comment_list_length+1);
            *comments=p;
            *length=len;
        }
        
        static void comment_pad(char **comments, int* length, int amount)
        {
            if(amount>0){
                int i;
                int newlen;
                char* p=*comments;
                /*Make sure there is at least amount worth of padding free, and
                 round up to the maximum that fits in the current ogg segments.*/
                newlen=(*length+amount+255)/255*255-1;
                p=(char *)realloc(p,newlen);
                if(p==NULL){
                    fprintf(stderr,"realloc failed in comment_pad()\n");
                    exit(1);
                }
                for(i=*length;i<newlen;i++)p[i]=0;
                *comments=p;
                *length=newlen;
            }
        }
        
        
        
#ifndef WEBRTC_CODEC_OPUS
        
        ACMOpus::ACMOpus(int16_t /* codec_id */)
        : encoder_inst_ptr_(NULL),
        sample_freq_(0),
        bitrate_(0),
        channels_(1),
        packet_loss_rate_(0) {
            return;
        }
        
        ACMOpus::~ACMOpus() {
            return;
        }
        
        int16_t ACMOpus::InternalEncode(uint8_t* /* bitstream */,
                                        int16_t* /* bitstream_len_byte */) {
            return -1;
        }
        
        int16_t ACMOpus::InternalInitEncoder(WebRtcACMCodecParams* /* codec_params */) {
            return -1;
        }
        
        ACMGenericCodec* ACMOpus::CreateInstance(void) {
            return NULL;
        }
        
        int16_t ACMOpus::InternalCreateEncoder() {
            return -1;
        }
        
        void ACMOpus::DestructEncoderSafe() {
            return;
        }
        
        int16_t ACMOpus::SetBitRateSafe(const int32_t /*rate*/) {
            return -1;
        }
        
#else  //===================== Actual Implementation =======================
        
        ACMOpus::ACMOpus(int16_t codec_id)
        : encoder_inst_ptr_(NULL),
        sample_freq_(32000),  // Default sampling frequency.
        bitrate_(20000),  // Default bit-rate.
        channels_(1),  // Default mono.
        packet_loss_rate_(0)
#if 0
        , file_(NULL)
#endif
        {  // Initial packet loss rate.
            codec_id_ = codec_id;
            // Opus has internal DTX, but we dont use it for now.
            has_internal_dtx_ = false;
            has_internal_fec_ = true;
            
            if (codec_id_ != ACMCodecDB::kOpus && codec_id != ACMCodecDB::kOpus8k && codec_id != ACMCodecDB::kOpus16k) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "Wrong codec id for Opus.");
                sample_freq_ = 0xFFFF;
                bitrate_ = -1;
            }
#if 0
            if (file_opus) {
                file_ = fopen(file_opus, "wb+");
            }
#endif
            return;
        }
        
        ACMOpus::~ACMOpus() {
            if (encoder_inst_ptr_ != NULL) {
                WebRtcOpus_EncoderFree(encoder_inst_ptr_);
                encoder_inst_ptr_ = NULL;
            }
#if 0
                if (file_) {
                    fflush(file_);
                    fclose(file_);
                }
#endif
        }
        
        int16_t ACMOpus::InternalEncode(uint8_t* bitstream,
                                        int16_t* bitstream_len_byte) {
            // Call Encoder.
            *bitstream_len_byte = WebRtcOpus_Encode(encoder_inst_ptr_,
                                                    &in_audio_[in_audio_ix_read_],
                                                    frame_len_smpl_,
                                                    MAX_PAYLOAD_SIZE_BYTE, bitstream);
            // Check for error reported from encoder.
            if (*bitstream_len_byte < 0) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "InternalEncode: Encode error for Opus");
                *bitstream_len_byte = 0;
                return -1;
            }
            
            
            // Increment the read index. This tells the caller how far
            // we have gone forward in reading the audio buffer.
            in_audio_ix_read_ += frame_len_smpl_ * channels_;
            
#if 0
                fwrite(bitstream, 1, *bitstream_len_byte, file_);
                fflush(file_);
#endif
            return *bitstream_len_byte;
        }
#define PACKAGE_NAME "opus-tools"
#define PACKAGE_VERSION ""
        int16_t ACMOpus::InternalInitEncoder(WebRtcACMCodecParams* codec_params) {
            int16_t ret;
            if (encoder_inst_ptr_ != NULL) {
                WebRtcOpus_EncoderFree(encoder_inst_ptr_);
                encoder_inst_ptr_ = NULL;
            }
            ret = WebRtcOpus_EncoderCreate(&encoder_inst_ptr_,
                                           codec_params->codec_inst.channels, codec_params->codec_inst.plfreq);
            // Store number of channels.
            channels_ = codec_params->codec_inst.channels;
            
            if (ret < 0) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "Encoder creation failed for Opus");
                return ret;
            }
            ret = WebRtcOpus_SetBitRate(encoder_inst_ptr_,
                                        codec_params->codec_inst.rate);
            if (ret < 0) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "Setting initial bitrate failed for Opus");
                return ret;
            }
            
            ret = WebRtcOpus_SetMaxPlaybackRate(encoder_inst_ptr_, codec_params->codec_inst.plfreq);
            if (ret < 0) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "Setting initial playback rate failed for Opus");
                return ret;
            }
            
            ret = WebRtcOpus_SetPacketLossRate(encoder_inst_ptr_, 40);//sean test audio mixer, original 5
            if (ret < 0) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "Setting initial playback loss rate failed for Opus");
                return ret;
            }
            
            ret = WebRtcOpus_EnableFec(encoder_inst_ptr_);//sean just for mos
            if (ret < 0) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceWarning, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "Setting initial Fec failed for Opus");
            }
            
            //    ret = WebRtcOpus_EnableDtx(encoder_inst_ptr_);
            //    if (ret < 0) {
            //        WEBRTC_TRACE(cloopenwebrtc::kTraceWarning, cloopenwebrtc::kTraceAudioCoding, unique_id_,
            //                     "Setting initial Dtx failed for Opus");
            //    }
            
            // Store bitrate.
            bitrate_ = codec_params->codec_inst.rate;
            
            // TODO(tlegrand): Remove this code when we have proper APIs to set the
            // complexity at a higher level.
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS) || defined(WEBRTC_ARCH_ARM)
            // If we are on Android, iOS and/or ARM, use a lower complexity setting as
            // default, to save encoder complexity.
            const int kOpusComplexity5 = 5;
            WebRtcOpus_SetComplexity(encoder_inst_ptr_, kOpusComplexity5);
            if (ret < 0) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "Setting complexity failed for Opus");
                return ret;
            }
#endif
            
            return 0;
        }
        
        ACMGenericCodec* ACMOpus::CreateInstance(void) {
            return NULL;
        }
        
        int16_t ACMOpus::InternalCreateEncoder() {
            // Real encoder will be created in InternalInitEncoder.
            return 0;
        }
        
        void ACMOpus::DestructEncoderSafe() {
            if (encoder_inst_ptr_) {
                WebRtcOpus_EncoderFree(encoder_inst_ptr_);
                encoder_inst_ptr_ = NULL;
            }
        }
        
        int16_t ACMOpus::SetBitRateSafe(const int32_t rate) {
            if (rate < 6000 || rate > 510000) {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceAudioCoding, unique_id_,
                             "SetBitRateSafe: Invalid rate Opus");
                return -1;
            }
            
            bitrate_ = rate;
            
            // Ask the encoder for the new rate.
            if (WebRtcOpus_SetBitRate(encoder_inst_ptr_, bitrate_) >= 0) {
                encoder_params_.codec_inst.rate = bitrate_;
                return 0;
            }
            
            return -1;
        }
        
        int ACMOpus::SetFEC(bool enable_fec) {
            // Ask the encoder to enable FEC.
            //sean test audio mixer begin
            if (WebRtcOpus_EnableFec(encoder_inst_ptr_) == 0)
                return 0;
            //sean test audio mixer end
            if (enable_fec) {
                if (WebRtcOpus_EnableFec(encoder_inst_ptr_) == 0)
                    return 0;
            } else {
                if (WebRtcOpus_DisableFec(encoder_inst_ptr_) == 0)
                    return 0;
            }
            return -1;
        }
        
        int ACMOpus::SetPacketLossRate(int loss_rate) {
//            return 0;//sean test audio mixer
            if (loss_rate < 5) {
                return 0;
            }
//            printf("sean haha loss_rate:%d\n",loss_rate);
//            loss_rate = 72;
            // Optimize the loss rate to configure Opus. Basically, optimized loss rate is
            // the input loss rate rounded down to various levels, because a robustly good
            // audio quality is achieved by lowering the packet loss down.
            // Additionally, to prevent toggling, margins are used, i.e., when jumping to
            // a loss rate from below, a higher threshold is used than jumping to the same
            // level from above.
            const int kPacketLossRate90 = 90;
            const int kPacketLossRate80 = 80;
            const int kPacketLossRate70 = 70;
            const int kPacketLossRate60 = 60;
            const int kPacketLossRate50 = 50;
            const int kPacketLossRate40 = 40;
            const int kPacketLossRate35 = 35;
            const int kPacketLossRate30 = 30;
            const int kPacketLossRate25 = 25;
            const int kPacketLossRate20 = 20;
            const int kPacketLossRate15 = 15;
            const int kPacketLossRate10 = 10;
            const int kPacketLossRate5 = 5;
            const int kPacketLossRate1 = 1;
            const int kLossRate20Margin = 2;
            const int kLossRate10Margin = 1;
            const int kLossRate5Margin = 1;
            int opt_loss_rate;
            
            if (loss_rate >= kPacketLossRate90 + kLossRate20Margin *
                (kPacketLossRate20 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate90;
            }
            else if (loss_rate >= kPacketLossRate80 + kLossRate20Margin *
                (kPacketLossRate20 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate80;
            }
            else if (loss_rate >= kPacketLossRate70 + kLossRate20Margin *
                (kPacketLossRate20 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate70;
            }
            else if (loss_rate >= kPacketLossRate60 + kLossRate20Margin *
                (kPacketLossRate20 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate60;
            }
            else if (loss_rate >= kPacketLossRate50 + kLossRate20Margin *
                (kPacketLossRate20 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate50;
            }
            else if (loss_rate >= kPacketLossRate40 + kLossRate20Margin *
                     (kPacketLossRate40 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate40;
            }
            else if (loss_rate >= kPacketLossRate35 + kLossRate20Margin *
                     (kPacketLossRate35 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate35;
            }
            else if (loss_rate >= kPacketLossRate30 + kLossRate20Margin *
                     (kPacketLossRate30 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate30;
            }
            else if (loss_rate >= kPacketLossRate25 + kLossRate20Margin *
                     (kPacketLossRate25 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate25;
            }
            else if (loss_rate >= kPacketLossRate20 + kLossRate20Margin *
                     (kPacketLossRate20 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate20;
            }
            else if (loss_rate >= kPacketLossRate15 + kLossRate20Margin *
                     (kPacketLossRate15 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate15;
            }
            else if (loss_rate >= kPacketLossRate10 + kLossRate10Margin *
                       (kPacketLossRate10 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate10;
            } else if (loss_rate >= kPacketLossRate5 + kLossRate5Margin *
                       (kPacketLossRate5 - packet_loss_rate_ > 0 ? 1 : -1)) {
                opt_loss_rate = kPacketLossRate5;
            } else if (loss_rate >= kPacketLossRate1) {
                opt_loss_rate = kPacketLossRate1;
            } else {
                opt_loss_rate = 0;
            }
            
            if (packet_loss_rate_ == opt_loss_rate) {
                return 0;
            }
            printf("sean haha actual loss_rate for opus:%d\n",opt_loss_rate);
            // Ask the encoder to change the target packet loss rate.
            if (WebRtcOpus_SetPacketLossRate(encoder_inst_ptr_, opt_loss_rate) == 0) {
                packet_loss_rate_ = opt_loss_rate;
                return 0;
            }
            
            return -1;
        }
        
        int ACMOpus::SetOpusMaxPlaybackRate(int frequency_hz) {
            // Informs Opus encoder of the maximum playback rate the receiver will render.
            return WebRtcOpus_SetMaxPlaybackRate(encoder_inst_ptr_, frequency_hz);
        }
        
#endif  // WEBRTC_CODEC_OPUS
        
    }  // namespace acm2
    
}  // namespace cloopenwebrtc
