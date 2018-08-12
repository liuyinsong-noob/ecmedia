/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifdef WEBRTC_IOS
#include "module/audio_device/main/source/audio_device_config.h"
#include "module/audio_device/main/source/ios/audio_device_utility_ios.h"

#include "system_wrappers/include/critical_section_wrapper.h"
#include "system_wrappers/include/trace.h"

namespace yuntongxunwebrtc {
AudioDeviceUtilityIOS::AudioDeviceUtilityIOS(const int32_t id)
:
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _lastError(AudioDeviceModule::kAdmErrNone) {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}

AudioDeviceUtilityIOS::~AudioDeviceUtilityIOS() {
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    {
        CriticalSectionScoped lock(&_critSect);
    }
    delete &_critSect;
}

int32_t AudioDeviceUtilityIOS::Init() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  OS info: %s", "iOS");

    return 0;
}

}  // namespace webrtc
#endif
