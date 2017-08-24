/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_INCLUDE_FLEXFEC_SENDER_H_
#define WEBRTC_MODULES_RTP_RTCP_INCLUDE_FLEXFEC_SENDER_H_

#include <memory>
#include <vector>

#include "../base/basictypes.h"
#include "../base/random.h"
#include "../base/sequenced_task_checker.h"
#include "../config.h"
#include "../module/interface/module_common_types.h"
#include "../module/rtp_rtcp/include/flexfec_sender.h"
#include "../module/rtp_rtcp/source/rtp_header_extension.h"
#include "../module/rtp_rtcp/source/rtp_packet_to_send.h"
#include "../module/rtp_rtcp/source/ulpfec_generator.h"
#include "../system_wrappers/include/clock.h"

namespace cloopenwebrtc {

class RtpPacketToSend;

// Note that this class is not thread safe, and thus requires external
// synchronization.

class FlexfecSender {
 public:
  FlexfecSender(int payload_type,
                uint32_t ssrc,
                uint32_t protected_media_ssrc,
                const std::vector<RtpExtension>& rtp_header_extensions,
                Clock* clock);
  ~FlexfecSender();

  uint32_t ssrc() const { return ssrc_; }

  // Sets the FEC rate, max frames sent before FEC packets are sent,
  // and what type of generator matrices are used.
  void SetFecParameters(const FecProtectionParams& params);

  // Adds a media packet to the internal buffer. When enough media packets
  // have been added, the FEC packets are generated and stored internally.
  // These FEC packets are then obtained by calling GetFecPackets().
  // Returns true if the media packet was successfully added.
  bool AddRtpPacketAndGenerateFec(const RtpPacketToSend& packet);

  // Returns true if there are generated FEC packets available.
  bool FecAvailable() const;

  // Returns generated FlexFEC packets.
  std::vector<std::unique_ptr<RtpPacketToSend>> GetFecPackets();

  // Returns the overhead, per packet, for FlexFEC.
  size_t MaxPacketOverhead() const;

 private:
  // Utility.
  Clock* const clock_;
  Random random_;
  int64_t last_generated_packet_ms_;

  // Config.
  const int payload_type_;
  const uint32_t timestamp_offset_;
  const uint32_t ssrc_;
  const uint32_t protected_media_ssrc_;
  // Sequence number of next packet to generate.
  uint16_t seq_num_;

  // Implementation.
  UlpfecGenerator ulpfec_generator_;
  const RtpHeaderExtensionMap rtp_header_extension_map_;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_MODULES_RTP_RTCP_INCLUDE_FLEXFEC_SENDER_H_
