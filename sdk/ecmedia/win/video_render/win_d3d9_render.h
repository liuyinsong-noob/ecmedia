#ifndef WIN_D3D9_RENDER_H
#define WIN_D3D9_RENDER_H

#include <d3d9.h>
#include <windows.h>
#include <thread>

#include "i_video_render.h"
#include "rtc_base/platform_thread.h"
#include "video_render_frames.h"

class WinD3d9Render : public IVideoRender {
 public:
  WinD3d9Render(
                void* window,
                const int render_mode,
				bool mirror);
  ~WinD3d9Render();

  int Init() override;

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
  // Init/close the d3d device
  int InitDevice();
  int CloseDevice();
  int InitializeD3D(HWND hWnd, D3DPRESENT_PARAMETERS* pd3dpp);
  DWORD GetVertexProcessingCaps();
  int UpdateRenderSurface();
  int ResetDevice();
  int FrameSizeChange(int width, int height);
  int DeliverFrame(const webrtc::VideoFrame* video_frame);


  HWND _hWnd;
  //bool _fullScreen;
  int _renderMode;
  RECT _originalHwndRect;
  std::unique_ptr<rtc::PlatformThread> _screenUpdateThread;
  bool _running;
  // EventTimerWrapper* _screenUpdateEvent;
  bool _mirrorRender;

  // Window size
  // or screen size ??
  UINT _winWidth;
  UINT _winHeight;

  // the frame size
  int _width;
  int _height;
  VideoRenderFrames render_buffer_;
  rtc::CriticalSection render_buffer_lock_;

  // Device
  D3DPRESENT_PARAMETERS _d3dpp;
  LPDIRECT3D9 _pD3D;              // Used to create the D3DDevice
  LPDIRECT3DDEVICE9 _pd3dDevice;  // Our rendering device
  LPDIRECT3DVERTEXBUFFER9 _pVB;   // Buffer to hold Vertices
  LPDIRECT3DTEXTURE9 _pTexture;
  LPDIRECT3DSURFACE9 _pd3dSurface;
};

#endif