/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CAPTURER_H_

#include <vector>

#include "../base/thread_annotations.h"
#include "common_types.h"
#include "engine_configurations.h"
#include "video_capture.h"
#include "video_codec_interface.h"
#include "video_coding.h"
#include "video_processing.h"
#include "../system_wrappers/include/scoped_ptr.h"
#include "typedefs.h"
#include "vie_base.h"
#include "vie_capture.h"
#include "vie_defines.h"
#include "vie_frame_provider_base.h"

#include "send_statistics_proxy.h"
#include "vie_capturer.h"

#include "video_beauty_filter.h"
#include "ipl_color.h"

namespace cloopenwebrtc {

class Config;
class CriticalSectionWrapper;
class EventWrapper;
class CpuOveruseObserver;
class OveruseFrameDetector;
class ProcessThread;
class ThreadWrapper;
class ViEEffectFilter;
class ViEEncoder;
struct ViEPicture;

class ViECapturer
    : public ViEFrameProviderBase,
      public ViEExternalCapture,
      protected VideoCaptureDataCallback,
      protected VideoCaptureFeedBack {
 public:
  static ViECapturer* CreateViECapture(int capture_id,
                                       int engine_id,
                                       const Config& config,
                                       VideoCaptureModule* capture_module,
                                       ProcessThread& module_process_thread);

  static ViECapturer* CreateViECapture(
	  int capture_id,
	  int engine_id,
	  const Config& config,
	  const char* device_unique_idUTF8,
	  uint32_t device_unique_idUTF8Length,
	  ProcessThread& module_process_thread,
	  CaptureCapability *settings = NULL);

  ~ViECapturer();

  // Implements ViEFrameProviderBase.
  int FrameCallbackChanged();
  virtual int DeregisterFrameCallback(const ViEFrameCallback* callbackObject);
  bool IsFrameCallbackRegistered(const ViEFrameCallback* callbackObject);

  // Implements ExternalCapture.
  virtual int IncomingFrame(unsigned char* video_frame,
                            size_t video_frame_length,
                            uint16_t width,
                            uint16_t height,
                            RawVideoType video_type,
                            unsigned long long capture_time = 0);  // NOLINT

  virtual int IncomingFrameI420(const ViEVideoFrameI420& video_frame,
                                unsigned long long capture_time = 0);  // NOLINT

  virtual void SwapFrame(I420VideoFrame* frame) /*OVERRIDE*/;

  // Start/Stop.
  int32_t Start(
      const CaptureCapability& capture_capability = CaptureCapability());
  int32_t Stop();
  bool Started();

  // Overrides the capture delay.
  int32_t SetCaptureDelay(int32_t delay_ms);
  WebRtc_Word32 SetLocalVieoWindow(void* window);

  // Sets rotation of the incoming captured frame.
  int32_t SetRotateCapturedFrames(const RotateCapturedFrame rotation);

  // Effect filter.
  int32_t RegisterEffectFilter(ViEEffectFilter* effect_filter);
  int32_t EnableDeflickering(bool enable);
  int32_t EnableBrightnessAlarm(bool enable);

  //Beauty filter
  int32_t EnableBeautyFilter(bool enable);

  // Statistics observer.
  int32_t RegisterObserver(ViECaptureObserver* observer);
  int32_t DeRegisterObserver();
  bool IsObserverRegistered();

  // Information.
  const char* CurrentDeviceName() const;

  void RegisterCpuOveruseObserver(CpuOveruseObserver* observer);
  void SetCpuOveruseOptions(const CpuOveruseOptions& options);
  void GetCpuOveruseMetrics(CpuOveruseMetrics* metrics) const;

 protected:
  ViECapturer(int capture_id,
              int engine_id,
              const Config& config,
              ProcessThread& module_process_thread);

  int32_t Init(VideoCaptureModule* capture_module);

  int32_t Init(const char* device_unique_idUTF8,
               uint32_t device_unique_idUTF8Length,
			   CaptureCapability *settings = NULL);


  // Implements VideoCaptureDataCallback.
  virtual void OnIncomingCapturedFrame(const int32_t id,
                                       I420VideoFrame& video_frame);
  virtual void OnCaptureDelayChanged(const int32_t id,
                                     const int32_t delay);

  // Returns true if the capture capability has been set in |StartCapture|
  // function and may not be changed.
  bool CaptureCapabilityFixed();

  // Help function used for keeping track of VideoImageProcesingModule.
  // Creates the module if it is needed, returns 0 on success and guarantees
  // that the image proc module exist.
  int32_t IncImageProcRefCount();
  int32_t DecImageProcRefCount();

  // Implements VideoCaptureFeedBack
  virtual void OnCaptureFrameRate(const int32_t id,
                                  const uint32_t frame_rate);
  virtual void OnNoPictureAlarm(const int32_t id,
                                const VideoCaptureAlarm alarm);

  // Thread functions for deliver captured frames to receivers.
  static bool ViECaptureThreadFunction(void* obj);
  bool ViECaptureProcess();

  void DeliverI420Frame(I420VideoFrame* video_frame);
  void DeliverCodedFrame(VideoFrame* video_frame);

 private:
  bool SwapCapturedAndDeliverFrameIfAvailable();

  // Never take capture_cs_ before deliver_cs_!
  scoped_ptr<CriticalSectionWrapper> capture_cs_;
  scoped_ptr<CriticalSectionWrapper> deliver_cs_;
  VideoCaptureModule* capture_module_;
  VideoCaptureExternal* external_capture_module_;
  ProcessThread& module_process_thread_;
  const int capture_id_;

  // Frame used in IncomingFrameI420.
  scoped_ptr<CriticalSectionWrapper> incoming_frame_cs_;
  I420VideoFrame incoming_frame_;

  // Capture thread.
  ThreadWrapper& capture_thread_;
  EventWrapper& capture_event_;
  EventWrapper& deliver_event_;

  scoped_ptr<I420VideoFrame> captured_frame_;
  scoped_ptr<I420VideoFrame> deliver_frame_;

  // Image processing.
  ViEEffectFilter* effect_filter_;
  VideoProcessingModule* image_proc_module_;
  int image_proc_module_ref_counter_;
  VideoProcessingModule::FrameStats* deflicker_frame_stats_;
  VideoProcessingModule::FrameStats* brightness_frame_stats_;
  Brightness current_brightness_level_;
  Brightness reported_brightness_level_;

  //beauty_filter
  void *beauty_filter_inst_;
  scoped_ptr<CriticalSectionWrapper> beauty_filter_cs_;

  // Statistics observer.
  scoped_ptr<CriticalSectionWrapper> observer_cs_;
  ViECaptureObserver* observer_ GUARDED_BY(observer_cs_.get());

  CaptureCapability requested_capability_;

  scoped_ptr<OveruseFrameDetector> overuse_detector_;

  //---begin
  public:
	  virtual int RegisterFrameCallback(int capture_id, ViEFrameCallback* callbackObject);

	  // Set device image.
	  WebRtc_Word32 SetCaptureDeviceImage(const I420VideoFrame& capture_device_image);

	  int SetCaptureSettings(VideoCaptureCapability settings);
	  int UpdateLossRate(int lossRate);
	  int SetSendStatisticsProxy(SendStatisticsProxy* p_sendStats);
	  //---end
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_CAPTURER_H_
