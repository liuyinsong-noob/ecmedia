/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RENDER_MANAGER_H_
#define RENDER_MANAGER_H_

#include <memory>
#include <string>

#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/thread_checker.h"
#import "sdk/objc/components/renderer/opengl/RTCEAGLVideoView.h"
#import "VideoRenderView.h"


class RenderManager {
 public:
  RenderManager();
  ~RenderManager();
  void SetRemoteWindowView(int channelID, void* remoteView ) ;
  rtc::VideoSinkInterface<webrtc::VideoFrame>*  getRemoteVideoSilkByChannelID(int channelID);
private:

  std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> remote_sink_;
  std::map<int,VideoRenderView*> remote_views;
  std::map<int,std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>> remote_sinks;
  rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> video_source_;
  
     
};

//}  // namespace webrtc_examples

#endif  // EXAMPLES_OBJCNATIVEAPI_OBJCCALLCLIENT_H_
