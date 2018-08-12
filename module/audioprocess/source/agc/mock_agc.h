/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AGC_MOCK_AGC_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AGC_MOCK_AGC_H_

#include "agc.h"

#include "gmock/gmock.h"
#include "module_common_types.h"

namespace yuntongxunwebrtc {

class MockAgc : public Agc {
 public:
  MOCK_METHOD2(AnalyzePreproc, float(const int16_t* audio, int length));
  MOCK_METHOD3(Process, int(const int16_t* audio, int length,
                            int sample_rate_hz));
  MOCK_METHOD1(GetRmsErrorDb, bool(int* error));
  MOCK_METHOD0(Reset, void());
  MOCK_METHOD1(set_target_level_dbfs, int(int level));
  MOCK_CONST_METHOD0(target_level_dbfs, int());
  MOCK_METHOD1(EnableStandaloneVad, void(bool enable));
  MOCK_CONST_METHOD0(standalone_vad_enabled, bool());
};

}  // namespace yuntongxunwebrtc

#endif  // WEBRTC_MODULES_AUDIO_PROCESSING_AGC_MOCK_AGC_H_
