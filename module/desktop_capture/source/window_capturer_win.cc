/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "window_capturer.h"

#include <assert.h>

#include "scoped_ptr.h"
#include "win32.h"
#include "desktop_frame_win.h"
#include "window_capture_utils.h"
#include "trace.h"
#include <lm.h>  
#pragma comment(lib, "netapi32.lib")  


namespace cloopenwebrtc {

namespace {

typedef HRESULT (WINAPI *DwmIsCompositionEnabledFunc)(BOOL* enabled);

BOOL CALLBACK WindowsEnumerationHandler(HWND hwnd, LPARAM param) {
  WindowCapturer::WindowList* list =
      reinterpret_cast<WindowCapturer::WindowList*>(param);

  // Skip windows that are invisible, have no title, or are owned,
  // unless they have the app window style set.
  int len = GetWindowTextLength(hwnd);
  HWND owner = GetWindow(hwnd, GW_OWNER);
  LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  if (len == 0  || !IsWindowVisible(hwnd) ||
      (owner && !(exstyle & WS_EX_APPWINDOW))) {
    return TRUE;
  }

  // Skip the Program Manager window and the Start button.
  const size_t kClassLength = 256;
  WCHAR class_name[kClassLength];
  GetClassName(hwnd, class_name, kClassLength);
  // Skip Program Manager window and the Start button. This is the same logic
  // that's used in Win32WindowPicker in libjingle. Consider filtering other
  // windows as well (e.g. toolbars).
  if (wcscmp(class_name, L"Progman") == 0 || wcscmp(class_name, L"Button") == 0)
    return TRUE;

  WindowCapturer::Window window;
  window.id = reinterpret_cast<WindowCapturer::WindowId>(hwnd);

  const size_t kTitleLength = 500;
  WCHAR window_title[kTitleLength];
  // Truncate the title if it's longer than kTitleLength.
  GetWindowText(hwnd, window_title, kTitleLength);
  window.title = cloopenwebrtc::ToUtf8(window_title);

  // Skip windows when we failed to convert the title or it is empty.
  if (window.title.empty())
    return TRUE;

  list->push_back(window);

  return TRUE;
}

class WindowCapturerWin : public WindowCapturer {
 public:
  WindowCapturerWin();
  virtual ~WindowCapturerWin();

  // WindowCapturer interface.
  bool GetWindowList(WindowList* windows) ;
  bool SelectWindow(WindowId id) ;
  bool BringSelectedWindowToFront() ;
  bool GetShareCaptureRect(int &width, int &height);

  // DesktopCapturer interface.
  void Start(Callback* callback) ;
  void Capture(const DesktopRegion& region) ;

 private:
  bool IsAeroEnabled();

  Callback* callback_;

  // HWND and HDC for the currently selected window or NULL if window is not
  // selected.
  HWND window_;

  // dwmapi.dll is used to determine if desktop compositing is enabled.
  HMODULE dwmapi_library_;
  DwmIsCompositionEnabledFunc is_composition_enabled_func_;

  DesktopSize previous_size_;

