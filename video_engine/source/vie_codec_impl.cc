/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_codec_impl.h"

#include <list>

#include "engine_configurations.h"
#include "video_coding.h"
#include "../system_wrappers/include/logging.h"
#include "vie_errors.h"
#include "vie_capturer.h"
#include "vie_channel.h"
#include "vie_channel_manager.h"
#include "vie_defines.h"
#include "vie_encoder.h"
#include "vie_impl.h"
#include "vie_input_manager.h"
#include "vie_shared_data.h"

namespace cloopenwebrtc {

static void LogCodec(const VideoCodec& codec) {
  LOG(LS_INFO) << "CodecType " << codec.codecType
               << ", pl_type " << static_cast<int>(codec.plType)
               << ", resolution " << codec.width
               << " x " << codec.height
               << ", start br " << codec.startBitrate
               << ", min br " << codec.minBitrate
               << ", max br " << codec.maxBitrate
               << ", max fps " << static_cast<int>(codec.maxFramerate)
               << ", max qp " << codec.qpMax
               << ", number of streams "
               << static_cast<int>(codec.numberOfSimulcastStreams);
  if (codec.codecType == kVideoCodecVP8) {
    LOG(LS_INFO) << "VP8 specific settings";
    LOG(LS_INFO) << "pictureLossIndicationOn "
                 << codec.codecSpecific.VP8.pictureLossIndicationOn
                 << ", feedbackModeOn "
                 << codec.codecSpecific.VP8.feedbackModeOn
                 << ", complexity "
                 << codec.codecSpecific.VP8.complexity
                 << ", resilience "
                 << codec.codecSpecific.VP8.resilience
                 << ", numberOfTemporalLayers "
                 << static_cast<int>(
                     codec.codecSpecific.VP8.numberOfTemporalLayers)
                 << ", keyFrameinterval "
                 << codec.codecSpecific.VP8.keyFrameInterval;
    for (int idx = 0; idx < codec.numberOfSimulcastStreams; ++idx) {
      LOG(LS_INFO) << "Stream " << codec.simulcastStream[idx].width
                   << " x " << codec.simulcastStream[idx].height;
      LOG(LS_INFO) << "Temporal layers "
                   << static_cast<int>(
                       codec.simulcastStream[idx].numberOfTemporalLayers)
                   << ", min br "
                   << codec.simulcastStream[idx].minBitrate
                   << ", target br "
                   << codec.simulcastStream[idx].targetBitrate
                   << ", max br "
                   << codec.simulcastStream[idx].maxBitrate
                   << ", qp max "
                   << codec.simulcastStream[idx].qpMax;
    }
  } else if (codec.codecType == kVideoCodecH264) {
    LOG(LS_INFO) << "H264 specific settings";
    LOG(LS_INFO) << "profile: "
                 <<  codec.codecSpecific.H264.profile
                 << ", framedropping: "
                 << codec.codecSpecific.H264.frameDroppingOn
                 << ", keyFrameInterval: "
                 << codec.codecSpecific.H264.keyFrameInterval
                 << ", spslen: "
                 << codec.codecSpecific.H264.spsLen
                 << ", ppslen: "
                 << codec.codecSpecific.H264.ppsLen;
  }
}


ViECodec* ViECodec::GetInterface(VideoEngine* video_engine) {
#ifdef WEBRTC_VIDEO_ENGINE_CODEC_API
  if (!video_engine) {
    return NULL;
  }
  VideoEngineImpl* vie_impl = static_cast<VideoEngineImpl*>(video_engine);
  ViECodecImpl* vie_codec_impl = vie_impl;
  // Increase ref count.
  (*vie_codec_impl)++;
  return vie_codec_impl;
#else
  return NULL;
#endif
}

int ViECodecImpl::Release() {
  LOG(LS_INFO) << "ViECodec::Release.";
  // Decrease ref count.
  (*this)--;

  int32_t ref_count = GetCount();
  if (ref_count < 0) {
    LOG(LS_WARNING) << "ViECodec released too many times.";
    shared_data_->SetLastError(kViEAPIDoesNotExist);
    return -1;
  }
  return ref_count;
}

ViECodecImpl::ViECodecImpl(ViESharedData* shared_data)
    : shared_data_(shared_data) {
}

ViECodecImpl::~ViECodecImpl() {
}

int ViECodecImpl::NumberOfCodecs() const {
  // +2 because of FEC(RED and ULPFEC)
  return static_cast<int>((VideoCodingModule::NumberOfCodecs() + 2));
}

int ViECodecImpl::GetCodec(const unsigned char list_number,
                           VideoCodec& video_codec) const {
  if (list_number == VideoCodingModule::NumberOfCodecs()) {
    memset(&video_codec, 0, sizeof(VideoCodec));
    strcpy(video_codec.plName, "red");
    video_codec.codecType = kVideoCodecRED;
    video_codec.plType = VCM_RED_PAYLOAD_TYPE;
  } else if (list_number == VideoCodingModule::NumberOfCodecs() + 1) {
    memset(&video_codec, 0, sizeof(VideoCodec));
    strcpy(video_codec.plName, "ulpfec");
    video_codec.codecType = kVideoCodecULPFEC;
    video_codec.plType = VCM_ULPFEC_PAYLOAD_TYPE;
  } else if (VideoCodingModule::Codec(list_number, &video_codec) != VCM_OK) {
    shared_data_->SetLastError(kViECodecInvalidArgument);
    return -1;
  }
  return 0;
}

int ViECodecImpl::SetSendCodec(const int video_channel,
                               const VideoCodec& video_codec) {
  LOG(LS_INFO) << "SetSendCodec for channel " << video_channel;
  LogCodec(video_codec);
  if (!CodecValid(video_codec)) {
    // Error logged.
    shared_data_->SetLastError(kViECodecInvalidCodec);
    return -1;
  }

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }

  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  assert(vie_encoder);
  if (vie_encoder->Owner() != video_channel) {
    LOG_F(LS_ERROR) << "Receive only channel.";
    shared_data_->SetLastError(kViECodecReceiveOnlyChannel);
    return -1;
  }
  // Set a max_bitrate if the user hasn't set one.
  VideoCodec video_codec_internal;
  memcpy(&video_codec_internal, &video_codec, sizeof(VideoCodec));
  if (video_codec_internal.maxBitrate == 0) {
    // Max is one bit per pixel.
    video_codec_internal.maxBitrate = (video_codec_internal.width *
                                       video_codec_internal.height *
                                       video_codec_internal.maxFramerate)
                                       / 1000;
    LOG(LS_INFO) << "New max bitrate set " << video_codec_internal.maxBitrate;
  }

