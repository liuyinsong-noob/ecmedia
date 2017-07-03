/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_channel.h"

#include <algorithm>
#include <vector>

#include "../system_wrappers/include/common.h"
#include "../common_video/source/libyuv/include/webrtc_libyuv.h"
#include "experiments.h"
#include "frame_callback.h"
#include "../pacing/paced_sender.h"
#include "rtp_receiver.h"
#include "rtp_rtcp.h"
#include "process_thread.h"
#include "video_coding.h"
#include "video_processing.h"
#include "video_render_defines.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "logging.h"
#include "../system_wrappers/include/metrics.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include "receive_statistics_proxy.h"
#include "call_stats.h"
#include "vie_codec.h"
#include "vie_errors.h"
#include "vie_image_process.h"
#include "vie_rtp_rtcp.h"
#include "report_block_stats.h"
#include "vie_defines.h"

#include <time.h>

#include "rtp.h"
#include "../system_wrappers/include/trace.h"

#ifdef _WIN32
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define snprintf _snprintf
#endif

namespace cloopenwebrtc {

#ifdef WIN32
static const ResolutionInst g_resolution_table[] = {
	{R_128_96_15,   128,  96,   300},
	{R_160_120_15,  160,  120,  300},
	{R_176_144_15,  176,  144,  300},
	{R_320_240_15,  320,  240,  500},
	{R_352_288_15,  352,  288,  500},
	{R_480_360_15,  480,  360,  600},
	{R_640_360_15,  640,  360,  700},
	{R_640_480_15,  640,  480,  700},
	{R_640_480_30,  640,  480,  700},
	{R_848_480_15,  848,  480,  800},
	{R_848_480_30,  848,  480,  800},
	{R_1280_720_15, 1280, 720,  1000},
	{R_1280_720_30, 1280, 720,  1300},
	{R_1920_1080_15,1920, 1080, 1500},
	{R_1920_1080_30,1920, 1080, 1800},
	{R_2048_1080_30,2048, 1080, 2000}
};
#else
static const ResolutionInst g_resolution_table[] = {
	{ R_128_96_15,   96,   128,  300 },
	{ R_160_120_15,  120,  160,  300 },
	{ R_176_144_15,  144,  176,  300 },
	{ R_320_240_15,  240,  320,  500 },
	{ R_352_288_15,  288,  352,  500 },
	{ R_480_360_15,  360,  480,  600 },
	{ R_640_360_15,  360,  640,  700 },
	{ R_640_480_15,  480,  640,  700 },
	{ R_640_480_30,  480,  640,  700 },
	{ R_848_480_15,  480,  848,  800 },
	{ R_848_480_30,  480,  848,  800 },
	{ R_1280_720_15, 720,  1280, 1000 },
	{ R_1280_720_30, 720,  1280, 1300 },
	{ R_1920_1080_15,1080, 1920, 1500 },
	{ R_1920_1080_30,1080, 1920, 1800 },
	{ R_2048_1080_30,1080, 2048, 2000 }
};
#endif

const int kMaxDecodeWaitTimeMs = 50;
const int kInvalidRtpExtensionId = 0;
static const int kMaxTargetDelayMs = 10000;
static const float kMaxIncompleteTimeMultiplier = 3.5f;

// Helper class receiving statistics callbacks.
class ChannelStatsObserver : public CallStatsObserver {
 public:
  explicit ChannelStatsObserver(ViEChannel* owner) : owner_(owner) {}
  virtual ~ChannelStatsObserver() {}

  // Implements StatsObserver.
  virtual void OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms) {
    owner_->OnRttUpdate(avg_rtt_ms);
  }

 private:
  ViEChannel* owner_;
};

ViEChannel::ViEChannel(int32_t channel_id,
                       int32_t engine_id,
                       uint32_t number_of_cores,
                       const Config& config,
                       ProcessThread& module_process_thread,
                       RtcpIntraFrameObserver* intra_frame_observer,
                       RtcpBandwidthObserver* bandwidth_observer,
                       RemoteBitrateEstimator* remote_bitrate_estimator,
                       RtcpRttStats* rtt_stats,
                       PacedSender* paced_sender,
                       RtpRtcp* default_rtp_rtcp,
                       bool sender)
    : ViEFrameProviderBase(channel_id, engine_id),
      channel_id_(channel_id),
      engine_id_(engine_id),
      number_of_cores_(number_of_cores),
      num_socket_threads_(kViESocketThreads),
      callback_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      rtp_rtcp_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      default_rtp_rtcp_(default_rtp_rtcp),
      vcm_(VideoCodingModule::Create()),
      vie_receiver_(channel_id, vcm_, remote_bitrate_estimator, this),
      vie_sender_(channel_id),
      vie_sync_(vcm_, this),
      stats_observer_(new ChannelStatsObserver(this)),
      vcm_receive_stats_callback_(NULL),
	  //receive_stats_proxy_callback_(NULL),
      module_process_thread_(module_process_thread),
      codec_observer_(NULL),
      do_key_frame_callbackRequest_(false),
      rtp_observer_(NULL),
      intra_frame_observer_(intra_frame_observer),
      rtt_stats_(rtt_stats),
      paced_sender_(paced_sender),
      bandwidth_observer_(bandwidth_observer),
      send_timestamp_extension_id_(kInvalidRtpExtensionId),
      absolute_send_time_extension_id_(kInvalidRtpExtensionId),
      external_transport_(NULL),
      decoder_reset_(true),
      wait_for_key_frame_(false),
      decode_thread_(NULL),
      effect_filter_(NULL),
      color_enhancement_(true),//sean for test only video
      mtu_(0),
      sender_(sender),
      nack_history_size_sender_(kSendSidePacketHistorySize),
      max_nack_reordering_threshold_(kMaxPacketAgeToNack),
      pre_render_callback_(NULL),
      report_block_stats_sender_(new ReportBlockStats()),
      report_block_stats_receiver_(new ReportBlockStats()),
	  //    sean add begin 0915
	  _shield_mosaic(false),
	  //    sean add end 0915
	  //sean add begin 20140705 video conference
	  _videoConferencePacketReceived(false),
	  _isVideoConference(false),
	  //sean add end 20140705 video conference
	  _startNetworkTime(0),
	  _recvDataTotalSim(0),
	  _recvDataTotalWifi(0),
	  _isWifi(false),
	  critsect_net_statistic(CriticalSectionWrapper::CreateCriticalSection()),
	  _conferenceNo(NULL),
	  _conferPassword(NULL),
	  _sipNo(NULL),
	  _selfSipNo(NULL),
	  _confPort(0),
	  _confIP(NULL),	  
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	  ssrc_all_num_(0),
	  local_ssrc_main_(0),
	  local_ssrc_slave_(0),
	  remote_ssrc_(0),
	  socket_transport_(NULL),
#endif
	 isSVCChannel_(true),
#ifdef WEBRTC_SRTP
	_srtpModule(*SrtpModule::CreateSrtpModule(ViEModuleId(engine_id, channel_id))),
#endif
	_encrypting(false),
	_decrypting(false),
	external_encryption_(NULL),
	file_recorder_(channel_id),
    _video_conf_cb(NULL),
    _video_data_cb(NULL),
    _stun_cb(NULL),
    _key_frame_cb(NULL){
  RtpRtcp::Configuration configuration = CreateRtpRtcpConfiguration();
  configuration.remote_bitrate_estimator = remote_bitrate_estimator;
  configuration.receive_statistics = vie_receiver_.GetReceiveStatistics();
  rtp_rtcp_.reset(RtpRtcp::CreateRtpRtcp(configuration));
  vie_receiver_.SetRtpRtcpModule(rtp_rtcp_.get());
  vcm_->SetNackSettings(kMaxNackListSize, max_nack_reordering_threshold_, 0);

  receive_statistics_proxy_.reset(new ReceiveStatisticsProxy(channel_id));
  RegisterPreRenderCallback(receive_statistics_proxy_.get());
  RegisterSendRtcpPacketTypeCountObserver(receive_statistics_proxy_.get());
  RegisterCodecObserver(receive_statistics_proxy_.get());
  RegisterReceiveChannelRtpStatisticsCallback(receive_statistics_proxy_.get());
  RegisterReceiveChannelRtcpStatisticsCallback(receive_statistics_proxy_.get());
}

int32_t ViEChannel::Init() {
  if (module_process_thread_.RegisterModule(
      vie_receiver_.GetReceiveStatistics()) != 0) {
    return -1;
  }
  // RTP/RTCP initialization.
  rtp_rtcp_->SetSendingMediaStatus(false);
  if (module_process_thread_.RegisterModule(rtp_rtcp_.get()) != 0) {
    return -1;
  }
  rtp_rtcp_->SetKeyFrameRequestMethod(kKeyFrameReqFirRtcp);
  rtp_rtcp_->SetRTCPStatus(kCompound);
  if (paced_sender_) {
    rtp_rtcp_->SetStorePacketsStatus(true, nack_history_size_sender_);
  }
  if (vcm_->InitializeReceiver() != 0) {
    return -1;
  }
  if (vcm_->SetVideoProtection(kProtectionKeyOnLoss, true)) {
	  return -1;
  }

  if (vcm_->RegisterReceiveCallback(this) != 0) {
    return -1;
  }
  vcm_->RegisterFrameTypeCallback(this); //请求关键帧回调
  vcm_->RegisterReceiveStatisticsCallback(this); //接收端统计信息回调
  vcm_->RegisterDecoderTimingCallback(this);
  vcm_->SetRenderDelay(kViEDefaultRenderDelayMs);
  
  if (module_process_thread_.RegisterModule(vcm_) != 0) {
    return -1;
  }
#ifdef VIDEOCODEC_VP8
  VideoCodec video_codec;
  if (vcm_->Codec(kVideoCodecVP8, &video_codec) == VCM_OK) {
    rtp_rtcp_->RegisterSendPayload(video_codec);
    // TODO(holmer): Can we call SetReceiveCodec() here instead?
    if (!vie_receiver_.RegisterPayload(video_codec)) {
      return -1;
    }
    vcm_->RegisterReceiveCodec(&video_codec, number_of_cores_);
    vcm_->RegisterSendCodec(&video_codec, number_of_cores_,
                           rtp_rtcp_->MaxRtpPacketSize());
  } else {
    assert(false);
  }
#elif VIDEOCODEC_H264
    VideoCodec video_codec;
    if (vcm_->Codec(kVideoCodecH264, &video_codec) == VCM_OK) {
        rtp_rtcp_->RegisterSendPayload(video_codec);
        // TODO(holmer): Can we call SetReceiveCodec() here instead?
        if (!vie_receiver_.RegisterPayload(video_codec)) {
            return -1;
        }
        vcm_->RegisterReceiveCodec(&video_codec, number_of_cores_);
        vcm_->RegisterSendCodec(&video_codec, number_of_cores_,
                                rtp_rtcp_->MaxDataPayloadLength());
    } else {
        assert(false);
    }
#endif

  return 0;
}

ViEChannel::~ViEChannel() {
  UpdateHistograms();
  // Make sure we don't get more callbacks from the RTP module.
#ifndef WEBRTC_EXTERNAL_TRANSPORT
  if (local_ssrc_main_ != 0)
  socket_transport_->SubRecieveChannel(local_ssrc_main_);
  if (remote_ssrc_ != 0)
	  socket_transport_->SubRecieveChannel(remote_ssrc_);
#endif
  module_process_thread_.DeRegisterModule(vie_receiver_.GetReceiveStatistics());
  module_process_thread_.DeRegisterModule(rtp_rtcp_.get());
  module_process_thread_.DeRegisterModule(vcm_);
  module_process_thread_.DeRegisterModule(&vie_sync_);
  module_process_thread_.DeRegisterModule(receive_statistics_proxy_.get());
    
  while (simulcast_rtp_rtcp_.size() > 0) {
    std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
    RtpRtcp* rtp_rtcp = *it;
    module_process_thread_.DeRegisterModule(rtp_rtcp);
    delete rtp_rtcp;
    simulcast_rtp_rtcp_.erase(it);
  }
  while (removed_rtp_rtcp_.size() > 0) {
    std::list<RtpRtcp*>::iterator it = removed_rtp_rtcp_.begin();
    delete *it;
    removed_rtp_rtcp_.erase(it);
  }
  if (decode_thread_) {
    StopDecodeThread();
  }
#ifdef WEBRTC_SRTP
  SrtpModule::DestroySrtpModule(&_srtpModule);
#endif
  if(_isVideoConference) {
	  if(_sipNo) {
		  delete[] _sipNo;
		  _sipNo = NULL;
	  }
	  if(_conferenceNo) {
		  delete[] _conferenceNo;
		  _conferenceNo = NULL;
	  }
	  if(_conferPassword) {
		  delete[] _conferPassword;
		  _conferPassword = NULL;
	  }
	  if(_selfSipNo) {
		  delete[] _selfSipNo;
		  _selfSipNo = NULL;
	  }
	  if(_confIP) {
		  delete[] _confIP;
		  _confIP = NULL;
	  }
  }
  // Release modules.

  VideoCodingModule::Destroy(vcm_);
}

int32_t ViEChannel::SetUdpTransport(UdpTransport *transport)
{
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	socket_transport_ = transport;

	if (socket_transport_->GetLocalSSrc() != 0){//receive channel
		if (SetSSRC(socket_transport_->GetLocalSSrc(), kViEStreamTypeNormal, 0) != 0) {
			LOG(LS_ERROR) << "set receive channel local ssrc failed";
			return -1;
		}
	}
#endif
	return 0;
}

UdpTransport *ViEChannel::GetUdpTransport()
{
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	return socket_transport_;
#else
	return NULL;
#endif
}

