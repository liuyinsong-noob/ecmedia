#ifndef VIDEO_CAPTURER_H
#define VIDEO_CAPTURER_H

#include "i_video_capturer.h"

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "api/video/video_source_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "modules/video_capture/video_capture.h"
#include "pc/video_track_source.h"


class VideoCapturer : public rtc::VideoSourceInterface<webrtc::VideoFrame>,
                      public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static VideoCapturer* CreateCaptureDevice(
      const char* device_unique_idUTF8,
      const uint32_t device_unique_idUTF8Length);
  
  static VideoCapturer* CreateCaptureDevice(int width,
                                                           int height,
                                                           int fps,
                                                           int index);
  ~VideoCapturer();

  int StartCapture(webrtc::VideoCaptureCapability &capability);
  int StopCapture();

  // Implement VideoSourceInterface<>
  void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;

  // Implement VideoSinkInterface<>
  void OnFrame(const webrtc::VideoFrame& frame) override;

 protected:
  VideoCapturer();
  bool Init(const char* device_unique_idUTF8,
            const uint32_t device_unique_idUTF8Length);
  bool Init(int width,
            int height,
            int fps,
            const char* device_unique_idUTF8,
            const uint32_t device_unique_idUTF8Length);

 private:
  void UpdateVideoAdapter();
  void Destroy();

  rtc::VideoBroadcaster broadcaster_;
  cricket::VideoAdapter video_adapter_;

  rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
  //std::unique_ptr<webrtc::VideoCaptureModule> vcm_;
  webrtc::VideoCaptureCapability capability_;

  // IVideoCapturer* ptr_capturer_;
};

#endif