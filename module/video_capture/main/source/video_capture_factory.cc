/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video_capture_factory.h"

#include "video_capture_impl.h"

#ifdef WEBRTC_ANDROID
#include "video_capture_android.h"
#endif


namespace yuntongxunwebrtc
{

	VideoCaptureModule* VideoCaptureFactory::Create(const int32_t id,
		const char* deviceUniqueIdUTF8, VideoCaptureCapability *settings) {
			return videocapturemodule::VideoCaptureImpl::Create(id, deviceUniqueIdUTF8, settings);
	}


VideoCaptureModule* VideoCaptureFactory::Create(const int32_t id,
    VideoCaptureExternal*& externalCapture) {
  return videocapturemodule::VideoCaptureImpl::Create(id, externalCapture);
}

VideoCaptureModule::DeviceInfo* VideoCaptureFactory::CreateDeviceInfo(
    const int32_t id) {
  return videocapturemodule::VideoCaptureImpl::CreateDeviceInfo(id);
}

#ifdef WEBRTC_ANDROID
WebRtc_Word32 VideoCaptureFactory::SetAndroidObjects(void* javaVM, void* env, void* javaContext)
{
	return videocapturemodule::VideoCaptureAndroid::SetAndroidObjects(javaVM, env, javaContext);
}
#endif
}  // namespace webrtc
