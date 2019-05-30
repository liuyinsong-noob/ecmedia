/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_input_manager.h"

#include <assert.h>

#include "common_types.h"
#include "video_capture_factory.h"
#include "video_coding.h"
#include "video_coding_defines.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/logging.h"
#include "../system_wrappers/include/rw_lock_wrapper.h"
#include "vie_errors.h"
#include "vie_capturer.h"
#include "vie_defines.h"

#include "../system_wrappers/include/trace.h"
#include "vie_file_player.h"
#include "vie_watermark.h"
 //add by dingxf
#include "vie_file_capturer.h"

namespace yuntongxunwebrtc {

ViEInputManager::ViEInputManager(const int engine_id, const Config& config)
    : config_(config),
      engine_id_(engine_id),
      map_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      device_info_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      vie_frame_provider_map_(),
      capture_device_info_(NULL),
      module_process_thread_(NULL) {
  for (int idx = 0; idx < kViEMaxCaptureDevices; idx++) {
    free_capture_device_id_[idx] = true;
  }
}


ViEInputManager::~ViEInputManager() {
	for (FrameProviderMap::iterator it = vie_frame_provider_map_.begin();
		it != vie_frame_provider_map_.end();
		++it) {
			delete it->second;
	}
	//while (vie_frame_provider_map_.Size() != 0) {
	//	MapItem* item = vie_frame_provider_map_.First();
	//	assert(item);
	//	ViEFrameProviderBase* frame_provider =
	//		static_cast<ViEFrameProviderBase*>(item->GetItem());
	//	vie_frame_provider_map_.Erase(item);
	//	delete frame_provider;
	//}

	if (capture_device_info_) { 
		delete capture_device_info_;
		capture_device_info_ = NULL;
	}  

  //delete capture_device_info_; //退出时在此崩溃
}
void ViEInputManager::SetModuleProcessThread(
    ProcessThread* module_process_thread) {
  assert(!module_process_thread_);
  module_process_thread_ = module_process_thread;
}

int ViEInputManager::NumberOfCaptureDevices() {
  CriticalSectionScoped cs(device_info_cs_.get());
  if (capture_device_info_ == NULL)
    capture_device_info_ = VideoCaptureFactory::CreateDeviceInfo(
        ViEModuleId(engine_id_));
  assert(capture_device_info_);
  return capture_device_info_->NumberOfDevices();
}

int ViEInputManager::GetDeviceName(uint32_t device_number,
                                   char* device_nameUTF8,
                                   uint32_t device_name_length,
                                   char* device_unique_idUTF8,
                                   uint32_t device_unique_idUTF8Length) {
  CriticalSectionScoped cs(device_info_cs_.get());
  if (capture_device_info_ == NULL)
    capture_device_info_ = VideoCaptureFactory::CreateDeviceInfo(
        ViEModuleId(engine_id_));
  assert(capture_device_info_);
  return capture_device_info_->GetDeviceName(device_number, device_nameUTF8,
                                             device_name_length,
                                             device_unique_idUTF8,
                                             device_unique_idUTF8Length);
}

int ViEInputManager::NumberOfCaptureCapabilities(
  const char* device_unique_idUTF8) {
  CriticalSectionScoped cs(device_info_cs_.get());
  if (capture_device_info_ == NULL)
    capture_device_info_ = VideoCaptureFactory::CreateDeviceInfo(
        ViEModuleId(engine_id_));
  assert(capture_device_info_);
  return capture_device_info_->NumberOfCapabilities(device_unique_idUTF8);
}

int ViEInputManager::GetCaptureCapability(
    const char* device_unique_idUTF8,
    const uint32_t device_capability_number,
    CaptureCapability& capability) {
  CriticalSectionScoped cs(device_info_cs_.get());
  if (capture_device_info_ == NULL)
    capture_device_info_ = VideoCaptureFactory::CreateDeviceInfo(
        ViEModuleId(engine_id_));
  assert(capture_device_info_);
  VideoCaptureCapability module_capability;
  int result = capture_device_info_->GetCapability(device_unique_idUTF8,
                                                   device_capability_number,
                                                   module_capability);
  if (result != 0)
    return result;

  // Copy from module type to public type.
  capability.expectedCaptureDelay = module_capability.expectedCaptureDelay;
  capability.height = module_capability.height;
  capability.width = module_capability.width;
  capability.interlaced = module_capability.interlaced;
  capability.rawType = module_capability.rawType;
  capability.codecType = module_capability.codecType;
  capability.maxFPS = module_capability.maxFPS;
  return result;
}

//add by dingxf
int ViEInputManager::CreateCaptureFile(int& capture_id, const char *fileUTF8, const char *filesSplit) {
	CriticalSectionScoped cs(map_cs_.get());

	int newcapture_id = 0;
	if (!GetFreeCaptureId(&newcapture_id)) {
		LOG(LS_ERROR) << "All capture devices already allocated.";
		return kViECaptureDeviceMaxNoDevicesAllocated;
	}
	ViEFileCapturer* vie_capture = ViEFileCapturer::CreateViEFileCapturer(
		newcapture_id, engine_id_, config_, fileUTF8, filesSplit, *module_process_thread_);
	if (!vie_capture) {
		ReturnCaptureId(newcapture_id);
		return kViECaptureDeviceUnknownError;
	}

	vie_frame_provider_map_[newcapture_id] = vie_capture;
	capture_id = newcapture_id;
	return 0;
}

//add by dingxf
int ViEInputManager::GetCaptureCapability(int capture_id, CaptureCapability& capability) {
	ViECapturer* vie_capture = ViECapturePtr(capture_id);
	if (!vie_capture) {
		LOG(LS_ERROR) << "No such capture device id: " << capture_id;
		return -1;
	}
	return vie_capture->GetCaptureCapability(capability);
}

int ViEInputManager::GetOrientation(const char* device_unique_idUTF8,
                                    RotateCapturedFrame& orientation) {
  CriticalSectionScoped cs(device_info_cs_.get());
  if (capture_device_info_ == NULL)
    capture_device_info_ = VideoCaptureFactory::CreateDeviceInfo(
        ViEModuleId(engine_id_));
  assert(capture_device_info_);
  VideoCaptureRotation module_orientation;
  int result = capture_device_info_->GetOrientation(device_unique_idUTF8,
                                                    module_orientation);
  // Copy from module type to public type.
  switch (module_orientation) {
    case kCameraRotate0:
      orientation = RotateCapturedFrame_0;
      break;
    case kCameraRotate90:
      orientation = RotateCapturedFrame_90;
      break;
    case kCameraRotate180:
      orientation = RotateCapturedFrame_180;
      break;
    case kCameraRotate270:
      orientation = RotateCapturedFrame_270;
      break;
  }
  return result;
}

int ViEInputManager::DisplayCaptureSettingsDialogBox(
    const char* device_unique_idUTF8,
    const char* dialog_titleUTF8,
    void* parent_window,
    uint32_t positionX,
    uint32_t positionY) {
  CriticalSectionScoped cs(device_info_cs_.get());
  if (capture_device_info_ == NULL)
    capture_device_info_ = VideoCaptureFactory::CreateDeviceInfo(
        ViEModuleId(engine_id_));
  assert(capture_device_info_);
  return capture_device_info_->DisplayCaptureSettingsDialogBox(
           device_unique_idUTF8, dialog_titleUTF8, parent_window, positionX,
           positionY);
}


int ViEInputManager::CreateCaptureDevice(
    const char* device_unique_idUTF8,
    const uint32_t device_unique_idUTF8Length,
    int& capture_id,
	CaptureCapability *settings) {
  CriticalSectionScoped cs(map_cs_.get());

  // Make sure the device is not already allocated.
  for (FrameProviderMap::iterator it = vie_frame_provider_map_.begin();
       it != vie_frame_provider_map_.end();
       ++it) {
    // Make sure this is a capture device.
    if (it->first >= kViECaptureIdBase && it->first <= kViECaptureIdMax) {
      ViECapturer* vie_capture = static_cast<ViECapturer*>(it->second);
      assert(vie_capture);
      // TODO(mflodman) Can we change input to avoid this cast?
      const char* device_name =
          reinterpret_cast<const char*>(vie_capture->CurrentDeviceName());
      if (strncmp(device_name, device_unique_idUTF8,
                  strlen(device_name)) == 0) {
        return kViECaptureDeviceAlreadyAllocated;
      }
    }
  }

  // Make sure the device name is valid.
  bool found_device = false;
  CriticalSectionScoped cs_devinfo(device_info_cs_.get());
  if (capture_device_info_ == NULL)
    capture_device_info_ = VideoCaptureFactory::CreateDeviceInfo(
        ViEModuleId(engine_id_));
  assert(capture_device_info_);
  for (uint32_t device_index = 0;
       device_index < capture_device_info_->NumberOfDevices(); ++device_index) {
    if (device_unique_idUTF8Length > kVideoCaptureUniqueNameLength) {
      // User's string length is longer than the max.
      return -1;
    }

    char found_name[kVideoCaptureDeviceNameLength] = "";
    char found_unique_name[kVideoCaptureUniqueNameLength] = "";
    capture_device_info_->GetDeviceName(device_index, found_name,
                                        kVideoCaptureDeviceNameLength,
                                        found_unique_name,
                                        kVideoCaptureUniqueNameLength);

    // TODO(mflodman) Can we change input to avoid this cast?
    const char* cast_id = reinterpret_cast<const char*>(device_unique_idUTF8);
    if (strncmp(cast_id, reinterpret_cast<const char*>(found_unique_name),
                strlen(cast_id)) == 0) {
      found_device = true;
      break;
    }
  }
  if (!found_device) {
    LOG(LS_ERROR) << "Capture device not found: " << device_unique_idUTF8;
    return kViECaptureDeviceDoesNotExist;
  }

  int newcapture_id = 0;
  if (!GetFreeCaptureId(&newcapture_id)) {
    LOG(LS_ERROR) << "All capture devices already allocated.";
    return kViECaptureDeviceMaxNoDevicesAllocated;
  }
  ViECapturer* vie_capture = ViECapturer::CreateViECapture(
      newcapture_id, engine_id_, config_, device_unique_idUTF8,
      device_unique_idUTF8Length, *module_process_thread_,settings);
  if (!vie_capture) {
    ReturnCaptureId(newcapture_id);
    return kViECaptureDeviceUnknownError;
  }

  vie_frame_provider_map_[newcapture_id] = vie_capture; //ViECapture 子类， ViEFrameProviderBase 父类
  capture_id = newcapture_id;
  return 0;
}


int ViEInputManager::CreateCaptureDevice(VideoCaptureModule* capture_module,
                                         int& capture_id) {
  CriticalSectionScoped cs(map_cs_.get());
  int newcapture_id = 0;
  if (!GetFreeCaptureId(&newcapture_id)) {
    LOG(LS_ERROR) << "All capture devices already allocated.";
    return kViECaptureDeviceMaxNoDevicesAllocated;
  }

  ViECapturer* vie_capture = ViECapturer::CreateViECapture(
      newcapture_id, engine_id_, config_,
      capture_module, *module_process_thread_);
  if (!vie_capture) {
    ReturnCaptureId(newcapture_id);
    return kViECaptureDeviceUnknownError;
  }
  vie_frame_provider_map_[newcapture_id] = vie_capture;
  capture_id = newcapture_id;
  return 0;
}

int ViEInputManager::DestroyCaptureDevice(const int capture_id) {
  ViECapturer* vie_capture = NULL;
  //{
    // We need exclusive access to the object to delete it.
    // Take this write lock first since the read lock is taken before map_cs_.
    ViEManagerWriteScoped wl(this);
    CriticalSectionScoped cs(map_cs_.get());

    vie_capture = ViECapturePtr(capture_id);
    if (!vie_capture) {
      LOG(LS_ERROR) << "No such capture device id: " << capture_id;
      return -1;
    }
    uint32_t num_callbacks =
        vie_capture->NumberOfRegisteredFrameCallbacks();
    if (num_callbacks > 0) {
      LOG(LS_WARNING) << num_callbacks << " still registered to capture id "
                      << capture_id << " when destroying capture device.";
    }
    vie_frame_provider_map_.erase(capture_id);
//	vie_frame_provider_map_.Erase(capture_id);
    ReturnCaptureId(capture_id);
    // Leave cs before deleting the capture object. This is because deleting the
    // object might cause deletions of renderers so we prefer to not have a lock
    // at that time.
  //}
  delete vie_capture;
  vie_capture = NULL;
  return 0;
}

//add by chwd
int ViEInputManager::CreateWaterMark(int capture_id, WaterMark watermark, int width, int height) {
	CriticalSectionScoped cs(map_cs_.get());

	VIEWaterMark* vie_watermark = VIEWaterMark::CreateWatermark(watermark, width, height);
	if (!vie_watermark) {
		LOG(LS_ERROR) << "Create vie_water_mark error:" << capture_id << " width:"<<width <<" height:"<<height;
		return -1;
	}

	ViECapturer* vie_capture = ViECapturePtr(capture_id);
	if (!vie_capture) {
		LOG(LS_ERROR) << "No such capture device id: " << capture_id;
		return -1;
	}

	return vie_capture->SetFrameWaterMark(vie_watermark);
}

int ViEInputManager::CreateExternalCaptureDevice(
    ViEExternalCapture*& external_capture,
    int& capture_id) {
  CriticalSectionScoped cs(map_cs_.get());

  int newcapture_id = 0;
  if (GetFreeCaptureId(&newcapture_id) == false) {
    LOG(LS_ERROR) << "All capture devices already allocated.";
    return kViECaptureDeviceMaxNoDevicesAllocated;
  }

  ViECapturer* vie_capture = ViECapturer::CreateViECapture(
      newcapture_id, engine_id_, config_, NULL, 0, *module_process_thread_);
  if (!vie_capture) {
    ReturnCaptureId(newcapture_id);
    return kViECaptureDeviceUnknownError;
  }

  vie_frame_provider_map_[newcapture_id] = vie_capture;
  /*if (vie_frame_provider_map_.Insert(newcapture_id, vie_capture) != 0) {
	  ReturnCaptureId(newcapture_id);
	  WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
		  "%s: Could not insert capture module for external capture.",
		  __FUNCTION__);
	  return kViECaptureDeviceUnknownError;
  }*/
  capture_id = newcapture_id;
  external_capture = vie_capture;
  return 0;
}

bool ViEInputManager::GetFreeCaptureId(int* freecapture_id) {
  for (int id = 0; id < kViEMaxCaptureDevices; id++) {
    if (free_capture_device_id_[id]) {
      // We found a free capture device id.
      free_capture_device_id_[id] = false;
      *freecapture_id = id + kViECaptureIdBase;
      return true;
    }
  }
  return false;
}

void ViEInputManager::ReturnCaptureId(int capture_id) {
  CriticalSectionScoped cs(map_cs_.get());
  if (capture_id >= kViECaptureIdBase &&
      capture_id < kViEMaxCaptureDevices + kViECaptureIdBase) {
    free_capture_device_id_[capture_id - kViECaptureIdBase] = true;
  }
  return;
}

ViEFrameProviderBase* ViEInputManager::ViEFrameProvider(
    const ViEFrameCallback* capture_observer) const {
  assert(capture_observer);
  CriticalSectionScoped cs(map_cs_.get());

   for (FrameProviderMap::const_iterator it = vie_frame_provider_map_.begin();
  it != vie_frame_provider_map_.end();
  ++it) {
  if (it->second->IsFrameCallbackRegistered(capture_observer))
  return it->second;
  }
  //for (MapItem* provider_item = vie_frame_provider_map_.First(); provider_item
	 // != NULL; provider_item = vie_frame_provider_map_.Next(provider_item)) {
		//  ViEFrameProviderBase* vie_frame_provider =
		//	  static_cast<ViEFrameProviderBase*>(provider_item->GetItem());
		//  assert(vie_frame_provider != NULL);

		//  if (vie_frame_provider->IsFrameCallbackRegistered(capture_observer)) {
		//	  // We found it.
		//	  return vie_frame_provider;
		//  }
  //}

  // No capture device set for this channel.
  return NULL;
}

ViEFrameProviderBase* ViEInputManager::ViEFrameProvider(int provider_id) const {
  CriticalSectionScoped cs(map_cs_.get());

  FrameProviderMap::const_iterator it =
      vie_frame_provider_map_.find(provider_id);
  if (it == vie_frame_provider_map_.end())
    return NULL;
  return it->second;
  /*MapItem* map_item = vie_frame_provider_map_.Find(provider_id);
  if (!map_item) {
	  return NULL;
  }
  ViEFrameProviderBase* vie_frame_provider =
	  static_cast<ViEFrameProviderBase*>(map_item->GetItem());
  return vie_frame_provider;*/
}

ViECapturer* ViEInputManager::ViECapturePtr(int capture_id) const {
  if (!(capture_id >= kViECaptureIdBase &&
        capture_id <= kViECaptureIdBase + kViEMaxCaptureDevices)) {
      LOG(LS_ERROR) << "Capture device doesn't exist, captured_id: " << capture_id << ".";
    return NULL;
  }

  return static_cast<ViECapturer*>(ViEFrameProvider(capture_id));
}

ViEInputManagerScoped::ViEInputManagerScoped(
    const ViEInputManager& vie_input_manager)
    : ViEManagerScopedBase(vie_input_manager) {
}

ViECapturer* ViEInputManagerScoped::Capture(int capture_id) const {
  return static_cast<const ViEInputManager*>(vie_manager_)->ViECapturePtr(
      capture_id);
}

ViEFrameProviderBase* ViEInputManagerScoped::FrameProvider(
    const ViEFrameCallback* capture_observer) const {
  return static_cast<const ViEInputManager*>(vie_manager_)->ViEFrameProvider(
      capture_observer);
}

ViEFrameProviderBase* ViEInputManagerScoped::FrameProvider(
    int provider_id) const {
  return static_cast<const ViEInputManager*>(vie_manager_)->ViEFrameProvider(
      provider_id);
}

ViEFilePlayer* ViEInputManagerScoped::FilePlayer(int file_id) const {
	return static_cast<const ViEInputManager*>(vie_manager_)->ViEFilePlayerPtr(
		file_id);
}

int ViEInputManager::CreateFilePlayer(const char* file_nameUTF8,
	const bool loop,
	const yuntongxunwebrtc::FileFormats file_format,
	VoiceEngine* voe_ptr, int& file_id) {
		WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
			"%s(device_unique_id: %s)", __FUNCTION__, file_nameUTF8);

