# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libEC_zlib
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		adler32.c \
		compress.c \
		crc32.c \
		deflate.c \
		gzclose.c \
		gzlib.c \
		gzread.c \
		gzwrite.c \
		infback.c \
		inffast.c \
		inflate.c \
		inftrees.c \
		trees.c \
		uncompr.c \
		zutil.c \
		./contrib/minizip/ioapi.c \
		./contrib/minizip/unzip.c \
		./contrib/minizip/zip.c 



# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
	-fPIC

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH) \
		$(LOCAL_PATH)contrib\minizip \


ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
