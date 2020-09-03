#ifndef VIDEO_RENDERER_IMPL_H
#define VIDEO_RENDERER_IMPL_H

#include "i_video_render.h"
#include "sdk/ecmedia/video_renderer.h"
#include "sdk/ecmedia/sdk_common.h"
#include "common_video/include/jpeg.h"
class VideoRenderImpl : public VideoRenderer {
 public:
  VideoRenderImpl(int channelid,
                  void* windows,
                  int render_mode,
                  bool mirror,
                  webrtc::VideoTrackInterface* track_to_render,
                  rtc::Thread* worker_thread,
                  VideoRenderType render_type,
                  rtc::VideoSinkWants wants);

  virtual ~VideoRenderImpl();

  int StartRender() override;
  int StopRender() override;

  int UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render,
                       rtc::VideoSinkWants wants) override;
  int SaveVideoSnapshot(const char* fileName)override;
  // implement rtc::VideoSinkInterface<webrtc::VideoFrame>
  void OnFrame(const webrtc::VideoFrame& frame) override;
  bool RegisterRemoteVideoResoluteCallback(
      ECMedia_FrameSizeChangeCallback* callback) override;

  void* WindowPtr() override { return video_window_; }

 private:
  rtc::CriticalSection snapshot_;
  std::vector<webrtc::VideoFrame> shot_frames_;
  int channelid_;
  rtc::Thread* worker_thread_;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
  IVideoRender* _ptrRenderer;
  void* video_window_;
  int last_width_;
  int last_height_;
  ECMedia_FrameSizeChangeCallback* callback_;
};

#endif  // VIDEO_RENDERER_H