  if (video_codec_internal.startBitrate < video_codec_internal.minBitrate) {
    video_codec_internal.startBitrate = video_codec_internal.minBitrate;
  }
  if (video_codec_internal.startBitrate > video_codec_internal.maxBitrate) {
    video_codec_internal.startBitrate = video_codec_internal.maxBitrate;
  }

  if (video_codec_internal.numberOfSimulcastStreams == 2) {
	  if (vie_channel->GetSSRCNum() != 2) {//only one resolution in vie_channel
		  video_codec_internal.numberOfSimulcastStreams = 0;
	  }
	  else//get small resolution
	  {
		  unsigned int ssrc_slave = 0;
		  if (vie_channel->GetLocalSSRC(0, &ssrc_slave) != 0) {//small resolution number is 0
			  LOG_F(LS_ERROR) << "Could not get ssrc slave.";
			  return -1;
		  }
		  else {
			  //set simulcast
			  ResolutionIndex resolution_index = static_cast<ResolutionIndex>(ssrc_slave & 0x0F);
			  struct ResolutionInst resolution_slave;
			  resolution_slave.index = resolution_index;
			  if (vie_channel->GetResolution(resolution_slave) != 0) {
				  LOG_F(LS_ERROR) << "Can not get resolution of slave ssrc " << ssrc_slave;
				  return -1;
			  }

			  resolution_slave.targetBitrate = resolution_slave.width*resolution_slave.height * 3 * 15 * 0.07 / 1000;

			  video_codec_internal.simulcastStream[0].maxBitrate = resolution_slave.targetBitrate*1.5;
			  video_codec_internal.simulcastStream[0].width = resolution_slave.width;
			  video_codec_internal.simulcastStream[0].height = resolution_slave.height;
			  video_codec_internal.simulcastStream[0].numberOfTemporalLayers = 1;
			  video_codec_internal.simulcastStream[0].targetBitrate = resolution_slave.targetBitrate;

			  video_codec_internal.simulcastStream[1].maxBitrate = video_codec_internal.maxBitrate;
			  video_codec_internal.simulcastStream[1].width = video_codec_internal.width;
			  video_codec_internal.simulcastStream[1].height = video_codec_internal.height;
			  video_codec_internal.simulcastStream[1].numberOfTemporalLayers = 1;
			  video_codec_internal.simulcastStream[1].targetBitrate = video_codec_internal.startBitrate - resolution_slave.targetBitrate;
		  }
	  }
  }

