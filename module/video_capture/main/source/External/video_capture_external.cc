/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video_capture_impl.h"
#include "../system_wrappers/include/ref_count.h"

namespace yuntongxunwebrtc {

namespace videocapturemodule {

VideoCaptureModule* VideoCaptureImpl::Create(
    const int32_t id,
    const char* deviceUniqueIdUTF8,
	VideoCaptureCapability *settings) {
  RefCountImpl<VideoCaptureImpl>* implementation =
      new RefCountImpl<VideoCaptureImpl>(id);
  return implementation;
}

}  // namespace videocapturemodule

}  // namespace webrtc
