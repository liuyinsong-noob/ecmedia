/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_desktop_share_manager.h"
#include <cassert>
#include "common_types.h"
#include "video_capture_factory.h"
#include "video_coding.h"
#include "video_coding_defines.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/rw_lock_wrapper.h"
#include "../system_wrappers/include/trace.h"
#include "vie_errors.h"
#include "vie_desktop_capture.h"
#include "vie_defines.h"
#include "vie_file_player.h"

namespace yuntongxunwebrtc {

ViEDesktopShareManager::ViEDesktopShareManager(const int engine_id)
    : engine_id_(engine_id),
      map_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      vie_frame_provider_map_() {
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceMemory, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
               "%s", __FUNCTION__);

  for (int idx = 0; idx < kViEMaxDesktopCapture; idx++) {
    free_desktop_capture_id_[idx] = true;
  }
}

ViEDesktopShareManager::~ViEDesktopShareManager() {
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceMemory, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
               "%s", __FUNCTION__);
  while (vie_frame_provider_map_.Size() != 0) {
    MapItem* item = vie_frame_provider_map_.First();
    assert(item);
    ViEFrameProviderBase* frame_provider =
        static_cast<ViEFrameProviderBase*>(item->GetItem());
    vie_frame_provider_map_.Erase(item);
    delete frame_provider;
  }
   
}


int ViEDesktopShareManager::CreateDesktopCapturer(int& desktop_capture_id,const DesktopShareType desktop_share_type) {
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
               "%s(device_unique_id)", __FUNCTION__);
  CriticalSectionScoped cs(map_cs_.get());

  int newcapture_id = 0;
  if (GetFreeDesktopCaptureId(newcapture_id) == false) {
    WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
                 "%s: Maximum supported number of capture devices already in "
                 "use", __FUNCTION__);
    return kViECaptureDeviceMaxNoDevicesAllocated;
  }
  VieDesktopCapturer* vie_desktop_capture = VieDesktopCapturer::CreateCapture(newcapture_id,desktop_share_type,engine_id_);
  if (!vie_desktop_capture) {
    ReturnDesktopCaptureId(newcapture_id);
    WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
                 "%s: Could not create capture module for", __FUNCTION__);
    return kViECaptureDeviceUnknownError;
  }

  if (vie_frame_provider_map_.Insert(newcapture_id, vie_desktop_capture) != 0) {
    ReturnDesktopCaptureId(newcapture_id);
    WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
               "%s: Could not insert capture module ", __FUNCTION__);
    return kViECaptureDeviceUnknownError;
  }
  desktop_capture_id = newcapture_id;
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
               "%s (capture_id: %d)", __FUNCTION__, desktop_capture_id);
  return 0;
}

int ViEDesktopShareManager::DestroyDesktopCapture(const int desktop_capture_id) {
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
               "%s(capture_id: %d)", __FUNCTION__, desktop_capture_id);
  VieDesktopCapturer* vie_capture = NULL;
  {
    // We need exclusive access to the object to delete it.
    // Take this write lock first since the read lock is taken before map_cs_.
    ViEManagerWriteScoped wl(this);
    CriticalSectionScoped cs(map_cs_.get());

    vie_capture = VieDesktopCapturePtr(desktop_capture_id);
    if (!vie_capture) {
      WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
                   "%s(capture_id: %d) - No such capture device id",
                   __FUNCTION__, desktop_capture_id);
      return -1;
    }

    vie_frame_provider_map_.Erase(desktop_capture_id);
    ReturnDesktopCaptureId(desktop_capture_id);
    // Leave cs before deleting the capture object. This is because deleting the
    // object might cause deletions of renderers so we prefer to not have a lock
    // at that time.
  }
  delete vie_capture;
  return 0;
}

bool ViEDesktopShareManager::GetFreeDesktopCaptureId(int& freecapture_id) {
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_), "%s",
               __FUNCTION__);
  for (int id = 0; id < kViEMaxDesktopCapture; id++) {
    if (free_desktop_capture_id_[id]) {
      // We found a free capture device id.
      free_desktop_capture_id_[id] = false;
      freecapture_id = id + kViEDesktopIdBase;
      WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
                   "%s: new id: %d", __FUNCTION__, freecapture_id);
      return true;
    }
  }
  return false;
}

