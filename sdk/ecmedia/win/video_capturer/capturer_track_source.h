#ifndef CAPTURER_TRACK_SOURCE_H
#define CAPTURER_TRACK_SOURCE_H

#include "api/video/video_frame.h"
#include "pc/video_track_source.h"
#include "video_capturer.h"

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create(int index = 1);
 protected:
  explicit CapturerTrackSource(std::unique_ptr<VideoCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<VideoCapturer> capturer_;
};
#endif