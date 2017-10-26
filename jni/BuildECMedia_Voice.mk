# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

MY_WEBRTC_ROOT_PATH := $(call my-dir)/..
JNI_PATH := $(call my-dir)

APP_ALLOW_MISSING_DEPS := true

# voice
include $(MY_WEBRTC_ROOT_PATH)/system_wrappers/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audioprocess/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/g711/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/cng/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/g729/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/opencore-amr/amrnb/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/opus/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/pcm16b/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/neteq/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/main/source/codecs/isac/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_device/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/rtp_rtcp/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/udp_transport/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/utility/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/common_video/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_conference_mixer/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/bitrate_controller/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/congestion_controller/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/remote_bitrate_estimator/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/media_file/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/common_audio/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/video_engine/source/Android_stats.mk
include $(MY_WEBRTC_ROOT_PATH)/voice_engine/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/ECMedia/source/Android_voice.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/oRTP/build/android/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/pacing/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/srtp/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/protobuf/src/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/SoundTouch/SoundTouch/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/libevent/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/libjpeg_turbo/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/libyuv/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/zlib/Android.mk

# build .so
LOCAL_PATH := $(JNI_PATH)
include $(CLEAR_VARS)

include $(JNI_PATH)/../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libECMedia_Voice
LOCAL_MODULE_TAGS := optional
LOCAL_LDLIBS := -L$(JNI_PATH)

LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_STATIC_LIBRARIES := \

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libMedia \
	libwebrtc_voe_core \
	libwebrtc_stats  \
	libwebrtc_bitrate_controller \
	libwebrtc_congestion_controller \
	libwebrtc_common_video \
	libwebrtc_remote_bitrate_estimator \
	libwebrtc_audio_coding \
	libwebrtc_audio_device \
	libwebrtc_apm \
	libwebrtc_neteq \
	libwebrtc_iSAC \
	libwebrtc_g711 \
	libwebrtc_g729 \
	libwebrtc_cng \
	libwebrtc_amr_nb \
	libwebrtc_opus \
	libwebrtc_pcm16b \
	libwebrtc_common_audio \
	libwebrtc_spl \
	libwebrtc_rtp_rtcp \
	libwebrtc_udp_transport \
	libwebrtc_audio_conference_mixer \
	libwebrtc_utility \
	libwebrtc_media_file \
	libstlport_static \
	libcpufeatures \
	libwebrtc_audio_paced_sender \
	libwebrtc_audio_codecs \
	libwebrtc_system_wrappers \
	libwebrtc_audio_paced_sender \
	libortp \
	libsrtp \
	libjpeg_turbo \
	libyuv \
	libevent \
	libProtobuf_lite \
	libSoundTouch \
	libEC_zlib \


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl

LOCAL_LDLIBS += -llog -lGLESv2

LOCAL_LDLIBS += \
	./third_party_libs/$(TARGET_ARCH_ABI)/libcpufeatures.a \
	./third_party_libs/$(TARGET_ARCH_ABI)/libopus.a \
	./third_party_libs/$(TARGET_ARCH_ABI)/libssl.a \
	./third_party_libs/$(TARGET_ARCH_ABI)/libcrypto.a \

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
