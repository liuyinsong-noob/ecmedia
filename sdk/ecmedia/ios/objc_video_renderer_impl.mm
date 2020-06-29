/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/ecmedia/ios/objc_video_renderer_impl.h"

#import "base/RTCVideoFrame.h"
#import "base/RTCVideoRenderer.h"

#include "sdk/objc/native/src/objc_video_frame.h"

// TODO : STANDARD_RENDERING is platform relevant
#define STANDARD_RENDERING kRenderWindows

VideoRenderer* VideoRenderer::CreateVideoRenderer(
    void* windows,
    int render_mode,
    bool mirror,
    webrtc::VideoTrackInterface* track_to_render,
    rtc::Thread* worker_thread,
    VideoRenderType type,
    rtc::VideoSinkWants wants) {
  VideoRenderType render_type = type;
  if (render_type == kRenderDefault) {
    render_type = STANDARD_RENDERING;
  }

  RTC_LOG(LS_INFO) << " CreateVideoRenderer "
                   << " hubin_render";
  
  if(windows){
    if( ![[NSThread currentThread] isMainThread] ){
      return nullptr;
    }
    else{
      webrtc::ObjCVideoRendererImpl* objimpl =
      new webrtc::ObjCVideoRendererImpl( windows, render_mode, mirror, track_to_render, worker_thread, type);
      return objimpl;
    }
  }
  
  return nullptr;
}


namespace webrtc {

ObjCVideoRendererImpl::ObjCVideoRendererImpl( void* parent, int render_mode,
                                             bool mirror, webrtc::VideoTrackInterface* track,
                                             rtc::Thread* worker_thread, VideoRenderType type)
: size_(CGSizeZero), track_(track),render_mode_(render_mode),
parent_(parent), mirror_(mirror), type_(type) ,worker_thread_(worker_thread){
  renderer_ = GetWindwoRenderPtr(parent);
}
ObjCVideoRendererImpl::~ObjCVideoRendererImpl(){
   if(track_){
     track_->RemoveSink(this);
     worker_thread_ = nullptr;
     parent_ = nullptr;
     mirror_ = false;
     render_mode_ = 0;
     type_ = kRenderiOS;
     if( ![[NSThread currentThread] isMainThread] ){
         VideoRenderView* oldvideoview = videoview;
         videoview = nil;
        dispatch_async(dispatch_get_main_queue(), ^{
          if (oldvideoview && [oldvideoview superview]) {
            [oldvideoview removeFromSuperview];
          }
        });
     }else{
       if (videoview && [videoview superview]) {
         [videoview removeFromSuperview];
         videoview = nil;
       }
       if(videoview)
             videoview = nil;
     }
   }
}

void ObjCVideoRendererImpl::OnFrame(const VideoFrame& nativeVideoFrame) {
  RTCVideoFrame* videoFrame = ToObjCVideoFrame(nativeVideoFrame);

  CGSize current_size = (videoFrame.rotation % 180 == 0) ?
      CGSizeMake(videoFrame.width, videoFrame.height) :
      CGSizeMake(videoFrame.height, videoFrame.width);

  if (!CGSizeEqualToSize(size_, current_size)) {
    size_ = current_size;
    [renderer_ setSize:size_];
  }
  [renderer_ renderFrame:videoFrame];
}

id<RTCVideoRenderer> ObjCVideoRendererImpl::GetWindwoRenderPtr(void* remoteView){
  
  if(remoteView){
    UIView *parentView = (__bridge UIView *)remoteView;
    videoview = [[VideoRenderView alloc] initWithFrame:CGRectZero];
    videoview.bounds = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
    videoview.frame = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
  
    videoview.parentView = parentView;
    videoview.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleRightMargin |UIViewAutoresizingFlexibleTopMargin |
      UIViewAutoresizingFlexibleBottomMargin;
    videoview.contentMode = parentView.contentMode;
    
    if(mirror_){
      CGAffineTransform transform = CGAffineTransformIdentity;
      transform = CGAffineTransformScale(transform, -1, 1);
      videoview.transform = transform;
    }
    render_mode_ == 0 ? videoview.videoContentMode = UIViewContentModeScaleAspectFill :
                        videoview.videoContentMode = UIViewContentModeScaleAspectFit;
    videoview.contentMode =videoview.videoContentMode;
    parentView.contentMode = videoview.videoContentMode;
    if (videoview.superview != parentView) {
      [videoview removeFromSuperview];
      [parentView addSubview:videoview];
    }
    return videoview.remoteVideoView;
  }
  return nullptr;
}
int ObjCVideoRendererImpl::UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render, rtc::VideoSinkWants wants){
  if(!track_to_render)
    return 0;
  track_ = track_to_render;
  track_to_render->AddOrUpdateSink(this, rtc::VideoSinkWants());
  return 0;
 }
}  // namespace webrtc
