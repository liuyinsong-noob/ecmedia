/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_DESKTOP_SHARE_MANAGER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_DESKTOP_SHARE_MANAGER_H_

#include "video_capture.h"
#include "../system_wrappers/include/map_wrapper.h"
#include "../system_wrappers/include/scoped_ptr.h"
#include "typedefs.h"
#include "vie_defines.h"
#include "vie_frame_provider_base.h"
#include "vie_manager_base.h"
#include "vie_desktop_share.h"

namespace cloopenwebrtc {

class CriticalSectionWrapper;
class ProcessThread;
class RWLockWrapper;
class VieDesktopCapturer;
class VoiceEngine;

class ViEDesktopShareManager : private ViEManagerBase {
  friend class ViEDesktopShareScoped;
 public:
  explicit ViEDesktopShareManager(int engine_id);
  ~ViEDesktopShareManager();

  // Creates a capture module for the specified desktop capture device.
  // Return zero on success, ViEError on failure.
  int CreateDesktopCapturer(int& desktop_capture_id,const DesktopShareType desktop_share_type);

  int DestroyDesktopCapture(int desktop_capture_id);

 private:
  // Gets and allocates a free capture device id. Assumed protected by caller.
  bool GetFreeDesktopCaptureId(int& freecapture_id);

  // Frees a capture id assigned in GetFreeCaptureId.
  void ReturnDesktopCaptureId(int capture_id);

  // Gets the ViEFrameProvider for this capture observer.
  ViEFrameProviderBase* ViEFrameProvider(
      const ViEFrameCallback* capture_observer) const;

  // Gets the ViEFrameProvider for this capture observer.
  ViEFrameProviderBase* ViEFrameProvider(int provider_id) const;

  // Gets the ViECapturer for the capture device id.
  VieDesktopCapturer* VieDesktopCapturePtr(int capture_id) const;

  // Gets the the entire map with GetViECaptures.
  void GetViECaptures(MapWrapper& vie_capture_map);

  int engine_id_;
  scoped_ptr<CriticalSectionWrapper> map_cs_;
  MapWrapper vie_frame_provider_map_;

  // Capture devices.
  int free_desktop_capture_id_[kViEMaxCaptureDevices];

};

// Provides protected access to ViEInputManater.
class ViEDesktopShareScoped: private ViEManagerScopedBase {
 public:
  explicit ViEDesktopShareScoped(const ViEDesktopShareManager& vie_input_manager);

  VieDesktopCapturer* DesktopCapture(int capture_id) const;
  ViEFrameProviderBase* FrameProvider(int provider_id) const;
  ViEFrameProviderBase* FrameProvider(const ViEFrameCallback*
                                      capture_observer) const;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_INPUT_MANAGER_H_
