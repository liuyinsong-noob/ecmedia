/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_PACING_PACKET_ROUTER_H_
#define WEBRTC_MODULES_PACING_PACKET_ROUTER_H_

#include <list>


#include "../base/constructormagic.h"
#include "../base/criticalsection.h"
#include "../base/thread_annotations.h"
#include "../base/thread_checker.h"
#include "../common_types.h"
#include "../module/pacing/paced_sender.h"
#include "../module/rtp_rtcp/include/rtp_rtcp_defines.h"

namespace yuntongxunwebrtc {

class RtpRtcp;
namespace rtcp {
class TransportFeedback;
}  // namespace rtcp

// PacketRouter routes outgoing data to the correct sending RTP module, based
// on the simulcast layer in RTPVideoHeader.
class PacketRouter : public PacedSender::PacketSender,
                     public TransportSequenceNumberAllocator {
 public:
  PacketRouter();
  virtual ~PacketRouter();

  void AddRtpModule(RtpRtcp* rtp_module);
  void RemoveRtpModule(RtpRtcp* rtp_module);

  // Implements PacedSender::Callback.
  bool TimeToSendPacket(uint32_t ssrc,
                        uint16_t sequence_number,
                        int64_t capture_timestamp,
                        bool retransmission,
                        const PacedPacketInfo& packet_info) override;

  size_t TimeToSendPadding(size_t bytes,
                           const PacedPacketInfo& packet_info) override;

  void SetTransportWideSequenceNumber(uint16_t sequence_number);
  uint16_t AllocateSequenceNumber() override;

  // Send transport feedback packet to send-side.
  virtual bool SendFeedback(rtcp::TransportFeedback* packet);

 private:
  yuntongxunwebrtc::ThreadChecker pacer_thread_checker_;
  yuntongxunwebrtc::CriticalSection modules_crit_;
  std::list<RtpRtcp*> rtp_modules_ GUARDED_BY(modules_crit_);

  volatile int transport_seq_;

  DISALLOW_COPY_AND_ASSIGN(PacketRouter);
};
}  // namespace yuntongxunwebrtc
#endif  // WEBRTC_MODULES_PACING_PACKET_ROUTER_H_
