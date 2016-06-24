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
LOCAL_MODULE := libserphone_service_core
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		address.cpp \
		callbacks.cpp \
		chat.cpp \
		enum.cpp \
		friends.cpp \
		lpconfig.cpp \
		mediaprocess.cpp \
		offeranswer.cpp \
		prensence.cpp \
		proxy.cpp \
		sal_eXosip2.cpp \
		sal_eXosip2_presence.cpp \
		sal_eXosip2_sdp.cpp \
		salpr.cpp \
		serphonecall.cpp \
		serphoneinterface.cpp \
		servicecore.cpp \
		siplogin.cpp \
		sipsetup.cpp \
		sometools.cpp \
		CCPClient.cpp \
		tinyxml2.cpp \
		AuthToken.cpp \
		base64.c \
		ice.cc \
		voe_observer.cpp \
		RecordVoip.cpp \
		videoframe.cpp \
		./Http/http.cpp \
		./Http/HttpClient.cpp \
		./Http/RESTClient.cpp \
		./Utility/md5.cpp \
		./Utility/base64_2.cpp \
		./Utility/cJSON.c
	
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
     '-DOLDERRORCODE' \
     '-DXINWEI'
# ssl
#    '-DSUPPORT_SSL' \
#   '-DENABLE_LOG'

#ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS += '-DVIDEO_ENABLED'
#endif

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/../include \
		$(LOCAL_PATH)/Http \
		$(LOCAL_PATH)/Utility \
		$(LOCAL_PATH)/../../system_wrappers/interface \
		$(LOCAL_PATH)/../../voice_engine/main/include \
		$(LOCAL_PATH)/../../voice_engine/main/source \
		$(LOCAL_PATH)/../../video_engine/include \
		$(LOCAL_PATH)/../../video_engine/source \
		$(LOCAL_PATH)/../interface \
		$(LOCAL_PATH)/../.. \
		$(LOCAL_PATH)/../../module \
    $(LOCAL_PATH)/../../module/interface \
    $(LOCAL_PATH)/../../module/exosip/include \
    $(LOCAL_PATH)/../../module/exosip/source \
    $(LOCAL_PATH)/../../module/osip/include \
    $(LOCAL_PATH)/../../module/videojpegyuv/include \
  $(LOCAL_PATH)/../../third_party/oRTP/include \
	$(LOCAL_PATH)/../../third_party/oRTP/include/ortp \
	$(LOCAL_PATH)/../../module/rtp_rtcp/source/oRTP/include/ortp/ \
	$(LOCAL_PATH)/../../third_party/srtp \
	$(LOCAL_PATH)/../../third_party/srtp/crypto/include \
	$(LOCAL_PATH)/../../third_party/gsm/inc/ \
	$(LOCAL_PATH)/../../third_party/openssl \
	$(LOCAL_PATH)/../../module/video_coding/codecs/interface \
	$(LOCAL_PATH)/../../module/video_coding/codecs/h264
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
