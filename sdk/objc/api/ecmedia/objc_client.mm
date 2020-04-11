/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "objc_client.h"

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
#include "video_capture_iOS_device_info.h"

#import "sdk/objc/components/capturer/RTCCameraVideoCapturer.h"
#if defined(RTC_SUPPORTS_METAL)
#import "sdk/objc/components/renderer/metal/RTCMTLVideoView.h"  // nogncheck
#endif
#import "sdk/objc/components/renderer/opengl/RTCEAGLVideoView.h"
#import "sdk/objc/helpers/RTCCameraPreviewView.h"
#import "RenderManager.h"

RTCCameraVideoCapturer *capturer_;
RTCCameraPreviewView *localVideoView;
__kindof UIView<RTCVideoRenderer> *remoteVideoView;
const Float64 kFramerateLimit = 30.0;
bool kLoudSpeakerStatus = false;
RenderManager* rm_;

//namespace  {
ObjCCallClient* ObjCCallClient::m_pInstance = NULL;
ObjCCallClient* ObjCCallClient::GetInstance(){
    if (m_pInstance == NULL) {
       m_pInstance = new ObjCCallClient();
     }
     return m_pInstance;
}
ObjCCallClient::ObjCCallClient()
: call_started_(false){
  rm_ = new RenderManager();
  m_fps = 25;
  _usingFrontCamera = true;
  targetWidth = 640;
  targetHeight = 480;
}
ObjCCallClient::~ObjCCallClient(){
  rm_ = nullptr;
  Hangup();
}

void ObjCCallClient::Hangup() {
  remote_sink_ = nullptr;
  video_source_ = nullptr;
  StopCapture();
  localVideoView.captureSession = nullptr;
  remote_sinks.clear();
  capturer_ = nullptr;
}

void ObjCCallClient::CreatePeerConnection() {
  rtc::CritScope lock(&pc_mutex_);
  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  // DTLS SRTP has to be disabled for loopback to work.
  config.enable_dtls_srtp = false;
  pc_ = pcf_->CreatePeerConnection(
                                   config, nullptr /* port_allocator */, nullptr /* cert_generator */, nullptr);
  RTC_LOG(LS_INFO) << "PeerConnection created: " << pc_;
  
  rtc::scoped_refptr<webrtc::VideoTrackInterface> local_video_track =
  pcf_->CreateVideoTrack("video", video_source_);
  pc_->AddTransceiver(local_video_track);
  RTC_LOG(LS_INFO) << "Local video sink set up: " << local_video_track;
  
  for (const rtc::scoped_refptr<webrtc::RtpTransceiverInterface>& tranceiver :
       pc_->GetTransceivers()) {
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track = tranceiver->receiver()->track();
    if (track && track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
      static_cast<webrtc::VideoTrackInterface*>(track.get())
      ->AddOrUpdateSink(remote_sink_.get(), rtc::VideoSinkWants());
      RTC_LOG(LS_INFO) << "Remote video sink set up: " << track;
      break;
    }
  }
}

bool ObjCCallClient::InitDevice(rtc::Thread* signaling_thread,rtc::Thread* worker_thread){
  capturer_ = [[RTCCameraVideoCapturer alloc] init];
  
  video_source_ =  webrtc::ObjCToNativeVideoCapturer(capturer_, signaling_thread, worker_thread);
  
  localVideoView = [[RTCCameraPreviewView alloc] init];
  
  if(capturer_ && localVideoView){
    localVideoView.captureSession = capturer_.captureSession;
  }
  //remoteVideoView = [[RTCEAGLVideoView alloc] init];
  //id<RTCVideoRenderer> remote_renderer = remoteVideoView;
  //remote_sink_ =  webrtc::ObjCToNativeVideoRenderer(remote_renderer);
 
 //[this performSelectorOnMainThread:@selector(StartCapture:) withObject:nil waitUntilDone:NO];
  //StartCapture();
    
    _captureInfo = new VideoCaptureiOSDeviceInfo(0);
     
  NSLog(@"init device success....");
  return true;
}

