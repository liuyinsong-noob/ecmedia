//#include <d3dx9.h>

#include "win_d3d9_render.h"
#include <map>

#include "api/video/i420_buffer.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/sleep.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"

#pragma comment(lib, "d3d9.lib")

// A structure for our custom vertex type
struct CUSTOMVERTEX {
  FLOAT x, y, z;
  DWORD color;  // The vertex color
  FLOAT u, v;
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

WinD3d9Render::WinD3d9Render(void* window, const int render_mode, bool mirror)
    : _hWnd((HWND)window),
      _renderMode(render_mode),
      _screenUpdateThread(new rtc::PlatformThread(ScreenUpdateThreadProc,
                                                  this,
                                                  "win_thread_render_update",
                                                  rtc::kNormalPriority)),
      _running(false),
      _mirrorRender(mirror),
      _pD3D(nullptr),
      _pd3dDevice(nullptr),
      _pVB(nullptr),
      _pTexture(nullptr),
      _pd3dSurface(nullptr) {
  // _screenUpdateEvent = EventTimerWrapper::Create();
  SetRect(&_originalHwndRect, 0, 0, 0, 0);
}

WinD3d9Render::~WinD3d9Render() {
  if (_running) {
    _running = false;
    _screenUpdateThread->Stop();
  }

  CloseDevice();
  // release the texture
  if (_pTexture != NULL) {
    _pTexture->Release();
    _pTexture = NULL;
  }
}

int WinD3d9Render::Init() {
  RTC_LOG(LS_INFO) << "WinD3d9Render::Init";

  if (!_screenUpdateThread) {
    RTC_LOG(LS_ERROR) << "WinD3d9Render Thread not created";
    return -1;
  }
  int ret = InitDevice();

  return ret;
}

int WinD3d9Render::CloseDevice() {
  RTC_LOG(LS_INFO) << "WinD3d9Render::CloseDevice";
  if (_pVB != NULL) {
    _pVB->Release();
    _pVB = NULL;
  }

  if (_pd3dDevice != NULL) {
    _pd3dDevice->Release();
    _pd3dDevice = NULL;
  }

  if (_pD3D != NULL) {
    _pD3D->Release();
    _pD3D = NULL;
  }

  if (_pd3dSurface != NULL)
    _pd3dSurface->Release();
  return 0;
}

/*
 *
 *    Rendering process
 *
 */
void WinD3d9Render::ScreenUpdateThreadProc(void* obj) {
  static_cast<WinD3d9Render*>(obj)->ScreenUpdateProcess();
}

bool WinD3d9Render::ScreenUpdateProcess() {
  if (!_pd3dDevice) {
    RTC_LOG(LS_ERROR) << "d3dDevice not created.";
    return true;
  }

  while (_running) {
    rtc::CritScope lock(&render_buffer_lock_);
    webrtc::VideoFrame* frame_render = render_buffer_.FrameToRender();
    if (frame_render) {
      RTC_LOG(LS_INFO) << "render a frame, width:" << _width
                       << " _height:" << _height;

      if (_width != frame_render->width() ||
          _height != frame_render->height()) {
        if (FrameSizeChange(frame_render->width(), frame_render->height()) ==
            -1)
          return false;
      }

      DeliverFrame(frame_render);
    } else {
      RTC_LOG(LS_INFO) << "no frame this:" << this
                       << " theadid:" << &_screenUpdateThread;
      webrtc::SleepMs(1);
      continue;
    }
    UpdateRenderSurface();
    render_buffer_.ReturnFrame(frame_render);
  }

  return true;
}

int WinD3d9Render::UpdateRenderSurface() {
  HRESULT hr;
  if (!_pd3dDevice) {
    RTC_LOG(LS_ERROR) << "d3dDevice not created.";
    return -1;
  }
  if (FAILED(hr = _pd3dDevice->TestCooperativeLevel())) {
    // the device has been lost but cannot be reset at this time
    if (hr == D3DERR_DEVICELOST) {
      return 0;
    }

    // the device has been lost and can be reset
    if (hr == D3DERR_DEVICENOTRESET) {
      // do lost/reset/restore cycle
      hr = _pd3dDevice->Reset(&_d3dpp);
      if (FAILED(hr)) {
        CloseDevice();
        InitDevice();
        if (_pTexture != NULL) {
          _pTexture->Release();
          _pTexture = NULL;
        }
        return 0;
      }
    }
  }
  // RTC_LOG(LS_INFO) << "UpdateRenderSurface:";
  // Clear the backbuffer to a black color
  _pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

  // Begin the scene
  if (SUCCEEDED(_pd3dDevice->BeginScene())) {
    _pd3dDevice->SetStreamSource(
        0, _pVB, 0, sizeof(CUSTOMVERTEX));  //将顶点缓冲区绑定到设备数据流。
    _pd3dDevice->SetFVF(
        D3DFVF_CUSTOMVERTEX);  // Gets the fixed vertex function declaration.

    //// draw the video stream
    _pd3dDevice->SetTexture(0, _pTexture);
    _pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // End the scene
    _pd3dDevice->EndScene();
  }

  // Present the backbuffer contents to the display
  _pd3dDevice->Present(NULL, NULL, NULL, NULL);

  return 0;
}

int WinD3d9Render::InitDevice() {
  // Set up the structure used to create the D3DDevice
  ZeroMemory(&_d3dpp, sizeof(_d3dpp));
  _d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  _d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;

  if (GetWindowRect(_hWnd, &_originalHwndRect) == 0) {
    RTC_LOG(LS_ERROR)
        << "VideoRenderDirect3D9::InitDevice Could not get window";
    return -1;
  }

  _winWidth = (LONG)::GetSystemMetrics(SM_CXSCREEN);
  _winHeight = (LONG)::GetSystemMetrics(SM_CYSCREEN);

  unsigned int width = _originalHwndRect.right - _originalHwndRect.left;
  unsigned int height = _originalHwndRect.bottom - _originalHwndRect.top;

  _d3dpp.BackBufferWidth = width;
  _d3dpp.BackBufferHeight = height;

  _d3dpp.Windowed = TRUE;

  if (InitializeD3D(_hWnd, &_d3dpp) == -1) {
    return -1;
  }

  // Turn off culling, so we see the front and back of the triangle
  _pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  // Turn off D3D lighting, since we are providing our own vertex colors
  _pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

  // Settings for alpha blending
  _pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  _pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  _pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

  _pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  _pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  _pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

  // Initialize Vertices
  CUSTOMVERTEX Vertices[] = {// front
                             {-0.5f, -1.0f, 0.0f, 0xffffffff, 0.3, 1},
                             {-0.5f, 1.0f, 0.0f, 0xffffffff, 0.3, 0},
                             {0.5f, -1.0f, 0.0f, 0xffffffff, 0.8, 0.6},
                             {0.5f, 1.0f, 0.0f, 0xffffffff, 0.8, 0.4}};

  // Create the vertex buffer.
  if (FAILED(_pd3dDevice->CreateVertexBuffer(sizeof(Vertices), 0,
                                             D3DFVF_CUSTOMVERTEX,
                                             D3DPOOL_DEFAULT, &_pVB, NULL))) {
    RTC_LOG(LS_ERROR) << "Failed to create the vertex buffer.";
    return -1;
  }

  // Now we fill the vertex buffer.
  VOID* pVertices;
  if (FAILED(_pVB->Lock(0, sizeof(Vertices), (void**)&pVertices, 0))) {
    RTC_LOG(LS_ERROR) << "Failed to  lock the vertex buffer.";

    return -1;
  }
  memcpy(pVertices, Vertices, sizeof(Vertices));
  _pVB->Unlock();

  return 0;
}

int WinD3d9Render::InitializeD3D(HWND hWnd, D3DPRESENT_PARAMETERS* pd3dpp) {
  // initialize Direct3D
  if (NULL == (_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) {
    RTC_LOG(LS_ERROR) << " Direct3DCreate9 fail ";
    return -1;
  }

  // determine what type of vertex processing to use based on the device
  // capabilities
  DWORD dwVertexProcessing = GetVertexProcessingCaps();

  // get the display mode
  D3DDISPLAYMODE d3ddm;
  _pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
  pd3dpp->BackBufferFormat = d3ddm.Format;

  // create the D3D device
  if (FAILED(_pD3D->CreateDevice(
          D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
          dwVertexProcessing | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
          pd3dpp, &_pd3dDevice))) {
    // try the ref device
    if (FAILED(_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
                                   dwVertexProcessing |
                                       D3DCREATE_MULTITHREADED |
                                       D3DCREATE_FPU_PRESERVE,
                                   pd3dpp, &_pd3dDevice))) {
      RTC_LOG(LS_ERROR) << " D3D CreateDevice fail";
      return -1;
    }
  }

  return 0;
}

DWORD WinD3d9Render::GetVertexProcessingCaps() {
  D3DCAPS9 caps;
  DWORD dwVertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  if (SUCCEEDED(
          _pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps))) {
    if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ==
        D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
      dwVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    }
  }
  return dwVertexProcessing;
}

