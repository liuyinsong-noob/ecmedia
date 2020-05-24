/*
 *  Copyright 2015 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import <UIKit/UIKit.h>
#import <WebRTC/RTCVideoRenderer.h>

@class VideoRenderView;
@protocol ARDVideoCallViewDelegate <NSObject>

// Called when the camera switch button is pressed.
- (void)videoCallViewDidSwitchCamera:(VideoRenderView *)view;

// Called when the route change button is pressed.
- (void)videoCallViewDidChangeRoute:(VideoRenderView *)view;

// Called when the hangup button is pressed.
- (void)videoCallViewDidHangup:(VideoRenderView *)view;

// Called when stats are enabled by triple tapping.
- (void)videoCallViewDidEnableStats:(VideoRenderView *)view;

@end

// Video call view that shows local and remote video, provides a label to
// display status, and also a hangup button.
@interface VideoRenderView : UIView

@property(nonatomic) UIViewContentMode videoContentMode;
@property(nonatomic, readonly) UILabel *statusLabel;
@property (nonatomic, retain) UIView* parentView;
//@property(nonatomic, readonly) RTCCameraPreviewView *localVideoView;
@property(nonatomic, readonly) __kindof UIView<RTCVideoRenderer> *remoteVideoView;
//@property(nonatomic, readonly) ARDStatsView *statsView;
//@property(nonatomic, weak) id<ARDVideoCallViewDelegate> delegate;

@end