void ViEChannel::UpdateHistograms() {
  int64_t now = Clock::GetRealTimeClock()->TimeInMilliseconds();
  RtcpPacketTypeCounter rtcp_sent;
  RtcpPacketTypeCounter rtcp_received;
  GetRtcpPacketTypeCounters(&rtcp_sent, &rtcp_received);

  if (sender_) {
    int64_t elapsed_sec = rtcp_received.TimeSinceFirstPacketInMs(now) / 1000;
    if (elapsed_sec > metrics::kMinRunTimeInSeconds) {
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.NackPacketsReceivedPerMinute",
          rtcp_received.nack_packets / elapsed_sec / 60);
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.FirPacketsReceivedPerMinute",
          rtcp_received.fir_packets / elapsed_sec / 60);
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.PliPacketsReceivedPerMinute",
          rtcp_received.pli_packets / elapsed_sec / 60);
      if (rtcp_received.nack_requests > 0) {
        RTC_HISTOGRAM_PERCENTAGE(
            "WebRTC.Video.UniqueNackRequestsReceivedInPercent",
                rtcp_received.UniqueNackRequestsInPercent());
      }
      RTC_HISTOGRAM_PERCENTAGE("WebRTC.Video.SentPacketsLostInPercent",
          report_block_stats_sender_->FractionLostInPercent());
    }
  } else if (vie_receiver_.GetRemoteSsrc() > 0) {
    // Get receive stats if we are receiving packets, i.e. there is a remote
    // ssrc.
    int64_t elapsed_sec = rtcp_sent.TimeSinceFirstPacketInMs(now) / 1000;
    if (elapsed_sec > metrics::kMinRunTimeInSeconds) {
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.NackPacketsSentPerMinute",
          rtcp_sent.nack_packets / elapsed_sec / 60);
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.FirPacketsSentPerMinute",
          rtcp_sent.fir_packets / elapsed_sec / 60);
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.PliPacketsSentPerMinute",
          rtcp_sent.pli_packets / elapsed_sec / 60);
      if (rtcp_sent.nack_requests > 0) {
        RTC_HISTOGRAM_PERCENTAGE("WebRTC.Video.UniqueNackRequestsSentInPercent",
            rtcp_sent.UniqueNackRequestsInPercent());
      }
      RTC_HISTOGRAM_PERCENTAGE("WebRTC.Video.ReceivedPacketsLostInPercent",
          report_block_stats_receiver_->FractionLostInPercent());
    }

    StreamDataCounters rtp;
    StreamDataCounters rtx;
    GetReceiveStreamDataCounters(&rtp, &rtx);
    StreamDataCounters rtp_rtx = rtp;
    rtp_rtx.Add(rtx);
    elapsed_sec = rtp_rtx.TimeSinceFirstPacketInMs(now) / 1000;
    if (elapsed_sec > metrics::kMinRunTimeInSeconds) {
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.BitrateReceivedInKbps",
          rtp_rtx.transmitted.TotalBytes() * 8 / elapsed_sec / 1000);
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.MediaBitrateReceivedInKbps",
          rtp.MediaPayloadBytes() * 8 / elapsed_sec / 1000);
      RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.PaddingBitrateReceivedInKbps",
          rtp_rtx.transmitted.padding_bytes * 8 / elapsed_sec / 1000);
      RTC_HISTOGRAM_COUNTS_10000(
          "WebRTC.Video.RetransmittedBitrateReceivedInKbps",
              rtp_rtx.retransmitted.TotalBytes() * 8 / elapsed_sec / 1000);
      uint32_t ssrc = 0;
      if (vie_receiver_.GetRtxSsrc(&ssrc)) {
        RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.RtxBitrateReceivedInKbps",
            rtx.transmitted.TotalBytes() * 8 / elapsed_sec / 1000);
      }
    }
  }
}

void ViEChannel::UpdateHistogramsAtStopSend() {
  StreamDataCounters rtp;
  StreamDataCounters rtx;
  GetSendStreamDataCounters(&rtp, &rtx);
  StreamDataCounters rtp_rtx = rtp;
  rtp_rtx.Add(rtx);

  int64_t elapsed_sec = rtp_rtx.TimeSinceFirstPacketInMs(
      Clock::GetRealTimeClock()->TimeInMilliseconds()) / 1000;
  if (elapsed_sec < metrics::kMinRunTimeInSeconds) {
    return;
  }
  RTC_HISTOGRAM_COUNTS_100000("WebRTC.Video.BitrateSentInKbps",
      rtp_rtx.transmitted.TotalBytes() * 8 / elapsed_sec / 1000);
  RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.MediaBitrateSentInKbps",
      rtp.MediaPayloadBytes() * 8 / elapsed_sec / 1000);
  RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.PaddingBitrateSentInKbps",
      rtp_rtx.transmitted.padding_bytes * 8 / elapsed_sec / 1000);
  RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.RetransmittedBitrateSentInKbps",
      rtp_rtx.retransmitted.TotalBytes() * 8 / elapsed_sec / 1000);
  uint32_t ssrc = 0;
  if (vie_receiver_.GetRtxSsrc(&ssrc)) {
    RTC_HISTOGRAM_COUNTS_10000("WebRTC.Video.RtxBitrateSentInKbps",
        rtx.transmitted.TotalBytes() * 8 / elapsed_sec / 1000);
  }
}

int32_t ViEChannel::SetSendCodec(const VideoCodec& video_codec,
                                 bool new_stream) {
  if (!sender_) {
    return 0;
  }
  if (video_codec.codecType == kVideoCodecRED ||
      video_codec.codecType == kVideoCodecULPFEC) {
    LOG_F(LS_ERROR) << "Not a valid send codec " << video_codec.codecType;
    return -1;
  }
  if (kMaxSimulcastStreams < video_codec.numberOfSimulcastStreams) {
    LOG_F(LS_ERROR) << "Incorrect config "
                    << video_codec.numberOfSimulcastStreams;
    return -1;
  }
  // Update the RTP module with the settings.
  // Stop and Start the RTP module -> trigger new SSRC, if an SSRC hasn't been
  // set explicitly.
  bool restart_rtp = false;
  if (rtp_rtcp_->Sending() && new_stream) {
    restart_rtp = true;
    rtp_rtcp_->SetSendingStatus(false);
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end(); ++it) {
      (*it)->SetSendingStatus(false);
      (*it)->SetSendingMediaStatus(false);
    }
  }

  bool fec_enabled = false;
  uint8_t payload_type_red;
  uint8_t payload_type_fec;
  //rtp_rtcp_->GenericFECStatus(fec_enabled, payload_type_red, payload_type_fec);

  CriticalSectionScoped cs(rtp_rtcp_cs_.get());

  if (video_codec.numberOfSimulcastStreams > 0) {
    // Set correct bitrate to base layer.
    // Create our simulcast RTP modules.
    int num_modules_to_add = video_codec.numberOfSimulcastStreams -
        simulcast_rtp_rtcp_.size() - 1;
    if (num_modules_to_add < 0) {
      num_modules_to_add = 0;
    }

    // Add back removed rtp modules. Order is important (allocate from front of
    // removed modules) to preserve RTP settings such as SSRCs for simulcast
    // streams.
    std::list<RtpRtcp*> new_rtp_modules;
    for (; removed_rtp_rtcp_.size() > 0 && num_modules_to_add > 0;
         --num_modules_to_add) {
      new_rtp_modules.push_back(removed_rtp_rtcp_.front());
      removed_rtp_rtcp_.pop_front();
    }

    for (int i = 0; i < num_modules_to_add; ++i)
      new_rtp_modules.push_back(CreateRtpRtcpModule());

    // Initialize newly added modules.
    for (std::list<RtpRtcp*>::iterator it = new_rtp_modules.begin();
         it != new_rtp_modules.end();
         ++it) {
      RtpRtcp* rtp_rtcp = *it;

      rtp_rtcp->SetRTCPStatus(rtp_rtcp_->RTCP());

      if (rtp_rtcp_->StorePackets()) {
        rtp_rtcp->SetStorePacketsStatus(true, nack_history_size_sender_);
      } else if (paced_sender_) {
        rtp_rtcp->SetStorePacketsStatus(true, nack_history_size_sender_);
      }

      if (fec_enabled) {
        //rtp_rtcp->SetGenericFECStatus(
        //    fec_enabled, payload_type_red, payload_type_fec);
      }
      rtp_rtcp->SetSendingStatus(rtp_rtcp_->Sending());
      rtp_rtcp->SetSendingMediaStatus(rtp_rtcp_->SendingMedia());
      rtp_rtcp->SetRtxSendStatus(rtp_rtcp_->RtxSendStatus());
      simulcast_rtp_rtcp_.push_back(rtp_rtcp);

      // Silently ignore error.
      module_process_thread_.RegisterModule(rtp_rtcp);
    }

    // Remove last in list if we have too many.
    for (int j = simulcast_rtp_rtcp_.size();
         j > (video_codec.numberOfSimulcastStreams - 1);
         j--) {
      RtpRtcp* rtp_rtcp = simulcast_rtp_rtcp_.back();
      module_process_thread_.DeRegisterModule(rtp_rtcp);
      rtp_rtcp->SetSendingStatus(false);
      rtp_rtcp->SetSendingMediaStatus(false);
      rtp_rtcp->RegisterRtcpStatisticsCallback(NULL);
      rtp_rtcp->RegisterSendChannelRtpStatisticsCallback(NULL);
      simulcast_rtp_rtcp_.pop_back();
      removed_rtp_rtcp_.push_front(rtp_rtcp);
    }
    uint8_t idx = 0;
    // Configure all simulcast modules.
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end();
         it++) {
      idx++;
      RtpRtcp* rtp_rtcp = *it;
      rtp_rtcp->DeRegisterSendPayload(video_codec.plType);
      if (rtp_rtcp->RegisterSendPayload(video_codec) != 0) {
        return -1;
      }
      if (mtu_ != 0) {
        rtp_rtcp->SetMaxTransferUnit(mtu_);
      }
      if (restart_rtp) {
        rtp_rtcp->SetSendingStatus(true);
        rtp_rtcp->SetSendingMediaStatus(true);
      }
      if (send_timestamp_extension_id_ != kInvalidRtpExtensionId) {
        // Deregister in case the extension was previously enabled.
        rtp_rtcp->DeregisterSendRtpHeaderExtension(
            kRtpExtensionTransmissionTimeOffset);
        if (rtp_rtcp->RegisterSendRtpHeaderExtension(
            kRtpExtensionTransmissionTimeOffset,
            send_timestamp_extension_id_) != 0) {
        }
      } else {
        rtp_rtcp->DeregisterSendRtpHeaderExtension(
            kRtpExtensionTransmissionTimeOffset);
      }
      if (absolute_send_time_extension_id_ != kInvalidRtpExtensionId) {
        // Deregister in case the extension was previously enabled.
        rtp_rtcp->DeregisterSendRtpHeaderExtension(
            kRtpExtensionAbsoluteSendTime);
        if (rtp_rtcp->RegisterSendRtpHeaderExtension(
            kRtpExtensionAbsoluteSendTime,
            absolute_send_time_extension_id_) != 0) {
        }
      } else {
        rtp_rtcp->DeregisterSendRtpHeaderExtension(
            kRtpExtensionAbsoluteSendTime);
      }
      rtp_rtcp->RegisterRtcpStatisticsCallback(
          rtp_rtcp_->GetRtcpStatisticsCallback());
      rtp_rtcp->RegisterSendChannelRtpStatisticsCallback(
          rtp_rtcp_->GetSendChannelRtpStatisticsCallback());
    }
    // |RegisterSimulcastRtpRtcpModules| resets all old weak pointers and old
    // modules can be deleted after this step.
    vie_receiver_.RegisterSimulcastRtpRtcpModules(simulcast_rtp_rtcp_);
  } else {
    while (!simulcast_rtp_rtcp_.empty()) {
      RtpRtcp* rtp_rtcp = simulcast_rtp_rtcp_.back();
      module_process_thread_.DeRegisterModule(rtp_rtcp);
      rtp_rtcp->SetSendingStatus(false);
      rtp_rtcp->SetSendingMediaStatus(false);
      rtp_rtcp->RegisterRtcpStatisticsCallback(NULL);
      rtp_rtcp->RegisterSendChannelRtpStatisticsCallback(NULL);
      simulcast_rtp_rtcp_.pop_back();
      removed_rtp_rtcp_.push_front(rtp_rtcp);
    }
    // Clear any previous modules.

	//add by ylr ---begin
	if (send_timestamp_extension_id_ != kInvalidRtpExtensionId) {
		// Deregister in case the extension was previously enabled.
		rtp_rtcp_->DeregisterSendRtpHeaderExtension(
			kRtpExtensionTransmissionTimeOffset);
		if (rtp_rtcp_->RegisterSendRtpHeaderExtension(
			kRtpExtensionTransmissionTimeOffset,
			send_timestamp_extension_id_) != 0) {
		}
	} else {
		rtp_rtcp_->DeregisterSendRtpHeaderExtension(
			kRtpExtensionTransmissionTimeOffset);
	}
	if (absolute_send_time_extension_id_ != kInvalidRtpExtensionId) {
		// Deregister in case the extension was previously enabled.
		rtp_rtcp_->DeregisterSendRtpHeaderExtension(
			kRtpExtensionAbsoluteSendTime);
		if (rtp_rtcp_->RegisterSendRtpHeaderExtension(
			kRtpExtensionAbsoluteSendTime,
			absolute_send_time_extension_id_) != 0) {
		}
	} else {
		rtp_rtcp_->DeregisterSendRtpHeaderExtension(
			kRtpExtensionAbsoluteSendTime);
	}

	//add by ylr ---end 
	
    vie_receiver_.RegisterSimulcastRtpRtcpModules(simulcast_rtp_rtcp_);
  }

  // Don't log this error, no way to check in advance if this pl_type is
  // registered or not...
  rtp_rtcp_->DeRegisterSendPayload(video_codec.plType);
  if (rtp_rtcp_->RegisterSendPayload(video_codec) != 0) {
    return -1;
  }
  if (restart_rtp) {
    rtp_rtcp_->SetSendingStatus(true);
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end(); ++it) {
      (*it)->SetSendingStatus(true);
      (*it)->SetSendingMediaStatus(true);
    }
  }
  return 0;
}

int32_t ViEChannel::SetReceiveCodec(const VideoCodec& video_codec) {
  if (!vie_receiver_.SetReceiveCodec(video_codec)) {
    return -1;
  }

  if (video_codec.codecType != kVideoCodecRED &&
      video_codec.codecType != kVideoCodecULPFEC) {
    // Register codec type with VCM, but do not register RED or ULPFEC.
    if (vcm_->RegisterReceiveCodec(&video_codec, number_of_cores_,
                                  wait_for_key_frame_) != VCM_OK) {
      return -1;
    }
  }
  return 0;
}

int32_t ViEChannel::GetReceiveCodec(VideoCodec* video_codec) {
  if (vcm_->ReceiveCodec(video_codec) != 0) {
    return -1;
  }
  return 0;
}

int32_t ViEChannel::RegisterCodecObserver(ViEDecoderObserver* observer) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (observer) {
    if (codec_observer_) {
      LOG_F(LS_ERROR) << "Observer already registered.";
      return -1;
    }
    codec_observer_ = observer;
  } else {
    codec_observer_ = NULL;
  }
  return 0;
}

