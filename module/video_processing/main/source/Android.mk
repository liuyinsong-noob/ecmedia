# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_video_processing
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc 
LOCAL_SRC_FILES := \
    brighten.cc \
    brightness_detection.cc \
    color_enhancement.cc \
    content_analysis.cc \
    deflickering.cc \
    denoising.cc \
    frame_preprocessor.cc \
    spatial_resampler.cc \
    video_decimator.cc \
    video_processing_impl.cc \
	video_denoiser.cc \
	./util/denoiser_filter.cc \
	./util/denoiser_filter_c.cc \
	./util/noise_estimation.cc \
	./util/skin_detection.cc \

ifeq ($(TARGET_ARCH),x86)
LOCAL_SRC_FILES += \
    content_analysis_sse2.cc
endif

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
	-D__GXX_EXPERIMENTAL_CXX0X__ \
	-fpermissive

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/./util \
	$(LOCAL_PATH)/./beauty_filter \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../../.. \
    $(LOCAL_PATH)/../../../.. \
    $(LOCAL_PATH)/../../../interface \
    $(LOCAL_PATH)/../../../audio_coding/main/include \
    $(LOCAL_PATH)/../../../videojpegyuv/include \
    $(LOCAL_PATH)/../../../utility/include \
    $(LOCAL_PATH)/../../../signalprocess/include \
    $(LOCAL_PATH)/../../../common_video/interface \
    $(LOCAL_PATH)/../../../common_video/source \
    $(LOCAL_PATH)/../../../common_audio/source/signal_processing/include \
    $(LOCAL_PATH)/../../../video_coding/main/include \
    $(LOCAL_PATH)/../../../video_coding/main/source \
    $(LOCAL_PATH)/../../../common_video/source/libyuv/include \
    $(LOCAL_PATH)/../../../../common_video/vplib/main/interface \
    $(LOCAL_PATH)/../../../../system_wrappers/interface \
	$(LOCAL_PATH)/../../../../system_wrappers/source \
	$(LOCAL_PATH)/../../../../base

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
