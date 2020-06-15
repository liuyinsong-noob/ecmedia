/*
 *  Copyright 2015 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "VideoRenderView.h"

#import <AVFoundation/AVFoundation.h>

#import <WebRTC/RTCEAGLVideoView.h>
#import <WebRTC/RTCMTLVideoView.h>

//#import "UIImage+ARDUtilities.h"

@interface VideoRenderView () <RTCVideoViewDelegate>
@end

@implementation VideoRenderView {
  UIButton *_routeChangeButton;
  UIButton *_cameraSwitchButton;
  UIButton *_hangupButton;
  CGSize _remoteVideoSize;
}

@synthesize statusLabel = _statusLabel;
//@synthesize localVideoView = _localVideoView;
@synthesize remoteVideoView = _remoteVideoView;
//@synthesize statsView = _statsView;
//@synthesize delegate = _delegate;

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    
#if defined(RTC_SUPPORTS_METAL) && false//somting error not support
    _remoteVideoView = [[RTCMTLVideoView alloc] initWithFrame:CGRectZero];
    //   _remoteVideoView =[[RTCEAGLVideoView alloc] initWithFrame:CGRectZero];
#else
    RTCEAGLVideoView *remoteView = [[RTCEAGLVideoView alloc] initWithFrame:CGRectZero];
    remoteView.delegate = self;
    _remoteVideoView = remoteView;
#endif
    [self addSubview:_remoteVideoView];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  self.contentMode = _videoContentMode;
  CGRect bounds = self.bounds;
  NSLog(@"yukening renderview  w: %d ,h: %d",(int)self.bounds.size.width, (int)self.bounds.size.height);
  if (_remoteVideoSize.width > 0 && _remoteVideoSize.height > 0 ) {
    
    if(_videoContentMode == UIViewContentModeScaleAspectFill){
//      // Aspect fill remote video into bounds.
//      CGRect remoteVideoFrame =
//      AVMakeRectWithAspectRatioInsideRect(_remoteVideoSize, bounds);
//      CGFloat scale = 1;
//      if (remoteVideoFrame.size.width > remoteVideoFrame.size.height) {
//        // Scale by height.
//        scale = bounds.size.height / remoteVideoFrame.size.height;
//      } else {
//        // Scale by width.
//        scale = bounds.size.width / remoteVideoFrame.size.width;
//      }
//
//      remoteVideoFrame.size.height *= scale;
//      remoteVideoFrame.size.width *= scale;
//
//      _remoteVideoView.frame = remoteVideoFrame;
//      _remoteVideoView.center =
//      CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
      CGRect remoteVideoFrame = CGRectMake(0, 0, _remoteVideoSize.width,  _remoteVideoSize.height);
      CGFloat scale = 1;
      CGFloat scale_widht = 1;
      CGFloat scale_height = 1;

      if(remoteVideoFrame.size.width > bounds.size.width){
             if(remoteVideoFrame.size.height > bounds.size.height){
               scale_widht = bounds.size.width / remoteVideoFrame.size.width;
               scale_height = bounds.size.height / remoteVideoFrame.size.height;
               scale = scale_widht > scale_height ? scale_widht : scale_height;
             }else{
               scale = bounds.size.height / remoteVideoFrame.size.height;
             }
           }else{
             if(remoteVideoFrame.size.height < bounds.size.height){
               scale_widht = bounds.size.width / remoteVideoFrame.size.width;
               scale_height = bounds.size.height / remoteVideoFrame.size.height;
               scale = scale_widht > scale_height ? scale_widht : scale_height;
             }else{
               scale = bounds.size.width / remoteVideoFrame.size.width;
             }
           }
           NSLog(@"remoteVideoFrame--- w %f,h:%f, scaw:%f,scah:%f self is :%@",remoteVideoFrame.size.width,remoteVideoFrame.size.height,scale_widht,scale_height,self );
           remoteVideoFrame.size.height *= scale;
           remoteVideoFrame.size.width *= scale;
           _remoteVideoView.frame = remoteVideoFrame;
                _remoteVideoView.center =
                CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
      
      
    }else{
      CGRect remoteVideoFrame =  CGRectMake(0, 0, _remoteVideoSize.width,  _remoteVideoSize.height);
      CGFloat scale = 1;
      CGFloat scale_widht = 1;
      CGFloat scale_height = 1;
      if(remoteVideoFrame.size.width > bounds.size.width){
        if(remoteVideoFrame.size.height > bounds.size.height){
          scale_widht = bounds.size.width / remoteVideoFrame.size.width;
          scale_height = bounds.size.height / remoteVideoFrame.size.height;
          scale = scale_widht > scale_height ? scale_height : scale_widht;
        }else{
          scale = bounds.size.width / remoteVideoFrame.size.width;
        }
      }else{
        if(remoteVideoFrame.size.height < bounds.size.height){
          scale_widht = bounds.size.width / remoteVideoFrame.size.width;
          scale_height = bounds.size.height / remoteVideoFrame.size.height;
          scale = scale_widht > scale_height ? scale_height : scale_widht;
        }else{
          scale = bounds.size.height / remoteVideoFrame.size.height;
        }
      }
      NSLog(@"remoteVideoFrame w %f,h:%f, scaw:%f,scah:%f  self:%@",remoteVideoFrame.size.width,remoteVideoFrame.size.height,scale_widht,scale_height,self );
      remoteVideoFrame.size.height *= scale;
      remoteVideoFrame.size.width *= scale;
      _remoteVideoView.frame = remoteVideoFrame;
           _remoteVideoView.center =
           CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
      
    }
  } else {
    _remoteVideoView.frame = bounds;
  }
}

#pragma mark - RTCVideoViewDelegate

- (void)videoView:(id<RTCVideoRenderer>)videoView didChangeVideoSize:(CGSize)size {
  if (videoView == _remoteVideoView) {
    _remoteVideoSize = size;
  }
  [self setNeedsLayout];
}

#pragma mark - Private


@end
