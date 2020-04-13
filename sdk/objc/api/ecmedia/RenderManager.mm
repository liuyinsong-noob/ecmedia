/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "RenderManager.h"

#include <utility>

#import "sdk/objc/base/RTCVideoRenderer.h"
#import "sdk/objc/components/video_codec/RTCDefaultVideoDecoderFactory.h"
#import "sdk/objc/components/video_codec/RTCDefaultVideoEncoderFactory.h"
#import "sdk/objc/helpers/RTCCameraPreviewView.h"

#include "absl/memory/memory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/peer_connection_interface.h"
#include "api/video/builtin_video_bitrate_allocator_factory.h"
#include "logging/rtc_event_log/rtc_event_log_factory.h"
#include "media/engine/webrtc_media_engine.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "sdk/objc/native/api/video_capturer.h"
#include "sdk/objc/native/api/video_decoder_factory.h"
#include "sdk/objc/native/api/video_encoder_factory.h"
#include "sdk/objc/native/api/video_renderer.h"

#import "sdk/objc/components/capturer/RTCCameraVideoCapturer.h"
#if defined(RTC_SUPPORTS_METAL)
#import "sdk/objc/components/renderer/metal/RTCMTLVideoView.h"  // nogncheck
#endif
#import "sdk/objc/components/renderer/opengl/RTCEAGLVideoView.h"
#import "sdk/objc/helpers/RTCCameraPreviewView.h"


RenderManager::RenderManager()
{
}
RenderManager::~RenderManager(){
  remote_views.clear();
  remote_sinks.clear();
}
void RenderManager::SetRemoteWindowView(int channelID, void* remoteView ){
  if(remoteView){
    UIView *parentView = (__bridge UIView *)remoteView;
    VideoRenderView * videoview;
    std::map<int,VideoRenderView*>::iterator it = remote_views.find(channelID);
    if(it != remote_views.end()){
      videoview = it->second;
      videoview.bounds = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
      videoview.frame = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
      [videoview removeFromSuperview];
    }else{
      videoview = [[VideoRenderView alloc] initWithFrame:CGRectZero];
      remote_views[channelID] = videoview;
      remote_sinks[channelID] = ((webrtc::ObjCToNativeVideoRenderer(videoview.remoteVideoView)));
      videoview.bounds = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
      videoview.frame = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
    }
    videoview.parentView = parentView;
    videoview.contentMode = parentView.contentMode;
    if (videoview.superview != parentView) {
      [videoview removeFromSuperview];
      [parentView addSubview:videoview];
    }
  }
}
rtc::VideoSinkInterface<webrtc::VideoFrame>*  RenderManager::getRemoteVideoSilkByChannelID(int channelID){
  std::map<int,std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>>::iterator it = remote_sinks.find(channelID);
  if(it != remote_sinks.end())
    return it->second.get();
  if( ![[NSThread currentThread] isMainThread] ){
    dispatch_sync(dispatch_get_main_queue(), ^{
      VideoRenderView* videoview =  [[VideoRenderView alloc] initWithFrame:CGRectZero];
      if(!videoview)
        return;
      remote_views[channelID] = videoview;
      remote_sinks[channelID] = ((webrtc::ObjCToNativeVideoRenderer(videoview.remoteVideoView)));
      //return remote_sinks[channelID].get();
    });
  }
  else {
       VideoRenderView* videoview =  [[VideoRenderView alloc] initWithFrame:CGRectZero];
        if(!videoview)
          return nullptr;
        remote_views[channelID] = videoview;
        remote_sinks[channelID] = ((webrtc::ObjCToNativeVideoRenderer(videoview.remoteVideoView)));
    return remote_sinks[channelID].get();
  }
  return remote_sinks[channelID].get();

}






