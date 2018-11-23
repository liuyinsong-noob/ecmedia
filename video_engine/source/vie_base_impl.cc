/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_base_impl.h"

#include <string>
#include <utility>

#include "engine_configurations.h"
#include "rtp_rtcp.h"
#include "video_coding.h"
#include "video_processing.h"
#include "video_render.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
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

#include "screen_capturer.h"

#ifdef WEBRTC_ANDROID
#include "video_capture_factory.h"
#endif

#ifndef WEBRTC_EXTERNAL_TRANSPORT
#include "udp_transport.h"
#endif

#include "../system_wrappers/include/Trace.h"

// Global counter to get an id for each new ViE instance.
static WebRtc_Word32 g_vie_active_instance_counter = 0;

namespace yuntongxunwebrtc {

ViEBase* ViEBase::GetInterface(VideoEngine* video_engine) {
  if (!video_engine) {
    return NULL;
  }
  VideoEngineImpl* vie_impl = static_cast<VideoEngineImpl*>(video_engine);
  ViEBaseImpl* vie_base_impl = vie_impl;
  (*vie_base_impl)++;  // Increase ref count.

  return vie_base_impl;
}

int ViEBaseImpl::Release() {
  (*this)--;  // Decrease ref count.

  int32_t ref_count = GetCount();
  if (ref_count < 0) {
    LOG(LS_WARNING) << "ViEBase released too many times.";
    return -1;
  }
  return ref_count;
}

ViEBaseImpl::ViEBaseImpl(const Config& config)
    : shared_data_(config) {
}


ViEBaseImpl::~ViEBaseImpl() {}

int ViEBaseImpl::Init() {
  return 0;
}

int ViEBaseImpl::SetVoiceEngine(VoiceEngine* voice_engine) {
  LOG_F(LS_INFO) << "SetVoiceEngine";
  if (shared_data_.channel_manager()->SetVoiceEngine(voice_engine) != 0) {
    shared_data_.SetLastError(kViEBaseVoEFailure);
    return -1;
  }
  return 0;
}

int ViEBaseImpl::RegisterCpuOveruseObserver(int video_channel,
                                            CpuOveruseObserver* observer) {
  LOG_F(LS_INFO) << "RegisterCpuOveruseObserver on channel " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  assert(vie_encoder);

  ViEInputManagerScoped is(*(shared_data_.input_manager()));
  ViEFrameProviderBase* provider = is.FrameProvider(vie_encoder);
  if (provider) {
    ViECapturer* capturer = is.Capture(provider->Id());
    assert(capturer);
    capturer->RegisterCpuOveruseObserver(observer);
  }

  shared_data_.overuse_observers()->insert(
      std::pair<int, CpuOveruseObserver*>(video_channel, observer));
  return 0;
}


int ViEBaseImpl::DeregisterCpuOveruseObserver(int channel)
{
	LOG_F(LS_INFO) << "DeregisterCpuOveruseObserver on channel " << channel;
	std::map<int, CpuOveruseObserver*>::iterator it = 
		shared_data_.overuse_observers()->find(channel);
	if (it != shared_data_.overuse_observers()->end()) {
		shared_data_.overuse_observers()->erase(it);
	}
	return 0;
}

int ViEBaseImpl::SetCpuOveruseOptions(int video_channel,
                                      const CpuOveruseOptions& options) {
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  assert(vie_encoder);

  ViEInputManagerScoped is(*(shared_data_.input_manager()));
  ViEFrameProviderBase* provider = is.FrameProvider(vie_encoder);
  if (provider) {
    ViECapturer* capturer = is.Capture(provider->Id());
    if (capturer) {
      capturer->SetCpuOveruseOptions(options);
      return 0;
    }
  }
  return -1;
}

int ViEBaseImpl::GetCpuOveruseMetrics(int video_channel,
                                      CpuOveruseMetrics* metrics) {
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  assert(vie_encoder);

  ViEInputManagerScoped is(*(shared_data_.input_manager()));
  ViEFrameProviderBase* provider = is.FrameProvider(vie_encoder);
  if (provider) {
    ViECapturer* capturer = is.Capture(provider->Id());
    if (capturer) {
      capturer->GetCpuOveruseMetrics(metrics);
      return 0;
    }
  }
  return -1;
}

void ViEBaseImpl::RegisterSendSideDelayObserver(
    int channel, SendSideDelayObserver* observer) {
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(channel);
  assert(vie_channel);
  vie_channel->RegisterSendSideDelayObserver(observer);
}

int ViEBaseImpl::CreateChannel(int& video_channel) {  // NOLINT	
  return CreateChannel(video_channel, static_cast<const Config*>(NULL));
}

int ViEBaseImpl::CreateChannel(int& video_channel,  // NOLINT
                               const Config* config) {
  if (shared_data_.channel_manager()->CreateChannel(&video_channel,
                                                    config) == -1) {
    video_channel = -1;
    shared_data_.SetLastError(kViEBaseChannelCreationFailed);
    return -1;
  }
  
  LOG(LS_INFO) << "Video channel created: " << video_channel;
  return 0;
}

int ViEBaseImpl::CreateChannel(int& video_channel,  // NOLINT
                               int original_channel) {
  return CreateChannel(video_channel, original_channel, true);
}

int ViEBaseImpl::CreateReceiveChannel(int& video_channel,  // NOLINT
                                      int original_channel) {
  return CreateChannel(video_channel, original_channel, false);
}

int ViEBaseImpl::DeleteChannel(const int video_channel) {
  {
    ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
    ViEChannel* vie_channel = cs.Channel(video_channel);
    if (!vie_channel) {
      shared_data_.SetLastError(kViEBaseInvalidChannelId);
      return -1;
    }

    // Deregister the ViEEncoder if no other channel is using it.
    ViEEncoder* vie_encoder = cs.Encoder(video_channel);
    if (cs.ChannelUsingViEEncoder(video_channel) == false) {
      ViEInputManagerScoped is(*(shared_data_.input_manager()));
      ViEFrameProviderBase* provider = is.FrameProvider(vie_encoder);
      if (provider) {
        provider->DeregisterFrameCallback(vie_encoder);
      }
    }
  }

  if (shared_data_.channel_manager()->DeleteChannel(video_channel) == -1) {
    shared_data_.SetLastError(kViEBaseUnknownError);
    return -1;
  }
  LOG(LS_INFO) << "ViEBaseImpl::DeleteChannel Channel deleted " << video_channel;
  return 0;
}

int ViEBaseImpl::ConnectAudioChannel(const int video_channel,
                                     const int audio_channel) {
  LOG_F(LS_INFO) << "ConnectAudioChannel, video channel " << video_channel
                 << ", audio channel " << audio_channel;
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  if (!cs.Channel(video_channel)) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }

