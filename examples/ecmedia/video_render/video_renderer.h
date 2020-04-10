#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include "i_video_render.h"

class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static VideoRenderer* CreateVideoRenderer(
      void* windows,
      bool full_screen,
      webrtc::VideoTrackInterface* track_to_render,
      rtc::Thread* worker_thread,
      const VideoRenderType type = kRenderDefault);

  virtual ~VideoRenderer() {}

  virtual int StartRender(int channel_id) = 0;
  virtual int StopRender(int channel_id) = 0;
  virtual void OnFrame(const webrtc::VideoFrame& frame) = 0;
};

class VideoRenderImpl : public VideoRenderer {
 public:
  VideoRenderImpl(void* windows,
                  bool full_screen,
                  webrtc::VideoTrackInterface* track_to_render,
                  rtc::Thread* worker_thread,
                  VideoRenderType render_type);

  virtual ~VideoRenderImpl();

  int StartRender(int channel_id) override;
  int StopRender(int channel_id) override;

  // implement rtc::VideoSinkInterface<webrtc::VideoFrame>
  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  rtc::Thread* worker_thread_;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
  IVideoRender* _ptrRenderer;
};

#endif  // VIDEO_RENDERER_H
