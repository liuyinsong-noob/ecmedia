#ifndef WIN_GDI_RENDER_H
#define WIN_GDI_RENDER_H

#include <windows.h>
#include <thread>

#include "i_video_render.h"
#include "rtc_base/platform_thread.h"
#include "video_render_frames.h"


class WinGdiRender: public IVideoRender {
 public:
  WinGdiRender(void* window, const int render_mode, bool mirror);

  ~WinGdiRender();

  int Init() override;

  int StartRenderInternal() override;
  int StopRenderInternal() override;

  VideoRenderType RenderType() override;

  int RenderFrame(const webrtc::VideoFrame& videoFrame) override;

  bool IsRunning() override { return _running; }

 protected:
  void SetSize(int width, int height);
  void Paint();

 private:
  HWND handle() const { return _hWnd; }

 private:
  HWND _hWnd; 
  bool _mirrorRender;
  int _renderMode;
  HDC hDC_;
  BITMAPINFO _bmi;
  std::unique_ptr<uint8_t[]> _image;
  rtc::CriticalSection _render_buffer_lock;
  std::unique_ptr<rtc::PlatformThread> _screenUpdateThread;
  VideoRenderFrames _render_buffer;
  bool _running;

};
 



#endif
