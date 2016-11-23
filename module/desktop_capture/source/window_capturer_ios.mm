/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import <UIKit/UIKit.h>

#include "screen_capturer.h"

#include "desktop_frame.h"

#include <assert.h>

#include "scoped_ptr.h"
#include "trace.h"

#include "window_capturer_ios.h"
namespace cloopenwebrtc {

ScreenCapturerIos::ScreenCapturerIos()
    : callback_(NULL){
  // Try to load dwmapi.dll dynamically since it is not available on XP.
  
}

ScreenCapturerIos::~ScreenCapturerIos() {

}

bool ScreenCapturerIos::IsAeroEnabled() {
  bool result = false;
    return result;
}

bool  ScreenCapturerIos::GetShareCaptureRect(int &width, int &height)
{
    
    width = [UIScreen mainScreen].bounds.size.width * [UIScreen mainScreen].scale;
    height = [UIScreen mainScreen].bounds.size.height * [UIScreen mainScreen].scale;
    return true;
}

void ScreenCapturerIos::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}

bool ScreenCapturerIos::GetScreenList(ScreenList* screens)
    {
        return false;
    }
    
    // Select the screen to be captured. Returns false in case of a failure (e.g.
    // if there is no screen with the specified id). If this is never called, the
    // full desktop is captured.
bool ScreenCapturerIos::SelectScreen(ScreenId id)
    {
        return false;
    }

    
void ScreenCapturerIos::Capture(const DesktopRegion& region) {
  
  // Stop capturing if the window has been closed or hidden.
  if (![UIApplication sharedApplication].isFirstResponder) {
    callback_->OnCaptureCompleted(NULL, kCapture_Window_Closed);
    return;
  }

  if(![UIApplication sharedApplication].isFirstResponder)
  {
      callback_->OnCaptureCompleted(NULL, kCapture_Window_Hidden);
      return;
  }


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

	
    NSArray* windows = [[UIApplication sharedApplication] windows];
    UIGraphicsBeginImageContext([UIScreen mainScreen].nativeBounds.size);//全屏截图，包括window nativeBounds:ios 8以上使用
    for (UIWindow *screenWindow in windows) {
        [screenWindow drawViewHierarchyInRect:[UIScreen mainScreen].nativeBounds afterScreenUpdates:NO];
    }
    UIImage *viewImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    //保存到相册
    UIImageWriteToSavedPhotosAlbum(viewImage, nil, nil, nil);
    
    NSData *data = UIImageJPEGRepresentation(viewImage, 1.0f);
    
    int width = [UIScreen mainScreen].bounds.size.width * [UIScreen mainScreen].scale;
    int height = [UIScreen mainScreen].bounds.size.height * [UIScreen mainScreen].scale;
    
    cloopenwebrtc::scoped_ptr<BasicDesktopFrame> frame(new BasicDesktopFrame(DesktopSize(width, height)));
    if (!frame.get()) {
        callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
        return;
    }
    
    memcpy((frame.get())->data(), data.bytes, data.length);


  callback_->OnCaptureCompleted(frame.release(), kCapture_Ok, NULL);
}

// static
ScreenCapturer* ScreenCapturer::Create(const DesktopCaptureOptions& options) {
  return new ScreenCapturerIos();
}
    
ScreenCapturer* ScreenCapturer::Create() {
    return new ScreenCapturerIos();
}
 }  // namespace cloopenwebrtc

