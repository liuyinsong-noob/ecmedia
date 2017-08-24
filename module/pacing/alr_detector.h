/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_PACING_ALR_DETECTOR_H_
#define WEBRTC_MODULES_PACING_ALR_DETECTOR_H_

#include "../base/rate_statistics.h"
#include "../common_types.h"
#include "../module/pacing/paced_sender.h"
#include "../typedefs.h"

namespace cloopenwebrtc {

// Application limited region detector is a class that utilizes signals of
// elapsed time and bytes sent to estimate whether network traffic is
// currently limited by the application's ability to generate traffic.
//
// AlrDetector provides a signal that can be utilized to adjust
// estimate bandwidth.
// Note: This class is not thread-safe.
class AlrDetector {
 public:
  AlrDetector();
  ~AlrDetector();

  void OnBytesSent(size_t bytes_sent, int64_t now_ms);

  // Set current estimated bandwidth.
  void SetEstimatedBitrate(int bitrate_bps);

  // Returns time in milliseconds when the current application-limited region
  // started or empty result if the sender is currently not application-limited.
  cloopenwebrtc::Optional<int64_t> GetApplicationLimitedRegionStartTime() const;

 private:
  RateStatistics rate_;
  int estimated_bitrate_bps_ = 0;

  // Non-empty in ALR state.
  cloopenwebrtc::Optional<int64_t> alr_started_time_ms_;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_MODULES_PACING_ALR_DETECTOR_H_
