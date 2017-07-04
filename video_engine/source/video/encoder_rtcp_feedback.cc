/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./video/encoder_rtcp_feedback.h"

#include "../base/checks.h"
#include "vie_encoder.h"

static const int kMinKeyFrameRequestIntervalMs = 300;

namespace cloopenwebrtc {

EncoderRtcpFeedback::EncoderRtcpFeedback(Clock* clock,
                                         const std::vector<uint32_t>& ssrcs,
                                         ViEEncoder* encoder)
    : clock_(clock),
      ssrcs_(ssrcs),
      vie_encoder_(encoder),
      time_last_intra_request_ms_(ssrcs.size(), -1) {
//  DCHECK(!ssrcs.empty());
}

bool EncoderRtcpFeedback::HasSsrc(uint32_t ssrc) {
  for (uint32_t registered_ssrc : ssrcs_) {
    if (registered_ssrc == ssrc) {
      return true;
    }
  }
  return false;
}

size_t EncoderRtcpFeedback::GetStreamIndex(uint32_t ssrc) {
  for (size_t i = 0; i < ssrcs_.size(); ++i) {
    if (ssrcs_[i] == ssrc)
      return i;
  }
  NOTREACHED() << "Unknown ssrc " << ssrc;
  return 0;
}

void EncoderRtcpFeedback::OnReceivedIntraFrameRequest(uint32_t ssrc) {
  DCHECK(HasSsrc(ssrc));
  size_t index = GetStreamIndex(ssrc);
  {
    // TODO(mflodman): Move to ViEEncoder after some more changes making it
    // easier to test there.
    int64_t now_ms = clock_->TimeInMilliseconds();
    CritScope lock(&crit_);
    if (time_last_intra_request_ms_[index] + kMinKeyFrameRequestIntervalMs >
        now_ms) {
      return;
    }
    time_last_intra_request_ms_[index] = now_ms;
  }

  vie_encoder_->OnReceivedIntraFrameRequest(index);
}

void EncoderRtcpFeedback::OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) {
  DCHECK(HasSsrc(ssrc));

  vie_encoder_->OnReceivedSLI(ssrc, picture_id);
}

void EncoderRtcpFeedback::OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) {
  DCHECK(HasSsrc(ssrc));

  vie_encoder_->OnReceivedRPSI(ssrc, picture_id);  
}

void EncoderRtcpFeedback::SetSsrcs(std::vector<uint32_t>& ssrcs) {
	ssrcs_ = ssrcs;
	std::vector<int64_t> temp(ssrcs.size(), -1);
	time_last_intra_request_ms_ = temp;
}
}  // namespace webrtc
