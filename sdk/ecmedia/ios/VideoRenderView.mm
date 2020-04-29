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

//#if defined(RTC_SUPPORTS_METAL)
   // _remoteVideoView = [[RTCMTLVideoView alloc] initWithFrame:CGRectZero];
  //   _remoteVideoView =[[RTCEAGLVideoView alloc] initWithFrame:CGRectZero];
//#else
    RTCEAGLVideoView *remoteView = [[RTCEAGLVideoView alloc] initWithFrame:CGRectZero];
    remoteView.delegate = self;
    _remoteVideoView = remoteView;
//#endif

    [self addSubview:_remoteVideoView];

   // _localVideoView = [[RTCCameraPreviewView alloc] initWithFrame:CGRectZero];
    //[self addSubview:_localVideoView];

   // _statsView = [[ARDStatsView alloc] initWithFrame:CGRectZero];
    //_statsView.hidden = YES;
    //[self addSubview:_statsView];

//    _routeChangeButton = [UIButton buttonWithType:UIButtonTypeCustom];
//    _routeChangeButton.backgroundColor = [UIColor whiteColor];
//    _routeChangeButton.layer.cornerRadius = kButtonSize / 2;
//    _routeChangeButton.layer.masksToBounds = YES;
//    UIImage *image = [UIImage imageNamed:@"ic_surround_sound_black_24dp.png"];
//    [_routeChangeButton setImage:image forState:UIControlStateNormal];
//    [_routeChangeButton addTarget:self
//                           action:@selector(onRouteChange:)
//                 forControlEvents:UIControlEventTouchUpInside];
//    [self addSubview:_routeChangeButton];

    // TODO(tkchin): don't display this if we can't actually do camera switch.
//    _cameraSwitchButton = [UIButton buttonWithType:UIButtonTypeCustom];
//    _cameraSwitchButton.backgroundColor = [UIColor whiteColor];
//    _cameraSwitchButton.layer.cornerRadius = kButtonSize / 2;
//    _cameraSwitchButton.layer.masksToBounds = YES;
//    image = [UIImage imageNamed:@"ic_switch_video_black_24dp.png"];
//    [_cameraSwitchButton setImage:image forState:UIControlStateNormal];
//    [_cameraSwitchButton addTarget:self
//                      action:@selector(onCameraSwitch:)
//            forControlEvents:UIControlEventTouchUpInside];
//    [self addSubview:_cameraSwitchButton];

//    _hangupButton = [UIButton buttonWithType:UIButtonTypeCustom];
//    _hangupButton.backgroundColor = [UIColor redColor];
//    _hangupButton.layer.cornerRadius = kButtonSize / 2;
//    _hangupButton.layer.masksToBounds = YES;
//    image = [UIImage imageForName:@"ic_call_end_black_24dp.png"
//                            color:[UIColor whiteColor]];
//    [_hangupButton setImage:image forState:UIControlStateNormal];
//    [_hangupButton addTarget:self
//                      action:@selector(onHangup:)
//            forControlEvents:UIControlEventTouchUpInside];
//    [self addSubview:_hangupButton];

//    _statusLabel = [[UILabel alloc] initWithFrame:CGRectZero];
//    _statusLabel.font = [UIFont fontWithName:@"Roboto" size:16];
//    _statusLabel.textColor = [UIColor whiteColor];
//    [self addSubview:_statusLabel];
//
//    UITapGestureRecognizer *tapRecognizer =
//        [[UITapGestureRecognizer alloc]
//            initWithTarget:self
//                    action:@selector(didTripleTap:)];
//    tapRecognizer.numberOfTapsRequired = 3;
//    [self addGestureRecognizer:tapRecognizer];
  }
  return self;
}

- (void)layoutSubviews {
  CGRect bounds = self.bounds;
  if (_remoteVideoSize.width > 0 && _remoteVideoSize.height > 0) {
    // Aspect fill remote video into bounds.
    CGRect remoteVideoFrame =
        AVMakeRectWithAspectRatioInsideRect(_remoteVideoSize, bounds);
    CGFloat scale = 1;
    if (remoteVideoFrame.size.width > remoteVideoFrame.size.height) {
      // Scale by height.
      scale = bounds.size.height / remoteVideoFrame.size.height;
    } else {
      // Scale by width.
      scale = bounds.size.width / remoteVideoFrame.size.width;
    }
    //CGFloat scale_widht = bounds.size.width / remoteVideoFrame.size.width;
    //CGFloat scale_height = bounds.size.height / remoteVideoFrame.size.height;
    //scale = scale_widht > scale_height ? scale_height : scale_widht;
    remoteVideoFrame.size.height *= scale;
    remoteVideoFrame.size.width *= scale;
//    if(remoteVideoFrame.size.height >   bounds.size.height)
//      remoteVideoFrame.size.height =  bounds.size.height;
//    if(remoteVideoFrame.size.width > bounds.size.width)
//      remoteVideoFrame.size.width =  bounds.size.width;
      
    _remoteVideoView.frame = remoteVideoFrame;
    _remoteVideoView.center =
        CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
  } else {
    _remoteVideoView.frame = bounds;
  }

//  // Aspect fit local video view into a square box.
//  CGRect localVideoFrame =
//      CGRectMake(0, 0, kLocalVideoViewSize, kLocalVideoViewSize);
//  // Place the view in the bottom right.
//  localVideoFrame.origin.x = CGRectGetMaxX(bounds)
//      - localVideoFrame.size.width - kLocalVideoViewPadding;
//  localVideoFrame.origin.y = CGRectGetMaxY(bounds)
//      - localVideoFrame.size.height - kLocalVideoViewPadding;
//  _localVideoView.frame = localVideoFrame;
//
//  // Place stats at the top.
//  CGSize statsSize = [_statsView sizeThatFits:bounds.size];
//  _statsView.frame = CGRectMake(CGRectGetMinX(bounds),
//                                CGRectGetMinY(bounds) + kStatusBarHeight,
//                                statsSize.width, statsSize.height);
//
//  // Place hangup button in the bottom left.
//  _hangupButton.frame =
//      CGRectMake(CGRectGetMinX(bounds) + kButtonPadding,
//                 CGRectGetMaxY(bounds) - kButtonPadding -
//                     kButtonSize,
//                 kButtonSize,
//                 kButtonSize);
//
//  // Place button to the right of hangup button.
//  CGRect cameraSwitchFrame = _hangupButton.frame;
//  cameraSwitchFrame.origin.x =
//      CGRectGetMaxX(cameraSwitchFrame) + kButtonPadding;
//  _cameraSwitchButton.frame = cameraSwitchFrame;
//
//  // Place route button to the right of camera button.
//  CGRect routeChangeFrame = _cameraSwitchButton.frame;
//  routeChangeFrame.origin.x =
//      CGRectGetMaxX(routeChangeFrame) + kButtonPadding;
//  _routeChangeButton.frame = routeChangeFrame;
//
//  [_statusLabel sizeToFit];
//  _statusLabel.center =
//      CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
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
