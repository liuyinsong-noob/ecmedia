#include "capturer_track_source.h"


CapturerTrackSource::CapturerTrackSource(bool remote, rtc::VideoSourceInterface<webrtc::VideoFrame>* video_capturer) :  webrtc::VideoTrackSource(remote), video_capturer_(video_capturer)
{
}

rtc::VideoSourceInterface<webrtc::VideoFrame>* CapturerTrackSource::source() {
  return video_capturer_;
 }