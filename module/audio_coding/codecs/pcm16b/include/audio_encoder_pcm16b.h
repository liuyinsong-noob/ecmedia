/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_PCM16B_INCLUDE_AUDIO_ENCODER_PCM16B_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_PCM16B_INCLUDE_AUDIO_ENCODER_PCM16B_H_

#include "audio_encoder_pcm.h"

namespace yuntongxunwebrtc {

class AudioEncoderPcm16B : public AudioEncoderPcm {
 public:
  struct Config : public AudioEncoderPcm::Config {
   public:
    Config() : AudioEncoderPcm::Config(107), sample_rate_hz(8000) {}

    int sample_rate_hz;
  };

  explicit AudioEncoderPcm16B(const Config& config)
      : AudioEncoderPcm(config, config.sample_rate_hz) {}

 protected:
  virtual int16_t EncodeCall(const int16_t* audio,
                             size_t input_len,
                             uint8_t* encoded) OVERRIDE;
};

}  // namespace webrtc
#endif  // WEBRTC_MODULES_AUDIO_CODING_CODECS_PCM16B_INCLUDE_AUDIO_ENCODER_PCM16B_H_