int32_t ViEChannel::RegisterExternalDecoder(const uint8_t pl_type,
                                            VideoDecoder* decoder,
                                            bool buffered_rendering,
                                            int32_t render_delay) {
  int32_t result;
  result = vcm_->RegisterExternalDecoder(decoder, pl_type, buffered_rendering);
  if (result != VCM_OK) {
    return result;
  }
  return vcm_->SetRenderDelay(render_delay);
}

int32_t ViEChannel::DeRegisterExternalDecoder(const uint8_t pl_type) {
  VideoCodec current_receive_codec;
  int32_t result = 0;
  result = vcm_->ReceiveCodec(&current_receive_codec);
  if (vcm_->RegisterExternalDecoder(NULL, pl_type, false) != VCM_OK) {
    return -1;
  }

  if (result == 0 && current_receive_codec.plType == pl_type) {
    result = vcm_->RegisterReceiveCodec(
        &current_receive_codec, number_of_cores_, wait_for_key_frame_);
  }
  return result;
}

int32_t ViEChannel::ReceiveCodecStatistics(uint32_t* num_key_frames,
                                           uint32_t* num_delta_frames) {
  CriticalSectionScoped cs(callback_cs_.get());
  *num_key_frames = receive_frame_counts_.key_frames;
  *num_delta_frames = receive_frame_counts_.delta_frames;
  return 0;
}

uint32_t ViEChannel::DiscardedPackets() const {
  return vcm_->DiscardedPackets();
}

int ViEChannel::ReceiveDelay() const {
  return vcm_->Delay();
}

int32_t ViEChannel::WaitForKeyFrame(bool wait) {
  wait_for_key_frame_ = wait;
  return 0;
}

int32_t ViEChannel::SetSignalPacketLossStatus(bool enable,
                                              bool only_key_frames) {
  if (enable) {
    if (only_key_frames) {
      vcm_->SetVideoProtection(kProtectionKeyOnLoss, false);
      if (vcm_->SetVideoProtection(kProtectionKeyOnKeyLoss, true) != VCM_OK) {
        return -1;
      }
    } else {
      vcm_->SetVideoProtection(kProtectionKeyOnKeyLoss, false);
      if (vcm_->SetVideoProtection(kProtectionKeyOnLoss, true) != VCM_OK) {
        return -1;
      }
    }
  } else {
    vcm_->SetVideoProtection(kProtectionKeyOnLoss, false);
    vcm_->SetVideoProtection(kProtectionKeyOnKeyLoss, false);
  }
  return 0;
}

void ViEChannel::SetRTCPMode(const RtcpMode rtcp_mode) {
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       it++) {
    RtpRtcp* rtp_rtcp = *it;
    rtp_rtcp->SetRTCPStatus(rtcp_mode);
  }
  rtp_rtcp_->SetRTCPStatus(rtcp_mode);
}

RtcpMode ViEChannel::GetRTCPMode() const {
  return rtp_rtcp_->RTCP();
}

int32_t ViEChannel::SetNACKStatus(const bool enable) {
  // Update the decoding VCM.
  if (vcm_->SetVideoProtection(kProtectionNack, enable) != VCM_OK) {
    return -1;
  }
  if (enable) {
    // Disable possible FEC.
    SetFECStatus(false, 0, 0);
  }
  // Update the decoding VCM.
  if (vcm_->SetVideoProtection(kProtectionNack, enable) != VCM_OK) {
    return -1;
  }
  return ProcessNACKRequest(enable);
}

int32_t ViEChannel::ProcessNACKRequest(const bool enable) {
  if (enable) {
    // Turn on NACK.
    if (rtp_rtcp_->RTCP() == kOff) {
      return -1;
    }
    vie_receiver_.SetNackStatus(true, max_nack_reordering_threshold_);
    rtp_rtcp_->SetStorePacketsStatus(true, nack_history_size_sender_);
    vcm_->RegisterPacketRequestCallback(this);

    CriticalSectionScoped cs(rtp_rtcp_cs_.get());

    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end();
         it++) {
      RtpRtcp* rtp_rtcp = *it;
      rtp_rtcp->SetStorePacketsStatus(true, nack_history_size_sender_);
    }
    // Don't introduce errors when NACK is enabled.
    vcm_->SetDecodeErrorMode(kNoErrors); 
	
  } else {
    CriticalSectionScoped cs(rtp_rtcp_cs_.get());
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end();
         it++) {
      RtpRtcp* rtp_rtcp = *it;
      if (paced_sender_ == NULL) {
        rtp_rtcp->SetStorePacketsStatus(false, 0);
      }
    }
    vcm_->RegisterPacketRequestCallback(NULL);
    if (paced_sender_ == NULL) {
      rtp_rtcp_->SetStorePacketsStatus(false, 0);
    }
    vie_receiver_.SetNackStatus(false, max_nack_reordering_threshold_);
    // When NACK is off, allow decoding with errors. Otherwise, the video
    // will freeze, and will only recover with a complete key frame.
    vcm_->SetDecodeErrorMode(kWithErrors);
  }
  return 0;
}

int32_t ViEChannel::SetFECStatus(const bool enable,
                                       const unsigned char payload_typeRED,
                                       const unsigned char payload_typeFEC) {
  // Disable possible NACK.
  if (enable) {
    SetNACKStatus(false);
  }

  return ProcessFECRequest(enable, payload_typeRED, payload_typeFEC);
}

int32_t ViEChannel::ProcessFECRequest(
    const bool enable,
    const unsigned char payload_typeRED,
    const unsigned char payload_typeFEC) {
#if 0
  if (rtp_rtcp_->SetGenericFECStatus(enable, payload_typeRED,
                                    payload_typeFEC) != 0) {
    return -1;
  }
#endif
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       it++) {
    RtpRtcp* rtp_rtcp = *it;
    //rtp_rtcp->SetGenericFECStatus(enable, payload_typeRED, payload_typeFEC);
  }
  return 0;
}

int32_t ViEChannel::SetHybridNACKFECStatus(
    const bool enable,
    const unsigned char payload_typeRED,
    const unsigned char payload_typeFEC) {
  if (vcm_->SetVideoProtection(kProtectionNackFEC, enable) != VCM_OK) {
    return -1;
  }

  int32_t ret_val = 0;
  ret_val = ProcessNACKRequest(enable);
  if (ret_val < 0) {
    return ret_val;
  }
  return ProcessFECRequest(enable, payload_typeRED, payload_typeFEC);
}

int ViEChannel::SetSenderBufferingMode(int target_delay_ms) {
  if ((target_delay_ms < 0) || (target_delay_ms > kMaxTargetDelayMs)) {
    LOG(LS_ERROR) << "Invalid send buffer value.";
    return -1;
  }
  if (target_delay_ms == 0) {
    // Real-time mode.
    nack_history_size_sender_ = kSendSidePacketHistorySize;
  } else {
    nack_history_size_sender_ = GetRequiredNackListSize(target_delay_ms);
    // Don't allow a number lower than the default value.
    if (nack_history_size_sender_ < kSendSidePacketHistorySize) {
      nack_history_size_sender_ = kSendSidePacketHistorySize;
    }
  }
  rtp_rtcp_->SetStorePacketsStatus(true, nack_history_size_sender_);
  return 0;
}

int ViEChannel::SetReceiverBufferingMode(int target_delay_ms) {
  if ((target_delay_ms < 0) || (target_delay_ms > kMaxTargetDelayMs)) {
    LOG(LS_ERROR) << "Invalid receive buffer delay value.";
    return -1;
  }
  int max_nack_list_size;
  int max_incomplete_time_ms;
  if (target_delay_ms == 0) {
    // Real-time mode - restore default settings.
    max_nack_reordering_threshold_ = kMaxPacketAgeToNack;
    max_nack_list_size = kMaxNackListSize;
    max_incomplete_time_ms = 0;
  } else {
    max_nack_list_size =  3 * GetRequiredNackListSize(target_delay_ms) / 4;
    max_nack_reordering_threshold_ = max_nack_list_size;
    // Calculate the max incomplete time and round to int.
    max_incomplete_time_ms = static_cast<int>(kMaxIncompleteTimeMultiplier *
        target_delay_ms + 0.5f);
  }
  vcm_->SetNackSettings(max_nack_list_size, max_nack_reordering_threshold_,
                       max_incomplete_time_ms);
  vcm_->SetMinReceiverDelay(target_delay_ms);
  if (vie_sync_.SetTargetBufferingDelay(target_delay_ms) < 0)
    return -1;
  return 0;
}

int ViEChannel::GetRequiredNackListSize(int target_delay_ms) {
  // The max size of the nack list should be large enough to accommodate the
  // the number of packets (frames) resulting from the increased delay.
  // Roughly estimating for ~40 packets per frame @ 30fps.
  return target_delay_ms * 40 * 30 / 1000;
}

int32_t ViEChannel::SetKeyFrameRequestMethod(
    const KeyFrameRequestMethod method) {
  return rtp_rtcp_->SetKeyFrameRequestMethod(method);
}

void ViEChannel::EnableRemb(bool enable) {
  rtp_rtcp_->SetREMBStatus(enable);
}

int ViEChannel::SetSendTimestampOffsetStatus(bool enable, int id) {
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  int error = 0;
  if (enable) {
    // Enable the extension, but disable possible old id to avoid errors.
    send_timestamp_extension_id_ = id;
    rtp_rtcp_->DeregisterSendRtpHeaderExtension(
        kRtpExtensionTransmissionTimeOffset);
    error = rtp_rtcp_->RegisterSendRtpHeaderExtension(
        kRtpExtensionTransmissionTimeOffset, id);
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end(); it++) {
      (*it)->DeregisterSendRtpHeaderExtension(
          kRtpExtensionTransmissionTimeOffset);
      error |= (*it)->RegisterSendRtpHeaderExtension(
          kRtpExtensionTransmissionTimeOffset, id);
    }
  } else {
    // Disable the extension.
    send_timestamp_extension_id_ = kInvalidRtpExtensionId;
    rtp_rtcp_->DeregisterSendRtpHeaderExtension(
        kRtpExtensionTransmissionTimeOffset);
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end(); it++) {
      (*it)->DeregisterSendRtpHeaderExtension(
          kRtpExtensionTransmissionTimeOffset);
    }
  }
  return error;
}

int ViEChannel::SetReceiveTimestampOffsetStatus(bool enable, int id) {
  return vie_receiver_.SetReceiveTimestampOffsetStatus(enable, id) ? 0 : -1;
}

int ViEChannel::SetSendAbsoluteSendTimeStatus(bool enable, int id) {
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  int error = 0;
  if (enable) {
    // Enable the extension, but disable possible old id to avoid errors.
    absolute_send_time_extension_id_ = id;
    rtp_rtcp_->DeregisterSendRtpHeaderExtension(
        kRtpExtensionAbsoluteSendTime);
    error = rtp_rtcp_->RegisterSendRtpHeaderExtension(
        kRtpExtensionAbsoluteSendTime, id);
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end(); it++) {
      (*it)->DeregisterSendRtpHeaderExtension(
          kRtpExtensionAbsoluteSendTime);
      error |= (*it)->RegisterSendRtpHeaderExtension(
          kRtpExtensionAbsoluteSendTime, id);
    }
  } else {
    // Disable the extension.
    absolute_send_time_extension_id_ = kInvalidRtpExtensionId;
    rtp_rtcp_->DeregisterSendRtpHeaderExtension(
        kRtpExtensionAbsoluteSendTime);
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end(); it++) {
      (*it)->DeregisterSendRtpHeaderExtension(
          kRtpExtensionAbsoluteSendTime);
    }
  }
  return error;
}

int ViEChannel::SetReceiveAbsoluteSendTimeStatus(bool enable, int id) {
  return vie_receiver_.SetReceiveAbsoluteSendTimeStatus(enable, id) ? 0 : -1;
}

void ViEChannel::SetRtcpXrRrtrStatus(bool enable) {
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  rtp_rtcp_->SetRtcpXrRrtrStatus(enable);
}

void ViEChannel::SetTransmissionSmoothingStatus(bool enable) {
  assert(paced_sender_ && "No paced sender registered.");
  //paced_sender_->SetStatus(enable);
}

void ViEChannel::EnableTMMBR(bool enable) {
  rtp_rtcp_->SetTMMBRStatus(enable);
}

int32_t ViEChannel::EnableKeyFrameRequestCallback(const bool enable) {

  CriticalSectionScoped cs(callback_cs_.get());
  if (enable && !codec_observer_) {
    LOG(LS_ERROR) << "No ViECodecObserver set.";
    return -1;
  }
  do_key_frame_callbackRequest_ = enable;
  return 0;
}

int32_t ViEChannel::GetResolution(ResolutionInst &info) {
	int i;
	int g_resolution_table_size = sizeof(g_resolution_table) / sizeof(g_resolution_table[0]);

	for (i = 0; i < g_resolution_table_size; i++) {
		if (g_resolution_table[i].index == info.index)
			break;
	}

	if (i == g_resolution_table_size) {
		LOG(LS_ERROR) << "No the resolution " << info.index;
		return -1;
	}

	memcpy(&info, &g_resolution_table[i], sizeof(ResolutionInst));
	return 0;
}

//judge whether trunk or svc through SSRC(0, trunk; other, svc)
int32_t ViEChannel::SetLocalSendSSRC(const uint32_t SSRC, const StreamType usage) {

	//trunk video/content
	if (SSRC == 0) {
		isSVCChannel_ = false;

		int idx = 0;
		GetLocalSSRC(idx, &local_ssrc_main_);
		socket_transport_->AddRecieveChannel(local_ssrc_main_, this);
		return 0;
	}

	//svc video/content
	int ssrc_num = 2;
	uint32_t ssrc_slave = 0;
	uint32_t resolution_index = SSRC & 0x0F;
	if (resolution_index <= R_176_144_15) {
		ssrc_num = 1;
	}else if (resolution_index <= R_480_360_15){
		ssrc_slave = (SSRC & 0xFFFFFFF0) | R_128_96_15;
	}else if (resolution_index <= R_848_480_30) {
		ssrc_slave = (SSRC & 0xFFFFFFF0) | R_160_120_15;
	}else {
		ssrc_slave = (SSRC & 0xFFFFFFF0) | R_176_144_15;
	}

	if (ssrc_num == 1){//only one ssrc(resolution)
		if (SetSSRC(SSRC, usage, 0) != 0) {
			LOG(LS_ERROR) << "set local only one ssrc failed";
			return -1;
		}
		local_ssrc_main_ = SSRC;//only one resolution
		ssrc_all_num_ = 1;

	}else if (ssrc_num == 2) {
		if (SetSSRC(ssrc_slave, usage, 0) != 0) {
			LOG(LS_ERROR) << "set local ssrc slave failed";
			return -1;
		}
		local_ssrc_slave_ = ssrc_slave;//small resolution

		if (SetSSRC(SSRC, usage, 1) != 0) {
			LOG(LS_ERROR) << "set local ssrc main failed";
			return -1;
		}
		local_ssrc_main_ = SSRC;//big resolution
		ssrc_all_num_ = 2;
	}

	socket_transport_->SetSVCVideoFlag();//this is a svc video/content channel
	socket_transport_->SetLocalSSrc(local_ssrc_main_);//other receive channel need local ssrc
	socket_transport_->AddRecieveChannel(local_ssrc_main_, this);//receive local rtcp
	
	return 0;
}

