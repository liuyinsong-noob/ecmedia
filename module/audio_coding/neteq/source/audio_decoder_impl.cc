/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_decoder_impl.h"

#include <assert.h>
#include <string.h>  // memmove

#include "checks.h"
#include "webrtc_cng.h"
#include "g711_interface.h"
#ifdef WEBRTC_CODEC_G722
#include "module/audio_coding/codecs/g722/include/g722_interface.h"
#endif
#ifdef WEBRTC_CODEC_ILBC
#include "module/audio_coding/codecs/ilbc/interface/ilbc.h"
#endif
#ifdef WEBRTC_CODEC_ISACFX
#include "module/audio_coding/main/source/codecs/isac/fix/interface/audio_encoder_isacfix.h"
#endif
#ifdef WEBRTC_CODEC_ISAC
#include "module/audio_coding/main/source/codecs/isac/main/interface/audio_encoder_isac.h"
#endif
#ifdef WEBRTC_CODEC_OPUS
#include "opus_interface.h"
#endif
#ifdef WEBRTC_CODEC_PCM16
#include "module/audio_coding/codecs/pcm16b/include/pcm16b.h"
#endif
#include "wavfile.h"
#include <stdio.h>
char *globalFilePathcapture;

namespace cloopenwebrtc {

// PCMu
int AudioDecoderPcmU::Decode(const uint8_t* encoded, size_t encoded_len,
                              int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type = 1;  // Default is speech.
  int16_t ret = WebRtcG711_DecodeU(
      reinterpret_cast<int16_t*>(const_cast<uint8_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderPcmU::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) {
  // One encoded byte per sample per channel.
  return static_cast<int>(encoded_len / channels_);
}

// PCMa
int AudioDecoderPcmA::Decode(const uint8_t* encoded, size_t encoded_len,
                              int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type = 1;  // Default is speech.
  int16_t ret = WebRtcG711_DecodeA(
      reinterpret_cast<int16_t*>(const_cast<uint8_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderPcmA::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) {
  // One encoded byte per sample per channel.
  return static_cast<int>(encoded_len / channels_);
}

// PCM16B
#ifdef WEBRTC_CODEC_PCM16
AudioDecoderPcm16B::AudioDecoderPcm16B() {}

int AudioDecoderPcm16B::Decode(const uint8_t* encoded, size_t encoded_len,
                               int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type = 1;  // Default is speech.
  int16_t ret = WebRtcPcm16b_DecodeW16(
      reinterpret_cast<int16_t*>(const_cast<uint8_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderPcm16B::PacketDuration(const uint8_t* encoded,
                                       size_t encoded_len) {
  // Two encoded byte per sample per channel.
  return static_cast<int>(encoded_len / (2 * channels_));
}

AudioDecoderPcm16BMultiCh::AudioDecoderPcm16BMultiCh(int num_channels) {
  DCHECK(num_channels > 0);
  channels_ = num_channels;
}
#endif

// iLBC
#ifdef WEBRTC_CODEC_ILBC
AudioDecoderIlbc::AudioDecoderIlbc() {
  WebRtcIlbcfix_DecoderCreate(&dec_state_);
}

AudioDecoderIlbc::~AudioDecoderIlbc() {
  WebRtcIlbcfix_DecoderFree(dec_state_);
}

int AudioDecoderIlbc::Decode(const uint8_t* encoded, size_t encoded_len,
                             int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type = 1;  // Default is speech.
  int16_t ret = WebRtcIlbcfix_Decode(dec_state_,
                                     reinterpret_cast<const int16_t*>(encoded),
                                     static_cast<int16_t>(encoded_len), decoded,
                                     &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderIlbc::DecodePlc(int num_frames, int16_t* decoded) {
  return WebRtcIlbcfix_NetEqPlc(dec_state_, decoded, num_frames);
}

int AudioDecoderIlbc::Init() {
  return WebRtcIlbcfix_Decoderinit30Ms(dec_state_);
}
#endif

// G.722
#ifdef WEBRTC_CODEC_G722
AudioDecoderG722::AudioDecoderG722() {
  WebRtcG722_CreateDecoder(&dec_state_);
}

AudioDecoderG722::~AudioDecoderG722() {
  WebRtcG722_FreeDecoder(dec_state_);
}

int AudioDecoderG722::Decode(const uint8_t* encoded, size_t encoded_len,
                             int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type = 1;  // Default is speech.
  int16_t ret = WebRtcG722_Decode(
      dec_state_,
      const_cast<int16_t*>(reinterpret_cast<const int16_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderG722::Init() {
  return WebRtcG722_DecoderInit(dec_state_);
}

int AudioDecoderG722::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) {
  // 1/2 encoded byte per sample per channel.
  return static_cast<int>(2 * encoded_len / channels_);
}

AudioDecoderG722Stereo::AudioDecoderG722Stereo() {
  channels_ = 2;
  WebRtcG722_CreateDecoder(&dec_state_left_);
  WebRtcG722_CreateDecoder(&dec_state_right_);
}

AudioDecoderG722Stereo::~AudioDecoderG722Stereo() {
  WebRtcG722_FreeDecoder(dec_state_left_);
  WebRtcG722_FreeDecoder(dec_state_right_);
}

int AudioDecoderG722Stereo::Decode(const uint8_t* encoded, size_t encoded_len,
                                   int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type = 1;  // Default is speech.
  // De-interleave the bit-stream into two separate payloads.
  uint8_t* encoded_deinterleaved = new uint8_t[encoded_len];
  SplitStereoPacket(encoded, encoded_len, encoded_deinterleaved);
  // Decode left and right.
  int16_t ret = WebRtcG722_Decode(
      dec_state_left_,
      reinterpret_cast<int16_t*>(encoded_deinterleaved),
      static_cast<int16_t>(encoded_len / 2), decoded, &temp_type);
  if (ret >= 0) {
    int decoded_len = ret;
    ret = WebRtcG722_Decode(
      dec_state_right_,
      reinterpret_cast<int16_t*>(&encoded_deinterleaved[encoded_len / 2]),
      static_cast<int16_t>(encoded_len / 2), &decoded[decoded_len], &temp_type);
    if (ret == decoded_len) {
      decoded_len += ret;
      // Interleave output.
      for (int k = decoded_len / 2; k < decoded_len; k++) {
          int16_t temp = decoded[k];
          memmove(&decoded[2 * k - decoded_len + 2],
                  &decoded[2 * k - decoded_len + 1],
                  (decoded_len - k - 1) * sizeof(int16_t));
          decoded[2 * k - decoded_len + 1] = temp;
      }
      ret = decoded_len;  // Return total number of samples.
    }
  }
  *speech_type = ConvertSpeechType(temp_type);
  delete [] encoded_deinterleaved;
  return ret;
}

int AudioDecoderG722Stereo::Init() {
  int r = WebRtcG722_DecoderInit(dec_state_left_);
  if (r != 0)
    return r;
  return WebRtcG722_DecoderInit(dec_state_right_);
}

// Split the stereo packet and place left and right channel after each other
// in the output array.
void AudioDecoderG722Stereo::SplitStereoPacket(const uint8_t* encoded,
                                               size_t encoded_len,
                                               uint8_t* encoded_deinterleaved) {
  assert(encoded);
  // Regroup the 4 bits/sample so |l1 l2| |r1 r2| |l3 l4| |r3 r4| ...,
  // where "lx" is 4 bits representing left sample number x, and "rx" right
  // sample. Two samples fit in one byte, represented with |...|.
  for (size_t i = 0; i + 1 < encoded_len; i += 2) {
    uint8_t right_byte = ((encoded[i] & 0x0F) << 4) + (encoded[i + 1] & 0x0F);
    encoded_deinterleaved[i] = (encoded[i] & 0xF0) + (encoded[i + 1] >> 4);
    encoded_deinterleaved[i + 1] = right_byte;
  }

  // Move one byte representing right channel each loop, and place it at the
  // end of the bytestream vector. After looping the data is reordered to:
  // |l1 l2| |l3 l4| ... |l(N-1) lN| |r1 r2| |r3 r4| ... |r(N-1) r(N)|,
  // where N is the total number of samples.
  for (size_t i = 0; i < encoded_len / 2; i++) {
    uint8_t right_byte = encoded_deinterleaved[i + 1];
    memmove(&encoded_deinterleaved[i + 1], &encoded_deinterleaved[i + 2],
            encoded_len - i - 2);
    encoded_deinterleaved[encoded_len - 1] = right_byte;
  }
}
#endif

#ifdef WEBRTC_CODEC_G729
    // G729
    AudioDecoderG729::AudioDecoderG729() {

 /*       InitWavFile(&wavParams_w);
        if( OpenWavFile(&wavParams_w, globalFilePathcapture, FILE_WRITE) == -1 )
        {

        }

        wavParams_w.waveFmt.nFormatTag      = LINIAR_PCM;
        wavParams_w.waveFmt.nChannels       = 1;
        wavParams_w.waveFmt.nSamplesPerSec  = 8000;
        wavParams_w.waveFmt.nBitPerSample   = 16;
        wavParams_w.waveFmt.nAvgBytesPerSec = 8000*1*(16>>3); //nSamplesPerSec*nChannles*nBitPerSample/8
        wavParams_w.waveFmt.nBlockAlign     = 1*(16>>3); //nChannles*nBitPerSample/;
        wavParams_w.bitrate                 = 8000*16;

        if( WavFileWriteHeader(&wavParams_w) != 0 )
        {

        }
*/

        WebRtcG729_CreateDec(&dec_state_);
    }

    AudioDecoderG729::~AudioDecoderG729() {

        WebRtcG729_FreeDec(dec_state_);
    }

    int AudioDecoderG729::Decode(const uint8_t* encoded, size_t encoded_len,
                                 int16_t* decoded, SpeechType* speech_type) {
        int16_t temp_type = 1;  // Default is speech.

        int16_t ret = WebRtcG729_Decode(dec_state_, reinterpret_cast<const int16_t*>(encoded),
                                           static_cast<int16_t>(encoded_len), decoded,
                                           &temp_type);

        *speech_type = ConvertSpeechType(temp_type);
        return ret;
    }

    int AudioDecoderG729::DecodePlc(int num_frames, int16_t* decoded) {
        return WebRtcG729_DecodePlc(dec_state_, decoded, num_frames);
    }

    int AudioDecoderG729::Init() {
        return WebRtcG729_DecoderInit(dec_state_);
    }
#endif

// Opus
#ifdef WEBRTC_CODEC_OPUS
AudioDecoderOpus::AudioDecoderOpus(int num_channels, int freq_samples) {
  DCHECK(num_channels == 1 || num_channels == 2);
  channels_ = num_channels;
    fs_ = freq_samples;
  WebRtcOpus_DecoderCreate(&dec_state_, static_cast<int>(channels_), freq_samples);
}

AudioDecoderOpus::~AudioDecoderOpus() {
  WebRtcOpus_DecoderFree(dec_state_);
}

int AudioDecoderOpus::Decode(const uint8_t* encoded, size_t encoded_len,
                             int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type = 1;  // Default is speech.
  int16_t ret = WebRtcOpus_Decode(dec_state_, encoded,
                                  static_cast<int16_t>(encoded_len), decoded,
                                  &temp_type, fs_);
  if (ret > 0)
    ret *= static_cast<int16_t>(channels_);  // Return total number of samples.
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderOpus::DecodeRedundant(const uint8_t* encoded,
                                      size_t encoded_len, int16_t* decoded,
                                      SpeechType* speech_type) {
// sean for mos test
//    if (!PacketHasFec(encoded, encoded_len)) {
//        // This packet is a RED packet.
//        return Decode(encoded, encoded_len, decoded,
//                              speech_type);
//    }
  int16_t temp_type = 1;  // Default is speech.
  int16_t ret = WebRtcOpus_DecodeFec(dec_state_, encoded,
                                     static_cast<int16_t>(encoded_len), decoded,
                                     &temp_type,fs_);
  if (ret > 0)
    ret *= static_cast<int16_t>(channels_);  // Return total number of samples.
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderOpus::Init() {
  return WebRtcOpus_DecoderInit(dec_state_);
}

int AudioDecoderOpus::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) const {
  return WebRtcOpus_DurationEst(dec_state_,
                                encoded, static_cast<int>(encoded_len),fs_);
}

int AudioDecoderOpus::PacketDurationRedundant(const uint8_t* encoded,
                                              size_t encoded_len) const {
    if (!PacketHasFec(encoded, encoded_len)) {
        // This packet is a RED packet.
        return PacketDuration(encoded, encoded_len);
    }
  return WebRtcOpus_FecDurationEst(encoded, static_cast<int>(encoded_len),fs_);
}

bool AudioDecoderOpus::PacketHasFec(const uint8_t* encoded,
                                    size_t encoded_len) const {
  int fec;
  fec = WebRtcOpus_PacketHasFec(encoded, static_cast<int>(encoded_len), fs_);
  return (fec == 1);
}
#endif

AudioDecoderCng::AudioDecoderCng() {
    int16_t ret;
	ret = WebRtcCng_CreateDec(&dec_state_);
//  CHECK_EQ(0, ret);
}

AudioDecoderCng::~AudioDecoderCng() {
  WebRtcCng_FreeDec(dec_state_);
}

int AudioDecoderCng::Init() {
  return WebRtcCng_InitDec(dec_state_);
}

bool CodecSupported(NetEqDecoder codec_type) {
  switch (codec_type) {
    case kDecoderPCMu:
    case kDecoderPCMa:
    case kDecoderPCMu_2ch:
    case kDecoderPCMa_2ch:
#ifdef WEBRTC_CODEC_ILBC
    case kDecoderILBC:
#endif
#if defined(WEBRTC_CODEC_ISACFX) || defined(WEBRTC_CODEC_ISAC)
    case kDecoderISAC:
#endif
#ifdef WEBRTC_CODEC_ISAC
    case kDecoderISACswb:
    case kDecoderISACfb:
#endif
#ifdef WEBRTC_CODEC_PCM16
    case kDecoderPCM16B:
    case kDecoderPCM16Bwb:
    case kDecoderPCM16Bswb32kHz:
    case kDecoderPCM16Bswb48kHz:
    case kDecoderPCM16B_2ch:
    case kDecoderPCM16Bwb_2ch:
    case kDecoderPCM16Bswb32kHz_2ch:
    case kDecoderPCM16Bswb48kHz_2ch:
    case kDecoderPCM16B_5ch:
#endif
#ifdef WEBRTC_CODEC_G722
    case kDecoderG722:
    case kDecoderG722_2ch:
#endif
#ifdef WEBRTC_CODEC_G729
    case kDecoderG729:
#endif
#ifdef WEBRTC_CODEC_AMR
    case kDecoderAMR:
#endif
#ifdef WEBRTC_CODEC_SILK
    case kDecoderSILK8K:
    case kDecoderSILK12K:
    case kDecoderSILK16K:
#endif
#ifdef WEBRTC_CODEC_OPUS
    case kDecoderOpus8K:
    case kDecoderOpus16K:
    case kDecoderOpus:
    case kDecoderOpus_2ch:
#endif
    case kDecoderRED:
    case kDecoderAVT:
    case kDecoderCNGnb:
    case kDecoderCNGwb:
    case kDecoderCNGswb32kHz:
    case kDecoderCNGswb48kHz:
    case kDecoderArbitrary: {
      return true;
    }
    default: {
      return false;
    }
  }
}

int CodecSampleRateHz(NetEqDecoder codec_type) {
  switch (codec_type) {
    case kDecoderPCMu:
    case kDecoderPCMa:
    case kDecoderPCMu_2ch:
    case kDecoderPCMa_2ch:
#ifdef WEBRTC_CODEC_ILBC
    case kDecoderILBC:
#endif
#ifdef WEBRTC_CODEC_PCM16
    case kDecoderPCM16B:
    case kDecoderPCM16B_2ch:
    case kDecoderPCM16B_5ch:
#endif
    case kDecoderCNGnb: {
      return 8000;
    }
#if defined(WEBRTC_CODEC_ISACFX) || defined(WEBRTC_CODEC_ISAC)
    case kDecoderISAC:
#endif
#ifdef WEBRTC_CODEC_PCM16
    case kDecoderPCM16Bwb:
    case kDecoderPCM16Bwb_2ch:
#endif
#ifdef WEBRTC_CODEC_G722
    case kDecoderG722:
    case kDecoderG722_2ch:
#endif
    case kDecoderCNGwb: {
      return 16000;
    }
#ifdef WEBRTC_CODEC_ISAC
    case kDecoderISACswb:
    case kDecoderISACfb:
#endif
#ifdef WEBRTC_CODEC_PCM16
    case kDecoderPCM16Bswb32kHz:
    case kDecoderPCM16Bswb32kHz_2ch:
#endif
    case kDecoderCNGswb32kHz: {
      return 32000;
    }
#ifdef WEBRTC_CODEC_PCM16
    case kDecoderPCM16Bswb48kHz:
    case kDecoderPCM16Bswb48kHz_2ch: {
      return 48000;
    }
#endif
#ifdef WEBRTC_CODEC_G729
      case kDecoderG729:
          return 8000;
#endif
#ifdef WEBRTC_CODEC_OPUS
    case kDecoderOpus8K:
          return 8000;
    case kDecoderOpus16K:
          return 16000;
    case kDecoderOpus:
    case kDecoderOpus_2ch: {
      return 48000;
    }
#endif
    case kDecoderCNGswb48kHz: {
      // TODO(tlegrand): Remove limitation once ACM has full 48 kHz support.
      return 32000;
    }
    default: {
      return -1;  // Undefined sample rate.
    }
  }
}

AudioDecoder* CreateAudioDecoder(NetEqDecoder codec_type) {
  if (!CodecSupported(codec_type)) {
    return NULL;
  }
  switch (codec_type) {
    case kDecoderPCMu:
      return new AudioDecoderPcmU;
    case kDecoderPCMa:
      return new AudioDecoderPcmA;
    case kDecoderPCMu_2ch:
      return new AudioDecoderPcmUMultiCh(2);
    case kDecoderPCMa_2ch:
      return new AudioDecoderPcmAMultiCh(2);
#ifdef WEBRTC_CODEC_ILBC
    case kDecoderILBC:
      return new AudioDecoderIlbc;
#endif
#if defined(WEBRTC_CODEC_ISACFX)
    case kDecoderISAC: {
      AudioEncoderDecoderIsacFix::Config config;
      return new AudioEncoderDecoderIsacFix(config);
    }
#elif defined(WEBRTC_CODEC_ISAC)
    case kDecoderISAC: {
      AudioEncoderDecoderIsac::Config config;
      config.sample_rate_hz = 16000;
      return new AudioEncoderDecoderIsac(config);
    }
    case kDecoderISACswb:
    case kDecoderISACfb: {
      AudioEncoderDecoderIsac::Config config;
      config.sample_rate_hz = 32000;
      return new AudioEncoderDecoderIsac(config);
    }
#endif
#ifdef WEBRTC_CODEC_PCM16
    case kDecoderPCM16B:
    case kDecoderPCM16Bwb:
    case kDecoderPCM16Bswb32kHz:
    case kDecoderPCM16Bswb48kHz:
      return new AudioDecoderPcm16B;
    case kDecoderPCM16B_2ch:
    case kDecoderPCM16Bwb_2ch:
    case kDecoderPCM16Bswb32kHz_2ch:
    case kDecoderPCM16Bswb48kHz_2ch:
      return new AudioDecoderPcm16BMultiCh(2);
    case kDecoderPCM16B_5ch:
      return new AudioDecoderPcm16BMultiCh(5);
#endif
#ifdef WEBRTC_CODEC_G722
    case kDecoderG722:
      return new AudioDecoderG722;
    case kDecoderG722_2ch:
      return new AudioDecoderG722Stereo;
#endif
#ifdef WEBRTC_CODEC_G729
    case kDecoderG729:
          return new AudioDecoderG729();
//          return new acmG
#endif
#ifdef WEBRTC_CODEC_OPUS
    case kDecoderOpus:
	  return new AudioDecoderOpus(1, 48000);
	case kDecoderOpus8K:
	  return new AudioDecoderOpus(1, 8000);
	case kDecoderOpus16K:
      return new AudioDecoderOpus(1, 16000);
    case kDecoderOpus_2ch:
      return new AudioDecoderOpus(2, 48000);
#endif
    case kDecoderCNGnb:
    case kDecoderCNGwb:
    case kDecoderCNGswb32kHz:
    case kDecoderCNGswb48kHz:
      return new AudioDecoderCng;
    case kDecoderRED:
    case kDecoderAVT:
    case kDecoderArbitrary:
    default: {
      return NULL;
    }
  }
}

}  // namespace cloopenwebrtc