void ViEDesktopShareManager::ReturnDesktopCaptureId(int capture_id) {
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
               "%s(%d)", __FUNCTION__, capture_id);
  CriticalSectionScoped cs(map_cs_.get());
  if (capture_id >= kViEDesktopIdBase &&
      capture_id < kViEMaxDesktopCapture + kViEDesktopIdBase) {
    free_desktop_capture_id_[capture_id - kViEDesktopIdBase] = true;
  }
  return;
}


ViEFrameProviderBase* ViEDesktopShareManager::ViEFrameProvider(
    const ViEFrameCallback* capture_observer) const {
  assert(capture_observer);
  CriticalSectionScoped cs(map_cs_.get());

  for (MapItem* provider_item = vie_frame_provider_map_.First(); provider_item
  != NULL; provider_item = vie_frame_provider_map_.Next(provider_item)) {
    ViEFrameProviderBase* vie_frame_provider =
        static_cast<ViEFrameProviderBase*>(provider_item->GetItem());
    assert(vie_frame_provider != NULL);

    if (vie_frame_provider->IsFrameCallbackRegistered(capture_observer)) {
      // We found it.
      return vie_frame_provider;
    }
  }
  // No capture device set for this channel.
  return NULL;
}

ViEFrameProviderBase* ViEDesktopShareManager::ViEFrameProvider(int provider_id) const {
  CriticalSectionScoped cs(map_cs_.get());
  MapItem* map_item = vie_frame_provider_map_.Find(provider_id);
  if (!map_item) {
    return NULL;
  }
  ViEFrameProviderBase* vie_frame_provider =
      static_cast<ViEFrameProviderBase*>(map_item->GetItem());
  return vie_frame_provider;
}

VieDesktopCapturer* ViEDesktopShareManager::VieDesktopCapturePtr(int capture_id) const {
  if (!(capture_id >= kViEDesktopIdBase &&
        capture_id <= kViEDesktopIdBase + kViEMaxDesktopCapture))
    return NULL;

  CriticalSectionScoped cs(map_cs_.get());
  MapItem* map_item = vie_frame_provider_map_.Find(capture_id);
  if (!map_item) {
    return NULL;
  }
  VieDesktopCapturer* vie_capture = static_cast<VieDesktopCapturer*>(map_item->GetItem());
  return vie_capture;
}

void ViEDesktopShareManager::GetViECaptures(MapWrapper& vie_capture_map) {
  CriticalSectionScoped cs(map_cs_.get());

  if (vie_frame_provider_map_.Size() == 0) {
    return;
  }
  // Add all items to the map.
  for (MapItem* item = vie_frame_provider_map_.First(); item != NULL;
       item = vie_frame_provider_map_.Next(item)) {
    vie_capture_map.Insert(item->GetId(), item->GetItem());
  }
  return;
}


ViEDesktopShareScoped::ViEDesktopShareScoped(
    const ViEDesktopShareManager& vie_input_manager)
    : ViEManagerScopedBase(vie_input_manager) {
}

VieDesktopCapturer* ViEDesktopShareScoped::DesktopCapture(int capture_id) const {
  return static_cast<const ViEDesktopShareManager*>(vie_manager_)->VieDesktopCapturePtr(
      capture_id);
}

ViEFrameProviderBase* ViEDesktopShareScoped::FrameProvider(
    const ViEFrameCallback* capture_observer) const {
  return static_cast<const ViEDesktopShareManager*>(vie_manager_)->ViEFrameProvider(
      capture_observer);
}

ViEFrameProviderBase* ViEDesktopShareScoped::FrameProvider(
    int provider_id) const {
  return static_cast<const ViEDesktopShareManager*>(vie_manager_)->ViEFrameProvider(
      provider_id);
}

}  // namespace yuntongxunwebrtc
