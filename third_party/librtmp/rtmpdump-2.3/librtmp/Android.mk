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

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := librtmp
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    amf.c \
    log.c \
    parseurl.c \
    rtmp.c \
	hashswf.c


# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../../openssl
	
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl

LOCAL_LDLIBS += -llog -lGLESv2 -lz

include $(BUILD_STATIC_LIBRARY)