int32_t ViEChannel::SetSSRC(const uint32_t SSRC,
                            const StreamType usage,
                            const uint8_t simulcast_idx) {
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  ReserveRtpRtcpModules(simulcast_idx + 1);
  RtpRtcp* rtp_rtcp = GetRtpRtcpModule(simulcast_idx);
  if (rtp_rtcp == NULL)
    return -1;
  if (usage == kViEStreamTypeRtx) {
    rtp_rtcp->SetRtxSsrc(SSRC);
  } else {
    rtp_rtcp->SetSSRC(SSRC);
  }

  return 0;
}

//maybe change remote ssrc but not calling "CancelRemoteSSRC()" and "StopReceive()"
int32_t ViEChannel::RequestRemoteSSRC(const uint32_t SSRC) {
	if (!isSVCChannel_)
		return 0;

	if (0 == SSRC) {
		LOG(LS_WARNING) << "request ssrc is 0";
		return 0;
	}

	uint32_t bandwidth = 1;
	int ret = 0;// rtp_rtcp_->SendSingleTMMBR(bandwidth, socket_transport_->GetLocalSSrc(), SSRC);
	if (ret != 0) {
		LOG(LS_ERROR) << "SendSingleTMMBR request remote failed";
		return -1;
	}

	if (remote_ssrc_ > 0) {//change remote_ssrc_
		socket_transport_->SubRecieveChannel(remote_ssrc_);
	}
	remote_ssrc_ = SSRC;
	socket_transport_->AddRecieveChannel(remote_ssrc_, this);
	return 0;
}

int32_t ViEChannel::CancelRemoteSSRC() {
	if (!isSVCChannel_)
		return 0;

	int ret = 0;// rtp_rtcp_->SendSingleTMMBR(0, socket_transport_->GetLocalSSrc(), remote_ssrc_);
	if (ret != 0) {
		LOG(LS_ERROR) << "SendSingleTMMBR cancel remote failed";
		return -1;
	}
	socket_transport_->SubRecieveChannel(remote_ssrc_);
	return 0;
}

int32_t ViEChannel::SetRemoteSSRCType(const StreamType usage,
                                      const uint32_t SSRC) {
  vie_receiver_.SetRtxSsrc(SSRC);
  return 0;
}

int32_t ViEChannel::GetLocalSSRC(uint8_t idx, unsigned int* ssrc) {
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  RtpRtcp* rtp_rtcp = GetRtpRtcpModule(idx);
  if (rtp_rtcp == NULL)
    return -1;
  *ssrc = rtp_rtcp->SSRC();
  return 0;
}

int32_t ViEChannel::GetRemoteSSRC(uint32_t* ssrc) {
  *ssrc = vie_receiver_.GetRemoteSsrc();
  return 0;
}

int32_t ViEChannel::GetRemoteCSRC(uint32_t CSRCs[kRtpCsrcSize]) {
  uint32_t arrayCSRC[kRtpCsrcSize];
  memset(arrayCSRC, 0, sizeof(arrayCSRC));

  int num_csrcs = vie_receiver_.GetCsrcs(arrayCSRC);
  if (num_csrcs > 0) {
    memcpy(CSRCs, arrayCSRC, num_csrcs * sizeof(uint32_t));
  }
  return 0;
}

int ViEChannel::SetRtxSendPayloadType(int payload_type) {
 // rtp_rtcp_->SetRtxSendPayloadType(payload_type);
  for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end(); it++) {
    //(*it)->SetRtxSendPayloadType(payload_type);
  }
  SetRtxSendStatus(true);
  return 0;
}

void ViEChannel::SetRtxSendStatus(bool enable) {
  int rtx_settings =
      enable ? kRtxRetransmitted | kRtxRedundantPayloads : kRtxOff;
  rtp_rtcp_->SetRtxSendStatus(rtx_settings);
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end(); it++) {
    (*it)->SetRtxSendStatus(rtx_settings);
  }
}

void ViEChannel::SetRtxReceivePayloadType(int payload_type) {
  vie_receiver_.SetRtxPayloadType(payload_type);
}

int32_t ViEChannel::SetStartSequenceNumber(uint16_t sequence_number) {
  if (rtp_rtcp_->Sending()) {
    return -1;
  }
  rtp_rtcp_->SetSequenceNumber(sequence_number);
  return 0;
}

void ViEChannel::SetRtpStateForSsrc(uint32_t ssrc, const RtpState& rtp_state) {
  assert(!rtp_rtcp_->Sending());
  //default_rtp_rtcp_->SetRtpStateForSsrc(ssrc, rtp_state);
}

RtpState ViEChannel::GetRtpStateForSsrc(uint32_t ssrc) {
  assert(!rtp_rtcp_->Sending());

  RtpState rtp_state;
#if 0
  if (!default_rtp_rtcp_->GetRtpStateForSsrc(ssrc, &rtp_state)) {
    LOG(LS_ERROR) << "Couldn't get RTP state for ssrc: " << ssrc;
  }
#endif
  return rtp_state;
}

int32_t ViEChannel::SetRTCPCName(const char rtcp_cname[]) {
  if (rtp_rtcp_->Sending()) {
    return -1;
  }
  return rtp_rtcp_->SetCNAME(rtcp_cname);
}

int32_t ViEChannel::GetRemoteRTCPCName(char rtcp_cname[]) {
  uint32_t remoteSSRC = vie_receiver_.GetRemoteSsrc();
  return rtp_rtcp_->RemoteCNAME(remoteSSRC, rtcp_cname);
}

int32_t ViEChannel::RegisterRtpObserver(ViERTPObserver* observer) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (observer) {
    if (rtp_observer_) {
      LOG_F(LS_ERROR) << "Observer already registered.";
      return -1;
    }
    rtp_observer_ = observer;
  } else {
    rtp_observer_ = NULL;
  }
  return 0;
}

int32_t ViEChannel::SendApplicationDefinedRTCPPacket(
    const uint8_t sub_type,
    uint32_t name,
    const uint8_t* data,
    uint16_t data_length_in_bytes) {
  if (!rtp_rtcp_->Sending()) {
    return -1;
  }
  if (!data) {
    LOG_F(LS_ERROR) << "Invalid input.";
    return -1;
  }
  if (data_length_in_bytes % 4 != 0) {
    LOG(LS_ERROR) << "Invalid input length.";
    return -1;
  }
  RtcpMode rtcp_method = rtp_rtcp_->RTCP();
  if (rtcp_method == kOff) {
    LOG_F(LS_ERROR) << "RTCP not enable.";
    return -1;
  }
  // Create and send packet.
  if (rtp_rtcp_->SetRTCPApplicationSpecificData(sub_type, name, data,
                                               data_length_in_bytes) != 0) {
    return -1;
  }
  return 0;
}

int32_t ViEChannel::GetSendRtcpStatistics(uint16_t* fraction_lost,
                                          uint32_t* cumulative_lost,
                                          uint32_t* extended_max,
                                          uint32_t* jitter_samples,
                                          int64_t* rtt_ms) {
  // Aggregate the report blocks associated with streams sent on this channel.
  std::vector<RTCPReportBlock> report_blocks;
  rtp_rtcp_->RemoteRTCPStat(&report_blocks);
  {
    CriticalSectionScoped lock(rtp_rtcp_cs_.get());
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
        it != simulcast_rtp_rtcp_.end();
        ++it) {
      (*it)->RemoteRTCPStat(&report_blocks);
    }
  }

  if (report_blocks.empty())
    return -1;

  uint32_t remote_ssrc = vie_receiver_.GetRemoteSsrc();
  std::vector<RTCPReportBlock>::const_iterator it = report_blocks.begin();
  for (; it != report_blocks.end(); ++it) {
    if (it->remoteSSRC == remote_ssrc)
      break;
  }
  if (it == report_blocks.end()) {
    // We have not received packets with an SSRC matching the report blocks. To
    // have a chance of calculating an RTT we will try with the SSRC of the
    // first report block received.
    // This is very important for send-only channels where we don't know the
    // SSRC of the other end.
    remote_ssrc = report_blocks[0].remoteSSRC;
  }

  // TODO(asapersson): Change report_block_stats to not rely on
  // GetSendRtcpStatistics to be called.
  RTCPReportBlock report =
      report_block_stats_sender_->AggregateAndStore(report_blocks);
  *fraction_lost = report.fractionLost;
  *cumulative_lost = report.cumulativeLost;
  *extended_max = report.extendedHighSeqNum;
  *jitter_samples = report.jitter;

  int64_t dummy;
  int64_t rtt = 0;
  if (rtp_rtcp_->RTT(remote_ssrc, &rtt, &dummy, &dummy, &dummy) != 0) {
    return -1;
  }
  *rtt_ms = rtt;
  return 0;
}

void ViEChannel::RegisterSendChannelRtcpStatisticsCallback(
    RtcpStatisticsCallback* callback) {
  rtp_rtcp_->RegisterRtcpStatisticsCallback(callback);
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       ++it) {
    (*it)->RegisterRtcpStatisticsCallback(callback);
  }
}

// TODO(holmer): This is a bad function name as it implies that it returns the
// received RTCP, while it actually returns the statistics which will be sent
// in the RTCP.
int32_t ViEChannel::GetReceivedRtcpStatistics(uint16_t* fraction_lost,
                                              uint32_t* cumulative_lost,
                                              uint32_t* extended_max,
                                              uint32_t* jitter_samples,
                                              int64_t* rtt_ms) {
  uint32_t remote_ssrc = vie_receiver_.GetRemoteSsrc();
  StreamStatistician* statistician =
      vie_receiver_.GetReceiveStatistics()->GetStatistician(remote_ssrc);
  RtcpStatistics receive_stats;
  if (!statistician || !statistician->GetStatistics(
      &receive_stats, rtp_rtcp_->RTCP() == kOff)) {
    return -1;
  }
  *fraction_lost = receive_stats.fraction_lost;
  *cumulative_lost = receive_stats.cumulative_lost;
  *extended_max = receive_stats.extended_max_sequence_number;
  *jitter_samples = receive_stats.jitter;

  // TODO(asapersson): Change report_block_stats to not rely on
  // GetReceivedRtcpStatistics to be called.
  report_block_stats_receiver_->Store(receive_stats, remote_ssrc, 0);

  int64_t dummy = 0;
  int64_t rtt = 0;
  rtp_rtcp_->RTT(remote_ssrc, &rtt, &dummy, &dummy, &dummy);
  *rtt_ms = rtt;
  return 0;
}

void ViEChannel::RegisterReceiveChannelRtcpStatisticsCallback(
    RtcpStatisticsCallback* callback) {
  vie_receiver_.GetReceiveStatistics()->RegisterRtcpStatisticsCallback(
      callback);
  rtp_rtcp_->RegisterRtcpStatisticsCallback(callback);
}

int32_t ViEChannel::GetRtpStatistics(size_t* bytes_sent,
                                     uint32_t* packets_sent,
                                     size_t* bytes_received,
                                     uint32_t* packets_received) const {
  StreamStatistician* statistician = vie_receiver_.GetReceiveStatistics()->
      GetStatistician(vie_receiver_.GetRemoteSsrc());
  *bytes_received = 0;
  *packets_received = 0;
  if (statistician)
    statistician->GetDataCounters(bytes_received, packets_received);
  if (rtp_rtcp_->DataCountersRTP(bytes_sent, packets_sent) != 0) {
    return -1;
  }
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       it++) {
    size_t bytes_sent_temp = 0;
    uint32_t packets_sent_temp = 0;
    RtpRtcp* rtp_rtcp = *it;
    rtp_rtcp->DataCountersRTP(&bytes_sent_temp, &packets_sent_temp);
    *bytes_sent += bytes_sent_temp;
    *packets_sent += packets_sent_temp;
  }
  for (std::list<RtpRtcp*>::const_iterator it = removed_rtp_rtcp_.begin();
       it != removed_rtp_rtcp_.end(); ++it) {
    size_t bytes_sent_temp = 0;
    uint32_t packets_sent_temp = 0;
    RtpRtcp* rtp_rtcp = *it;
    rtp_rtcp->DataCountersRTP(&bytes_sent_temp, &packets_sent_temp);
    *bytes_sent += bytes_sent_temp;
    *packets_sent += packets_sent_temp;
  }
  return 0;
}

void ViEChannel::GetSendStreamDataCounters(
    StreamDataCounters* rtp_counters,
    StreamDataCounters* rtx_counters) const {
  rtp_rtcp_->GetSendStreamDataCounters(rtp_counters, rtx_counters);
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       it++) {
    StreamDataCounters rtp_data;
    StreamDataCounters rtx_data;
    (*it)->GetSendStreamDataCounters(&rtp_data, &rtx_data);
    rtp_counters->Add(rtp_data);
    rtx_counters->Add(rtx_data);
  }
  for (std::list<RtpRtcp*>::const_iterator it = removed_rtp_rtcp_.begin();
       it != removed_rtp_rtcp_.end(); ++it) {
    StreamDataCounters rtp_data;
    StreamDataCounters rtx_data;
    (*it)->GetSendStreamDataCounters(&rtp_data, &rtx_data);
    rtp_counters->Add(rtp_data);
    rtx_counters->Add(rtx_data);
  }
}

void ViEChannel::GetReceiveStreamDataCounters(
    StreamDataCounters* rtp_counters,
    StreamDataCounters* rtx_counters) const {
  StreamStatistician* statistician = vie_receiver_.GetReceiveStatistics()->
      GetStatistician(vie_receiver_.GetRemoteSsrc());
  if (statistician) {
    statistician->GetReceiveStreamDataCounters(rtp_counters);
  }
  uint32_t rtx_ssrc = 0;
  if (vie_receiver_.GetRtxSsrc(&rtx_ssrc)) {
    StreamStatistician* statistician =
        vie_receiver_.GetReceiveStatistics()->GetStatistician(rtx_ssrc);
    if (statistician) {
      statistician->GetReceiveStreamDataCounters(rtx_counters);
    }
  }
}

