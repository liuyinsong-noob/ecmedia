/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef SDK_OBJC_NATIVE_SRC_OBJC_VIDEO_RENDERER_H_
#define SDK_OBJC_NATIVE_SRC_OBJC_VIDEO_RENDERER_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "sdk/ecmedia/video_renderer.h"

@protocol RTCVideoRenderer;

namespace webrtc {

class ObjCVideoRendererImpl : public VideoRenderer {
 public:
  ObjCVideoRendererImpl(void* parent, int render_mode, bool mirror, webrtc::VideoTrackInterface* track,
                        rtc::Thread* worker_thread_, VideoRenderType type);
  ~ObjCVideoRendererImpl();
  void OnFrame(const VideoFrame& nativeVideoFrame) override;
  int StartRender() override {return 0;}
  int StopRender() override {return 0;}
  int UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render) override;
  void* WindowPtr() override{return parent_;}
 
 private:
  id<RTCVideoRenderer> GetWindwoRenderPtr(void* remoteView);
  id<RTCVideoRenderer> renderer_;
  CGSize size_;
  webrtc::VideoTrackInterface* track_;
  int render_mode_;
  void* parent_;
  bool mirror_;
  VideoRenderType type_;
  rtc::Thread* worker_thread_;
 
};

}  // namespace webrtc

#endif  // SDK_OBJC_NATIVE_SRC_OBJC_VIDEO_RENDERER_H_
