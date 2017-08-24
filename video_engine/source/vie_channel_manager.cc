/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_channel_manager.h"

#include "../system_wrappers/include/common.h"
#include "engine_configurations.h"
#include "rtp_rtcp.h"
#include "process_thread.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/logging.h"
#include "call_stats.h"
#include "encoder_state_feedback.h"
#include "vie_channel.h"
#include "vie_defines.h"
#include "vie_encoder.h"
#include "vie_remb.h"
#include "voe_video_sync.h"

#include "send_statistics_proxy.h"
#include "../module/utility/include/process_thread.h"
#include "../logging/rtc_event_log/rtc_event_log.h"

namespace cloopenwebrtc {

ViEChannelManager::ViEChannelManager(
    int engine_id,
    int number_of_cores,
    const Config& config)
    : channel_id_critsect_(CriticalSectionWrapper::CreateCriticalSection()),
      engine_id_(engine_id),
      number_of_cores_(number_of_cores),
      free_channel_ids_(new bool[kViEMaxNumberOfChannels]),
      free_channel_ids_size_(kViEMaxNumberOfChannels),
      voice_sync_interface_(NULL),
      voice_engine_(NULL),
      module_process_thread_(NULL),
      engine_config_(config),
	  udptransport_critsect_(CriticalSectionWrapper::CreateCriticalSection()),
	  rtc_event_log_(RtcEventLog::Create()),
	  pacer_thread_(ProcessThread::CreateProcessThread()),
	  use_sendside_bwe_(/*true*/false),
	  call_stats_(new CallStats()){

  for (int idx = 0; idx < free_channel_ids_size_; idx++) {
    free_channel_ids_[idx] = true;
  }
}

ViEChannelManager::~ViEChannelManager() {
	//clean up pacer_thread
	pacer_thread_->Stop();
	ProcessThread::DestroyProcessThread(pacer_thread_);
	module_process_thread_->DeRegisterModule(call_stats_.get());
  while (channel_map_.size() > 0) {
    ChannelMap::iterator it = channel_map_.begin();
    // DeleteChannel will erase this channel from the map and invalidate |it|.
    DeleteChannel(it->first);
  }

  if (voice_sync_interface_) {
    voice_sync_interface_->Release();
  }
  if (channel_id_critsect_) {
    delete channel_id_critsect_;
    channel_id_critsect_ = NULL;
  }
  if (free_channel_ids_) {
    delete[] free_channel_ids_;
    free_channel_ids_ = NULL;
    free_channel_ids_size_ = 0;
  }

  while (rtp_udptransport_map_.size() > 0) {
	  RtpUdpTransportMap::iterator r_it = rtp_udptransport_map_.begin();
	  r_it->second->StopReceiving();
	  UdpTransport::Destroy(r_it->second);
	  rtp_udptransport_map_.erase(r_it);
  }

  if (udptransport_critsect_) {
	  delete udptransport_critsect_;
	  udptransport_critsect_ = NULL;
  }
  rtc_event_log_->StopLogging();
  assert(channel_groups_.empty());
  assert(channel_map_.empty());
  assert(vie_encoder_map_.empty());
  assert(rtp_udptransport_map_.empty());
}

void ViEChannelManager::SetModuleProcessThread(
    ProcessThread* module_process_thread) {
  assert(!module_process_thread_);
  module_process_thread_ = module_process_thread;
}

int ViEChannelManager::CreateChannel(int* channel_id,
                                     const Config* channel_group_config) {
  CriticalSectionScoped cs(channel_id_critsect_);

  // Get a new channel id.
  int new_channel_id = FreeChannelId();
  if (new_channel_id == -1) {
    return -1;
  }

  // Create a new channel group and add this channel.
  if (!congestion_controller_.get())
  {
	  congestion_controller_.reset(new CongestionController(Clock::GetRealTimeClock(),
									this,
									&remb_,
									rtc_event_log_.get(),
									&packet_router_));
	  //set bwe config
	  //congestion_controller_->SignalNetworkState(kNetworkDown);
	  congestion_controller_->pacer()->SetEstimatedBitrate(300000);
	  congestion_controller_->SetBweBitrates(30000, 300000, 2000000);
	  congestion_controller_->EnablePeriodicAlrProbing(true);
  }

  BitrateController* bitrate_controller = congestion_controller_->GetBitrateController();
  PacedSender* paced_sender = congestion_controller_->pacer();
  RemoteBitrateEstimator* remote_bitrate_estimator =
	  congestion_controller_->GetRemoteBitrateEstimator(use_sendside_bwe_);

  ChannelGroup* group = new ChannelGroup(engine_id_, 
										module_process_thread_,
										channel_group_config,
										bitrate_controller,
										remote_bitrate_estimator,
										&remb_,
										call_stats_.get());

  ViEEncoder* vie_encoder = new ViEEncoder(engine_id_, new_channel_id,
                                           number_of_cores_,
                                           engine_config_,
                                           *module_process_thread_,
                                           bitrate_controller,
										   paced_sender,
										   &packet_router_,
											congestion_controller_->GetTransportFeedbackObserver(),
											congestion_controller_->GetRetransmissionRateLimiter(),
											rtc_event_log_.get());

  RtcpBandwidthObserver* bandwidth_observer =
      bitrate_controller->CreateRtcpBandwidthObserver();

  EncoderStateFeedback* encoder_state_feedback =
      group->GetEncoderStateFeedback();
  RtcpRttStats* rtcp_rtt_stats =
      group->GetCallStats()->rtcp_rtt_stats();

  if (!(vie_encoder->Init() &&
        CreateChannelObject(new_channel_id, vie_encoder, bandwidth_observer,
                            remote_bitrate_estimator, rtcp_rtt_stats,
                            encoder_state_feedback->GetRtcpIntraFrameObserver(),
                            true))) {
    delete vie_encoder;
    vie_encoder = NULL;
    ReturnChannelId(new_channel_id);
    delete group;
    return -1;
  }

	*channel_id = new_channel_id;
	group->AddChannel(*channel_id);
	channel_groups_.push_back(group);

	return 0;
}
int ViEChannelManager::AddSsrcToEncoder(int channelid)
{
  // Add ViEEncoder to EncoderFeedBackObserver.
	CriticalSectionScoped cs(channel_id_critsect_);

	ChannelMap::iterator c_it = channel_map_.find(channelid);
	if (c_it == channel_map_.end()) {
		// No such channel.
		return -1;
	}
	unsigned int ssrc = 0;
	int idx = 0;
	channel_map_[channelid]->GetLocalSSRC(idx, &ssrc);

	ChannelGroup* group = FindGroup(channelid);
	if (!group){
		LOG(LS_ERROR) << "can not find group of channel " << channelid;
		return -1;
	}

	EncoderMap::const_iterator it = vie_encoder_map_.find(channelid);
	if (it == vie_encoder_map_.end()) {
		LOG(LS_ERROR) << "can not find encoder of channel " << channelid;
        return -1;
	}
	ViEEncoder* vie_encoder = it->second;

	EncoderStateFeedback* encoder_state_feedback =
		group->GetEncoderStateFeedback();
  encoder_state_feedback->AddEncoder(ssrc, vie_encoder);
  std::list<unsigned int> ssrcs;
  ssrcs.push_back(ssrc);
  vie_encoder->SetSsrcs(ssrcs);
	// Register the channel to receive stats updates.
	group->GetCallStats()->RegisterStatsObserver(
		channel_map_[channelid]->GetStatsObserver());

  //add by ylr
  SendStatisticsProxy *p_sendStats = vie_encoder->GetSendStatisticsProxy();
  
  if (p_sendStats)
  {
	  group->GetCallStats()->RegisterStatsObserver(p_sendStats);
      RemoteBitrateEstimator* remote_bitrate_estimator =
      group->GetRemoteBitrateEstimator();
	  p_sendStats->SetRemoteBitrateEstimator(remote_bitrate_estimator);
	  //bitrate_controller->RegisterSendsideBweObserver(p_sendStats);
  }
  
  
  return 0;
}

int ViEChannelManager::CreateChannel(int* channel_id,
                                     int original_channel,
                                     bool sender) {
  CriticalSectionScoped cs(channel_id_critsect_);

  ChannelGroup* channel_group = FindGroup(original_channel);
  if (!channel_group) {
    return -1;
  }
  int new_channel_id = FreeChannelId();
  if (new_channel_id == -1) {
    return -1;
  }
  BitrateController* bitrate_controller = congestion_controller_->GetBitrateController();
  RtcpBandwidthObserver* bandwidth_observer =
      bitrate_controller->CreateRtcpBandwidthObserver();
  RemoteBitrateEstimator* remote_bitrate_estimator =
	  congestion_controller_->GetRemoteBitrateEstimator(use_sendside_bwe_);
  EncoderStateFeedback* encoder_state_feedback =
      channel_group->GetEncoderStateFeedback();
    RtcpRttStats* rtcp_rtt_stats =
        channel_group->GetCallStats()->rtcp_rtt_stats();

  ViEEncoder* vie_encoder = NULL;
  if (sender) {
    // We need to create a new ViEEncoder.
    vie_encoder = new ViEEncoder(engine_id_, new_channel_id, number_of_cores_,
                                 engine_config_,
                                 *module_process_thread_,
                                 bitrate_controller,
								 congestion_controller_->pacer(),
								 &packet_router_,
								 congestion_controller_->GetTransportFeedbackObserver(),
		                         congestion_controller_->GetRetransmissionRateLimiter(),
								  rtc_event_log_.get());
    if (!(vie_encoder->Init() &&
        CreateChannelObject(
            new_channel_id,
            vie_encoder,
            bandwidth_observer,
            remote_bitrate_estimator,
            rtcp_rtt_stats,
            encoder_state_feedback->GetRtcpIntraFrameObserver(),
            sender))) {
      delete vie_encoder;
      vie_encoder = NULL;
    }
    // Register the ViEEncoder to get key frame requests for this channel.
    unsigned int ssrc = 0;
    int stream_idx = 0;
    channel_map_[new_channel_id]->GetLocalSSRC(stream_idx, &ssrc);
    encoder_state_feedback->AddEncoder(ssrc, vie_encoder);
  } else {
    vie_encoder = ViEEncoderPtr(original_channel);
    assert(vie_encoder);
    if (!CreateChannelObject(
        new_channel_id,
        vie_encoder,
        bandwidth_observer,
        remote_bitrate_estimator,
        rtcp_rtt_stats,
        encoder_state_feedback->GetRtcpIntraFrameObserver(),
        sender)) {
      vie_encoder = NULL;
    }
  }
  if (!vie_encoder) {
    ReturnChannelId(new_channel_id);
    return -1;
  }
  *channel_id = new_channel_id;
  channel_group->AddChannel(*channel_id);
  // Register the channel to receive stats updates.
  channel_group->GetCallStats()->RegisterStatsObserver(
      channel_map_[new_channel_id]->GetStatsObserver());

  //add by ylr
  SendStatisticsProxy *p_sendStats = vie_encoder->GetSendStatisticsProxy();
  p_sendStats->SetRemoteBitrateEstimator(remote_bitrate_estimator);

  return 0;
}

int ViEChannelManager::DeleteChannel(int channel_id) {
  ViEChannel* vie_channel = NULL;
  ViEEncoder* vie_encoder = NULL;
  ChannelGroup* group = NULL;
  UdpTransport * transport = NULL;

  {
    // Write lock to make sure no one is using the channel.
    ViEManagerWriteScoped wl(this);

    // Protect the maps.
    CriticalSectionScoped cs(channel_id_critsect_);

    ChannelMap::iterator c_it = channel_map_.find(channel_id);
    if (c_it == channel_map_.end()) {
      // No such channel.
      return -1;
    }
    vie_channel = c_it->second;
    channel_map_.erase(c_it);

    ReturnChannelId(channel_id);

	transport = vie_channel->GetUdpTransport();

    // Find the encoder object.
    EncoderMap::iterator e_it = vie_encoder_map_.find(channel_id);
    assert(e_it != vie_encoder_map_.end());
    vie_encoder = e_it->second;

	group = FindGroup(channel_id);
    group->GetCallStats()->DeregisterStatsObserver(
        vie_channel->GetStatsObserver());
    group->SetChannelRembStatus(channel_id, false, false, vie_channel);

	//add by ylr
	if (vie_encoder->channel_id() == channel_id)
	{
		SendStatisticsProxy *p_sendStats = vie_encoder->GetSendStatisticsProxy();
		group->GetCallStats()->DeregisterStatsObserver(p_sendStats);
		//group->GetBitrateController()->DeregisterSendsideBweObserver();
	}

    // Remove the feedback if we're owning the encoder.
    if (vie_encoder->channel_id() == channel_id) {
      group->GetEncoderStateFeedback()->RemoveEncoder(vie_encoder);
    }

    unsigned int remote_ssrc = 0;
    vie_channel->GetRemoteSSRC(&remote_ssrc);
    group->RemoveChannel(channel_id, remote_ssrc);

    // Check if other channels are using the same encoder.
    if (ChannelUsingViEEncoder(channel_id)) {
      vie_encoder = NULL;
    } else {
      // Delete later when we've released the critsect.
    }

    // We can't erase the item before we've checked for other channels using
    // same ViEEncoder.
    vie_encoder_map_.erase(e_it);

    if (group->Empty()) {
      channel_groups_.remove(group);
    } else {
      group = NULL;  // Prevent group from being deleted.
    }
  }
  delete vie_channel;
  // Leave the write critsect before deleting the objects.
  // Deleting a channel can cause other objects, such as renderers, to be
  // deleted, which might take time.
  // If statment just to show that this object is not always deleted.
  if (vie_encoder) {
    LOG(LS_VERBOSE) << "ViEEncoder deleted for channel " << channel_id;
    delete vie_encoder;
  }
  // If statment just to show that this object is not always deleted.
  if (group) {
    // Delete the group if empty last since the encoder holds a pointer to the
    // BitrateController object that the group owns.
    LOG(LS_VERBOSE) << "Channel group deleted for channel " << channel_id;
    delete group;
  }
  if (transport) {
	  LOG(LS_VERBOSE) << "delete udptransport for channel " << channel_id;
	  DeleteUdptransport(transport);
  }
  LOG(LS_VERBOSE) << "Channel deleted " << channel_id;
  
   if (channel_map_.empty())
  {
	  congestion_controller_.reset();
  }

  return 0;
}

int ViEChannelManager::SetVoiceEngine(VoiceEngine* voice_engine) {
  // Write lock to make sure no one is using the channel.
  ViEManagerWriteScoped wl(this);

  CriticalSectionScoped cs(channel_id_critsect_);

  VoEVideoSync* sync_interface = NULL;
  if (voice_engine) {
    // Get new sync interface.
    sync_interface = VoEVideoSync::GetInterface(voice_engine);
    if (!sync_interface) {
      return -1;
    }
  }

  for (ChannelMap::iterator it = channel_map_.begin(); it != channel_map_.end();
       ++it) {
    it->second->SetVoiceChannel(-1, sync_interface);
  }
  if (voice_sync_interface_) {
    voice_sync_interface_->Release();
  }
  voice_engine_ = voice_engine;
  voice_sync_interface_ = sync_interface;
  return 0;
}

int ViEChannelManager::ConnectVoiceChannel(int channel_id,
                                           int audio_channel_id) {
  CriticalSectionScoped cs(channel_id_critsect_);
  if (!voice_sync_interface_) {
    LOG_F(LS_ERROR) << "No VoE set.";
    return -1;
  }
  ViEChannel* channel = ViEChannelPtr(channel_id);
  if (!channel) {
    return -1;
  }
  return channel->SetVoiceChannel(audio_channel_id, voice_sync_interface_);
}

int ViEChannelManager::DisconnectVoiceChannel(int channel_id) {
  CriticalSectionScoped cs(channel_id_critsect_);
  ViEChannel* channel = ViEChannelPtr(channel_id);
  if (channel) {
    channel->SetVoiceChannel(-1, NULL);
    return 0;
  }
  return -1;
}

VoiceEngine* ViEChannelManager::GetVoiceEngine() {
  CriticalSectionScoped cs(channel_id_critsect_);
  return voice_engine_;
}

bool ViEChannelManager::SetRembStatus(int channel_id, bool sender,
                                      bool receiver) {
  CriticalSectionScoped cs(channel_id_critsect_);
  ChannelGroup* group = FindGroup(channel_id);
  if (!group) {
    return false;
  }
  ViEChannel* channel = ViEChannelPtr(channel_id);
  assert(channel);

  group->SetChannelRembStatus(channel_id, sender, receiver, channel);
  return true;
}

bool ViEChannelManager::SetReservedTransmitBitrate(
    int channel_id, uint32_t reserved_transmit_bitrate_bps) {
  CriticalSectionScoped cs(channel_id_critsect_);
  ChannelGroup* group = FindGroup(channel_id);
  if (!group) {
    return false;
  }

  //BitrateController* bitrate_controller = group->GetBitrateController();
  BitrateController* bitrate_controller = congestion_controller_->GetBitrateController();
  bitrate_controller->SetReservedBitrate(reserved_transmit_bitrate_bps);
  return true;
}

void ViEChannelManager::UpdateSsrcs(int channel_id,
                                    const std::list<unsigned int>& ssrcs) {
  CriticalSectionScoped cs(channel_id_critsect_);
  ChannelGroup* channel_group = FindGroup(channel_id);
  if (channel_group == NULL) {
    return;
  }
  ViEEncoder* encoder = ViEEncoderPtr(channel_id);
  assert(encoder);

  EncoderStateFeedback* encoder_state_feedback =
      channel_group->GetEncoderStateFeedback();
  // Remove a possible previous setting for this encoder before adding the new
  // setting.
  encoder_state_feedback->RemoveEncoder(encoder);
  for (std::list<unsigned int>::const_iterator it = ssrcs.begin();
       it != ssrcs.end(); ++it) {
    encoder_state_feedback->AddEncoder(*it, encoder);
  }
}

bool ViEChannelManager::SetBandwidthEstimationConfig(
    int channel_id, const cloopenwebrtc::Config& config) {
  CriticalSectionScoped cs(channel_id_critsect_);
  ChannelGroup* group = FindGroup(channel_id);
  if (!group) {
    return false;
  }
  group->SetBandwidthEstimationConfig(config);
  return true;
}

bool ViEChannelManager::GetEstimatedSendBandwidth(
    int channel_id, uint32_t* estimated_bandwidth) const {
  CriticalSectionScoped cs(channel_id_critsect_);
  ChannelGroup* group = FindGroup(channel_id);
  if (!group) {
    return false;
  }
  group->GetBitrateController()->AvailableBandwidth(estimated_bandwidth);
  return true;
}

bool ViEChannelManager::GetEstimatedReceiveBandwidth(
    int channel_id, uint32_t* estimated_bandwidth) const {
  CriticalSectionScoped cs(channel_id_critsect_);
  ChannelGroup* group = FindGroup(channel_id);
  if (!group) {
    return false;
  }
  std::vector<unsigned int> ssrcs;
  if (!group->GetRemoteBitrateEstimator()->LatestEstimate(
      &ssrcs, estimated_bandwidth) || ssrcs.empty()) {
    *estimated_bandwidth = 0;
  }
  return true;
}

bool ViEChannelManager::CreateChannelObject(
    int channel_id,
    ViEEncoder* vie_encoder,
    RtcpBandwidthObserver* bandwidth_observer,
    RemoteBitrateEstimator* remote_bitrate_estimator,
    RtcpRttStats* rtcp_rtt_stats,
    RtcpIntraFrameObserver* intra_frame_observer,
    bool sender) {
  PacedSender* paced_sender = congestion_controller_->pacer();

  // Register the channel at the encoder.
  RtpRtcp* send_rtp_rtcp_module = vie_encoder->SendRtpRtcpModule();

  ViEChannel* vie_channel = new ViEChannel(channel_id, engine_id_,
                                           number_of_cores_,
                                           engine_config_,
                                           *module_process_thread_,
                                           intra_frame_observer,
                                           bandwidth_observer,
                                           remote_bitrate_estimator,
                                           rtcp_rtt_stats,
                                           paced_sender,
                                           send_rtp_rtcp_module,
                                           sender,
										   congestion_controller_->GetTransportFeedbackObserver());
  
  if (vie_channel->Init() != 0) {
    delete vie_channel;
    return false;
  }
  VideoCodec encoder;
  if (vie_encoder->GetEncoder(&encoder) != 0) {
    delete vie_channel;
    return false;
  }
  if (sender && vie_channel->SetSendCodec(encoder) != 0) {
    delete vie_channel;
    return false;
  }

  vie_encoder->SendRtpRtcpModule()->RegisterRtpReceiver(vie_channel->GetRtpReceiver());
  vie_channel->SetRtcEventLog(rtc_event_log_.get());
  vie_channel->SetSsrcObserver(vie_encoder);
  // Store the channel, add it to the channel group and save the vie_encoder.
  channel_map_[channel_id] = vie_channel;
  vie_encoder_map_[channel_id] = vie_encoder;
  return true;
}

ViEChannel* ViEChannelManager::ViEChannelPtr(int channel_id) const {
  CriticalSectionScoped cs(channel_id_critsect_);
  ChannelMap::const_iterator it = channel_map_.find(channel_id);
  if (it == channel_map_.end()) {
    LOG(LS_ERROR) << "Channel doesn't exist " << channel_id;
    return NULL;
  }
  return it->second;
}

ViEEncoder* ViEChannelManager::ViEEncoderPtr(int video_channel_id) const {
  CriticalSectionScoped cs(channel_id_critsect_);
  EncoderMap::const_iterator it = vie_encoder_map_.find(video_channel_id);
  if (it == vie_encoder_map_.end()) {
    return NULL;
  }
  return it->second;
}

int ViEChannelManager::FreeChannelId() {
  int idx = 0;
  while (idx < free_channel_ids_size_) {
    if (free_channel_ids_[idx] == true) {
      // We've found a free id, allocate it and return.
      free_channel_ids_[idx] = false;
      return idx + kViEChannelIdBase;
    }
    idx++;
  }
  LOG(LS_ERROR) << "Max number of channels reached.";
  return -1;
}

void ViEChannelManager::ReturnChannelId(int channel_id) {
  CriticalSectionScoped cs(channel_id_critsect_);
  assert(channel_id < kViEMaxNumberOfChannels + kViEChannelIdBase &&
         channel_id >= kViEChannelIdBase);
  free_channel_ids_[channel_id - kViEChannelIdBase] = true;
}

ChannelGroup* ViEChannelManager::FindGroup(int channel_id) const {
  for (ChannelGroups::const_iterator it = channel_groups_.begin();
       it != channel_groups_.end(); ++it) {
    if ((*it)->HasChannel(channel_id)) {
      return *it;
    }
  }
  return NULL;
}

bool ViEChannelManager::ChannelUsingViEEncoder(int channel_id) const {
  CriticalSectionScoped cs(channel_id_critsect_);
  EncoderMap::const_iterator orig_it = vie_encoder_map_.find(channel_id);
  if (orig_it == vie_encoder_map_.end()) {
    // No ViEEncoder for this channel.
    return false;
  }

  // Loop through all other channels to see if anyone points at the same
  // ViEEncoder.
  for (EncoderMap::const_iterator comp_it = vie_encoder_map_.begin();
       comp_it != vie_encoder_map_.end(); ++comp_it) {
    // Make sure we're not comparing the same channel with itself.
    if (comp_it->first != channel_id) {
      if (comp_it->second == orig_it->second) {
        return true;
      }
    }
  }
  return false;
}

void ViEChannelManager::ChannelsUsingViEEncoder(int channel_id,
                                                ChannelList* channels) const {
  CriticalSectionScoped cs(channel_id_critsect_);
  EncoderMap::const_iterator orig_it = vie_encoder_map_.find(channel_id);

  for (ChannelMap::const_iterator c_it = channel_map_.begin();
       c_it != channel_map_.end(); ++c_it) {
    EncoderMap::const_iterator comp_it = vie_encoder_map_.find(c_it->first);
    assert(comp_it != vie_encoder_map_.end());
    if (comp_it->second == orig_it->second) {
      channels->push_back(c_it->second);
    }
  }
}

void ViEChannelManager::OnNetworkChanged(uint32_t bitrate_bps,
										uint8_t fraction_loss,
										int64_t rtt_ms,
										int64_t probing_interval_ms)
{
	//todo : tell vie_encoder, later we can add bitrateAllocator here 
	;
	for (EncoderMap::iterator it=vie_encoder_map_.begin(); it!=vie_encoder_map_.end(); it++)
	{
		ViEEncoder *vie_encoder = it->second;
		vie_encoder->OnNetworkChanged(bitrate_bps, fraction_loss, rtt_ms);
	}
}

void ViEChannelManager::UpdateNetworkState(int channel_id, bool startSend)
{
	//need to fix: multi channel
	if (startSend)
	{
		call_stats_->RegisterStatsObserver(congestion_controller_.get());
		module_process_thread_->RegisterModule(call_stats_.get());
		module_process_thread_->RegisterModule(congestion_controller_.get());
		pacer_thread_->RegisterModule(congestion_controller_->pacer());
		pacer_thread_->RegisterModule(congestion_controller_->GetRemoteBitrateEstimator(true));
		pacer_thread_->Start();
		congestion_controller_->SignalNetworkState(kNetworkUp);
	}
	else {
		//clean up pacer_thread
		pacer_thread_->Stop();
		pacer_thread_->DeRegisterModule(congestion_controller_->pacer());
		pacer_thread_->DeRegisterModule(congestion_controller_->GetRemoteBitrateEstimator(true));
		module_process_thread_->DeRegisterModule(call_stats_.get());
		module_process_thread_->DeRegisterModule(congestion_controller_.get());
		call_stats_->DeregisterStatsObserver(congestion_controller_.get());
		congestion_controller_->SignalNetworkState(kNetworkDown);
	}	
}

UdpTransport *ViEChannelManager::CreateUdptransport(int rtp_port, int rtcp_port, bool ipv6flag) {
	CriticalSectionScoped cs(udptransport_critsect_);

	RtpUdpTransportMap::iterator r_it = rtp_udptransport_map_.find(rtp_port);
	if (r_it == rtp_udptransport_map_.end()) {// No such rtp_port.
		uint8_t num_socket_threads = 1;
		UdpTransport *transport = UdpTransport::Create(ViEModuleId(engine_id_, -1), num_socket_threads);
        if (ipv6flag) {
            transport->EnableIpV6();
        }
		if (transport) {//create socket
			const char* multicast_ip_address = NULL;
			if (transport->InitializeReceiveSockets(NULL, rtp_port,
				NULL,
				multicast_ip_address,
				rtcp_port) != 0) {
				LOG(LS_ERROR) << "can not create socket of rtp_port " << rtp_port;
				UdpTransport::Destroy(transport);
				return NULL;
			}
			rtp_udptransport_map_[rtp_port] = transport;
			transport->AddRefNum();
			return transport;
		}else {
			return NULL;
		}
	}

	//the transport has already int map.
	r_it->second->AddRefNum();
	return r_it->second;
}

int ViEChannelManager::DeleteUdptransport(UdpTransport * transport) {
	CriticalSectionScoped cs(udptransport_critsect_);
	if (!transport)
		return -1;

	transport->SubRefNum();
	if (transport->GetRefNum() == 0) {
		RtpUdpTransportMap::iterator r_it = rtp_udptransport_map_.begin();
		for (; r_it != rtp_udptransport_map_.end(); ++r_it) {
			if (r_it->second == transport) {
				rtp_udptransport_map_.erase(r_it);
				break;
			}
		}
		transport->StopReceiving();
		UdpTransport::Destroy(transport);
	}
	return 0;
}

ViEChannelManagerScoped::ViEChannelManagerScoped(
    const ViEChannelManager& vie_channel_manager)
    : ViEManagerScopedBase(vie_channel_manager) {
}

ViEChannel* ViEChannelManagerScoped::Channel(int vie_channel_id) const {
  return static_cast<const ViEChannelManager*>(vie_manager_)->ViEChannelPtr(
      vie_channel_id);
}
ViEEncoder* ViEChannelManagerScoped::Encoder(int vie_channel_id) const {
  return static_cast<const ViEChannelManager*>(vie_manager_)->ViEEncoderPtr(
      vie_channel_id);
}

bool ViEChannelManagerScoped::ChannelUsingViEEncoder(int channel_id) const {
  return (static_cast<const ViEChannelManager*>(vie_manager_))->
      ChannelUsingViEEncoder(channel_id);
}

void ViEChannelManagerScoped::ChannelsUsingViEEncoder(
    int channel_id, ChannelList* channels) const {
  (static_cast<const ViEChannelManager*>(vie_manager_))->
      ChannelsUsingViEEncoder(channel_id, channels);
}

}  // namespace webrtc