VideoRenderType WinD3d9Render::RenderType() {
  return kRenderWindows;
}

int WinD3d9Render::RenderFrame(const webrtc::VideoFrame& videoFrame) {
  DeliverFrame(&videoFrame);
  UpdateRenderSurface();
  return 0;

  // rtc::CritScope lock(&render_buffer_lock_);
  // return render_buffer_.AddFrame(&videoFrame);
}

int WinD3d9Render::FrameSizeChange(int width, int height) {
  RTC_LOG(LS_INFO) << "FrameSizeChange, width: " << width
                   << " height: " << height;

  // CriticalSectionScoped cs(_critSect);
  _width = width;
  _height = height;

  if (GetWindowRect(_hWnd, &_originalHwndRect) == 0) {
    RTC_LOG(LS_ERROR)
        << "VideoRenderDirect3D9::InitDevice Could not get window";
    return -1;
  }

  unsigned int windowsWidth = _originalHwndRect.right - _originalHwndRect.left;
  unsigned int windowsHeight = _originalHwndRect.bottom - _originalHwndRect.top;

//   _d3dpp.BackBufferWidth = windowsWidth;
//   _d3dpp.BackBufferHeight = windowsHeight;

  //如果窗口宽高发生变化
  if (_d3dpp.BackBufferWidth != windowsWidth ||
      _d3dpp.BackBufferHeight != windowsHeight) {
    CloseDevice();
    InitDevice();
  }

  // clean the previous texture
  if (_pTexture != NULL) {
    _pTexture->Release();
    _pTexture = NULL;
  }

  HRESULT ret = E_POINTER;
  if (_pd3dDevice)
    ret = _pd3dDevice->CreateTexture(_width, _height, 1, 0, D3DFMT_A8R8G8B8,
                                     D3DPOOL_MANAGED, &_pTexture, NULL);
  if (FAILED(ret)) {
    _pTexture = NULL;
    return -1;
  }

  // Initialize Vertices
  CUSTOMVERTEX Vertices[] = {// front
                             {-1.0f, -1.0f, 0.0f, 0xffffffff, 0, 1},
                             {-1.0f, 1.0f, 0.0f, 0xffffffff, 0, 0},
                             {1.0f, -1.0f, 0.0f, 0xffffffff, 1, 1},
                             {1.0f, 1.0f, 0.0f, 0xffffffff, 1, 0}};
  float winRatio = (float)_winWidth / (float)_winHeight;
  float videoRatio = (float)_width / (float)_height;

  if (_renderMode == 0) {
    //剪切模式 crop
    if (winRatio >= videoRatio) {
      Vertices[0].x = Vertices[1].x = -1.0f;
      Vertices[2].x = Vertices[3].x = 1.0f;

      Vertices[1].y = Vertices[3].y = ((float)_winWidth * (float)_height) /
                                      ((float)_winHeight * (float)_width);
      Vertices[0].y = Vertices[2].y = -Vertices[1].y;
    } else {
      Vertices[2].x = Vertices[3].x = ((float)_winHeight * (float)_width) /
                                      ((float)_winWidth * (float)_height);
      Vertices[0].x = Vertices[1].x = -Vertices[2].x;

      Vertices[0].y = Vertices[2].y = -1.0f;
      Vertices[1].y = Vertices[3].y = 1.0f;
    }
  } else {
    //黑边模式 fit
    if (winRatio >= videoRatio) {
      Vertices[2].x = Vertices[3].x = ((float)_winHeight * (float)_width) /
                                      ((float)_winWidth * (float)_height);
      Vertices[0].x = Vertices[1].x = -Vertices[2].x;

      Vertices[0].y = Vertices[2].y = -1.0f;
      Vertices[1].y = Vertices[3].y = 1.0f;
    } else {
      Vertices[0].x = Vertices[1].x = -1.0f;
      Vertices[2].x = Vertices[3].x = 1.0;

      Vertices[1].y = Vertices[3].y = ((float)_winWidth * (float)_height) /
                                      ((float)_winHeight * (float)_width);
      Vertices[0].y = Vertices[2].y = -Vertices[1].y;
    }
  }

  RTC_LOG(LS_INFO) << " winWidth:" << _winWidth << " winHeight:" << _winHeight
                   << " videoW:" << _width << " videoH:" << _height;

  RTC_LOG(LS_INFO) << "\n 0.x=" << Vertices[0].x << " 0.y=" << Vertices[0].y
                   << "\n 1.x=" << Vertices[1].x << " 1.y=" << Vertices[1].y
                   << "\n 2.x=" << Vertices[2].x << " 2.y=" << Vertices[2].y
                   << "\n 3.x=" << Vertices[3].x << " 3.y=" << Vertices[3].y;

  // Now we fill the vertex buffer.
  VOID* pVertices;
  if (FAILED(_pVB->Lock(0, sizeof(Vertices), (void**)&pVertices, 0))) {
    RTC_LOG(LS_ERROR) << "Failed to  lock the vertex buffer.";
    return -1;
  }
  memcpy(pVertices, Vertices, sizeof(Vertices));
  _pVB->Unlock();

  return 0;
}

