/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "screen_capturer.h"

#include "desktop_capture_options.h"
#include "screen_capturer_win_gdi.h"
#include "screen_capturer_win_magnifier.h"

namespace cloopenwebrtc {

// static
ScreenCapturer* ScreenCapturer::Create(const DesktopCaptureOptions& options) {
  cloopenwebrtc::scoped_ptr<ScreenCapturer> gdi_capturer(
      new ScreenCapturerWinGdi(options));
  //now  disallow
  //if (options.allow_use_magnification_api())
  //  return new ScreenCapturerWinMagnifier(gdi_capturer.Pass());

  return gdi_capturer.release();
}

}  // namespace cloopenwebrtc