void ViEChannel::RegisterSendChannelRtpStatisticsCallback(
      StreamDataCountersCallback* callback) {
  rtp_rtcp_->RegisterSendChannelRtpStatisticsCallback(callback);
  {
    CriticalSectionScoped cs(rtp_rtcp_cs_.get());
    for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
         it != simulcast_rtp_rtcp_.end();
         it++) {
      (*it)->RegisterSendChannelRtpStatisticsCallback(callback);
    }
  }
}

void ViEChannel::RegisterReceiveChannelRtpStatisticsCallback(
    StreamDataCountersCallback* callback) {
  vie_receiver_.GetReceiveStatistics()->RegisterRtpStatisticsCallback(callback);
}

void ViEChannel::GetRtcpPacketTypeCounters(
    RtcpPacketTypeCounter* packets_sent,
    RtcpPacketTypeCounter* packets_received) const {
  rtp_rtcp_->GetRtcpPacketTypeCounters(packets_sent, packets_received);

  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end(); ++it) {
    RtcpPacketTypeCounter sent;
    RtcpPacketTypeCounter received;
    (*it)->GetRtcpPacketTypeCounters(&sent, &received);
    packets_sent->Add(sent);
    packets_received->Add(received);
  }
  for (std::list<RtpRtcp*>::const_iterator it = removed_rtp_rtcp_.begin();
       it != removed_rtp_rtcp_.end(); ++it) {
    RtcpPacketTypeCounter sent;
    RtcpPacketTypeCounter received;
    (*it)->GetRtcpPacketTypeCounters(&sent, &received);
    packets_sent->Add(sent);
    packets_received->Add(received);
  }
}

void ViEChannel::GetBandwidthUsage(uint32_t* total_bitrate_sent,
                                   uint32_t* video_bitrate_sent,
                                   uint32_t* fec_bitrate_sent,
                                   uint32_t* nackBitrateSent) const {
  rtp_rtcp_->BitrateSent(total_bitrate_sent, video_bitrate_sent,
                         fec_bitrate_sent, nackBitrateSent);
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end(); it++) {
    uint32_t stream_rate = 0;
    uint32_t video_rate = 0;
    uint32_t fec_rate = 0;
    uint32_t nackRate = 0;
    RtpRtcp* rtp_rtcp = *it;
    rtp_rtcp->BitrateSent(&stream_rate, &video_rate, &fec_rate, &nackRate);
    *total_bitrate_sent += stream_rate;
    *video_bitrate_sent += video_rate;
    *fec_bitrate_sent += fec_rate;
    *nackBitrateSent += nackRate;
  }
}

bool ViEChannel::GetSendSideDelay(int* avg_send_delay,
                                  int* max_send_delay) const {
  *avg_send_delay = 0;
  *max_send_delay = 0;
  bool valid_estimate = false;
  int num_send_delays = 0;
#if 0
  if (rtp_rtcp_->GetSendSideDelay(avg_send_delay, max_send_delay)) {
    ++num_send_delays;
    valid_estimate = true;
  }
#endif
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end(); it++) {
    RtpRtcp* rtp_rtcp = *it;
    int sub_stream_avg_delay = 0;
    int sub_stream_max_delay = 0;
#if 0
    if (rtp_rtcp->GetSendSideDelay(&sub_stream_avg_delay,
                                   &sub_stream_max_delay)) {
      *avg_send_delay += sub_stream_avg_delay;
      *max_send_delay = std::max(*max_send_delay, sub_stream_max_delay);
      ++num_send_delays;
    }
#endif
  }
  if (num_send_delays > 0) {
    valid_estimate = true;
    *avg_send_delay = *avg_send_delay / num_send_delays;
    *avg_send_delay = (*avg_send_delay + num_send_delays / 2) / num_send_delays;
  }
  return valid_estimate;
}

void ViEChannel::RegisterSendSideDelayObserver(
    SendSideDelayObserver* observer) {
  send_side_delay_observer_.Set(observer);
}

void ViEChannel::RegisterSendBitrateObserver(
    BitrateStatisticsObserver* observer) {
  send_bitrate_observer_.Set(observer);
}

void ViEChannel::GetReceiveBandwidthEstimatorStats(
    ReceiveBandwidthEstimatorStats* output) const {
  vie_receiver_.GetReceiveBandwidthEstimatorStats(output);
}

int32_t ViEChannel::StartRTPDump(const char file_nameUTF8[1024],
                                 RTPDirections direction) {
  if (direction == kRtpIncoming) {
    return vie_receiver_.StartRTPDump(file_nameUTF8);
  } else {
    return vie_sender_.StartRTPDump(file_nameUTF8);
  }
}

int32_t ViEChannel::StopRTPDump(RTPDirections direction) {
  if (direction == kRtpIncoming) {
    return vie_receiver_.StopRTPDump();
  } else {
    return vie_sender_.StopRTPDump();
  }
}

//int32_t ViEChannel::StartSend() {
//  CriticalSectionScoped cs(callback_cs_.get());
//  if (!external_transport_) {
//    LOG(LS_ERROR) << "No transport set.";
//    return -1;
//  }
//  rtp_rtcp_->SetSendingMediaStatus(true);
//
//  if (rtp_rtcp_->Sending()) {
//    return kViEBaseAlreadySending;
//  }
//  if (rtp_rtcp_->SetSendingStatus(true) != 0) {
//    return -1;
//  }
//  CriticalSectionScoped cs_rtp(rtp_rtcp_cs_.get());
//  for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
//       it != simulcast_rtp_rtcp_.end();
//       it++) {
//    RtpRtcp* rtp_rtcp = *it;
//    rtp_rtcp->SetSendingMediaStatus(true);
//    rtp_rtcp->SetSendingStatus(true);
//  }
//  return 0;
//}

int32_t ViEChannel::StopSend() {
  UpdateHistogramsAtStopSend();
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  rtp_rtcp_->SetSendingMediaStatus(false);
  for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       it++) {
    RtpRtcp* rtp_rtcp = *it;
    rtp_rtcp->SetSendingMediaStatus(false);
  }
  if (!rtp_rtcp_->Sending()) {
    return kViEBaseNotSending;
  }

  // Reset.
  rtp_rtcp_->ResetSendDataCountersRTP();
  if (rtp_rtcp_->SetSendingStatus(false) != 0) {
    return -1;
  }
  for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       it++) {
    RtpRtcp* rtp_rtcp = *it;
    rtp_rtcp->ResetSendDataCountersRTP();
    rtp_rtcp->SetSendingStatus(false);
  }
  return 0;
}

bool ViEChannel::Sending() {
  return rtp_rtcp_->Sending();
}

int32_t ViEChannel::StartReceive() {

	if (!external_transport_) {
		if (!socket_transport_->Receiving()) {

			if (socket_transport_->ReceiveSocketsInitialized() == false) {
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
					"%s: receive sockets not initialized", __FUNCTION__);
				return -1;
			}
			if (socket_transport_->StartReceiving(kViENumReceiveSocketBuffers) != 0) {
				int32_t socket_error = socket_transport_->LastError();
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
					"%s: could not get receive socket information. Socket error:%d",
					__FUNCTION__, socket_error);
				return -1;
			}
		}
	}

  if (StartDecodeThread() != 0) {

	  if (!isSVCChannel_)//trunk, one channel ---- one udp transport
			socket_transport_->StopReceiving();

    vie_receiver_.StopReceive();
    return -1;
  }

  vie_receiver_.StartReceive();
  module_process_thread_.RegisterModule(receive_statistics_proxy_.get());

  if (remote_ssrc_ != 0)//svc channel, start receive remote video/content
	  socket_transport_->AddRecieveChannel(remote_ssrc_, this);

  return 0;
}

//must not stop socket_transport_ when svc
int32_t ViEChannel::StopReceive() {
	if (remote_ssrc_ != 0)//svc channel, stop receive remote video/content
		socket_transport_->SubRecieveChannel(remote_ssrc_);

	vie_receiver_.StopReceive();
	StopDecodeThread();
	vcm_->ResetDecoder();

	if (!isSVCChannel_) {//trunk, one channel ---- one udp transport
		if (socket_transport_->Receiving() == false) {
			// Warning, don't return error
			WEBRTC_TRACE(kTraceWarning, kTraceVideo,
				ViEId(engine_id_, channel_id_), "%s: not receiving",
				__FUNCTION__);
			return 0;
		}
		if (socket_transport_->StopReceiving() != 0) {
			int32_t socket_error = socket_transport_->LastError();
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: Socket error: %d", __FUNCTION__, socket_error);
			return -1;
		}
	}

	return 0;
}

//rtmp
int32_t ViEChannel::RegisterSendTransport(Transport* transport) {

#ifndef WEBRTC_EXTERNAL_TRANSPORT
	if (!socket_transport_) {
		socket_transport_ = UdpTransport::Create(ViEModuleId(engine_id_, channel_id_), num_socket_threads_);
		if (!socket_transport_) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s:  create socket_transport_ failed", __FUNCTION__);
			return -1;
		}
	}

	if (socket_transport_->SendSocketsInitialized() ||
		socket_transport_->ReceiveSocketsInitialized()) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s:  socket transport already initialized", __FUNCTION__);
			return -1;
	}
#endif
  if (rtp_rtcp_->Sending()) {
    return -1;
  }

  CriticalSectionScoped cs(callback_cs_.get());
  if (external_transport_) {
    LOG_F(LS_ERROR) << "Transport already registered.";
    return -1;
  }
  external_transport_ = transport;
  vie_sender_.RegisterSendTransport(transport);
  return 0;
}

int32_t ViEChannel::DeregisterSendTransport() {
  CriticalSectionScoped cs(callback_cs_.get());
  if (!external_transport_) {
    return 0;
  }
  if (rtp_rtcp_->Sending()) {
    LOG_F(LS_ERROR) << "Can't deregister transport when sending.";
    return -1;
  }
  external_transport_ = NULL;
  vie_sender_.DeregisterSendTransport();

#ifndef WEBRTC_EXTERNAL_TRANSPORT
  UdpTransport::Destroy(socket_transport_);
  socket_transport_ = NULL;
#endif
  return 0;
}

int32_t ViEChannel::ReceivedRTPPacket(
    const void* rtp_packet, const size_t rtp_packet_length,
    const PacketTime& packet_time) {
  {
    CriticalSectionScoped cs(callback_cs_.get());
    if (!external_transport_) {
      return -1;
    }
  }

  return vie_receiver_.ReceivedRTPPacket(
      rtp_packet, rtp_packet_length, packet_time);
}

int32_t ViEChannel::ReceivedRTCPPacket(
  const void* rtcp_packet, const size_t rtcp_packet_length) {
  {
    CriticalSectionScoped cs(callback_cs_.get());
    if (!external_transport_) {
      return -1;
    }
  }
  return vie_receiver_.ReceivedRTCPPacket(rtcp_packet, rtcp_packet_length);
}

int32_t ViEChannel::SetMTU(uint16_t mtu) {
  if (rtp_rtcp_->SetMaxTransferUnit(mtu) != 0) {
    return -1;
  }
  CriticalSectionScoped cs(rtp_rtcp_cs_.get());
  for (std::list<RtpRtcp*>::iterator it = simulcast_rtp_rtcp_.begin();
       it != simulcast_rtp_rtcp_.end();
       it++) {
    RtpRtcp* rtp_rtcp = *it;
    rtp_rtcp->SetMaxTransferUnit(mtu);
  }
  mtu_ = mtu;
  return 0;
}

uint16_t ViEChannel::MaxDataPayloadLength() const {
  return rtp_rtcp_->MaxRtpPacketSize();
}

int32_t ViEChannel::EnableColorEnhancement(bool enable) {
  CriticalSectionScoped cs(callback_cs_.get());
  color_enhancement_ = enable;
  return 0;
}

RtpRtcp* ViEChannel::rtp_rtcp() {
  return rtp_rtcp_.get();
}

CallStatsObserver* ViEChannel::GetStatsObserver() {
  return stats_observer_.get();
}

// Do not acquire the lock of |vcm_| in this function. Decode callback won't
// necessarily be called from the decoding thread. The decoding thread may have
// held the lock when calling VideoDecoder::Decode, Reset, or Release. Acquiring
// the same lock in the path of decode callback can deadlock.
int32_t ViEChannel::FrameToRender(
    I420VideoFrame& video_frame) {  // NOLINT
  CriticalSectionScoped cs(callback_cs_.get());

  if (decoder_reset_) {
    // Trigger a callback to the user if the incoming codec has changed.
    if (codec_observer_) {
      // The codec set by RegisterReceiveCodec might not be the size we're
      // actually decoding.
      receive_codec_.width = static_cast<uint16_t>(video_frame.width());
      receive_codec_.height = static_cast<uint16_t>(video_frame.height());
      //codec_observer_->IncomingCodecChanged(channel_id_, receive_codec_);
	  codec_observer_->IncomingCodecChanged(channel_id_, receive_codec_);
    }
    decoder_reset_ = false;
  }
  // Post processing is not supported if the frame is backed by a texture.
  if (video_frame.native_handle() == NULL) {
    if (pre_render_callback_ != NULL)
      pre_render_callback_->FrameCallback(&video_frame);
    if (effect_filter_) {
      size_t length =
          CalcBufferSize(kI420, video_frame.width(), video_frame.height());
      scoped_ptr<uint8_t[]> video_buffer(new uint8_t[length]);
      ExtractBuffer(video_frame, length, video_buffer.get());
      effect_filter_->Transform(length,
                                video_buffer.get(),
                                video_frame.ntp_time_ms(),
                                video_frame.timestamp(),
                                video_frame.width(),
                                video_frame.height());
    }
    if (color_enhancement_) {
      VideoProcessingModule::ColorEnhancement(&video_frame);
    }
  }

  // Record videoframe.
  file_recorder_.RecordVideoFrame(video_frame);

  uint32_t arr_ofCSRC[kRtpCsrcSize];
  int32_t no_of_csrcs = vie_receiver_.GetCsrcs(arr_ofCSRC);
  if (no_of_csrcs <= 0) {
    arr_ofCSRC[0] = vie_receiver_.GetRemoteSsrc();
    no_of_csrcs = 1;
  }
  std::vector<uint32_t> csrcs(arr_ofCSRC, arr_ofCSRC + no_of_csrcs);
  DeliverFrame(&video_frame, csrcs);

  return 0;
}

