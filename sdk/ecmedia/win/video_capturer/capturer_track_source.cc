#include "capturer_track_source.h"

rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::Create(int index) {
  const size_t kWidth = 1280;
  const size_t kHeight = 960;
  const size_t kFps = 15;
  std::unique_ptr<VideoCapturer> capturer = absl::WrapUnique(
      VideoCapturer::CreateCaptureDevice(kWidth, kHeight, kFps, index));
  if (capturer) {
    return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
  }
  return nullptr;
}