  VideoCodec encoder;
  vie_encoder->GetEncoder(&encoder);

  // Make sure to generate a new SSRC if the codec type and/or resolution has
  // changed. This won't have any effect if the user has set an SSRC.
  bool new_rtp_stream = false;
  if (encoder.codecType != video_codec_internal.codecType) {
    new_rtp_stream = true;
  }

  ViEInputManagerScoped is(*(shared_data_->input_manager()));

  // Stop the media flow while reconfiguring.
  vie_encoder->Pause();

  if (video_codec_internal.numberOfSimulcastStreams == 2 && video_codec_internal.codecType==kVideoCodecVP8)
  {
	  video_codec_internal.codecSpecific.VP8.automaticResizeOn = false;
  }

  if (vie_encoder->SetEncoder(video_codec_internal) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }

  if (video_codec_internal.numberOfSimulcastStreams > 0)
  {
	  vie_channel->SetDefaultSimulcatRtpRtcp(vie_encoder->DefaultSimulcastRtpRtcp());
  }

  // Give the channel(s) the new information.
  ChannelList channels;
  cs.ChannelsUsingViEEncoder(video_channel, &channels);
  for (ChannelList::iterator it = channels.begin(); it != channels.end();
       ++it) {
    bool ret = true;
    if ((*it)->SetSendCodec(video_codec_internal, new_rtp_stream) != 0) {
      ret = false;
    }
    if (!ret) {
      shared_data_->SetLastError(kViECodecUnknownError);
      return -1;
    }
  }

  // TODO(mflodman) Break out this part in GetLocalSsrcList().
  // Update all SSRCs to ViEEncoder.
  std::list<unsigned int> ssrcs;
  if (video_codec_internal.numberOfSimulcastStreams == 0) {
    unsigned int ssrc = 0;
    if (vie_channel->GetLocalSSRC(0, &ssrc) != 0) {
      LOG_F(LS_ERROR) << "Could not get ssrc.";
    }
    ssrcs.push_back(ssrc);
  } else {
    for (int idx = 0; idx < video_codec_internal.numberOfSimulcastStreams;
         ++idx) {
      unsigned int ssrc = 0;
      if (vie_channel->GetLocalSSRC(idx, &ssrc) != 0) {
        LOG_F(LS_ERROR) << "Could not get ssrc for stream " << idx;
      }
      ssrcs.push_back(ssrc);
    }
  }
  vie_encoder->SetSsrcs(ssrcs);
  shared_data_->channel_manager()->UpdateSsrcs(video_channel, ssrcs);

  // Update the protection mode, we might be switching NACK/FEC.
  vie_encoder->UpdateProtectionMethod(vie_encoder->nack_enabled());

  // Get new best format for frame provider.
  ViEFrameProviderBase* frame_provider = is.FrameProvider(vie_encoder);
  if (frame_provider) {
    frame_provider->FrameCallbackChanged();
  }
  // Restart the media flow
  if (new_rtp_stream) {
    // Stream settings changed, make sure we get a key frame.
    vie_encoder->SendKeyFrame();
  }
  vie_encoder->Restart();
  return 0;
}

int ViECodecImpl::SetVideoSendQmMode(int channel_id, int mode) {
    LOG(LS_INFO) << "SetSendCodec for channel " << channel_id;

    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEEncoder* vie_encoder = cs.Encoder(channel_id);
    
    if(vie_encoder == NULL) {
        LOG(LS_INFO) << "Error, vie_encoder is null.";
        return -1;
    }
    
    if (vie_encoder->Owner() != channel_id) {
        LOG_F(LS_ERROR) << "Receive only channel.";
        shared_data_->SetLastError(kViECodecReceiveOnlyChannel);
        return -1;
    }
    vie_encoder->SetVideoQualityMode(mode);
    return 0;
}


