#ifndef LINUX_RENDER_H
#define LINUX_RENDER_H

#include <sys/shm.h>

#include "i_video_render.h"
#include "rtc_base/platform_thread.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "sdk/ecmedia/video_render_defines.h"
#include "video_render_frames.h"


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#define DEFAULT_RENDER_FRAME_WIDTH  640
#define DEFAULT_RENDER_FRAME_HEIGHT 480

class LinuxRender : public IVideoRender {
 public:
  LinuxRender(
                void* window,
                const int render_mode,
				bool mirror);
  ~LinuxRender();

  int Init()override;

  int StartRenderInternal() override;
  int StopRenderInternal() override;

  VideoRenderType RenderType() override;

  int RenderFrame(const webrtc::VideoFrame& videoFrame) override;

  bool IsRunning() override { return _running; }

 protected:
  // The thread rendering the screen
  static void ScreenUpdateThreadProc(void* obj);
  bool ScreenUpdateProcess();
 private:
  int FrameSizeChange(int width, int height);
  int DeliverFrame(const webrtc::VideoFrame* video_frame);
  int CreateLocalRenderer(int width, int height);
  int RemoveRenderer();
  int InitWindow(Window window, float left, float top,
                              float right, float bottom);

  //bool _fullScreen;
  int _renderMode;
  std::unique_ptr<rtc::PlatformThread> _screenUpdateThread;
  bool _running;
  // EventTimerWrapper* _screenUpdateEvent;
  bool _mirrorRender;

  int _winWidth;
  int _winHeight;
  VideoRenderFrames render_buffer_;
  rtc::CriticalSection render_buffer_lock_;

  int GetWidthHeight(webrtc::VideoType type, int bufferSize, int& width,
                       int& height);


  Display* _display;
  XShmSegmentInfo _shminfo;
  XImage* _image;
  Window _window;
  GC _gc;
  int _width; 
  int _height;
  int _outWidth; 
  int _outHeight; 
  int _xPos; 
  int _yPos;
  bool _prepared; 
  int _dispCount;

  unsigned char* _buffer;
  float _top;
  float _left;
  float _right;
  float _bottom;

  int _Id;
};

#endif
