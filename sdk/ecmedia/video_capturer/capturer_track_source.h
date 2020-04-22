#ifndef CAPTURER_TRACK_SOURCE_H
#define CAPTURER_TRACK_SOURCE_H

#include "api/video/video_frame.h"
#include "pc/video_track_source.h"

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  explicit CapturerTrackSource(bool remote,
                               rtc::VideoSourceInterface<webrtc::VideoFrame>* video_capturer);

 protected:
  virtual rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override;

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* video_capturer_;
};

#endif