int32_t ViECodecImpl::AddI420FrameCallback(const int video_channel, ECMedia_I420FrameCallBack callback) {
    LOG(LS_INFO) << "SetSendCodec for channel " << video_channel;
    if(!callback) {
        LOG(LS_INFO) << "Error, i420 frame callback is null.";
    }
    
    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEEncoder* vie_encoder = cs.Encoder(video_channel);
    
    if(vie_encoder == NULL) {
        LOG(LS_INFO) << "Error, vie_encoder is null.";
        return -1;
    }
    
    if (vie_encoder->Owner() != video_channel) {
        LOG_F(LS_ERROR) << "Receive only channel.";
        shared_data_->SetLastError(kViECodecReceiveOnlyChannel);
        return -1;
    }
    
    return vie_encoder->AddI420FrameCallback(callback);
}
    
int ViECodecImpl::SetFrameScaleType(const int video_channel,
                                    FrameScaleType frame_scale_type) {
    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEChannel* vie_channel = cs.Channel(video_channel);
    if (!vie_channel) {
        shared_data_->SetLastError(kViECodecInvalidChannelId);
        return -1;
    }
    
    ViEEncoder* vie_encoder = cs.Encoder(video_channel);
    vie_encoder->setFrameScaleType(frame_scale_type);
    return 0;
}
    
int ViECodecImpl::GetSendCodec(const int video_channel,
                               VideoCodec& video_codec) const {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  return vie_encoder->GetEncoder(&video_codec);
}

int ViECodecImpl::SetReceiveCodec(const int video_channel,
                                  const VideoCodec& video_codec) {
  LOG(LS_INFO) << "SetReceiveCodec for channel " << video_channel;
  LOG(LS_INFO) << "Codec type " << video_codec.codecType
               << ", payload type " << static_cast<int>(video_codec.plType);

  if (CodecValid(video_codec) == false) {
    shared_data_->SetLastError(kViECodecInvalidCodec);
    return -1;
  }

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }

  if (vie_channel->SetReceiveCodec(video_codec) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }

  VideoCodec video_codec_second;
  if (!stricmp(video_codec.plName, "VP8")) {//add h264 codec
	  memcpy(&video_codec_second, &video_codec, sizeof(VideoCodec));
	  memset(video_codec_second.plName, 0, cloopenwebrtc::kPayloadNameSize);
	  memcpy(video_codec_second.plName, "H264", 4);
	  video_codec_second.plType = 96;
	  video_codec_second.codecType = cloopenwebrtc::kVideoCodecH264;

	  if (vie_channel->SetReceiveCodec(video_codec_second) != 0) {
		  shared_data_->SetLastError(kViECodecUnknownError);
		  return -1;
	  }
  }
  else if (!stricmp(video_codec.plName, "H264")) {//add vp8 codec
	  memcpy(&video_codec_second, &video_codec, sizeof(VideoCodec));
	  memset(video_codec_second.plName, 0, cloopenwebrtc::kPayloadNameSize);
	  memcpy(video_codec_second.plName, "VP8", 3);
	  video_codec_second.plType = 120;
	  video_codec_second.codecType = cloopenwebrtc::kVideoCodecVP8;

	  if (vie_channel->SetReceiveCodec(video_codec_second) != 0) {
		  shared_data_->SetLastError(kViECodecUnknownError);
		  return -1;
	  }
  }
  else 
  {
  }

  LOG(LS_INFO) << "second Codec type " << video_codec_second.codecType
	  << ", second payload type " << static_cast<int>(video_codec_second.plType);

  return 0;
}

