# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libjpeg
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	jcapimin.c \
	jcapistd.c \
	jccoefct.c \
	jccolor.c \
	jcdctmgr.c \
	jchuff.c \
	jcinit.c \
	jcmainct.c \
	jcmarker.c \
	jcmaster.c \
	jcomapi.c \
	jcparam.c \
	jcphuff.c \
	jcprepct.c \
	jcsample.c \
	jdapimin.c \
	jdapistd.c \
	jdatadst.c \
	jdatasrc.c \
	jdcoefct.c \
	jdcolor.c \
	jddctmgr.c \
	jdhuff.c \
	jdinput.c \
	jdmainct.c \
	jdmarker.c \
	jdmaster.c \
	jdmerge.c \
	jdphuff.c \
	jdpostct.c \
	jdsample.c \
	jerror.c \
	jfdctflt.c \
	jfdctfst.c \
	jfdctint.c \
	jidctflt.c \
	jidctfst.c \
	jidctint.c \
	jidctred.c \
	jmemmgr.c \
	jmemnobs.c \
	jquant1.c \
	jquant2.c \
	jutils.c
	

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
  $(MY_WEBRTC_COMMON_DEFS)
 
# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/. \
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