/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_channel_group.h"

#include "../base/thread_annotations.h"
#include "../system_wrappers/include/common.h"
#include "experiments.h"
#include "bitrate_controller.h"
#include "remote_bitrate_estimator.h"
#include "../module/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.h"
#include "rtp_rtcp.h"
#include "process_thread.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/logging.h"
#include "call_stats.h"
#include "encoder_state_feedback.h"
#include "vie_channel.h"
#include "vie_encoder.h"
#include "vie_remb.h"

namespace cloopenwebrtc {
namespace {

static const uint32_t kTimeOffsetSwitchThreshold = 30;

class WrappingBitrateEstimator : public RemoteBitrateEstimator {
 public:
  WrappingBitrateEstimator(int engine_id,
                           RemoteBitrateObserver* observer,
                           Clock* clock,
                           const Config& config)
      : observer_(observer),
        clock_(clock),
        crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
        engine_id_(engine_id),
        min_bitrate_bps_(config.Get<RemoteBitrateEstimatorMinRate>().min_rate),
        rbe_(new cloopenwebrtc::RemoteBitrateEstimatorAbsSendTime(observer, clock)),
        using_absolute_send_time_(false),
        packets_since_absolute_send_time_(0) {
  }

  virtual ~WrappingBitrateEstimator() {}

  virtual void IncomingPacket(int64_t arrival_time_ms,
                              size_t payload_size,
                              const RTPHeader& header) OVERRIDE {
    CriticalSectionScoped cs(crit_sect_.get());
    PickEstimatorFromHeader(header);
    rbe_->IncomingPacket(arrival_time_ms, payload_size, header);
  }

  virtual int32_t Process() OVERRIDE {
    CriticalSectionScoped cs(crit_sect_.get());
    return rbe_->Process();
  }

  virtual int64_t TimeUntilNextProcess() OVERRIDE {
    CriticalSectionScoped cs(crit_sect_.get());
    return rbe_->TimeUntilNextProcess();
  }

  virtual void OnRttUpdate(int64_t avg_rtt, int64_t max_rtt) OVERRIDE {
    CriticalSectionScoped cs(crit_sect_.get());
    rbe_->OnRttUpdate(avg_rtt, max_rtt);
  }

  virtual void RemoveStream(unsigned int ssrc) OVERRIDE {
    CriticalSectionScoped cs(crit_sect_.get());
    rbe_->RemoveStream(ssrc);
  }

  virtual bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                              unsigned int* bitrate_bps) const OVERRIDE {
    CriticalSectionScoped cs(crit_sect_.get());
    return rbe_->LatestEstimate(ssrcs, bitrate_bps);
  }

  virtual void SetMinBitrate(int min_bitrate_bps) OVERRIDE {
      //min_bitrate_bps_ = min_bitrate_bps;
  }

  virtual bool GetStats(ReceiveBandwidthEstimatorStats* output) const OVERRIDE {
    CriticalSectionScoped cs(crit_sect_.get());
    return rbe_->GetStats(output);
  }

  void SetConfig(const cloopenwebrtc::Config& config) {
    CriticalSectionScoped cs(crit_sect_.get());
  }

 private:
  void PickEstimatorFromHeader(const RTPHeader& header)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_.get()) {
    if (header.extension.hasAbsoluteSendTime) {
      // If we see AST in header, switch RBE strategy immediately.
      if (!using_absolute_send_time_) {
        LOG(LS_INFO) <<
            "WrappingBitrateEstimator: Switching to absolute send time RBE.";
        using_absolute_send_time_ = true;
        //PickEstimator();
      }
      packets_since_absolute_send_time_ = 0;
    } else {
      // When we don't see AST, wait for a few packets before going back to TOF.
      if (using_absolute_send_time_) {
        ++packets_since_absolute_send_time_;
        if (packets_since_absolute_send_time_ >= kTimeOffsetSwitchThreshold) {
          LOG(LS_INFO) << "WrappingBitrateEstimator: Switching to transmission "
                       << "time offset RBE.";
          using_absolute_send_time_ = false;
          //PickEstimator();
        }
      }
    }
  }

  RemoteBitrateObserver* observer_;
  Clock* clock_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  const int engine_id_;
  const uint32_t min_bitrate_bps_;
  scoped_ptr<RemoteBitrateEstimator> rbe_;
  bool using_absolute_send_time_;
  uint32_t packets_since_absolute_send_time_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(WrappingBitrateEstimator);
};
}  // namespace

ChannelGroup::ChannelGroup(int engine_id,
                           ProcessThread* process_thread,
                           const Config* config,
							BitrateController* bitrate_controller,
							RemoteBitrateEstimator* remote_bitrate_estimator,
							VieRemb* remb,
							CallStats* call_stats)
    : remb_(remb),
      call_stats_(call_stats),
      encoder_state_feedback_(new EncoderStateFeedback()),
      config_(config),
      own_config_(),
      process_thread_(process_thread),
	  bitrate_controller_(bitrate_controller),
	  remote_bitrate_estimator_(remote_bitrate_estimator){
  if (!config) {
    own_config_.reset(new Config);
    config_ = own_config_.get();
	own_config_->Set<RemoteBitrateEstimatorMinRate>(new RemoteBitrateEstimatorMinRate());
  }
  assert(config_);  // Must have a valid config pointer here.

  call_stats_->RegisterStatsObserver(remote_bitrate_estimator_);
}

ChannelGroup::~ChannelGroup() {
  call_stats_->DeregisterStatsObserver(remote_bitrate_estimator_);
  assert(channels_.empty());
  assert(!remb_->InUse());
}

void ChannelGroup::AddChannel(int channel_id) {
  channels_.insert(channel_id);
}

void ChannelGroup::RemoveChannel(int channel_id, unsigned int ssrc) {
  channels_.erase(channel_id);
  remote_bitrate_estimator_->RemoveStream(ssrc);
}

bool ChannelGroup::HasChannel(int channel_id) {
  return channels_.find(channel_id) != channels_.end();
}

bool ChannelGroup::Empty() {
  return channels_.empty();
}

BitrateController* ChannelGroup::GetBitrateController() {
  return bitrate_controller_;
}

RemoteBitrateEstimator* ChannelGroup::GetRemoteBitrateEstimator() {
  return remote_bitrate_estimator_;
}

CallStats* ChannelGroup::GetCallStats() {
  return call_stats_;
}

EncoderStateFeedback* ChannelGroup::GetEncoderStateFeedback() {
  return encoder_state_feedback_.get();
}

void ChannelGroup::SetChannelRembStatus(int channel_id,
                                        bool sender,
                                        bool receiver,
                                        ViEChannel* channel) {
  // Update the channel state.
  channel->EnableRemb(sender || receiver);
  // Update the REMB instance with necessary RTP modules.
  RtpRtcp* rtp_module = channel->rtp_rtcp();
  if (sender) { //need to fix: use default_rtp_rtcp_. ylr
    remb_->AddRembSender(rtp_module);
  } else {
    remb_->RemoveRembSender(rtp_module);
  }
  if (receiver) {
    remb_->AddReceiveChannel(rtp_module);
  } else {
    remb_->RemoveReceiveChannel(rtp_module);
  }
}

void ChannelGroup::SetBandwidthEstimationConfig(const cloopenwebrtc::Config& config) {
  WrappingBitrateEstimator* estimator =
      static_cast<WrappingBitrateEstimator*>(remote_bitrate_estimator_);
  estimator->SetConfig(config);
}
}  // namespace webrtc