void ObjCCallClient::SetLocalWindowView(void* local)
{
  if(local )
  {
     if(m_localView == local)
         return;
      m_localView = local;
      NSLog(@"setLocal view...");
    UIView *parentView = (__bridge UIView *)local;
    dispatch_async(dispatch_get_main_queue(), ^{
      localVideoView.bounds = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
      localVideoView.frame = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
      localVideoView.contentMode = parentView.contentMode;
      [parentView addSubview:localVideoView];
    });
  }
}

void ObjCCallClient::SetRemoteWindowView(int channelID, void* remoteView ){
  if(rm_){
    if( ![[NSThread currentThread] isMainThread] ){
      dispatch_sync(dispatch_get_main_queue(), ^{
        rm_->SetRemoteWindowView(channelID,remoteView);
      });
    }
    else
      rm_->SetRemoteWindowView(channelID, remoteView);
  }
  return;
  if(remoteView){
    std::map<int,std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>>::iterator it = remote_sinks.find(channelID);
    if(it != remote_sinks.end())
      return;
    RTCEAGLVideoView* videoview = [[RTCEAGLVideoView alloc] init];
    remote_sinks[channelID] = ((webrtc::ObjCToNativeVideoRenderer(videoview)));
    UIView *parentView = (__bridge UIView *)remoteView;
    videoview.bounds = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
    videoview.frame = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
    videoview.contentMode = parentView.contentMode;
    [parentView addSubview:videoview];
  }
}

void ObjCCallClient::SetCameraFPS(int fps){
  m_fps = fps;
}

