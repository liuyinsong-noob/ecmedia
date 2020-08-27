#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H
#include "video_render_defines.h"
#include "sdk/ecmedia/sdk_common.h"

#if defined(WEBRTC_IOS)
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/thread_checker.h"
#endif


class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static VideoRenderer* CreateVideoRenderer(
	  int channelid,
      void* windows,
      int render_mode,
	    bool mirror,
      webrtc::VideoTrackInterface* track_to_render,
      rtc::Thread* worker_thread,
      const VideoRenderType type = kRenderDefault,
      rtc::VideoSinkWants wants = rtc::VideoSinkWants());
  virtual ~VideoRenderer() {}

  virtual int StartRender() = 0;
  virtual int StopRender() = 0;
  virtual int UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render,
                               rtc::VideoSinkWants wants) = 0;
  virtual int SaveVideoSnapshot(const char* fileName) = 0;
  virtual void OnFrame(const webrtc::VideoFrame& frame) = 0;
  virtual void* WindowPtr()= 0;
  virtual bool RegisterRemoteVideoResoluteCallback(
      ECMedia_FrameSizeChangeCallback* callback) = 0;
};

#endif  // VIDEO_RENDERER_H
