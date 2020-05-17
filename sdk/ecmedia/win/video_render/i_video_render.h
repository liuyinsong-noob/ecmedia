#ifndef I_VIDEO_RENDER_H
#define I_VIDEO_RENDER_H

#include "api/media_stream_interface.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "rtc_base/thread.h"
#include "sdk/ecmedia/video_render_defines.h"


class IVideoRender {
 public:
  virtual int32_t Init() = 0;
  virtual ~IVideoRender() {}

  virtual VideoRenderType RenderType() = 0;

  virtual int RenderFrame(const webrtc::VideoFrame& videoFrame) = 0;

  virtual int StartRenderInternal() = 0;
  virtual int StopRenderInternal() = 0;

  virtual bool IsRunning() = 0;
};


#endif
