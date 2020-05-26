#include "win_gdi_render.h"

#include "rtc_base/logging.h"
#include "rtc_base/checks.h"
#include "rtc_base/arraysize.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"
#include "system_wrappers/include/sleep.h"
#include "api/video/i420_buffer.h"


WinGdiRender::WinGdiRender(void* window, const int render_mode, bool mirror)
        : _hWnd((HWND)window),
          _mirrorRender(mirror),
          _renderMode(render_mode),
          _running(false) {
  rtc::CritScope lock(&_render_buffer_lock);
  ZeroMemory(&_bmi, sizeof(_bmi));
  _bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  _bmi.bmiHeader.biPlanes = 1;
  _bmi.bmiHeader.biBitCount = 32;
  _bmi.bmiHeader.biCompression = BI_RGB;
  _bmi.bmiHeader.biWidth = (LONG)::GetSystemMetrics(SM_CXSCREEN);
  _bmi.bmiHeader.biHeight = -(LONG)::GetSystemMetrics(SM_CYSCREEN);
  _bmi.bmiHeader.biSizeImage =
      _bmi.bmiHeader.biWidth*(-_bmi.bmiHeader.biHeight) * (_bmi.bmiHeader.biBitCount >> 3);

  if (_hWnd) {
    hDC_ = GetDC(_hWnd);
  } else {
    hDC_ = nullptr;
  }
  RTC_LOG(INFO) << "new gdi_render..." ;
}

WinGdiRender::~WinGdiRender() {
  if (_running) {
    _running = false;
  }
}

int WinGdiRender::Init() {
  return 0;
}

int WinGdiRender::StartRenderInternal() {
  if (_running) {
    RTC_LOG(LS_WARNING) << "render thread already running.";
    return -1;
  }

  _running = true;

  return 0;
}

int WinGdiRender::StopRenderInternal() {
  if (!_running)
    RTC_LOG(LS_WARNING) << "no render thread is running.";

  _running = false;
 
  return 0;
}

VideoRenderType WinGdiRender::RenderType() {
  return kRenderWindows;
}

int WinGdiRender::RenderFrame(const webrtc::VideoFrame& videoFrame) {
  rtc::CritScope lock(&_render_buffer_lock);
  rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
      videoFrame.video_frame_buffer()->ToI420());
  if (videoFrame.rotation() != webrtc::kVideoRotation_0) {
    buffer = webrtc::I420Buffer::Rotate(*buffer, videoFrame.rotation());
  }

  SetSize(buffer->width(), buffer->height());

  RTC_DCHECK(_image.get() != NULL);
  
  
  if (_mirrorRender) {
    std::unique_ptr<uint8_t[]> mirror_image;
    mirror_image.reset(new uint8_t[_bmi.bmiHeader.biSizeImage]);
    RTC_DCHECK(_image.get() != NULL);
    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                       buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                       mirror_image.get(),
                       _bmi.bmiHeader.biWidth * _bmi.bmiHeader.biBitCount / 8,
                       buffer->width(), buffer->height());
    libyuv::ARGBMirror(mirror_image.get(),
                       _bmi.bmiHeader.biWidth * _bmi.bmiHeader.biBitCount / 8,
                       _image.get(),
                       _bmi.bmiHeader.biWidth * _bmi.bmiHeader.biBitCount / 8,
                       buffer->width(), buffer->height());
  } else {
    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                       buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                       _image.get(),
                       _bmi.bmiHeader.biWidth * _bmi.bmiHeader.biBitCount / 8,
                       buffer->width(), buffer->height());
  }
  Paint();
  return 0;
}

  

void WinGdiRender::Paint(){
  if (_image != nullptr && handle() != nullptr && hDC_ != nullptr) {
    int height = abs(_bmi.bmiHeader.biHeight);
    int width = _bmi.bmiHeader.biWidth;

    RECT rc;
    ::GetClientRect(handle(), &rc);

    HDC dc_mem = ::CreateCompatibleDC(hDC_);
    ::SetStretchBltMode(dc_mem, HALFTONE);

    // Set the map mode so that the ratio will be maintained for us.
    HDC all_dc[] = {hDC_, dc_mem};
    for (size_t i = 0; i < arraysize(all_dc); ++i) {
      SetMapMode(all_dc[i], MM_ISOTROPIC);
      SetWindowExtEx(all_dc[i], width, height, NULL);
      SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
    }

    HBITMAP bmp_mem = ::CreateCompatibleBitmap(hDC_, rc.right, rc.bottom);
    HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

    POINT logical_area = {rc.right, rc.bottom};
    DPtoLP(hDC_, &logical_area, 1);

    HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
    RECT logical_rect = {0, 0, logical_area.x, logical_area.y};
    ::FillRect(dc_mem, &logical_rect, brush);
    ::DeleteObject(brush);

    int x = (logical_area.x - width) / 2;
    int y = (logical_area.y - height) / 2;
    x = x > 0 ? x : 0;
    y = y > 0 ? y : 0;
    /*  RTC_LOG(INFO) << __FUNCTION__ << "()"
                    << "x:" << x << " y:" << y << "  width:" << width
                    << " height:" << height << "logica_x:" << logical_area.x
                    << " logical_area.y: " << logical_area.y;*/
    if (_renderMode == 1) {
      StretchDIBits(dc_mem, x, y, width, height, 0, 0, width, height,
                    _image.get(), &_bmi, DIB_RGB_COLORS, SRCCOPY);
      //RTC_LOG(INFO) << "render mode = 1";
    } else {
      if (x > 0) {
        y = x * height / width;
        x = 0;
      } else {
        x = y * width / height;
        y = 0;
      }
      StretchDIBits(dc_mem, 0, 0, logical_area.x, logical_area.y, x, y,
                    width - x, height - y, _image.get(), &_bmi, DIB_RGB_COLORS,
                    SRCCOPY);
    }

    BitBlt(hDC_, 0, 0, logical_area.x, logical_area.y, dc_mem, 0, 0, SRCCOPY);

    ::SelectObject(dc_mem, bmp_old);
    ::DeleteObject(bmp_mem);
    ::DeleteDC(dc_mem);
  }

}

void WinGdiRender::SetSize(int width, int height) {
  rtc::CritScope lock(&_render_buffer_lock);
  if (width == _bmi.bmiHeader.biWidth && height == _bmi.bmiHeader.biHeight) {
    return;
  }
  _bmi.bmiHeader.biWidth = width;
  _bmi.bmiHeader.biHeight = -height;
  _bmi.bmiHeader.biSizeImage =
      width * height * (_bmi.bmiHeader.biBitCount >> 3);
  _image.reset(new uint8_t[_bmi.bmiHeader.biSizeImage]);
}
