# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

MY_WEBRTC_ROOT_PATH := $(call my-dir)/..
JNI_PATH := $(call my-dir)

# voice
include $(MY_WEBRTC_ROOT_PATH)/system_wrappers/source/Android.mk
#--include $(MY_WEBRTC_ROOT_PATH)/module/resampler/source/Android.mk
#--include $(MY_WEBRTC_ROOT_PATH)/module/signalprocess/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audioprocess/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/g711/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/cng/Android.mk
#include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/ilbc/Android.mk
#include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/silk/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/g729/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/opencore-amr/amrnb/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/opus/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/neteq/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/main/source/codecs/isac/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_device/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/osip/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/exosip/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/rtp_rtcp/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/udp_transport/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/utility/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_conference_mixer/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/bitrate_controller/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/media_file/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/common_audio/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/voice_engine/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/servicecore/source/Android_voice.mk
include $(MY_WEBRTC_ROOT_PATH)/ECMedia/source/Android_voice.mk

#include $(MY_WEBRTC_ROOT_PATH)/third_party/libjpeg/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/oRTP/build/android/Android.mk

# build .so
LOCAL_PATH := $(JNI_PATH)
include $(CLEAR_VARS)

include $(JNI_PATH)/../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libserphone
LOCAL_MODULE_TAGS := optional
LOCAL_LDLIBS := -L$(JNI_PATH)

LOCAL_SRC_FILES := \
		callback.cpp \
		com_CCP_phone_NativeInterface.cpp
		
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/../servicecore/include \
		$(LOCAL_PATH)/../servicecore/source/Utility \
		$(LOCAL_PATH)/../servicecore/interface \
		$(LOCAL_PATH)/../system_wrappers/interface \
		$(LOCAL_PATH)/../module \
		$(LOCAL_PATH)/../third_party/ffmpeg \
		$(LOCAL_PATH)/../module/audio_coding/codecs/opencore-amr/amrnb

LOCAL_WHOLE_STATIC_LIBRARIES :=

LOCAL_STATIC_LIBRARIES := \
	libserphone_service_core \
	libMedia \
	libwebrtc_voe_core \
	libwebrtc_audio_coding \
	libwebrtc_audio_device \
	libwebrtc_resampler \
	libwebrtc_apm \
	libwebrtc_neteq \
	libwebrtc_g711 \
	libwebrtc_g729 \
	libwebrtc_cng \
	libwebrtc_amr_nb \
	libwebrtc_opus \
	libwebrtc_common_audio \
	libwebrtc_spl \
	libwebrtc_exosip \
	libwebrtc_osip \
	libwebrtc_rtp_rtcp \
	libwebrtc_udp_transport \
	libwebrtc_audio_conference_mixer \
	libwebrtc_media_file \
	libwebrtc_utility \
	libstlport_static \
	libcpufeatures \
	libortp \
	libwebrtc_bitrate_controller \
	libwebrtc_iSAC \
	libwebrtc_audio_codecs \
	libwebrtc_media_file \
	libwebrtc_system_wrappers

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl 
#    libstlport 
#    libOpenSLES

LOCAL_LDLIBS += -llog -lGLESv2

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true

ifeq ($(TARGET_ARCH_ABI),armeabi)
	LOCAL_LDLIBS +=  -lopus  -lz -lcpufeatures
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_LDLIBS += -lopus_armv7a -lz -lcpufeatures
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
	LOCAL_LDLIBS += -lopus_armv8a -lz
endif

ifeq ($(TARGET_ARCH_ABI),x86)
	LOCAL_LDLIBS += -lopus_x86 -lz -lcpufeatures
endif

ifeq ($(TARGET_ARCH_ABI),x86_64)
	LOCAL_LDLIBS += -lopus_x86_64 -lz -lcpufeatures
endif


LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

#$(call import-module,android/cpufeatures)
