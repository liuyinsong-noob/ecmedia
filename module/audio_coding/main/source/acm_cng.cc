/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "acm_cng.h"

#include "webrtc_cng.h"
#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "../system_wrappers/include/trace.h"

namespace yuntongxunwebrtc {

namespace acm2 {

ACMCNG::ACMCNG(int16_t codec_id) {
  encoder_inst_ptr_ = NULL;
  codec_id_ = codec_id;
  samp_freq_hz_ = ACMCodecDB::CodecFreq(codec_id_);
  return;
}

ACMCNG::~ACMCNG() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcCng_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  return;
}

// CNG is not like a regular encoder, this function
// should not be called normally
// instead the following function is called from inside
// ACMGenericCodec::ProcessFrameVADDTX
int16_t ACMCNG::InternalEncode(uint8_t* /* bitstream */,
                               int16_t* /* bitstream_len_byte */) {
  return -1;
}

// CNG is not like a regular encoder,
// this function should not be called normally
// instead the following function is called from inside
// ACMGenericCodec::ProcessFrameVADDTX
int16_t ACMCNG::InternalInitEncoder(WebRtcACMCodecParams* /* codec_params */) {
  return -1;
}

ACMGenericCodec* ACMCNG::CreateInstance(void) { return NULL; }

int16_t ACMCNG::InternalCreateEncoder() {
  if (WebRtcCng_CreateEnc(&encoder_inst_ptr_) < 0) {
    encoder_inst_ptr_ = NULL;
    return -1;
  } else {
    return 0;
  }
}

void ACMCNG::DestructEncoderSafe() {
  if (encoder_inst_ptr_ != NULL) {
    WebRtcCng_FreeEnc(encoder_inst_ptr_);
    encoder_inst_ptr_ = NULL;
  }
  encoder_exist_ = false;
  encoder_initialized_ = false;
}

}  // namespace acm2

}  // namespace yuntongxunwebrtc
