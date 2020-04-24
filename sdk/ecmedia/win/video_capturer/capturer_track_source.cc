#include "capturer_track_source.h"


rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::Create(int width,
                                                      int height,
                                                      int fps,
                                                      int index) {
  std::unique_ptr<VideoCapturer> capturer = absl::WrapUnique(
      VideoCapturer::CreateCaptureDevice(width, height, fps, index));
  if (capturer) {
    webrtc::VideoCaptureCapability cap;
    cap.height = height;
    cap.maxFPS = fps;
    cap.width = width;

    capturer->StartCapture(cap);
    return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
  }
  return nullptr;
}
