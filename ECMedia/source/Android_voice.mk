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
LOCAL_MODULE := libMedia
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		base64.c \
		ECMedia.cpp \
		RecordVoip.cpp \
		sometools.cpp \
		voe_observer.cpp 

	
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DOLDERRORCODE'
# ssl
#    '-DSUPPORT_SSL' \
#   '-DENABLE_LOG'

#ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
#    LOCAL_CFLAGS += '-DVIDEO_ENABLED'
#endif

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/../include \
		$(LOCAL_PATH)/../../system_wrappers/interface \
		$(LOCAL_PATH)/../../voice_engine/main/include \
		$(LOCAL_PATH)/../../voice_engine/main/source \
		$(LOCAL_PATH)/../interface \
		$(LOCAL_PATH)/../.. \
		$(LOCAL_PATH)/../../module \
    $(LOCAL_PATH)/../../module/interface \
    $(LOCAL_PATH)/../../module/videojpegyuv/include \
  $(LOCAL_PATH)/../../third_party/oRTP/include \
	$(LOCAL_PATH)/../../third_party/oRTP/include/ortp \
	$(LOCAL_PATH)/../../module/rtp_rtcp/source/oRTP/include/ortp/ \
	$(LOCAL_PATH)/../../third_party/srtp \
	$(LOCAL_PATH)/../../third_party/srtp/crypto/include \
	$(LOCAL_PATH)/../../third_party/gsm/inc/ \
	$(LOCAL_PATH)/../../third_party/openssl
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
