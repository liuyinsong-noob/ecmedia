/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_VIDEO_RECEIVE_STREAM_H_
#define WEBRTC_VIDEO_VIDEO_RECEIVE_STREAM_H_

#include <vector>

#include "cloopenwebrtc/call.h"
#include "cloopenwebrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "cloopenwebrtc/modules/video_render/include/video_render_defines.h"
#include "cloopenwebrtc/system_wrappers/interface/clock.h"
#include "cloopenwebrtc/system_wrappers/interface/scoped_ptr.h"
#include "cloopenwebrtc/video/encoded_frame_callback_adapter.h"
#include "cloopenwebrtc/video/receive_statistics_proxy.h"
#include "cloopenwebrtc/video/transport_adapter.h"
#include "cloopenwebrtc/video_engine/include/vie_render.h"
#include "cloopenwebrtc/video_receive_stream.h"

namespace cloopenwebrtc {

class VideoEngine;
class ViEBase;
class ViECodec;
class ViEExternalCodec;
class ViEImageProcess;
class ViENetwork;
class ViERender;
class ViERTP_RTCP;
class VoiceEngine;

namespace internal {

class VideoReceiveStream : public cloopenwebrtc::VideoReceiveStream,
                           public I420FrameCallback,
                           public VideoRenderCallback {
 public:
  VideoReceiveStream(cloopenwebrtc::VideoEngine* video_engine,
                     const VideoReceiveStream::Config& config,
                     newapi::Transport* transport,
                     cloopenwebrtc::VoiceEngine* voice_engine,
                     int base_channel);
  virtual ~VideoReceiveStream();

  virtual void Start() OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual Stats GetStats() const OVERRIDE;

  // Overrides I420FrameCallback.
  virtual void FrameCallback(I420VideoFrame* video_frame) OVERRIDE;

  // Overrides VideoRenderCallback.
  virtual int32_t RenderFrame(const uint32_t stream_id,
                              I420VideoFrame& video_frame) OVERRIDE;

  void SignalNetworkState(Call::NetworkState state);

  virtual bool DeliverRtcp(const uint8_t* packet, size_t length);
  virtual bool DeliverRtp(const uint8_t* packet, size_t length);

 private:
  void SetRtcpMode(newapi::RtcpMode mode);

  TransportAdapter transport_adapter_;
  EncodedFrameCallbackAdapter encoded_frame_proxy_;
  const VideoReceiveStream::Config config_;
  Clock* const clock_;

  ViEBase* video_engine_base_;
  ViECodec* codec_;
  ViEExternalCodec* external_codec_;
  ViENetwork* network_;
  ViERender* render_;
  ViERTP_RTCP* rtp_rtcp_;
  ViEImageProcess* image_process_;

  scoped_ptr<ReceiveStatisticsProxy> stats_proxy_;

  int channel_;
};
}  // namespace internal
}  // namespace webrtc

#endif  // WEBRTC_VIDEO_VIDEO_RECEIVE_STREAM_H_