// Called from video engine when a new frame should be rendered.
int WinD3d9Render::DeliverFrame(const webrtc::VideoFrame* video_frame) {
  // CriticalSectionScoped cs(_critSect);

  rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
      video_frame->video_frame_buffer()->ToI420());

  if (video_frame->rotation() != webrtc::kVideoRotation_0) {
    buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame->rotation());
  }

  if (GetWindowRect(_hWnd, &_originalHwndRect) == 0) {
    RTC_LOG(LS_ERROR)
        << "VideoRenderDirect3D9::DeliverFrame Could not get window";
    return -1;
  }
  unsigned int width = _originalHwndRect.right - _originalHwndRect.left;
  unsigned int height = _originalHwndRect.bottom - _originalHwndRect.top;

  if (_width != buffer->width() || _height != buffer->height() ||
      width != _winWidth || height != _winHeight) {
    RTC_LOG(LS_INFO) << "VideoRenderDirect3D9::DeliverFrame width:" << width
                     << " height:" << height << " _winWidth:" << _winWidth
                     << " _winHeight:" << _winHeight << " _width:" << _width
                     << " _height:" << _height << " frame.w:" << buffer->width()
                     << " frame.h:" << buffer->height();
    _winWidth = width;
    _winHeight = height;
    if (FrameSizeChange(buffer->width(), buffer->height()) == -1)
      return -1;
  }

  if (!_pTexture) {
    RTC_LOG(LS_ERROR) << "Texture for rendering not initialized.";
    return -1;
  }

  D3DLOCKED_RECT lr;
  if (FAILED(_pTexture->LockRect(0, &lr, NULL, 0))) {
    RTC_LOG(LS_ERROR) << "Failed to lock a texture in D3D9.";
    return -1;
  }
  UCHAR* pRect = (UCHAR*)lr.pBits;

  if (_mirrorRender) {
    std::unique_ptr<uint8_t[]> mirror_image_;
    mirror_image_.reset(new uint8_t[buffer->width() * buffer->height() * 4]);

    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
        video_frame->video_frame_buffer()->ToI420());
    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                       buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                       mirror_image_.get(), lr.Pitch, buffer->width(),
                       buffer->height());

    libyuv::ARGBMirror(mirror_image_.get(), lr.Pitch, pRect, lr.Pitch,
                       buffer->width(), buffer->height());
  } else {
    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                       buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                       pRect, lr.Pitch, buffer->width(), buffer->height());
  }

  if (FAILED(_pTexture->UnlockRect(0))) {
    RTC_LOG(LS_ERROR) << "Failed to unlock a texture in D3D9.";
    return -1;
  }

  return 0;
}

int WinD3d9Render::StartRenderInternal() {
  if (_running) {
    RTC_LOG(LS_WARNING) << "render thread already running.";
    return -1;
  }

  _running = true;
  //_screenUpdateThread->Start();

  return 0;
}

int WinD3d9Render::StopRenderInternal() {
  if (!_running)
    RTC_LOG(LS_WARNING) << "no render thread is running.";

  _running = false;
  //_screenUpdateThread->Stop();
  return 0;
}