		CriticalSectionScoped cs(map_cs_.get());
		int new_file_id = 0;
		if (GetFreeFileId(new_file_id) == false) {
			WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
				"%s: Maximum supported number of file players already in use",
				__FUNCTION__);
			return kViEFileMaxNoOfFilesOpened;
		}

		ViEFilePlayer* vie_file_player = ViEFilePlayer::CreateViEFilePlayer(
			new_file_id, engine_id_, file_nameUTF8, loop, file_format, *this,
			voe_ptr);
		if (!vie_file_player) {
			ReturnFileId(new_file_id);
			WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
				"%s: Could not open file %s for playback", __FUNCTION__,
				file_nameUTF8);
			return kViEFileUnknownError;
		}

		vie_frame_provider_map_[new_file_id] = vie_file_player;
		/*if (vie_frame_provider_map_.Insert(new_file_id, vie_file_player) != 0) {
			ReturnCaptureId(new_file_id);
			WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
				"%s: Could not insert file player for %s", __FUNCTION__,
				file_nameUTF8);
			delete vie_file_player;
			return kViEFileUnknownError;
		}*/

		file_id = new_file_id;
		WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
			"%s(filename: %s, file_id: %d)", __FUNCTION__, file_nameUTF8,
			new_file_id);
		return 0;
}

