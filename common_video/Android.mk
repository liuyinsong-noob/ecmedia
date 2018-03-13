# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_common_video
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
        h264/h264_bitstream_parser.cc \
        h264/h264_common.cc \
        h264/pps_parser.cc \
        h264/profile_level_id.cc \
        h264/sps_parser.cc \
        h264/sps_vui_rewriter.cc \
		source/data_manager.cc \
		source/i420_video_frame.cc \
		source/jpeg.cc \
		source/plane.cc \
		source/texture_video_frame.cc \
		source/libyuv/scaler.cc \
		source/libyuv/webrtc_libyuv.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DUSE_SYSTEM_LIBJPEG' \

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/.. \
		$(LOCAL_PATH)/../include \
		$(LOCAL_PATH)/../module \
		$(LOCAL_PATH)/../base \
		$(LOCAL_PATH)/../module/video_coding/main/include \
		$(LOCAL_PATH)/../system_wrappers/interface \
		$(LOCAL_PATH)/../third_party/libyuv-android/include \
		$(LOCAL_PATH)/../third_party/libjpeg_turbo
    
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)

