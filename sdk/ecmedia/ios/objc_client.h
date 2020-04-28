/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef OBJCNATIVEAPI_OBJCCALLCLIENT_H_
#define OBJCNATIVEAPI_OBJCCALLCLIENT_H_

#include <memory>
#include <string>

#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/thread_checker.h"


class ObjCCallClient {
private:
    ObjCCallClient();
    static ObjCCallClient* m_pInstance;
public:
    static ObjCCallClient* GetInstance();
    ~ObjCCallClient();
    
    void Call();
    bool InitDevice(rtc::Thread* signaling_thread,rtc::Thread* worker_thread) ;
    void Hangup();
    void SetLocalWindowView(void* local) ;
    void SetRemoteWindowView(int channelID, void* remoteView ) ;
    rtc::VideoSinkInterface<webrtc::VideoFrame>*  getRemoteVideoSilkByChannelID(int channelID);
    webrtc::VideoTrackSourceInterface* getLocalVideoSource(rtc::Thread* signaling_thread,rtc::Thread* worker_thread);
    std::unique_ptr<webrtc::VideoEncoderFactory> getVideoEncoderFactory() ;
    std::unique_ptr<webrtc::VideoDecoderFactory> getVideoDecoderFactory() ;
    void SetCameraFPS(int fps);
    void StartCapture(int deviceid, webrtc::VideoCaptureCapability& cap) ;
    void StopCapture() ;
    void SwitchCamera() ;
    void SetCaptureTargetSize(int width, int height);
    int GetNumberOfVideoDevices();
    int NumberOfCapabilities(int deviceId);
    void SetCameraIndex(int index);
    int32_t GetCapability(const char* deviceUniqueIdUTF8,
                          const uint32_t deviceCapabilityNumber,
                           webrtc::VideoCaptureCapability& capability);
    
    void CreatePeerConnectionFactory() RTC_RUN_ON(thread_checker_);
    void CreatePeerConnection() RTC_RUN_ON(thread_checker_);
    void Connect() RTC_RUN_ON(thread_checker_);
    bool SetSpeakerStatus(bool enable);
    bool GetSpeakerStatus(bool& enable);
    bool PreviewTrack(int window_id, void* video_track);
    rtc::ThreadChecker thread_checker_;
    
    bool call_started_ RTC_GUARDED_BY(thread_checker_);
    
    bool  getRemoteView(int channelID);
    
    webrtc::VideoCaptureModule::DeviceInfo* getVideoCaptureDeviceInfo();
    
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pcf_
    RTC_GUARDED_BY(thread_checker_);
    std::unique_ptr<rtc::Thread> network_thread_ RTC_GUARDED_BY(thread_checker_);
    std::unique_ptr<rtc::Thread> worker_thread_ RTC_GUARDED_BY(thread_checker_);
    std::unique_ptr<rtc::Thread> signaling_thread_
    RTC_GUARDED_BY(thread_checker_);
    
    std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> remote_sink_;
    std::map<int,std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>> remote_sinks;
    //RTC_GUARDED_BY(thread_checker_);
    rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> video_source_;
    //RTC_GUARDED_BY(thread_checker_);
    
    rtc::CriticalSection pc_mutex_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc_
    RTC_GUARDED_BY(pc_mutex_);
    int m_fps;
    bool _usingFrontCamera;
    int targetWidth ;
    int targetHeight ;
    webrtc::VideoCaptureModule::DeviceInfo* _captureInfo;
};

//}  // namespace webrtc_examples

#endif  // EXAMPLES_OBJCNATIVEAPI_OBJCCALLCLIENT_H_