  DISALLOW_COPY_AND_ASSIGN(WindowCapturerWin);
};


/*
Windows 8 6.2
Windows 7 6.1
Windows Server 2008 R2 6.1
Windows Server 2008 6.0
Windows Vista 6.0
Windows Server 2003 R2 5.2
Windows Server 2003 5.2
Windows XP 5.1
Windows 2000 5.0
*/
WindowCapturerWin::WindowCapturerWin()
    : callback_(NULL),
      window_(NULL)
{
  // Try to load dwmapi.dll dynamically since it is not available on XP.
  dwmapi_library_ = LoadLibrary(L"dwmapi.dll");
  if (dwmapi_library_) {
    is_composition_enabled_func_ =
        reinterpret_cast<DwmIsCompositionEnabledFunc>(
            GetProcAddress(dwmapi_library_, "DwmIsCompositionEnabled"));
    assert(is_composition_enabled_func_);
  } else {
    is_composition_enabled_func_ = NULL;
  }

  HINSTANCE hUser32 = LoadLibrary(L"user32.dll");
  if (hUser32)
  {
	  typedef BOOL(WINAPI* LPSetProcessDPIAware)(void);
	  LPSetProcessDPIAware pSetProcessDPIAware = (LPSetProcessDPIAware)GetProcAddress(hUser32, "SetProcessDPIAware");
	  if (pSetProcessDPIAware)
	  {
		  pSetProcessDPIAware();
	  }
	  FreeLibrary(hUser32);
  }

}

WindowCapturerWin::~WindowCapturerWin() {
  if (dwmapi_library_)
    FreeLibrary(dwmapi_library_);
}


bool WindowCapturerWin::IsAeroEnabled() {
  BOOL result = FALSE;
  if (is_composition_enabled_func_)
    is_composition_enabled_func_(&result);
  return result != FALSE;
}

bool WindowCapturerWin::GetWindowList(WindowList* windows) {
  WindowList result;
  LPARAM param = reinterpret_cast<LPARAM>(&result);
  if (!EnumWindows(&WindowsEnumerationHandler, param))
    return false;
  windows->swap(result);
  return true;
}

bool WindowCapturerWin::SelectWindow(WindowId id) {
  HWND window = reinterpret_cast<HWND>(id);
  if (!IsWindow(window) || !IsWindowVisible(window))
    return false;

  if(IsIconic(window))
  {
	  ShowWindow(window, SW_SHOWNORMAL);
  }

  window_ = window;
  previous_size_.set(0, 0);
  return true;
}

bool WindowCapturerWin::BringSelectedWindowToFront() {
  if (!window_)
    return false;

  if (!IsWindow(window_) || !IsWindowVisible(window_))
    return false;

  if(IsIconic(window_))
  {
	  ShowWindow(window_, SW_SHOWNORMAL);
  }

  return SetForegroundWindow(window_) != 0;
}

bool  WindowCapturerWin::GetShareCaptureRect(int &width, int &height)
{
    if (!window_)
        return false;

    DesktopRect original_rect;
    DesktopRect cropped_rect;
    if (!GetCroppedWindowRect(window_, &cropped_rect, &original_rect)) {
        WEBRTC_TRACE(kTraceWarning, kTraceVideoCapture, -1,
            "Failed to get window info: %d",GetLastError());
        return false;
    }
    width = cropped_rect.width();
    height = cropped_rect.height();
    return true;
}

void WindowCapturerWin::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

void WindowCapturerWin::Capture(const DesktopRegion& region) {
  if (!window_) {
	WEBRTC_TRACE(kTraceError, kTraceVideoCapture, -1,
		"Window hasn't been selected: %d",GetLastError());
    callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
    return;
  }

  // Stop capturing if the window has been closed or hidden.
  if (!IsWindow(window_)) {
    callback_->OnCaptureCompleted(NULL, kCapture_Window_Closed);
    return;
  }

  if(!IsWindowVisible(window_))
  {
      callback_->OnCaptureCompleted(NULL, kCapture_Window_Hidden);
      return;
  }

  // Return a 1x1 black frame if the window is minimized, to match the behavior
  // on Mac.
  if (IsIconic(window_)) {
    BasicDesktopFrame* frame = new BasicDesktopFrame(DesktopSize(1, 1));
    memset(frame->data(), 0, frame->stride() * frame->size().height());

    previous_size_ = frame->size();
    callback_->OnCaptureCompleted(frame, kCapture_Window_IsIconic);
    return;
  }
  
  DesktopRect original_rect;
  DesktopRect cropped_rect;
  if (!GetCroppedWindowRect(window_, &cropped_rect, &original_rect)) {
	WEBRTC_TRACE(kTraceWarning, kTraceVideoCapture, -1,
		"Failed to get window info: %d",GetLastError());
    callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
    return;
  }
  //::SetWindowPos(window_, HWND_TOPMOST, 0, 0, original_rect.width(), original_rect.height(), SWP_NOMOVE);
  //Make sure the width and height of the frame are even numbers.
  //If you don't do this, it will cause the I420FRMAE to handle the crash.
  int width = cropped_rect.width();
  int height = cropped_rect.height();
  if(width % 2)
	  width += 1;
  if(height % 2)
	  height +=1;

  cropped_rect =  DesktopRect::MakeXYWH(cropped_rect.left(),cropped_rect.top(),
	  width, height);

  HDC window_dc = GetDC(NULL);
  if (!window_dc) {
	WEBRTC_TRACE(kTraceWarning, kTraceVideoCapture, -1,
		"Failed to get window DC: %d",GetLastError());
    callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
    return;
  }

  cloopenwebrtc::scoped_ptr<DesktopFrameWin> frame(
      DesktopFrameWin::Create(cropped_rect.size(), NULL, window_dc));
  if (!frame.get()) {
    ReleaseDC(window_, window_dc);
    callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
    return;
  }

  HDC mem_dc = CreateCompatibleDC(window_dc);
  HGDIOBJ previous_object = SelectObject(mem_dc, frame->bitmap());
  BOOL result = FALSE;

  // When desktop composition (Aero) is enabled each window is rendered to a
  // private buffer allowing BitBlt() to get the window content even if the
  // window is occluded. PrintWindow() is slower but lets rendering the window
  // contents to an off-screen device context when Aero is not available.
  // PrintWindow() is not supported by some applications.
  //
  // If Aero is enabled, we prefer BitBlt() because it's faster and avoids
  // window flickering. Otherwise, we prefer PrintWindow() because BitBlt() may
  // render occluding windows on top of the desired window.
  //
  // When composition is enabled the DC returned by GetWindowDC() doesn't always
  // have window frame rendered correctly. Windows renders it only once and then
  // caches the result between captures. We hack it around by calling
  // PrintWindow() whenever window size changes, including the first time of
  // capturing - it somehow affects what we get from BitBlt() on the subsequent
  // captures.

	if (!IsAeroEnabled() || !previous_size_.equals(frame->size())) {
		result = PrintWindow(window_, mem_dc, 0);
	}
	frame->size().height();
	// Aero is enabled or PrintWindow() failed, use BitBlt.
	if (!result) {
		result = BitBlt(mem_dc, 0, 0, 
			frame->size().width(),
			frame->size().height(),
			window_dc,
			cropped_rect.left(),
			cropped_rect.top(),
			SRCCOPY | CAPTUREBLT);
	}

  SelectObject(mem_dc, previous_object);
  DeleteDC(mem_dc);
  ReleaseDC(window_, window_dc);

  previous_size_ = frame->size();

  frame->mutable_updated_region()->SetRect(
      DesktopRect::MakeSize(frame->size()));

  if (!result) {
	WEBRTC_TRACE(kTraceError, kTraceVideoCapture, -1,
		"Both PrintWindow() and BitBlt() failed: %d",GetLastError());
    frame.reset();
  }

  callback_->OnCaptureCompleted(frame.release(), kCapture_Ok, &original_rect);
}

}  // namespace

// static
WindowCapturer* WindowCapturer::Create(const DesktopCaptureOptions& options) {
  return new WindowCapturerWin();
}

}  // namespace cloopenwebrtc

