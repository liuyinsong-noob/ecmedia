# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

AMR_NB_ROOT_PATH := $(call my-dir)


# amr_nb
include $(AMR_NB_ROOT_PATH)/../opencore/amr_nb/dec/Android.mk
include $(AMR_NB_ROOT_PATH)/../opencore/amr_nb/enc/Android.mk
include $(AMR_NB_ROOT_PATH)/../opencore/amr_nb/common/Android.mk

# build .a
LOCAL_PATH := $(AMR_NB_ROOT_PATH)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_amr_nb
LOCAL_MODULE_TAGS := optional

LOCAL_WHOLE_STATIC_LIBRARIES := \
		libpv_amr_nb_common_lib \
    libpvdecoder_gsmamr \
    libpvencoder_gsmamr 

LOCAL_SRC_FILES := \
		amr_interface.cpp \
		wrapper.cpp \
		amrnb_api.cpp


LOCAL_C_INCLUDES := \
		$(LOCAL_PATH) \
		$(LOCAL_PATH)/../opencore/amr_nb/dec/src \
		$(LOCAL_PATH)/../opencore/amr_nb/dec/include \
		$(LOCAL_PATH)/../opencore/amr_nb/enc/src \
		$(LOCAL_PATH)/../opencore/amr_nb/enc/include \
		$(LOCAL_PATH)/../opencore/amr_nb/common/include \
		$(LOCAL_PATH)/../opencore/common \
		$(LOCAL_PATH)/../opencore/common/dec/include \
		$(LOCAL_PATH)/../oscl
ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
