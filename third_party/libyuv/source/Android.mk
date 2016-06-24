# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libyuv
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	compare.cc \
	convert.cc \
	convert_from.cc \
	convertfrom.cc \
	cpu_id.cc \
	format_conversion.cc \
	mjpeg_decoder.cc \
	planar_functions.cc \
	rotate.cc \
	rotate_neon.cc \
	row_common.cc \
	row_neon.cc \
	row_posix.cc \
	row_win.cc \
	scale.cc \
	video_common.cc 

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
  $(MY_WEBRTC_COMMON_DEFS) \
  '-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_GPU=1' \
	'-DUSE_OPENSSL=1' \
	'-DENABLE_EGLIMAGE=1' \
	'-DUSE_SKIA=1' \
	'-DHAVE_JPEG' \
	'-DANDROID' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DHAVE_OFF64_T' \
	'-DHAVE_SYS_UIO_H' \
	'-DANDROID_BINSIZE_HACK' \
	'-DANDROID_UPSTREAM_BRINGUP=1' \
	'-DNDEBUG' \
	'-DNVALGRIND' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=0'

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_CFLAGS += -mfpu=neon -mcpu=cortex-a8 -mfloat-abi=softfp
endif

# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/.. \
    $(LOCAL_PATH)/../../libjpeg_turbo \
    $(LOCAL_PATH)/../include

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