int32_t ViEChannel::ReceivedDecodedReferenceFrame(
  const uint64_t picture_id) {
  return rtp_rtcp_->SendRTCPReferencePictureSelection(picture_id);
}

void ViEChannel::IncomingCodecChanged(const VideoCodec& codec) {
  CriticalSectionScoped cs(callback_cs_.get());
  receive_codec_ = codec;
}

void ViEChannel::OnReceiveRatesUpdated(uint32_t bit_rate, uint32_t frame_rate) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (codec_observer_)
	codec_observer_->IncomingRate(channel_id_, frame_rate, bit_rate);
}

void ViEChannel::OnDiscardedPacketsUpdated(int discarded_packets) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (vcm_receive_stats_callback_ != NULL)
  vcm_receive_stats_callback_->OnDiscardedPacketsUpdated(discarded_packets);

  if (receive_statistics_proxy_ != NULL)
	  receive_statistics_proxy_->OnDiscardedPacketsUpdated(discarded_packets);
}

void ViEChannel::OnFrameCountsUpdated(const FrameCounts& frame_counts) {
  CriticalSectionScoped cs(callback_cs_.get());
  receive_frame_counts_ = frame_counts;
  if (vcm_receive_stats_callback_ != NULL)
    vcm_receive_stats_callback_->OnFrameCountsUpdated(frame_counts);
}

void ViEChannel::OnDecoderTiming(int decode_ms,
                                 int max_decode_ms,
                                 int current_delay_ms,
                                 int target_delay_ms,
                                 int jitter_buffer_ms,
                                 int min_playout_delay_ms,
                                 int render_delay_ms) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (!codec_observer_)
    return;
  codec_observer_->DecoderTiming(decode_ms,
                                 max_decode_ms,
                                 current_delay_ms,
                                 target_delay_ms,
                                 jitter_buffer_ms,
                                 min_playout_delay_ms,
                                 render_delay_ms);
}

int32_t ViEChannel::RequestKeyFrame() {
  {
    CriticalSectionScoped cs(callback_cs_.get());
    if (codec_observer_ && do_key_frame_callbackRequest_) {
      codec_observer_->RequestNewKeyFrame(channel_id_);
    }
  }
    
    if (_key_frame_cb && !_isVideoConf) {
        _key_frame_cb(channel_id_);
    }
    else {
        rtp_rtcp_->RequestKeyFrame();
	}
	return 0;
}

int32_t ViEChannel::SliceLossIndicationRequest(
  const uint64_t picture_id) {
  return rtp_rtcp_->SendRTCPSliceLossIndication((uint8_t) picture_id);
}

int32_t ViEChannel::ResendPackets(const uint16_t* sequence_numbers,
                                        uint16_t length) {
  return rtp_rtcp_->SendNACK(sequence_numbers, length);
}

bool ViEChannel::ChannelDecodeThreadFunction(void* obj) {
  int ret = static_cast<ViEChannel*>(obj)->ChannelDecodeProcess();
  WEBRTC_TRACE(kTraceError, kTraceVideo, 0,
	  "%s:this channel id is %d, local_ssrc_main_=%u, remote_ssrc_=%u, decode ret=%d", __FUNCTION__, static_cast<ViEChannel*>(obj)->channel_id_, static_cast<ViEChannel*>(obj)->local_ssrc_main_, static_cast<ViEChannel*>(obj)->remote_ssrc_, ret);
  return ret;
}

bool ViEChannel::ChannelDecodeProcess() {
  vcm_->Decode(kMaxDecodeWaitTimeMs, _shield_mosaic);
  return true;
}

void ViEChannel::OnRttUpdate(int64_t rtt) {
  vcm_->SetReceiveChannelParameters(rtt);
}

void ViEChannel::ReserveRtpRtcpModules(size_t num_modules) {
  for (size_t total_modules =
           1 + simulcast_rtp_rtcp_.size() + removed_rtp_rtcp_.size();
       total_modules < num_modules;
       ++total_modules) {
    RtpRtcp* rtp_rtcp = CreateRtpRtcpModule();
    rtp_rtcp->SetSendingStatus(false);
    rtp_rtcp->SetSendingMediaStatus(false);
    rtp_rtcp->RegisterRtcpStatisticsCallback(NULL);
    rtp_rtcp->RegisterSendChannelRtpStatisticsCallback(NULL);
    removed_rtp_rtcp_.push_back(rtp_rtcp);
  }
}

RtpRtcp* ViEChannel::GetRtpRtcpModule(size_t index) const {
  if (index == 0)
    return rtp_rtcp_.get();
  if (index <= simulcast_rtp_rtcp_.size()) {
    std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
    for (size_t i = 1; i < index; ++i) {
      ++it;
    }
    return *it;
  }

  // If the requested module exists it must be in the removed list. Index
  // translation to this list must remove the default module as well as all
  // active simulcast modules.
  size_t removed_idx = index - simulcast_rtp_rtcp_.size() - 1;
  if (removed_idx >= removed_rtp_rtcp_.size())
    return NULL;

  std::list<RtpRtcp*>::const_iterator it = removed_rtp_rtcp_.begin();
  while (removed_idx-- > 0)
    ++it;

  return *it;
}

RtpRtcp::Configuration ViEChannel::CreateRtpRtcpConfiguration() {
  RtpRtcp::Configuration configuration;
  configuration.id = ViEModuleId(engine_id_, channel_id_);
  configuration.audio = false;
  configuration.outgoing_transport = &vie_sender_;
  configuration.intra_frame_callback = intra_frame_observer_;
  configuration.bandwidth_callback = bandwidth_observer_.get();
  configuration.rtt_stats = rtt_stats_;
  configuration.paced_sender = paced_sender_;
  configuration.send_bitrate_observer = &send_bitrate_observer_;
  configuration.send_frame_count_observer = &send_frame_count_observer_;
  configuration.receive_rtcp_packettype_count_observer = &receive_rtcp_packettype_count_observer_;
  configuration.send_side_delay_observer = &send_side_delay_observer_;

  return configuration;
}

RtpRtcp* ViEChannel::CreateRtpRtcpModule() {
  return RtpRtcp::CreateRtpRtcp(CreateRtpRtcpConfiguration());
}

int32_t ViEChannel::StartDecodeThread() {
  // Start the decode thread
  if (decode_thread_) {
    // Already started.
    return 0;
  }
  decode_thread_ = ThreadWrapper::CreateThread(ChannelDecodeThreadFunction,
                                                   this, kHighestPriority,
                                                   "DecodingThread");
  if (!decode_thread_) {
    return -1;
  }

  unsigned int thread_id;
  if (decode_thread_->Start(thread_id) == false) {
    delete decode_thread_;
    decode_thread_ = NULL;
    LOG(LS_ERROR) << "Could not start decode thread.";
    return -1;
  }
  return 0;
}

int32_t ViEChannel::StopDecodeThread() {
  if (!decode_thread_) {
    return 0;
  }

  decode_thread_->SetNotAlive();
  if (decode_thread_->Stop()) {
    delete decode_thread_;
  } else {
    assert(false && "could not stop decode thread");
  }
  decode_thread_ = NULL;
  return 0;
}

int32_t ViEChannel::SetVoiceChannel(int32_t ve_channel_id,
                                          VoEVideoSync* ve_sync_interface) {
  if (ve_sync_interface) {
    // Register lip sync
    module_process_thread_.RegisterModule(&vie_sync_);
  } else {
    module_process_thread_.DeRegisterModule(&vie_sync_);
  }
  return vie_sync_.ConfigureSync(ve_channel_id,
                                 ve_sync_interface,
                                 rtp_rtcp_.get(),
                                 vie_receiver_.GetRtpReceiver());
}

int32_t ViEChannel::VoiceChannel() {
  return vie_sync_.VoiceChannel();
}

int32_t ViEChannel::RegisterEffectFilter(ViEEffectFilter* effect_filter) {
  CriticalSectionScoped cs(callback_cs_.get());
  if (effect_filter && effect_filter_) {
    LOG(LS_ERROR) << "Effect filter already registered.";
    return -1;
  }
  effect_filter_ = effect_filter;
  return 0;
}

void ViEChannel::RegisterPreRenderCallback(
    I420FrameCallback* pre_render_callback) {
  CriticalSectionScoped cs(callback_cs_.get());
  pre_render_callback_ = pre_render_callback;
}

void ViEChannel::RegisterPreDecodeImageCallback(
    EncodedImageCallback* pre_decode_callback) {
  vcm_->RegisterPreDecodeImageCallback(pre_decode_callback);
}

int32_t ViEChannel::OnInitializeDecoder(
    const int32_t id,
    int8_t payloadType,
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    int frequency,
    size_t channels,
    uint32_t rate) {
  LOG(LS_INFO) << "OnInitializeDecoder " << static_cast<int>(payloadType)
               << " " << payloadName;
  vcm_->ResetDecoder();

  CriticalSectionScoped cs(callback_cs_.get());
  decoder_reset_ = true;
  return 0;
}

void ViEChannel::OnIncomingSSRCChanged(const int32_t id, const uint32_t ssrc) {
  assert(channel_id_ == ChannelId(id));
  rtp_rtcp_->SetRemoteSSRC(ssrc);

  CriticalSectionScoped cs(callback_cs_.get());
  {
    if (rtp_observer_) {
      rtp_observer_->IncomingSSRCChanged(channel_id_, ssrc);
    }
  }
}

void ViEChannel::OnIncomingCSRCChanged(const int32_t id,
                                       const uint32_t CSRC,
                                       const bool added) {
  assert(channel_id_ == ChannelId(id));
  CriticalSectionScoped cs(callback_cs_.get());
  {
    if (rtp_observer_) {
      rtp_observer_->IncomingCSRCChanged(channel_id_, CSRC, added);
    }
  }
}

void ViEChannel::ResetStatistics(uint32_t ssrc) {
  StreamStatistician* statistician =
      vie_receiver_.GetReceiveStatistics()->GetStatistician(ssrc);
  if (statistician)
    statistician->ResetStatistics();
}

void ViEChannel::RegisterSendFrameCountObserver(
    FrameCountObserver* observer) {
  send_frame_count_observer_.Set(observer);
}

void ViEChannel::RegisterReceiveRtcpPacketTypeCounterObserver(RtcpPacketTypeCounterObserver* observer){
		receive_rtcp_packettype_count_observer_.Set(observer);
}

void ViEChannel::RegisterSendRtcpPacketTypeCountObserver(RtcpPacketTypeCounterObserver* observer)
{
	rtp_rtcp_->SetSendRtcpPacketTypeCountObserver(observer);
}

void ViEChannel::ReceivedBWEPacket(int64_t arrival_time_ms,
                                   size_t payload_size,
                                   const RTPHeader& header) {
  vie_receiver_.ReceivedBWEPacket(arrival_time_ms, payload_size, header);
}

//sean add begin 0915
int32_t
	ViEChannel::SetShieldMosaic(bool flag)
{
	_shield_mosaic = flag;
//    if (_shield_mosaic) {
//        vcm_->SetDecodeErrorMode(kNoErrors);
//    }
	return 0;
}
//sean add end 0915
int32_t
	ViEChannel::setProcessData(bool flag)
{
	//Don't implement at the moment.
	return 0;
}


void ViEChannel::getNetworkStatistic(time_t &startTime, long long &sendLengthSim,  long long &recvLengthSim, long long &sendLengthWifi, long long &recvLengthWifi)
{
	CriticalSectionScoped cs(critsect_net_statistic.get());

	long long sendTime=0, sendLenSim = 0, sendLenWifi = 0;
	vie_sender_.getVieSenderStatistic(sendTime, sendLenSim, sendLenWifi);

	startTime = _startNetworkTime<sendTime ? _startNetworkTime:sendTime;
	sendLengthSim = sendLenSim;
	sendLengthWifi = sendLenWifi;
	recvLengthSim = _recvDataTotalSim;
	recvLengthWifi = _recvDataTotalWifi;
}

void ViEChannel::setNetworkType(bool isWifi)
{
	_isWifi = isWifi;
	vie_sender_.setNetworkStatus(isWifi);
}

int32_t ViEChannel::SetVideoConferenceFlag(const char *selfSipNo, const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip)
{
	if(!selfSipNo || !sipNo || !conferenceNo || !confPasswd || port<=0 || !ip)
		return -1;

	delete[] _selfSipNo;
	delete[] _sipNo;
	delete[] _conferenceNo;
	delete[] _conferPassword;
	delete[] _confIP;

	_selfSipNo = new char[strlen(selfSipNo)+1];
	if(_selfSipNo) {
		strcpy(_selfSipNo, selfSipNo);
		_selfSipNo[strlen(selfSipNo)] = '\0';
	}

	_sipNo = new char[strlen(sipNo)+1];
	if(_sipNo) {
		strcpy(_sipNo, sipNo);
		_sipNo[strlen(sipNo)] = '\0';
	}

	_conferenceNo = new char[strlen(conferenceNo)+1];
	if(_conferenceNo) {
		strcpy(_conferenceNo, conferenceNo);
		_conferenceNo[strlen(conferenceNo)] = '\0';
	}

	_conferPassword = new char[strlen(confPasswd)+1];
	if(_conferPassword) {
		strcpy(_conferPassword, confPasswd);
		_conferPassword[strlen(confPasswd)] = '\0';
	}

	_confIP = new char[strlen(ip)+1];
	if(_confIP) {
		strcpy(_confIP, ip);
		_confIP[strlen(ip)] = '\0';
	}

	_confPort = port;
	_isVideoConference = true;
	return 0;
}

