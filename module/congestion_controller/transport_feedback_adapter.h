/*
 *  Copyright (c) 2015 The yuntongxunwebrtc project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef yuntongxunwebrtc_MODULES_CONGESTION_CONTROLLER_TRANSPORT_FEEDBACK_ADAPTER_H_
#define yuntongxunwebrtc_MODULES_CONGESTION_CONTROLLER_TRANSPORT_FEEDBACK_ADAPTER_H_

#include <memory>
#include <vector>

#include "../base/criticalsection.h"
#include "../base/thread_annotations.h"
#include "../base/thread_checker.h"
#include "../module/congestion_controller/delay_based_bwe.h"
#include "../module/congestion_controller/network_control.h"
#include "../module/interface/module_common_types.h"
#include "../module/remote_bitrate_estimator/include/send_time_history.h"

namespace yuntongxunwebrtc {

class BitrateController;
class RtcEventLog;
class ProcessThread;

class TransportFeedbackAdapter : public TransportFeedbackObserver {
 public:
  TransportFeedbackAdapter(RtcEventLog* event_log,
                           Clock* clock,
                           NetworkControllerInterface* cc_controller);
  virtual ~TransportFeedbackAdapter();

  // Implements TransportFeedbackObserver.
  void AddPacket(uint16_t sequence_number,
                 size_t length,
                 const PacedPacketInfo& pacing_info) override;
  void OnSentPacket(uint16_t sequence_number, int64_t send_time_ms);

  // TODO(holmer): This method should return DelayBasedBwe::Result so that we
  // can get rid of the dependency on BitrateController. Requires changes
  // to the CongestionController interface.
  void OnTransportFeedback(const rtcp::TransportFeedback& feedback) override;
  std::vector<PacketFeedback> GetTransportFeedbackVector() const override;


  void SetTransportOverhead(int transport_overhead_bytes_per_packet);
 private:
  std::vector<PacketFeedback> GetPacketFeedbackVector(
      const rtcp::TransportFeedback& feedback);

  const bool send_side_bwe_with_overhead_;
  yuntongxunwebrtc::CriticalSection lock_;
  int transport_overhead_bytes_per_packet_ GUARDED_BY(&lock_);
  SendTimeHistory send_time_history_ GUARDED_BY(&lock_);
  RtcEventLog* const event_log_;
  Clock* const clock_;
  int64_t current_offset_ms_;
  int64_t last_timestamp_us_;
  std::vector<PacketFeedback> last_packet_feedback_vector_;
  NetworkControllerInterface* const cc_controller_;
};

}  // namespace yuntongxunwebrtc

#endif  // yuntongxunwebrtc_MODULES_CONGESTION_CONTROLLER_TRANSPORT_FEEDBACK_ADAPTER_H_