  if (shared_data_.channel_manager()->ConnectVoiceChannel(video_channel,
                                                          audio_channel) != 0) {
    shared_data_.SetLastError(kViEBaseVoEFailure);
    return -1;
  }
  return 0;
}

int ViEBaseImpl::DisconnectAudioChannel(const int video_channel) {
  LOG_F(LS_INFO) << "DisconnectAudioChannel " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  if (!cs.Channel(video_channel)) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }

  if (shared_data_.channel_manager()->DisconnectVoiceChannel(
      video_channel) != 0) {
    shared_data_.SetLastError(kViEBaseVoEFailure);
    return -1;
  }
  return 0;
}

int ViEBaseImpl::StartSend(const int video_channel) {
  LOG_F(LS_INFO) << "StartSend: " << video_channel;
  ViEChannelManager *channel_manager = shared_data_.channel_manager();
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }

  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  assert(vie_encoder != NULL);
  if (vie_encoder->Owner() != video_channel) {
    LOG_F(LS_ERROR) <<  "Can't start send on a receive only channel.";
    shared_data_.SetLastError(kViEBaseReceiveOnlyChannel);
    return -1;
  }

  // Pause and trigger a key frame.
  vie_encoder->Pause();
  int32_t error = vie_channel->StartSend();
  if (error != 0) {
    vie_encoder->Restart();
    if (error == kViEBaseAlreadySending) {
      shared_data_.SetLastError(kViEBaseAlreadySending);
    }
    LOG_F(LS_ERROR) << "Could not start sending " << video_channel;
    shared_data_.SetLastError(kViEBaseUnknownError);
    return -1;
  }
  vie_encoder->SendKeyFrame();
  vie_encoder->Restart();
  channel_manager->UpdateNetworkState(video_channel, true);
  return 0;
}

int ViEBaseImpl::StopSend(const int video_channel) {
  LOG_F(LS_INFO) << "StopSend " << video_channel;
  ViEChannelManager *channel_manager = shared_data_.channel_manager();
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }
  channel_manager->UpdateNetworkState(video_channel, false);
  int32_t error = vie_channel->StopSend();
  if (error != 0) {
    if (error == kViEBaseNotSending) {
      shared_data_.SetLastError(kViEBaseNotSending);
    } else {
      LOG_F(LS_ERROR) << "Could not stop sending " << video_channel;
      shared_data_.SetLastError(kViEBaseUnknownError);
    }
    return -1;
  }
  return 0;
}

int ViEBaseImpl::StartReceive(const int video_channel) {
  LOG_F(LS_INFO) << "StartReceive " << video_channel;

  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }
  if (vie_channel->StartReceive() != 0) {
    shared_data_.SetLastError(kViEBaseUnknownError);
    return -1;
  }
  return 0;
}

int ViEBaseImpl::StopReceive(const int video_channel) {
  LOG_F(LS_INFO) << "StopReceive " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }
  if (vie_channel->StopReceive() != 0) {
    shared_data_.SetLastError(kViEBaseUnknownError);
    return -1;
  }
  return 0;
}

