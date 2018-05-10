/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_MANAGER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_MANAGER_H_

#include <list>
#include <map>

#include "engine_configurations.h"
#include "../system_wrappers/include/scoped_ptr.h"
#include "typedefs.h"
#include "vie_rtp_rtcp.h"
#include "vie_channel_group.h"
#include "vie_defines.h"
#include "vie_manager_base.h"
#include "vie_remb.h"
#include "udp_transport.h"
#include "../module/congestion_controller/include/congestion_controller.h"

namespace cloopenwebrtc {

class Config;
class CriticalSectionWrapper;
class ProcessThread;
class RtcpRttStats;
class ViEChannel;
class ViEEncoder;
class VoEVideoSync;
class VoiceEngine;
class RtcEventLog;

typedef std::list<ChannelGroup*> ChannelGroups;
typedef std::list<ViEChannel*> ChannelList;
typedef std::map<int, ViEChannel*> ChannelMap;
typedef std::map<int, ViEEncoder*> EncoderMap;
    
#ifndef WEBRTC_EXTERNAL_TRANSPORT
typedef std::map<int, UdpTransport*> RtpUdpTransportMap;
#else
typedef std::map<int, TcpTransport*> RtpTcpTransportMap;
#endif
    
    
class ViEChannelManager: private ViEManagerBase,
    public CongestionController::Observer {
  friend class ViEChannelManagerScoped;
 public:
  ViEChannelManager(int engine_id,
                    int number_of_cores,
                    const Config& config);
  ~ViEChannelManager();

  void SetModuleProcessThread(ProcessThread* module_process_thread, ProcessThread* module_process_thread_pacer);

  // Creates a new channel. 'channel_id' will be the id of the created channel.
  int CreateChannel(int* channel_id,
                    const Config* config);

  int AddSsrcToEncoder(int channelid);

  // Creates a new channel grouped with |original_channel|. The new channel
  // will get its own |ViEEncoder| if |sender| is set to true. It will be a
  // receive only channel, without an own |ViEEncoder| if |sender| is false.
  int CreateChannel(int* channel_id, int original_channel, bool sender);

  // Deletes a channel.
  int DeleteChannel(int channel_id);

  // Set the voice engine instance to be used by all video channels.
  int SetVoiceEngine(VoiceEngine* voice_engine);

  // Enables lip sync of the channel.
  int ConnectVoiceChannel(int channel_id, int audio_channel_id);

  // Disables lip sync of the channel.
  int DisconnectVoiceChannel(int channel_id);

  VoiceEngine* GetVoiceEngine();

  // Adds a channel to include when sending REMB.
  bool SetRembStatus(int channel_id, bool sender, bool receiver);

  bool SetReservedTransmitBitrate(int channel_id,
                                  uint32_t reserved_transmit_bitrate_bps);

  // Updates the SSRCs for a channel. If one of the SSRCs already is registered,
  // it will simply be ignored and no error is returned.
  void UpdateSsrcs(int channel_id, const std::list<unsigned int>& ssrcs);

  // Sets bandwidth estimation related configurations.
  bool SetBandwidthEstimationConfig(int channel_id,
                                    const cloopenwebrtc::Config& config);

  bool GetEstimatedSendBandwidth(int channel_id,
                                 uint32_t* estimated_bandwidth) const;
  bool GetEstimatedReceiveBandwidth(int channel_id,
                                    uint32_t* estimated_bandwidth) const;

#ifndef WEBRTC_EXTERNAL_TRANSPORT
  UdpTransport *CreateUdptransport(int rtp_port, int rtcp_port, bool ipv6flag = false);
  int DeleteUdptransport(int rtp_port);
#else
  TcpTransport *CreateTcptransport(int rtp_port, int rtcp_port, bool ipv6flag = false);
  int DeleteTcptransport(int rtp_port);
#endif
    
  // Implements CongestionController::Observer.
  void OnNetworkChanged(uint32_t bitrate_bps,
					  uint8_t fraction_loss,
					  int64_t rtt_ms,
					  int64_t probing_interval_ms);

  void UpdateNetworkState(int channel_id, bool startSend);

  //add by dingxf
  int AddRemoteI420FrameCallback(const int video_channel, ECMedia_I420FrameCallBack callback);

        
  void UpdateGCCBitrateConfig(int min_bitrate_bps,
                              int start_bitrate_bps,
                              int max_bitrate_bps);


 private:
  // Creates a channel object connected to |vie_encoder|. Assumed to be called
  // protected.
  bool CreateChannelObject(int channel_id,
                           ViEEncoder* vie_encoder,
                           RtcpBandwidthObserver* bandwidth_observer,
                           RemoteBitrateEstimator* remote_bitrate_estimator,
                           RtcpRttStats* rtcp_rtt_stats,
                           RtcpIntraFrameObserver* intra_frame_observer,
                           bool sender);

  // Used by ViEChannelScoped, forcing a manager user to use scoped.
  // Returns a pointer to the channel with id 'channel_id'.
  ViEChannel* ViEChannelPtr(int channel_id) const;

  // Methods used by ViECaptureScoped and ViEEncoderScoped.
  // Gets the ViEEncoder used as input for video_channel_id
  ViEEncoder* ViEEncoderPtr(int video_channel_id) const;

  // Returns a free channel id, -1 if failing.
  int FreeChannelId();

  // Returns a previously allocated channel id.
  void ReturnChannelId(int channel_id);

  // Returns the iterator to the ChannelGroup containing |channel_id|.
  ChannelGroup* FindGroup(int channel_id) const;

  // Returns true if at least one other channels uses the same ViEEncoder as
  // channel_id.
  bool ChannelUsingViEEncoder(int channel_id) const;
  void ChannelsUsingViEEncoder(int channel_id, ChannelList* channels) const;

  // Protects channel_map_ and free_channel_ids_.
  CriticalSectionWrapper* channel_id_critsect_;
  int engine_id_;
  int number_of_cores_;

  // TODO(mflodman) Make part of channel group.
  ChannelMap channel_map_;
  bool* free_channel_ids_;
  int free_channel_ids_size_;

  // List with all channel groups.
  std::list<ChannelGroup*> channel_groups_;

  // TODO(mflodman) Make part of channel group.
  // Maps Channel id -> ViEEncoder.
  EncoderMap vie_encoder_map_;
  VoEVideoSync* voice_sync_interface_;

  VoiceEngine* voice_engine_;
  ProcessThread* module_process_thread_;
    ProcessThread* module_process_thread_pacer_;
  const Config& engine_config_;

#ifndef WEBRTC_EXTERNAL_TRANSPORT
  // Protects udpstransport_map_.
  // CriticalSectionWrapper* udptransport_critsect_;
  // RtpUdpTransportMap rtp_udptransport_map_;
#else
  // Protects tcpstransport_map_.
  CriticalSectionWrapper* tcptransport_critsect_;
  //RtpTcpTransportMap rtp_tcptransport_map_;
#endif

  // Protects udpstransport_map_.
  CriticalSectionWrapper* udptransport_critsect_;
  RtpUdpTransportMap rtp_udptransport_map_;

  std::unique_ptr<RtcEventLog> rtc_event_log_;
  std::unique_ptr<CongestionController> congestion_controller_;
  ProcessThread* pacer_thread_;
  bool use_sendside_bwe_;
  VieRemb remb_;
  std::unique_ptr<CallStats> call_stats_;
  PacketRouter packet_router_;
  int refcount_;
//  std::multimap<int, bool> network_stat_;
};

class ViEChannelManagerScoped: private ViEManagerScopedBase {
 public:
  explicit ViEChannelManagerScoped(
      const ViEChannelManager& vie_channel_manager);
  ViEChannel* Channel(int vie_channel_id) const;
  ViEEncoder* Encoder(int vie_channel_id) const;

  // Returns true if at least one other channels uses the same ViEEncoder as
  // channel_id.
  bool ChannelUsingViEEncoder(int channel_id) const;

  // Returns a list with pointers to all channels using the same encoder as the
  // channel with |channel_id|, including the one with the specified id.
  void ChannelsUsingViEEncoder(int channel_id, ChannelList* channels) const;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_MANAGER_H_
