# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_video_render
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    incoming_video_stream.cc \
    video_render_frames.cc \
    video_render_internal_impl.cc \
    external/video_render_external_impl.cc \
    android/video_render_android_impl.cc \
    android/video_render_android_native_opengl2.cc \
    android/video_render_android_surface_view.cc \
    android/video_render_opengles20.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DWEBRTC_INCLUDE_INTERNAL_VIDEO_RENDER'

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/android \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../../.. \
    $(LOCAL_PATH)/../../../.. \
    $(LOCAL_PATH)/../../../videojpegyuv/include \
    $(LOCAL_PATH)/../../../audio_coding/main/include \
    $(LOCAL_PATH)/../../../interface \
    $(LOCAL_PATH)/../../../utility/include \
    $(LOCAL_PATH)/../../../common_video/interface \
    $(LOCAL_PATH)/../../../video_coding/main/include \
    $(LOCAL_PATH)/../../../common_video/source \
    $(LOCAL_PATH)/../../../common_video/source/libyuv/include \
    $(LOCAL_PATH)/../../../video_render/main/source/external \
    $(LOCAL_PATH)/../../../../common_video/vplib/main/interface \
    $(LOCAL_PATH)/../../../../system_wrappers/interface 
		
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
