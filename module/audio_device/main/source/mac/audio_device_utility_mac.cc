/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifdef WEBRTC_MAC
#include "module/audio_device/main/source/mac/audio_device_utility_mac.h"
#include "system_wrappers/include/critical_section_wrapper.h"
#include "system_wrappers/include/trace.h"

namespace cloopenwebrtc
{

AudioDeviceUtilityMac::AudioDeviceUtilityMac(const int32_t id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}

// ----------------------------------------------------------------------------
//  AudioDeviceUtilityMac() - dtor
// ----------------------------------------------------------------------------

AudioDeviceUtilityMac::~AudioDeviceUtilityMac()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    {
        CriticalSectionScoped lock(&_critSect);

        // free stuff here...
    }

    delete &_critSect;
}

int32_t AudioDeviceUtilityMac::Init()
{

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, _id,
                 "  OS info: %s", "OS X");

    return 0;
}

}  // namespace webrtc
#endif