int ViEBaseImpl::GetVersion(char version[1024]) {
  assert(version != NULL);
  strcpy(version, "VideoEngine 40");
  return 0;
}

int ViEBaseImpl::LastError() {
  return shared_data_.LastErrorInternal();
}

int ViEBaseImpl::CreateChannel(int& video_channel,  // NOLINT
                               int original_channel, bool sender) {
  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  if (!cs.Channel(original_channel)) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return -1;
  }

  if (shared_data_.channel_manager()->CreateChannel(&video_channel,
                                                    original_channel,
                                                    sender) == -1) {
    video_channel = -1;
    shared_data_.SetLastError(kViEBaseChannelCreationFailed);
    return -1;
  }
  LOG_F(LS_INFO) << "VideoChannel created: " << video_channel
                 << ", base channel " << original_channel
                 << ", is send channel : " << sender;
  return 0;
}

void ViEBaseImpl::RegisterSendStatisticsProxy(
    int channel,
    SendStatisticsProxy* send_statistics_proxy) {
  LOG_F(LS_VERBOSE) << "RegisterSendStatisticsProxy on channel " << channel;

  ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
  ViEChannel* vie_channel = cs.Channel(channel);
  if (!vie_channel) {
    shared_data_.SetLastError(kViEBaseInvalidChannelId);
    return;
  }
  ViEEncoder* vie_encoder = cs.Encoder(channel);
    assert(vie_encoder);
  
  vie_channel->RegisterReceiveRtcpPacketTypeCounterObserver(send_statistics_proxy);
  vie_channel->RegisterSendFrameCountObserver(send_statistics_proxy);
  vie_channel->RegisterSendSideDelayObserver(send_statistics_proxy);
  //rtp/rtcp
  vie_channel->RegisterSendChannelRtpStatisticsCallback(send_statistics_proxy);
 // vie_channel->RegisterSendChannelRtcpStatisticsCallback(send_statistics_proxy);
  vie_channel->RegisterSendBitrateObserver(send_statistics_proxy);
}

SendStatisticsProxy* ViEBaseImpl::GetSendStatisticsProxy(const int video_channel)
{
	ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
	ViEChannel* vie_channel = cs.Channel(video_channel);
	if (!vie_channel) {
		shared_data_.SetLastError(kViEBaseInvalidChannelId);
		return NULL;
	}

	ViEEncoder* vie_encoder = cs.Encoder(video_channel);
	SendStatisticsProxy *p_sendStats = vie_encoder->GetSendStatisticsProxy();
	return p_sendStats;
}

ReceiveStatisticsProxy* ViEBaseImpl::GetReceiveStatisticsProxy(const int video_channel)
{
	ViEChannelManagerScoped cs(*(shared_data_.channel_manager()));
	ViEChannel* vie_channel = cs.Channel(video_channel);
	if (!vie_channel) {
		shared_data_.SetLastError(kViEBaseInvalidChannelId);
		return NULL;
	}
	ReceiveStatisticsProxy *p_receiveStats = vie_channel->GetReceiveStatisticsProxy();
	return p_receiveStats;
}

//Add by dingxf
int ViEBaseImpl::AddRemoteI420FrameCallback(const int video_channel, ECMedia_I420FrameCallBack callback)
{
	if (shared_data_.channel_manager())
	{
		return shared_data_.channel_manager()->AddRemoteI420FrameCallback(video_channel, callback);
	}
	return -1;
}

int VideoEngine::SetAndroidObjects(void* javaVM, void* env, void* javaContext) {
	WEBRTC_TRACE(kTraceApiCall, kTraceVideo, g_vie_active_instance_counter,
		"SetAndroidObjects()");

int ret = 0;
#ifdef WEBRTC_ANDROID
	if (VideoCaptureFactory::SetAndroidObjects(javaVM, env, javaContext) != 0) {
		WEBRTC_TRACE(kTraceError, kTraceVideo, g_vie_active_instance_counter,
			"Could not set capture module Android objects");
		ret = -1;
	}
	if (VideoRender::SetAndroidObjects(javaVM, env, javaContext) != 0) {
		WEBRTC_TRACE(kTraceError, kTraceVideo, g_vie_active_instance_counter,
			"Could not set render module Android objects");
		ret = -1;
	}
#ifdef ENABLE_SCREEN_SHARE
  if (ScreenCapturer::SetAndroidObjects(javaVM, env, javaContext) != 0) {
		WEBRTC_TRACE(kTraceError, kTraceVideo, g_vie_active_instance_counter,
			"Could not set ScreenCapture Android objects");
		ret = -1;
  }
#endif
	return ret;
#else
	WEBRTC_TRACE(kTraceError, kTraceVideo, g_vie_active_instance_counter,
		"WEBRTC_ANDROID not defined for VideoEngine::SetAndroidObjects");
	return -1;
#endif
}
int ViEBaseImpl::ReleaseAllUdp(){
    return UdpTransport::ReleaseAll();
}
}  // namespace webrtc