int ViECodecImpl::GetReceiveCodec(const int video_channel,
                                  VideoCodec& video_codec) const {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }

  if (vie_channel->GetReceiveCodec(&video_codec) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::GetCodecConfigParameters(
  const int video_channel,
  unsigned char config_parameters[kConfigParameterSize],
  unsigned char& config_parameters_size) const {
  LOG(LS_INFO) << "GetCodecConfigParameters " << video_channel;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }

  if (vie_encoder->GetCodecConfigParameters(config_parameters,
                                            config_parameters_size) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::SetImageScaleStatus(const int video_channel,
                                      const bool enable) {
  LOG(LS_INFO) << "SetImageScaleStates for channel " << video_channel
               << ", enable: " << enable;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }

  if (vie_encoder->ScaleInputImage(enable) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::GetSendCodecStastistics(const int video_channel,
                                          unsigned int& key_frames,
                                          unsigned int& delta_frames) const {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }

  if (vie_encoder->SendCodecStatistics(&key_frames, &delta_frames) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::GetReceiveCodecStastistics(const int video_channel,
                                             unsigned int& key_frames,
                                             unsigned int& delta_frames) const {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_channel->ReceiveCodecStatistics(&key_frames, &delta_frames) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::GetReceiveSideDelay(const int video_channel,
                                      int* delay_ms) const {
  assert(delay_ms != NULL);

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  *delay_ms = vie_channel->ReceiveDelay();
  if (*delay_ms < 0) {
    return -1;
  }
  return 0;
}


int ViECodecImpl::GetCodecTargetBitrate(const int video_channel,
                                        unsigned int* bitrate) const {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  return vie_encoder->CodecTargetBitrate(static_cast<uint32_t*>(bitrate));
}

int ViECodecImpl::GetNumDiscardedPackets(int video_channel) const {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  return static_cast<int>(vie_channel->DiscardedPackets());
}

int ViECodecImpl::SetKeyFrameRequestCallbackStatus(const int video_channel,
                                                   const bool enable) {
  LOG(LS_INFO) << "SetKeyFrameRequestCallbackStatus for " << video_channel
               << ", enable " << enable;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_channel->EnableKeyFrameRequestCallback(enable) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::SetSignalKeyPacketLossStatus(const int video_channel,
                                               const bool enable,
                                               const bool only_key_frames) {
  LOG(LS_INFO) << "SetSignalKeyPacketLossStatus for " << video_channel
               << "enable, " << enable
               << ", only key frames " << only_key_frames;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_channel->SetSignalPacketLossStatus(enable, only_key_frames) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::RegisterEncoderObserver(const int video_channel,
                                          ViEEncoderObserver& observer) {
  LOG(LS_INFO) << "RegisterEncoderObserver for channel " << video_channel;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_encoder->RegisterCodecObserver(&observer) != 0) {
    shared_data_->SetLastError(kViECodecObserverAlreadyRegistered);
    return -1;
  }
  return 0;
}

int ViECodecImpl::DeregisterEncoderObserver(const int video_channel) {
  LOG(LS_INFO) << "DeregisterEncoderObserver for channel " << video_channel;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_encoder->RegisterCodecObserver(NULL) != 0) {
    shared_data_->SetLastError(kViECodecObserverNotRegistered);
    return -1;
  }
  return 0;
}

int ViECodecImpl::RegisterDecoderObserver(const int video_channel,
                                          ViEDecoderObserver& observer) {
  LOG(LS_INFO) << "RegisterDecoderObserver for channel " << video_channel;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_channel->RegisterCodecObserver(&observer) != 0) {
    shared_data_->SetLastError(kViECodecObserverAlreadyRegistered);
    return -1;
  }
  return 0;
}

int ViECodecImpl::DeregisterDecoderObserver(const int video_channel) {
  LOG(LS_INFO) << "DeregisterDecodeObserver for channel " << video_channel;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_channel->RegisterCodecObserver(NULL) != 0) {
    shared_data_->SetLastError(kViECodecObserverNotRegistered);
    return -1;
  }
  return 0;
}

int ViECodecImpl::SendKeyFrame(const int video_channel) {
  LOG(LS_INFO) << "SendKeyFrame on channel " << video_channel;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_encoder->SendKeyFrame() != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::WaitForFirstKeyFrame(const int video_channel,
                                       const bool wait) {
  LOG(LS_INFO) << "WaitForFirstKeyFrame for channel " << video_channel
               << ", wait " << wait;

  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return -1;
  }
  if (vie_channel->WaitForKeyFrame(wait) != 0) {
    shared_data_->SetLastError(kViECodecUnknownError);
    return -1;
  }
  return 0;
}

int ViECodecImpl::StartDebugRecording(int video_channel,
                                      const char* file_name_utf8) {
  LOG(LS_INFO) << "StartDebugRecording for channel " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    return -1;
  }
  return vie_encoder->StartDebugRecording(file_name_utf8);
}

int ViECodecImpl::StopDebugRecording(int video_channel) {
  LOG(LS_INFO) << "StopDebugRecording for channel " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    return -1;
  }
  return vie_encoder->StopDebugRecording();
}

void ViECodecImpl::SuspendBelowMinBitrate(int video_channel) {
  LOG(LS_INFO) << "SuspendBelowMinBitrate for channel " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    return;
  }
  vie_encoder->SuspendBelowMinBitrate();
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    return;
  }
  // Must enable pacing when enabling SuspendBelowMinBitrate. Otherwise, no
  // padding will be sent when the video is suspended so the video will be
  // unable to recover.
  vie_channel->SetTransmissionSmoothingStatus(true);
}

