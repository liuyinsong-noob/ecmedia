/*
 * IJKMediaPlayback.h
 *
 * Copyright (c) 2013 Bilibili
 * Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, EC_MPMovieScalingMode) {
    EC_MPMovieScalingModeNone,       // No scaling
    EC_MPMovieScalingModeAspectFit,  // Uniform scale until one dimension fits
    EC_MPMovieScalingModeAspectFill, // Uniform scale until the movie fills the visible bounds. One dimension may have clipped contents
    EC_MPMovieScalingModeFill        // Non-uniform scale. Both render dimensions will exactly match the visible bounds
};

typedef NS_ENUM(NSInteger, EC_MPMoviePlaybackState) {
    EC_MPMoviePlaybackStateStopped,
    EC_MPMoviePlaybackStatePlaying,
    EC_MPMoviePlaybackStatePaused,
    EC_MPMoviePlaybackStateInterrupted,
    EC_MPMoviePlaybackStateSeekingForward,
    EC_MPMoviePlaybackStateSeekingBackward
};

typedef NS_OPTIONS(NSUInteger, EC_MPMovieLoadState) {
    EC_MPMovieLoadStateUnknown        = 0,
    EC_MPMovieLoadStatePlayable       = 1 << 0,
    EC_MPMovieLoadStatePlaythroughOK  = 1 << 1, // Playback will be automatically started in this state when shouldAutoplay is YES
    EC_MPMovieLoadStateStalled        = 1 << 2, // Playback will be automatically paused in this state, if started
};

typedef NS_ENUM(NSInteger, EC_MPMovieFinishReason) {
    EC_MPMovieFinishReasonPlaybackEnded,
    EC_MPMovieFinishReasonPlaybackError,
    EC_MPMovieFinishReasonUserExited
};

// -----------------------------------------------------------------------------
// Thumbnails

typedef NS_ENUM(NSInteger, EC_MPMovieTimeOption) {
    EC_MPMovieTimeOptionNearestKeyFrame,
    EC_MPMovieTimeOptionExact
};

@protocol EC_MediaPlayback;

#pragma mark IJKMediaPlayback

@protocol EC_MediaPlayback <NSObject>

- (void)prepareToPlay;
- (void)play;
- (void)pause;
- (void)stop;
- (BOOL)isPlaying;
- (void)shutdown;
- (void)setPauseInBackground:(BOOL)pause;

@property(nonatomic, readonly)  UIView *view;
@property(nonatomic)            NSTimeInterval currentPlaybackTime;
@property(nonatomic, readonly)  NSTimeInterval duration;
@property(nonatomic, readonly)  NSTimeInterval playableDuration;
@property(nonatomic, readonly)  NSInteger bufferingProgress;

@property(nonatomic, readonly)  BOOL isPreparedToPlay;
@property(nonatomic, readonly)  EC_MPMoviePlaybackState playbackState;
@property(nonatomic, readonly)  EC_MPMovieLoadState loadState;
@property(nonatomic, readonly) int isSeekBuffering;
@property(nonatomic, readonly) int isAudioSync;
@property(nonatomic, readonly) int isVideoSync;

@property(nonatomic, readonly) int64_t numberOfBytesTransferred;

@property(nonatomic, readonly) CGSize naturalSize;
@property(nonatomic) EC_MPMovieScalingMode scalingMode;
@property(nonatomic) BOOL shouldAutoplay;

@property (nonatomic) BOOL allowsMediaAirPlay;
@property (nonatomic) BOOL isDanmakuMediaAirPlay;
@property (nonatomic, readonly) BOOL airPlayMediaActive;

@property (nonatomic) float playbackRate;
@property (nonatomic) float playbackVolume;

- (UIImage *)thumbnailImageAtCurrentTime;

#pragma mark Notifications

#ifdef __cplusplus
#define IJK_EXTERN extern "C" __attribute__((visibility ("default")))
#else
#define IJK_EXTERN extern __attribute__((visibility ("default")))
#endif

// -----------------------------------------------------------------------------
//  MPMediaPlayback.h

// Posted when the prepared state changes of an object conforming to the MPMediaPlayback protocol changes.
// This supersedes MPMoviePlayerContentPreloadDidFinishNotification.
IJK_EXTERN NSString *const EC_MPMediaPlaybackIsPreparedToPlayDidChangeNotification;

// -----------------------------------------------------------------------------
//  MPMoviePlayerController.h
//  Movie Player Notifications

// Posted when the scaling mode changes.
IJK_EXTERN NSString* const EC_MPMoviePlayerScalingModeDidChangeNotification;

// Posted when movie playback ends or a user exits playback.
IJK_EXTERN NSString* const EC_MPMoviePlayerPlaybackDidFinishNotification;
IJK_EXTERN NSString* const EC_MPMoviePlayerPlaybackDidFinishReasonUserInfoKey; // NSNumber (IJKMPMovieFinishReason)

// Posted when the playback state changes, either programatically or by the user.
IJK_EXTERN NSString* const EC_MPMoviePlayerPlaybackStateDidChangeNotification;

// Posted when the network load state changes.
IJK_EXTERN NSString* const EC_MPMoviePlayerLoadStateDidChangeNotification;

// Posted when the movie player begins or ends playing video via AirPlay.
IJK_EXTERN NSString* const EC_MPMoviePlayerIsAirPlayVideoActiveDidChangeNotification;

// -----------------------------------------------------------------------------
// Movie Property Notifications

// Calling -prepareToPlay on the movie player will begin determining movie properties asynchronously.
// These notifications are posted when the associated movie property becomes available.
IJK_EXTERN NSString* const EC_MPMovieNaturalSizeAvailableNotification;

// -----------------------------------------------------------------------------
//  Extend Notifications

IJK_EXTERN NSString *const EC_MPMoviePlayerVideoDecoderOpenNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerFirstVideoFrameRenderedNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerFirstAudioFrameRenderedNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerFirstAudioFrameDecodedNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerFirstVideoFrameDecodedNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerOpenInputNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerFindStreamInfoNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerComponentOpenNotification;

IJK_EXTERN NSString *const EC_MPMoviePlayerDidSeekCompleteNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerDidSeekCompleteTargetKey;
IJK_EXTERN NSString *const EC_MPMoviePlayerDidSeekCompleteErrorKey;
IJK_EXTERN NSString *const EC_MPMoviePlayerDidAccurateSeekCompleteCurPos;
IJK_EXTERN NSString *const EC_MPMoviePlayerAccurateSeekCompleteNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerSeekAudioStartNotification;
IJK_EXTERN NSString *const EC_MPMoviePlayerSeekVideoStartNotification;

@end

#pragma mark IJKMediaUrlOpenDelegate

// Must equal to the defination in ijkavformat/ijkavformat.h
typedef NS_ENUM(NSInteger, EC_MediaEvent) {

    // Notify Events
    IJKMediaEvent_WillHttpOpen         = 1,       // attr: url
    IJKMediaEvent_DidHttpOpen          = 2,       // attr: url, error, http_code
    IJKMediaEvent_WillHttpSeek         = 3,       // attr: url, offset
    IJKMediaEvent_DidHttpSeek          = 4,       // attr: url, offset, error, http_code
    // Control Message
    IJKMediaCtrl_WillTcpOpen           = 0x20001, // IJKMediaUrlOpenData: no args
    IJKMediaCtrl_DidTcpOpen            = 0x20002, // IJKMediaUrlOpenData: error, family, ip, port, fd
    IJKMediaCtrl_WillHttpOpen          = 0x20003, // IJKMediaUrlOpenData: url, segmentIndex, retryCounter
    IJKMediaCtrl_WillLiveOpen          = 0x20005, // IJKMediaUrlOpenData: url, retryCounter
    IJKMediaCtrl_WillConcatSegmentOpen = 0x20007, // IJKMediaUrlOpenData: url, segmentIndex, retryCounter
};

#define IJKMediaEventAttrKey_url            @"url"
#define IJKMediaEventAttrKey_host           @"host"
#define IJKMediaEventAttrKey_error          @"error"
#define IJKMediaEventAttrKey_time_of_event  @"time_of_event"
#define IJKMediaEventAttrKey_http_code      @"http_code"
#define IJKMediaEventAttrKey_offset         @"offset"
#define IJKMediaEventAttrKey_file_size      @"file_size"

// event of IJKMediaUrlOpenEvent_xxx
@interface EC_MediaUrlOpenData: NSObject

- (id)initWithUrl:(NSString *)url
            event:(EC_MediaEvent)event
     segmentIndex:(int)segmentIndex
     retryCounter:(int)retryCounter;

@property(nonatomic, readonly) EC_MediaEvent event;
@property(nonatomic, readonly) int segmentIndex;
@property(nonatomic, readonly) int retryCounter;

@property(nonatomic, retain) NSString *url;
@property(nonatomic, assign) int fd;
@property(nonatomic, strong) NSString *msg;
@property(nonatomic) int error; // set a negative value to indicate an error has occured.
@property(nonatomic, getter=isHandled)    BOOL handled;     // auto set to YES if url changed
@property(nonatomic, getter=isUrlChanged) BOOL urlChanged;  // auto set to YES by url changed

@end

@protocol EC_MediaUrlOpenDelegate <NSObject>

- (void)willOpenUrl:(EC_MediaUrlOpenData*) urlOpenData;

@end

@protocol EC_MediaNativeInvokeDelegate <NSObject>

- (int)invoke:(EC_MediaEvent)event attributes:(NSDictionary *)attributes;

@end