void ObjCCallClient::StartCapture(int deviceid,webrtc::VideoCaptureCapability& cap){
  if(deviceid == 0)
    _usingFrontCamera = false;
  else
    _usingFrontCamera = true;
  targetWidth = cap.width > 0 ? cap.width : 640;
  targetHeight = cap.height > 0 ? cap.height : 480;
  AVCaptureDevicePosition position =
  _usingFrontCamera ? AVCaptureDevicePositionFront : AVCaptureDevicePositionBack;
  
  AVCaptureDevice *selectedDevice = nil;
  NSArray<AVCaptureDevice *> *captureDevices = [RTCCameraVideoCapturer captureDevices];
  for (AVCaptureDevice *device in captureDevices) {
    if (device.position == position) {
      selectedDevice = device;
      break;
    }
  }
  
  
  AVCaptureDeviceFormat *selectedFormat = nil;
  int currentDiff = INT_MAX;
  NSArray<AVCaptureDeviceFormat *> *formats =
  [RTCCameraVideoCapturer supportedFormatsForDevice:selectedDevice];
  for (AVCaptureDeviceFormat *format in formats) {
    CMVideoDimensions dimension = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
    FourCharCode pixelFormat = CMFormatDescriptionGetMediaSubType(format.formatDescription);
    int diff = abs(targetWidth - dimension.width) + abs(targetHeight - dimension.height);
    if (diff < currentDiff) {
      selectedFormat = format;
      currentDiff = diff;
    } else if (diff == currentDiff && pixelFormat ==  [capturer_ preferredOutputPixelFormat]) {
      selectedFormat = format;
    }
  }
  
  
  if (selectedFormat == nil) {
    NSLog(@"No valid formats for device %@", selectedDevice);
    return;
  }
  
  NSInteger fps = 25;
  if(m_fps == -1){
    Float64 maxSupportedFramerate = 0;
    for (AVFrameRateRange *fpsRange in selectedFormat.videoSupportedFrameRateRanges) {
      maxSupportedFramerate = fmax(maxSupportedFramerate, fpsRange.maxFrameRate);
    }
    fps =  fmin(maxSupportedFramerate, kFramerateLimit);
  }else
    fps = m_fps;
  dispatch_async(dispatch_get_main_queue(), ^{
    [capturer_ startCaptureWithDevice:selectedDevice format:selectedFormat fps:fps];
  });
 // [capturer_ performSelectorOnMainThread:@selector(addViewtoParent:) withObject:nil waitUntilDone:NO];
}
bool ObjCCallClient::SetSpeakerStatus(bool enable) {
    NSError* error = nil;
    AVAudioSession* session = [AVAudioSession sharedInstance];
    NSString* category = session.category;
    AVAudioSessionCategoryOptions options = session.categoryOptions;

//     Respect old category options if category is
//     AVAudioSessionCategoryPlayAndRecord. Otherwise reset it since old options
//     might not be valid for this category.
    if ([category isEqualToString:AVAudioSessionCategoryPlayAndRecord]) {
      if (enable) {
        options |= AVAudioSessionCategoryOptionDefaultToSpeaker;
      } else {
        options &= ~AVAudioSessionCategoryOptionDefaultToSpeaker;
      }
    } else {
        if (enable) {
            options = AVAudioSessionCategoryOptionDefaultToSpeaker;
        } else {
            [session setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
//            if (error != nil) {
//                NSLog(@"Error changing default output route");
//                return -1;
//            }

        }
    }
    options |= AVAudioSessionCategoryOptionMixWithOthers;
    [session setCategory:AVAudioSessionCategoryPlayAndRecord
             withOptions:options
                   error:&error];
    if (enable) {
        //设置外放
        [session overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:&error];
    }else{
        //设置听筒
        [session overrideOutputAudioPort:AVAudioSessionPortOverrideNone error:&error];
    }
    [session setActive:YES error:nil];
    if (error) {
        return -1;
    }
    kLoudSpeakerStatus = enable;
    return 0;
}

bool ObjCCallClient::GetSpeakerStatus(bool& enable) {
    enable = kLoudSpeakerStatus;
    return 0;
}

void ObjCCallClient::StopCapture(){
  [capturer_ stopCapture];
}

void ObjCCallClient::SwitchCamera(){
  _usingFrontCamera = !_usingFrontCamera;
  //StartCapture();
}

int ObjCCallClient::GetNumberOfVideoDevices(){
    if(_captureInfo)
        return  _captureInfo->NumberOfDevices();
     NSArray<AVCaptureDevice *> *captureDevices = [RTCCameraVideoCapturer captureDevices];
    return captureDevices.count;
}
rtc::VideoSinkInterface<webrtc::VideoFrame>*  ObjCCallClient::getRemoteVideoSilkByChannelID(int channelID){
  //     std::map<int,std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>>::iterator it = remote_sinks.find(channelID);
  //        if(it != remote_sinks.end())
  //            return it->second.get();
  if(rm_)
    return rm_->getRemoteVideoSilkByChannelID(channelID);
  return nullptr;
}
webrtc::VideoTrackSourceInterface* ObjCCallClient::getLocalVideoSource(rtc::Thread* signaling_thread,rtc::Thread* worker_thread){
    if(!video_source_)
        NSLog(@"getLocalVideoSource is null");
  return video_source_;
}

std::unique_ptr<webrtc::VideoEncoderFactory> ObjCCallClient::getVideoEncoderFactory(){
  std::unique_ptr<webrtc::VideoEncoderFactory> videoEncoderFactory =
  webrtc::ObjCToNativeVideoEncoderFactory([[RTCDefaultVideoEncoderFactory alloc] init]);
  return videoEncoderFactory;
}
std::unique_ptr<webrtc::VideoDecoderFactory> ObjCCallClient::getVideoDecoderFactory(){
  std::unique_ptr<webrtc::VideoDecoderFactory> videoDecoderFactory =
  webrtc::ObjCToNativeVideoDecoderFactory([[RTCDefaultVideoDecoderFactory alloc] init]);
  return videoDecoderFactory;
}

void ObjCCallClient::SetCaptureTargetSize(int width, int height){
  targetWidth = width;
  targetHeight = height;
}

int ObjCCallClient::NumberOfCapabilities(int deviceId){
    if(_captureInfo)
        return _captureInfo->NumberOfCapabilities(nullptr);
    return 4;
}

int32_t  ObjCCallClient::GetCapability(const char* deviceUniqueIdUTF8,
                      const uint32_t deviceCapabilityNumber,
                      webrtc::VideoCaptureCapability& capability){
    return _captureInfo->GetCapability(nullptr,deviceCapabilityNumber,capability);
    
}
webrtc::VideoCaptureModule::DeviceInfo* ObjCCallClient::getVideoCaptureDeviceInfo(){
    return new VideoCaptureiOSDeviceInfo(0);
}
void ObjCCallClient::SetCameraIndex(int index){
  
}
bool ObjCCallClient::PreviewTrack(int window_id, void* video_track){
  // iOS ignore ;
  return true;
}
//}  // namespace webrtc_examples