bool ViECodecImpl::GetSendSideDelay(int video_channel, int* avg_delay_ms,
                                   int* max_delay_ms) const {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViECodecInvalidChannelId);
    return false;
  }
  return vie_channel->GetSendSideDelay(avg_delay_ms, max_delay_ms);
}

bool ViECodecImpl::CodecValid(const VideoCodec& video_codec) {
  // Check pl_name matches codec_type.
  if (video_codec.codecType == kVideoCodecRED) {
#if defined(WIN32)
    if (_strnicmp(video_codec.plName, "red", 3) == 0) {
#else
    if (strncasecmp(video_codec.plName, "red", 3) == 0) {
#endif
      // We only care about the type and name for red.
      return true;
    }
    LOG_F(LS_ERROR) << "Invalid RED configuration.";
    return false;
  } else if (video_codec.codecType == kVideoCodecULPFEC) {
#if defined(WIN32)
    if (_strnicmp(video_codec.plName, "ULPFEC", 6) == 0) {
#else
    if (strncasecmp(video_codec.plName, "ULPFEC", 6) == 0) {
#endif
      // We only care about the type and name for ULPFEC.
      return true;
    }
    LOG_F(LS_ERROR) << "Invalid ULPFEC configuration.";
    return false;
  } else if ((video_codec.codecType == kVideoCodecVP8 &&
              strncmp(video_codec.plName, "VP8", 4) == 0) ||
			  (video_codec.codecType == kVideoCodecVP9 &&
			  strncmp(video_codec.plName, "VP9", 4) == 0) ||
             (video_codec.codecType == kVideoCodecI420 &&
              strncmp(video_codec.plName, "I420", 4) == 0) ||
             (video_codec.codecType == kVideoCodecH264 &&
              strncmp(video_codec.plName, "H264", 4) == 0) ||
             (video_codec.codecType == kVideoCodecH264HIGH &&
              strncmp(video_codec.plName, "H264", 4) == 0) ) {
    // OK.
  } else if (video_codec.codecType != kVideoCodecGeneric) {
    LOG(LS_ERROR) << "Codec type and name mismatch.";
    return false;
  }

  if (video_codec.plType == 0 || video_codec.plType > 127) {
    LOG(LS_ERROR) << "Invalif payload type: " << video_codec.plType;
    return false;
  }

  if (video_codec.width > kViEMaxCodecWidth ||
      video_codec.height > kViEMaxCodecHeight) {
    LOG(LS_ERROR) << "Invalid codec resolution " << video_codec.width
                  << " x " << video_codec.height;
    return false;
  }

  if (video_codec.startBitrate < kViEMinCodecBitrate) {
    LOG(LS_ERROR) << "Invalid start bitrate.";
    return false;
  }
  if (video_codec.minBitrate < kViEMinCodecBitrate) {
    LOG(LS_ERROR) << "Invalid min bitrate.";
    return false;
  }
  return true;
}

int ViECodecImpl::SetKeyFrameRequestCb(const int video_channel, bool isVideoConf,onEcMediaRequestKeyFrame cb)
{
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    LOG(LS_ERROR) << __FUNCTION__ << ": No channel " << video_channel;
      shared_data_->SetLastError(kViECodecInvalidChannelId);
      return -1;
  }
  if (vie_channel->SetRequestKeyFrameCb(isVideoConf,cb) != 0) {
      shared_data_->SetLastError(kViECodecUnknownError);
      return -1;
  }
  return 0;
}
      
int ViECodecImpl::RegisterCaptureObserver(const int video_channel, void *capture, const int capture_id){
  LOG(LS_INFO) << "RegisterCaptureObserver for channel " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
      return -1;
  }
  return vie_encoder->RegisterCaptureObserver(capture, capture_id);
  
}

int ViECodecImpl::DeRegisterCaptureObserver(const int video_channel){
  LOG(LS_INFO) << "DeRegisterCaptureObserver for channel " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
      return -1;
  }
  return vie_encoder->DeRegisterCaptureObserver();
};
      
void ViECodecImpl::EnableIOSH264HardEncode(bool state) {
  VideoCodingModule::EnableIOSH264HardEncode(state);
}

}  // namespace webrtc
