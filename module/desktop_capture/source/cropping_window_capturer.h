/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_CROPPING_WINDOW_CAPTURER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_CROPPING_WINDOW_CAPTURER_H_

#include "scoped_ptr.h"
#include "desktop_capture_options.h"
#include "screen_capturer.h"
#include "window_capturer.h"

namespace cloopenwebrtc {

// WindowCapturer implementation that uses a screen capturer to capture the
// whole screen and crops the video frame to the window area when the captured
// window is on top.
class CroppingWindowCapturer : public WindowCapturer,
                               public DesktopCapturer::Callback {
 public:
  static WindowCapturer* Create(const DesktopCaptureOptions& options);
  virtual ~CroppingWindowCapturer();

  // DesktopCapturer implementation.
  void Start(DesktopCapturer::Callback* callback) ;
  void Capture(const DesktopRegion& region) ;
  void SetExcludedWindow(WindowId window) ;

  // WindowCapturer implementation.
  bool GetWindowList(WindowList* windows) ;
  bool SelectWindow(WindowId id) ;
  bool BringSelectedWindowToFront() ;
  bool GetShareCaptureRect(int &width, int &height) ;

  // DesktopCapturer::Callback implementation, passed to |screen_capturer_| to
  // intercept the capture result.
  SharedMemory* CreateSharedMemory(size_t size) ;
  void OnCaptureCompleted(DesktopFrame* frame, CaptureErrCode errCode = kCapture_Ok, DesktopRect *window_rect = NULL) ;

 protected:
  explicit CroppingWindowCapturer(const DesktopCaptureOptions& options);

  // The platform implementation should override these methods.

  // Returns true if it is OK to capture the whole screen and crop to the
  // selected window, i.e. the selected window is opaque, rectangular, and not
  // occluded.
  virtual bool ShouldUseScreenCapturer() = 0;

  // Returns the window area relative to the top left of the virtual screen
  // within the bounds of the virtual screen.
  virtual DesktopRect GetWindowRectInVirtualScreen() = 0;

  WindowId selected_window() const { return selected_window_; }
  WindowId excluded_window() const { return excluded_window_; }

 private:
  DesktopCaptureOptions options_;
  DesktopCapturer::Callback* callback_;
  cloopenwebrtc::scoped_ptr<WindowCapturer> window_capturer_;
  cloopenwebrtc::scoped_ptr<ScreenCapturer> screen_capturer_;
  WindowId selected_window_;
  WindowId excluded_window_;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_MODULES_DESKTOP_CAPTURE_CROPPING_WINDOW_CAPTURER_H_

