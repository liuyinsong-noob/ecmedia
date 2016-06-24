/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_DUMMY_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_DUMMY_H

#include "audio_device_utility.h"
#include "audio_device.h"

namespace cloopenwebrtc
{
class CriticalSectionWrapper;

class AudioDeviceUtilityDummy: public AudioDeviceUtility
{
public:
    AudioDeviceUtilityDummy(const int32_t id) {}
    virtual ~AudioDeviceUtilityDummy() {}

    virtual int32_t Init() OVERRIDE;
};
}  // namespace cloopenwebrtc

#endif  // MODULES_AUDIO_DEVICE_MAIN_SOURCE_LINUX_AUDIO_DEVICE_UTILITY_DUMMY_H_
