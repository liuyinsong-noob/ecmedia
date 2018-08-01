# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

# These defines will apply to all source files
# Think again before changing it
MY_WEBRTC_COMMON_DEFS := \
    '-DWEBRTC_TARGET_PC' \
    '-DWEBRTC_LINUX' \
    '-DWEBRTC_THREAD_RR' \
    '-DWEBRTC_CLOCK_TYPE_REALTIME' \
    '-DWEBRTC_ANDROID' \
    '-DWEBRTC_POSIX' \
    '-DWEBRTC_BUILD_LIBEVENT' \
    '-DGOOGLE_PROTOBUF_NO_RTTI' \
    '-DENABLE_REMB_TMMBR_CONFIG' \
    '-DNDEBUG'	

APP_ALLOW_MISSING_DEPS=true
	
#    The following macros are used by modules,
#    we might need to re-organize them
#    '-DWEBRTC_ANDROID_OPENSLES' [module audio_device]
#    '-DNETEQ_VOICEENGINE_CODECS' [module audio_coding neteq]
#    '-DWEBRTC_MODULE_UTILITY_VIDEO' [module media_file] [module utility]

ifeq ($(TARGET_ARCH), arm) 
MY_WEBRTC_COMMON_DEFS += \
    '-DWEBRTC_ARCH_ARM'
	
#    '-DWEBRTC_DETECT_ARM_NEON' # only used in a build configuration without Neon
# TODO(kma): figure out if the above define could be moved to NDK build only.

# TODO(kma): test if the code under next two macros works with generic GCC compilers

ifeq ($(ARCH_ARM_HAVE_NEON),true)
MY_WEBRTC_COMMON_DEFS += \
    '-DWEBRTC_ARCH_ARM_NEON'
MY_ARM_CFLAGS_NEON := \
    -flax-vector-conversions
endif

ifneq (,$(filter '-DWEBRTC_DETECT_ARM_NEON' '-DWEBRTC_ARCH_ARM_NEON', \
    $(MY_WEBRTC_COMMON_DEFS)))
WEBRTC_BUILD_NEON_LIBS := true
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
MY_WEBRTC_COMMON_DEFS += \
    '-DWEBRTC_ARCH_ARM_V7A'
endif

ifeq ($(TARGET_ARCH_ABI),armeabi)
MY_WEBRTC_COMMON_DEFS += \
    '-DWEBRTC_ARCH_ARM_V5'
endif

endif # ifeq ($(TARGET_ARCH),arm)


ifeq ($(TARGET_ARCH), arm64) 
MY_WEBRTC_COMMON_DEFS += \
    '-DWEBRTC_ARCH_ARM'
	
ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
MY_WEBRTC_COMMON_DEFS += \
    '-DWEBRTC_ARCH_ARM64_V8A' \
	'-D__arm64__'
endif

endif #ifeq ($(TARGET_ARCH), arm64)