void  ViEChannel::IncomingRTPPacket(const int8_t* rtp_packet,
	const int32_t rtp_packet_length,
	const char* from_ip,
	const uint16_t from_port)
{
	unsigned int r_rtpSsrc = ((unsigned char)rtp_packet[8] << 24)
		| ((unsigned char)rtp_packet[9] << 16)
		| ((unsigned char)rtp_packet[10] << 8)
		| (unsigned char)rtp_packet[11];

	unsigned short seq_num = ((unsigned char)rtp_packet[2] << 8) | (unsigned char)rtp_packet[3];

	WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
		"%s: myself channelid is %d, local_ssrc_main_=%u, remote_ssrc_=%u,  recieve rmote ssrc=%u, seq_num=%u",
		__FUNCTION__, channel_id_, local_ssrc_main_, remote_ssrc_, r_rtpSsrc, seq_num);


	{
		CriticalSectionScoped cs(critsect_net_statistic.get());
		if(_startNetworkTime == 0)
			_startNetworkTime = time(NULL);
		if (_isWifi) {
			_recvDataTotalWifi += rtp_packet_length;
			_recvDataTotalWifi += 42;//14+20+8; //ethernet+ip+udp header
		}
		else
		{
			_recvDataTotalSim += rtp_packet_length;
			_recvDataTotalSim += 42;//14 + 20 + 8; //ethernet+ip+udp header
		}
	}


	/*add begin------------------Sean20130722----------for video ice------------*/
	rtp_header_t *rtp;
	//Sean ice for STUN Message
	if ( _stun_cb && rtp_packet_length>=12 ) //纯rtp�?
	{
		rtp = (rtp_header_t*)rtp_packet;
		if (rtp->version!=2)
		{
			//判断是否是stun response消息 
			unsigned short stunlen = *((unsigned short *)(rtp_packet + sizeof(unsigned short)));
			stunlen = ntohs(stunlen);
			if (stunlen + 20 ==rtp_packet_length) {
				_stun_cb(channel_id_, (void*)rtp_packet, rtp_packet_length, from_ip, from_port, false, true);
				return;

			}
		}
	}
	/*add end--------------------Sean20130722----------for video ice------------*/
	//sean add begin 20140705 video conference
	//    [result:0,payload:97]
	if (_video_conf_cb) {
		if (!strncasecmp("[result:", (const char *)rtp_packet+12, strlen("[result:"))) {


			//           _videoConferencePacketReceived = true;
			char * statusStr = NULL;
			char * startPos = (char*)strstr((const char *)rtp_packet+12, ":");
			char * endPos = strstr(startPos, ",");
			statusStr = new char[endPos-startPos];
			memcpy(statusStr, startPos+1, endPos-startPos-1);
			statusStr[endPos-startPos-1] = '\0';
			int status = atoi(statusStr);
			delete [] statusStr;

			char *payloadStr = NULL;
			char *payloadStartPos = strstr(endPos, ":");
			char *payloadEndPos = strstr(payloadStartPos, "]");
			payloadStr = new char[payloadEndPos-payloadStartPos];
			memcpy(payloadStr, payloadStartPos+1, payloadEndPos-payloadStartPos-1);
			payloadStr[payloadEndPos-payloadStartPos-1] = '\0';
			int payload = atoi(payloadStr);
			delete [] payloadStr;
			_video_conf_cb(channel_id_,status,payload);
			return;
		}
	}
	//sean add end 20140705 video conference

	vie_receiver_.ReceivedRTPPacket(rtp_packet, rtp_packet_length,cloopenwebrtc::PacketTime());
}

void  ViEChannel::IncomingRTCPPacket(const int8_t* rtcp_packet,
	const int32_t rtcp_packet_length,
	const char* from_ip,
	const uint16_t from_port)
{
	{
		CriticalSectionScoped cs(critsect_net_statistic.get());
		if(_startNetworkTime == 0)
			_startNetworkTime = time(NULL);
		if (_isWifi) {
			_recvDataTotalWifi += rtcp_packet_length;
			_recvDataTotalWifi += 42;//14 + 20 + 8;//ethernet+ip+udp header
		}
		else
		{
			_recvDataTotalSim += rtcp_packet_length;
			_recvDataTotalSim += 42;//14 + 20 + 8;//ethernet+ip+udp header
		}
	}


	rtp_header_t *rtp;
	//Sean ice for STUN Message
	if ( _stun_cb && rtcp_packet_length>=12 ) //纯rtp�?
	{
		rtp = (rtp_header_t*)rtcp_packet;
		if (rtp->version!=2)
		{
			//            WEBRTC_TRACE(kTraceDebug, kTraceVideo, 0,"rtp->version!=2, rtp->version = %d, this looks like a stun packet\n",rtp->version);
			//判断是否是stun response消息 
			unsigned short  stunlen = *((unsigned short *)(rtcp_packet + sizeof(unsigned short)));
			stunlen = ntohs(stunlen);
			if (stunlen + 20 ==rtcp_packet_length) {
				_stun_cb(channel_id_, (void*)rtcp_packet, rtcp_packet_length, from_ip, from_port, true, true);
				return;

			}
		}
	}
    /*add end--------------------Sean20130722----------for video ice------------*/
    //sean add begin 20140705 video conference
    //    [result:0,payload:97]
    if (_video_conf_cb) {
        if (!strncasecmp("[result:", (const char *)rtcp_packet+12, strlen("[result:"))) {
            
            
            //           _videoConferencePacketReceived = true;
            char * statusStr = NULL;
            char * startPos = (char*)strstr((const char *)rtcp_packet+12, ":");
            char * endPos = strstr(startPos, ",");
            statusStr = new char[endPos-startPos];
            memcpy(statusStr, startPos+1, endPos-startPos-1);
            statusStr[endPos-startPos-1] = '\0';
            int status = atoi(statusStr);
            delete [] statusStr;
            
            char *payloadStr = NULL;
            char *payloadStartPos = strstr(endPos, ":");
            char *payloadEndPos = strstr(payloadStartPos, "]");
            payloadStr = new char[payloadEndPos-payloadStartPos];
            memcpy(payloadStr, payloadStartPos+1, payloadEndPos-payloadStartPos-1);
            payloadStr[payloadEndPos-payloadStartPos-1] = '\0';
            int payload = atoi(payloadStr);
            delete [] payloadStr;
            _video_conf_cb(channel_id_,status,payload);
            return;
        }
    }
    
	vie_receiver_.ReceivedRTCPPacket(rtcp_packet, rtcp_packet_length);
}

int32_t ViEChannel::RegisterExternalEncryption(Encryption* encryption) {
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
		__FUNCTION__);

	CriticalSectionScoped cs(callback_cs_.get());
	if (external_encryption_) {
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: external encryption already registered", __FUNCTION__);
		return -1;
	}

	external_encryption_ = encryption;

	vie_receiver_.RegisterExternalDecryption(encryption);
	vie_sender_.RegisterExternalEncryption(encryption);

	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
		"%s", "external encryption object registerd with channel=%d",
		channel_id_);
	return 0;
}

int32_t ViEChannel::DeRegisterExternalEncryption() {
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
		__FUNCTION__);

	CriticalSectionScoped cs(callback_cs_.get());
	if (!external_encryption_) {
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: external encryption is not registered", __FUNCTION__);
		return -1;
	}

	external_transport_ = NULL;
	vie_receiver_.DeregisterExternalDecryption();
	vie_sender_.DeregisterExternalEncryption();
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
		"%s external encryption object de-registerd with channel=%d",
		__FUNCTION__, channel_id_);
	return 0;
}

#ifdef WEBRTC_SRTP
int ViEChannel::CcpSrtpInit()
{
	int err = _srtpModule.CcpSrtpInit(channel_id_);
	return err;
}

int ViEChannel::CcpSrtpShutdown()
{
	int err = _srtpModule.CcpSrtpShutdown(channel_id_);
	return err;
}

int ViEChannel::EnableSRTPSend(ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
		"ViEChannel::EnableSRTPSend()");

	CriticalSectionScoped cs(callback_cs_.get());

	if (_encrypting)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: encryption already enabled", __FUNCTION__);
		return -1;
	}

	if (key == NULL)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: invalid key string", __FUNCTION__);
		return -1;
	}

	//if (((kEncryption == level ||
	//	kEncryptionAndAuthentication == level) &&
	//	(cipherKeyLength < kMinSrtpEncryptLength ||
	//		cipherKeyLength > kMaxSrtpEncryptLength)) ||
	//	((kAuthentication == level ||
	//		kEncryptionAndAuthentication == level) &&
	//		kAuthHmacSha1 == authType &&
	//		(authKeyLength > kMaxSrtpAuthSha1Length ||
	//			authTagLength > kMaxSrtpAuthSha1Length)) ||
	//	((kAuthentication == level ||
	//		kEncryptionAndAuthentication == level) &&
	//		kAuthNull == authType &&
	//		(authKeyLength > kMaxSrtpKeyAuthNullLength ||
	//			authTagLength > kMaxSrtpTagAuthNullLength)))
	//{
	//	WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
	//		"%s: invalid key length(s) cipherKeyLength:%d authKeyLength:%d authTagLength:%d", __FUNCTION__, 
	//		cipherKeyLength, authKeyLength, authTagLength);
	//	return -1;
	//}
	unsigned int ssrc;
	GetLocalSSRC(0, &ssrc);
	if (_srtpModule.EnableSRTPSend(channel_id_, crypt_type, key, ssrc) == -1)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: failed to enable SRTP encryption", __FUNCTION__);
		return -1;
	}

	external_encryption_ = &_srtpModule;
	vie_sender_.RegisterExternalEncryption(external_encryption_);

	_encrypting = true;

	return 0;
}

int ViEChannel::DisableSRTPSend()
{
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
		"ViEChannel::DisableSRTPSend()");

	CriticalSectionScoped cs(callback_cs_.get());

	if (!_encrypting)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: SRTP encryption already disabled", __FUNCTION__);
		return 0;
	}

	_encrypting = false;

	//    if (_srtpModule.DisableSRTPEncrypt() == -1)
	if (_srtpModule.DisableSRTPSend(channel_id_) == -1)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: failed to disable SRTP encryption", __FUNCTION__);
		return -1;
	}
	external_transport_ = NULL;
	vie_sender_.DeregisterExternalEncryption();

	return 0;
}

int ViEChannel::EnableSRTPReceive(ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
		"ViEChannel::EnableSRTPReceive()");

	CriticalSectionScoped cs(callback_cs_.get());

	if (_decrypting)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: SRTP decryption already enabled", __FUNCTION__);
		return -1;
	}

	if (key == NULL)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: invalid key string", __FUNCTION__);
		return -1;
	}

	//if ((((kEncryption == level) ||
	//	(kEncryptionAndAuthentication == level)) &&
	//	((cipherKeyLength < kMinSrtpEncryptLength) ||
	//		(cipherKeyLength > kMaxSrtpEncryptLength))) ||
	//	(((kAuthentication == level) ||
	//		(kEncryptionAndAuthentication == level)) &&
	//		(kAuthHmacSha1 == authType) &&
	//		((authKeyLength > kMaxSrtpAuthSha1Length) ||
	//			(authTagLength > kMaxSrtpAuthSha1Length))) ||
	//	(((kAuthentication == level) ||
	//		(kEncryptionAndAuthentication == level)) &&
	//		(kAuthNull == authType) &&
	//		((authKeyLength > kMaxSrtpKeyAuthNullLength) ||
	//			(authTagLength > kMaxSrtpTagAuthNullLength))))
	//{
	//	WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
	//		"%s: invalid key length(s) cipherKeyLength:%d authKeyLength:%d authTagLength:%d", __FUNCTION__,
	//		cipherKeyLength, authKeyLength, authTagLength);
	//	return -1;
	//}

	if (_srtpModule.EnableSRTPReceive(channel_id_, crypt_type, key) == -1)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: failed to enable SRTP decryption", __FUNCTION__);
		return -1;
	}
	
	external_encryption_ = &_srtpModule;
	vie_receiver_.RegisterExternalDecryption(external_encryption_);
	_decrypting = true;

	return 0;
}

int ViEChannel::DisableSRTPReceive()
{
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
		"ViEChannel::DisableSRTPReceive()");

	CriticalSectionScoped cs(callback_cs_.get());

	if (!_decrypting)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: SRTP decryption already disabled", __FUNCTION__);
		return 0;
	}

	_decrypting = false;

	if (_srtpModule.DisableSRTPReceive(channel_id_))
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: failed to disable SRTP decryption", __FUNCTION__);
		return -1;
	}

	external_transport_ = NULL;
	vie_receiver_.DeregisterExternalDecryption();

	return 0;
}

#endif

int32_t ViEChannel::SetLocalReceiver(const uint16_t rtp_port,
	const uint16_t rtcp_port,
	const char* ip_address) {

#if 0
		WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
			__FUNCTION__);

		callback_cs_->Enter();
		if (external_transport_) {
			callback_cs_->Leave();
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: external transport registered", __FUNCTION__);
			return -1;
		}
		callback_cs_->Leave();

#ifndef WEBRTC_EXTERNAL_TRANSPORT
		if (socket_transport_->Receiving()) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: already receiving", __FUNCTION__);
			return 0;
		}

		const char* multicast_ip_address = NULL;
		if (socket_transport_->InitializeReceiveSockets(this, rtp_port,
			ip_address,
			multicast_ip_address,
			rtcp_port) != 0) {
				int32_t socket_error = socket_transport_->LastError();
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
					"%s: could not initialize receive sockets. Socket error: %d",
					__FUNCTION__, socket_error);
				return -1;
		}
		return 0;
#else
		WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: not available for external transport", __FUNCTION__);
		return -1;
#endif

#endif

		return 0;
}

int32_t ViEChannel::GetLocalReceiver(uint16_t& rtp_port,
	uint16_t& rtcp_port,
	char* ip_address) const {
		WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
			__FUNCTION__);

		callback_cs_->Enter();
		if (external_transport_) {
			callback_cs_->Leave();
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: external transport registered", __FUNCTION__);
			return -1;
		}
		callback_cs_->Leave();

#ifndef WEBRTC_EXTERNAL_TRANSPORT
		if (socket_transport_->ReceiveSocketsInitialized() == false) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: receive sockets not initialized", __FUNCTION__);
			return -1;
		}

		char multicast_ip_address[UdpTransport::kIpAddressVersion6Length];
		if (socket_transport_->ReceiveSocketInformation(ip_address, rtp_port,
			rtcp_port,
			multicast_ip_address) != 0) {
				int32_t socket_error = socket_transport_->LastError();
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
					"%s: could not get receive socket information. Socket error: %d",
					__FUNCTION__, socket_error);
				return -1;
		}
		return 0;
#else
		WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: not available for external transport", __FUNCTION__);
		return -1;
#endif
}

int32_t ViEChannel::SetSocks5SendData(unsigned char *data, int length, bool isRTCP) {
  callback_cs_->Enter();
  if (external_transport_) {
    callback_cs_->Leave();
    WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
            "%s: external transport registered", __FUNCTION__);
    return -1;
  }
  callback_cs_->Leave();
#ifndef WEBRTC_EXTERNAL_TRANSPORT
  socket_transport_->SetSocks5SendData(data, length, isRTCP);
#endif
  return 0;
};