int ViEInputManager::DestroyFilePlayer(int file_id) {
	WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
		"%s(file_id: %d)", __FUNCTION__, file_id);

	ViEFilePlayer* vie_file_player = NULL;
	{
		// We need exclusive access to the object to delete it.
		// Take this write lock first since the read lock is taken before map_cs_.
		ViEManagerWriteScoped wl(this);

		CriticalSectionScoped cs(map_cs_.get());
		vie_file_player = ViEFilePlayerPtr(file_id);
		if (!vie_file_player) {
			WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
				"%s(file_id: %d) - No such file player", __FUNCTION__,
				file_id);
			return -1;
		}
		int num_callbacks = vie_file_player->NumberOfRegisteredFrameCallbacks();
		if (num_callbacks > 0) {
			WEBRTC_TRACE(yuntongxunwebrtc::kTraceWarning, yuntongxunwebrtc::kTraceVideo,
				ViEId(engine_id_), "%s(file_id: %d) - %u registered "
				"callbacks when destroying file player", __FUNCTION__,
				file_id, num_callbacks);
		}
		vie_frame_provider_map_.erase(file_id);
		//vie_frame_provider_map_.Erase(file_id);
		ReturnFileId(file_id);
		// Leave cs before deleting the file object. This is because deleting the
		// object might cause deletions of renderers so we prefer to not have a lock
		// at that time.
	}
	delete vie_file_player;
	return 0;
}

