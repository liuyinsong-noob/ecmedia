/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_DTMF_QUEUE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_DTMF_QUEUE_H_

#include <list>

#include "../base/criticalsection.h"

namespace yuntongxunwebrtc {
class DtmfQueue {
 public:
  struct Event {
    uint16_t duration_ms = 0;
    uint8_t payload_type = 0;
    uint8_t key = 0;
    uint8_t level = 0;
  };

  DtmfQueue();
  ~DtmfQueue();

  bool AddDtmf(const Event& event);
  bool NextDtmf(Event* event);
  bool PendingDtmf() const;

 private:
  yuntongxunwebrtc::CriticalSection dtmf_critsect_;
  std::list<Event> queue_;
};
}  // namespace yuntongxunwebrtc

#endif  // WEBRTC_MODULES_RTP_RTCP_SOURCE_DTMF_QUEUE_H_