int32_t ViEChannel::SetSendDestination(
        const char *rtp_ip_address,
        const uint16_t rtp_port,
        const char *rtcp_ip_address,
        const uint16_t rtcp_port,
        const uint16_t source_rtp_port,
        const uint16_t source_rtcp_port) {
		WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
			__FUNCTION__);

		callback_cs_->Enter();
		if (external_transport_) {
			callback_cs_->Leave();
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: external transport registered", __FUNCTION__);
			return -1;
		}
		callback_cs_->Leave();

#ifndef WEBRTC_EXTERNAL_TRANSPORT
		const bool is_ipv6 = socket_transport_->IpV6Enabled();
		if (UdpTransport::IsIpAddressValid(rtp_ip_address, is_ipv6) == false || UdpTransport::IsIpAddressValid(rtcp_ip_address, is_ipv6) == false) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: Not a valid RTP IP address: %s or RTCP IP address: %s", __FUNCTION__, rtp_ip_address, rtcp_ip_address);
			return -1;
		}
		if (socket_transport_->InitializeSendSockets(rtp_ip_address, rtp_port, rtcp_ip_address, rtcp_port) != 0) {
				int32_t socket_error = socket_transport_->LastError();
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
					"%s: could not initialize send socket. Socket error: %d",
					__FUNCTION__, socket_error);
				return -1;
		}

		if (source_rtp_port != 0) {
			uint16_t receive_rtp_port = 0;
			uint16_t receive_rtcp_port = 0;
			if (socket_transport_->ReceiveSocketInformation(NULL, receive_rtp_port,
				receive_rtcp_port,
				NULL) != 0) {
					int32_t socket_error = socket_transport_->LastError();
					WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
						"%s: could not get receive port information. Socket error: %d",
						__FUNCTION__, socket_error);
					return -1;
			}
			// Initialize an extra socket only if send port differs from receive
			// port.
			if (source_rtp_port != receive_rtp_port) {
				if (socket_transport_->InitializeSourcePorts(source_rtp_port,
					source_rtcp_port) != 0) {
						int32_t socket_error = socket_transport_->LastError();
						WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
							"%s: could not set source ports. Socket error: %d",
							__FUNCTION__, socket_error);
						return -1;
				}
			}
		}
    
		vie_sender_.RegisterSendTransport(socket_transport_);


        /***** 这块 还没有做添加 rtcp ip address 之后的整理 zhaoyou *****/
		// Workaround to avoid SSRC colision detection in loppback tests.
		if (!is_ipv6) {
			WebRtc_UWord32 local_host_address = 0;
			const WebRtc_UWord32 current_ip_address =
				UdpTransport::InetAddrIPV4(rtp_ip_address);

			if ((UdpTransport::LocalHostAddress(local_host_address) == 0 &&
				local_host_address == current_ip_address) ||
				strncmp("127.0.0.1", rtp_ip_address, 9) == 0) {
					rtp_rtcp_->SetSSRC(0xFFFFFFFF);
					WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
						"Running in loopback. Forcing fixed SSRC");
			}
		} else {
			char local_host_address[16];
			char current_ip_address[16];

			int32_t conv_result =
				UdpTransport::LocalHostAddressIPV6(local_host_address);
			conv_result += socket_transport_->InetPresentationToNumeric(
				23, rtp_ip_address, current_ip_address);
			if (conv_result == 0) {
				bool local_host = true;
				for (int32_t i = 0; i < 16; i++) {
					if (local_host_address[i] != current_ip_address[i]) {
						local_host = false;
						break;
					}
				}
				if (!local_host) {
					local_host = true;
					for (int32_t i = 0; i < 15; i++) {
						if (current_ip_address[i] != 0) {
							local_host = false;
							break;
						}
					}
					if (local_host == true && current_ip_address[15] != 1) {
						local_host = false;
					}
				}
				if (local_host) {
					rtp_rtcp_->SetSSRC(0xFFFFFFFF);
					WEBRTC_TRACE(kTraceStateInfo, kTraceVideo,
						ViEId(engine_id_, channel_id_),
						"Running in loopback. Forcing fixed SSRC");
				}
			}
		}
		return 0;
#else
		WEBRTC_TRACE(kTraceStateInfo, kTraceVideo,
			ViEId(engine_id_, channel_id_),
			"%s: not available for external transport", __FUNCTION__);
		return -1;
#endif
}

int32_t ViEChannel::GetSendDestination(
	char* ip_address,
	uint16_t& rtp_port,
	uint16_t& rtcp_port,
	uint16_t& source_rtp_port,
	uint16_t& source_rtcp_port) const {
		WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
			__FUNCTION__);

		callback_cs_->Enter();
		if (external_transport_) {
			callback_cs_->Leave();
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: external transport registered", __FUNCTION__);
			return -1;
		}
		callback_cs_->Leave();

#ifndef WEBRTC_EXTERNAL_TRANSPORT
		if (socket_transport_->SendSocketsInitialized() == false) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: send sockets not initialized", __FUNCTION__);
			return -1;
		}
		if (socket_transport_->SendSocketInformation(ip_address, rtp_port, rtcp_port)
			!= 0) {
				int32_t socket_error = socket_transport_->LastError();
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
					"%s: could not get send socket information. Socket error: %d",
					__FUNCTION__, socket_error);
				return -1;
		}
		source_rtp_port = 0;
		source_rtcp_port = 0;
		if (socket_transport_->SourcePortsInitialized()) {
			socket_transport_->SourcePorts(source_rtp_port, source_rtcp_port);
		}
		return 0;
#else
		WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: not available for external transport", __FUNCTION__);
		return -1;
#endif
}

int32_t ViEChannel::StartSend() {
	CriticalSectionScoped cs(callback_cs_.get());
	WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
		"%s", __FUNCTION__);

#ifndef WEBRTC_EXTERNAL_TRANSPORT
	if (!external_transport_) {
		if (socket_transport_->SendSocketsInitialized() == false) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: send sockets not initialized", __FUNCTION__);
			return -1;
		}
	}
#endif
	rtp_rtcp_->SetSendingMediaStatus(true);

	if (!isSVCChannel_) {//if svc, simulcast not call this function
		if (rtp_rtcp_->Sending()) {
			// Already sending.
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
				"%s: Already sending", __FUNCTION__);
			return kViEBaseAlreadySending;
		}
	}

	if (rtp_rtcp_->SetSendingStatus(true) != 0) {
		WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: Could not start sending RTP", __FUNCTION__);
		return -1;
	}
	CriticalSectionScoped cs_rtp(rtp_rtcp_cs_.get());
	for (std::list<RtpRtcp*>::const_iterator it = simulcast_rtp_rtcp_.begin();
		it != simulcast_rtp_rtcp_.end();
		it++) {
			RtpRtcp* rtp_rtcp = *it;
			rtp_rtcp->SetSendingMediaStatus(true);
			rtp_rtcp->SetSendingStatus(true);
	}
	return 0;
}

bool ViEChannel::Receiving() {
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	return socket_transport_->Receiving();
#else
	return false;
#endif
}

int32_t ViEChannel::SendUDPPacket(const int8_t* data,
	const WebRtc_UWord32 length,
	int32_t& transmitted_bytes,
	bool use_rtcp_socket,
	/*add begin------------------Sean20130723----------for video ice------------*/
	uint16_t portnr,
	const char* ip
	/*add end--------------------Sean20130723----------for video ice------------*/
	) {
		WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
			__FUNCTION__);
		{
			CriticalSectionScoped cs(callback_cs_.get());
			if (external_transport_) {
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
					"%s: External transport registered", __FUNCTION__);
				return -1;
			}
		}

#ifndef WEBRTC_EXTERNAL_TRANSPORT
		transmitted_bytes = socket_transport_->SendRaw(data, length, use_rtcp_socket,portnr,ip);
		if (transmitted_bytes == -1) {
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
				__FUNCTION__);
			return -1;
		}



		return 0;
#else
		WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
			"%s: not available for external transport", __FUNCTION__);
		return -1;
#endif
}

void ViEChannel::RegisterFrameStorageCallback(VCMFrameStorageCallback* frameStorageCallback)
{
	vcm_->RegisterFrameStorageCallback(frameStorageCallback);
}


void ViEChannel::ReleaseIncomingFileRecorder() {
	// Stop getting callback of all frames before they are decoded.
	vcm_->RegisterFrameStorageCallback(NULL);
}


ViEFileRecorder& ViEChannel::GetIncomingFileRecorder() {
	// Start getting callback of all frames before they are decoded.
	vcm_->RegisterFrameStorageCallback(this);
	return file_recorder_;
}

int32_t ViEChannel::StoreReceivedFrame(
	const EncodedVideoData& frame_to_store) {
		return 0;
}

int ViEChannel::setVideoConfCb(onVideoConference video_conf_cb)
{
    _video_conf_cb = video_conf_cb;
    return 0;
}

int ViEChannel::setVideoDataCb(onEcMediaVideoData video_data_cb)
{
    vie_receiver_.SetVideoDataCb(video_data_cb);
    vie_sender_.SetVideoDataCb(video_data_cb);
    return 0;
}

int ViEChannel::setStunCb(onStunPacket stun_cb)
{
    _stun_cb = stun_cb;
    return 0;
}

int32_t ViEChannel::SetRequestKeyFrameCb(bool isVideoConf,onEcMediaRequestKeyFrame cb)
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
                 "%s,_isVideoConf:%d, channel:%d,_cb:%p", __FUNCTION__,_isVideoConf,channel_id_,_key_frame_cb);
    _key_frame_cb = cb;
    _isVideoConf = isVideoConf;
    return 0;
}

int32_t ViEChannel::EnableIPv6() {
    callback_cs_->Enter();
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
                 "%s", __FUNCTION__);
    
    if (external_transport_) {
        callback_cs_->Leave();
        WEBRTC_TRACE(kTraceError, kTraceVideo,
                     ViEId(engine_id_, channel_id_),
                     "%s: External transport registered", __FUNCTION__);
        return -1;
    }
    callback_cs_->Leave();
    
#ifndef WEBRTC_EXTERNAL_TRANSPORT
    if (socket_transport_->IpV6Enabled()) {
        WEBRTC_TRACE(kTraceWarning, kTraceVideo, ViEId(engine_id_, channel_id_),
                     "%s: IPv6 already enabled", __FUNCTION__);
        return -1;
    }
    
    if (socket_transport_->EnableIpV6() != 0) {
        int32_t socket_error = socket_transport_->LastError();
        WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
                     "%s: could not enable IPv6. Socket error: %d", __FUNCTION__,
                     socket_error);
        return -1;
    }
    return 0;
#else
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
                 "%s: not available for external transport", __FUNCTION__);
    return -1;
#endif
}

bool ViEChannel::IsIPv6Enabled() {
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, ViEId(engine_id_, channel_id_), "%s",
                 __FUNCTION__);
    {
        CriticalSectionScoped cs(callback_cs_.get());
        if (external_transport_) {
            WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, channel_id_),
                         "%s: External transport registered", __FUNCTION__);
            return false;
        }
    }
#ifndef WEBRTC_EXTERNAL_TRANSPORT
    return socket_transport_->IpV6Enabled();
#else
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, ViEId(engine_id_, channel_id_),
                 "%s: not available for external transport", __FUNCTION__);
    return false;
#endif
}
    
int32_t ViEChannel::SetKeepAliveStatus(
                                             const bool enable, const int8_t unknownPayloadType,
                                             const uint16_t deltaTransmitTimeMS)
{
    WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideo, ViEId(engine_id_, channel_id_),
                 "%s", __FUNCTION__);
    
    if (enable && rtp_rtcp_->RTPKeepalive())
    {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
                     ViEId(engine_id_, channel_id_),
                     "%s: RTP keepalive already enabled", __FUNCTION__);
        return -1;
    }
    else if (!enable && !rtp_rtcp_->RTPKeepalive())
    {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
                     ViEId(engine_id_, channel_id_),
                     "%s: RTP keepalive already disabled", __FUNCTION__);
        return -1;
    }
    
    if (rtp_rtcp_->SetRTPKeepaliveStatus(enable, unknownPayloadType,
                                         deltaTransmitTimeMS) != 0)
    {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
                     ViEId(engine_id_, channel_id_),
                     "%s: Could not set RTP keepalive status %d", __FUNCTION__,
                     enable);
        //        if (enable == false && !rtp_rtcp_->DefaultModuleRegistered())
        //        {
        //            // Not sending media and we try to disable keep alive
        //            _rtpRtcp.ResetSendDataCountersRTP();
        //            _rtpRtcp.SetSendingStatus(false);
        //        }
        return -1;
    }
    
    if (enable && !rtp_rtcp_->Sending())
    {
        // Enable sending to start sending Sender reports instead of receive
        // reports
        if (rtp_rtcp_->SetSendingStatus(true) != 0)
        {
            rtp_rtcp_->SetRTPKeepaliveStatus(false, 0, 0);
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
                         ViEId(engine_id_, channel_id_),
                         "%s: Could not start sending", __FUNCTION__);
            return -1;
        }
    }
    else if (!enable && !rtp_rtcp_->SendingMedia())
    {
        // Not sending media and we're disabling keep alive
        rtp_rtcp_->ResetSendDataCountersRTP();
        if (rtp_rtcp_->SetSendingStatus(false) != 0)
        {
            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
                         ViEId(engine_id_, channel_id_),
                         "%s: Could not stop sending", __FUNCTION__);
            return -1;
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------
// GetKeepAliveStatus
// ----------------------------------------------------------------------------

int32_t ViEChannel::GetKeepAliveStatus(
                                             bool& enabled, int8_t& unknownPayloadType,
                                             uint16_t& deltaTransmitTimeMs)
{
    WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideo, ViEId(engine_id_, channel_id_),
                 "%s", __FUNCTION__);
    if (rtp_rtcp_->RTPKeepaliveStatus(&enabled, &unknownPayloadType,
                                      &deltaTransmitTimeMs) != 0)
    {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
                     ViEId(engine_id_, channel_id_),
                     "%s: Could not get RTP keepalive status", __FUNCTION__);
        return -1;
    }
    WEBRTC_TRACE(
                 cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideo, ViEId(engine_id_, channel_id_),
                 "%s: enabled = %d, unknownPayloadType = %d, deltaTransmitTimeMs = %ul",
                 __FUNCTION__, enabled, (int32_t) unknownPayloadType,
                 deltaTransmitTimeMs);
    
    return 0;
}
    
ReceiveStatisticsProxy* ViEChannel::GetReceiveStatisticsProxy()
{
	return receive_statistics_proxy_.get();
}

}  // namespace webrtc