bool ViEInputManager::GetFreeFileId(int& free_file_id) {
	WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_), "%s",
		__FUNCTION__);

	for (int id = 0; id < kViEMaxFilePlayers; id++) {
		if (free_file_id_[id]) {
			// We found a free capture device id.
			free_file_id_[id] = false;
			free_file_id = id + kViEFileIdBase;
			WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
				"%s: new id: %d", __FUNCTION__, free_file_id);
			return true;
		}
	}
	return false;
}

void ViEInputManager::ReturnFileId(int file_id) {
	WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo, ViEId(engine_id_),
		"%s(%d)", __FUNCTION__, file_id);

	CriticalSectionScoped cs(map_cs_.get());
	if (file_id >= kViEFileIdBase &&
		file_id < kViEMaxFilePlayers + kViEFileIdBase) {
			free_file_id_[file_id - kViEFileIdBase] = true;
	}
	return;
}

ViEFilePlayer* ViEInputManager::ViEFilePlayerPtr(int file_id) const {
	if (file_id < kViEFileIdBase || file_id > kViEFileIdMax) {
		return NULL;
	}
	CriticalSectionScoped cs(map_cs_.get());

	FrameProviderMap::const_iterator it =
		vie_frame_provider_map_.find(file_id);
	if (it == vie_frame_provider_map_.end())
		return NULL;
	return static_cast<ViEFilePlayer*>(it->second);

	//MapItem* map_item = vie_frame_provider_map_.Find(file_id);
	//if (!map_item) {
	//	return NULL;
	//}
	//ViEFilePlayer* vie_file_player =
	//	static_cast<ViEFilePlayer*>(map_item->GetItem());
	//return vie_file_player;
}
}  // namespace webrtc
