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
include $(MY_WEBRTC_ROOT_PATH)/module/resampler/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/signalprocess/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audioprocess/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/g711/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/cng/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/ilbc/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/silk/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/g729/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/codecs/opencore-amr/amrnb/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_coding/neteq/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_device/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/osip/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/exosip/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/rtp_rtcp/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/udp_transport/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/utility/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/audio_conference_mixer/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/video_capture/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/video_render/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/video_processing/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/video_coding/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/videojpegyuv/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/video_coding/codecs/vp8/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/video_coding/codecs/h264/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/bitrate_controller/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/media_file/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/voice_engine/main/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/video_engine/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/servicecore/source/Android_video_haiyuntong.mk
#include $(MY_WEBRTC_ROOT_PATH)/third_party/libvpx/build/android/jni/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/libyuv/source/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/third_party/libjpeg/Android.mk
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

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/../servicecore/include \
		$(LOCAL_PATH)/../servicecore/source/Utility \
		$(LOCAL_PATH)/../servicecore/interface \
		$(LOCAL_PATH)/../system_wrappers/interface \
		$(LOCAL_PATH)/../module \
		$(LOCAL_PATH)/../third_party/ffmpeg \
		$(LOCAL_PATH)/../module/audio_coding/codecs/opencore-amr/amrnb
		
LOCAL_WHOLE_STATIC_LIBRARIES :=


# Add Neon libraries.
ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)
LOCAL_WHOLE_STATIC_LIBRARIES += \
    libwebrtc_aecm_neon \
    libwebrtc_ns_neon
endif

LOCAL_STATIC_LIBRARIES := \
	libserphone_service_core \
  libwebrtc_voe_core \
	libwebrtc_vie_core \
	libwebrtc_video_coding \
	libwebrtc_video_capture \
	libwebrtc_video_processing \
	libwebrtc_video_render \
	libwebrtc_vp8 \
	libwebrtc_h264 \
	libwebrtc_yuv \
	libwebrtc_bitrate_controller \
  libwebrtc_audio_coding \
  libwebrtc_audio_device \
  libwebrtc_resampler \
  libwebrtc_apm \
  libwebrtc_neteq \
  libwebrtc_g711 \
  libwebrtc_g729 \
  libwebrtc_silk \
  libwebrtc_cng \
  libwebrtc_amr_nb \
  libwebrtc_spl \
  libwebrtc_exosip \
	libwebrtc_osip \
  libwebrtc_rtp_rtcp \
  libwebrtc_udp_transport \
  libwebrtc_audio_conference_mixer \
  libwebrtc_media_file \
  libwebrtc_utility \
  libwebrtc_system_wrappers \
  libyuv	\
	libjpeg \
  libstlport_static \
  libcpufeatures \
  libortp \
  libwebrtc_ilbc
#    libpvdecoder_gsmamr \
#    libpvencoder_gsmamr \
#    libpv_amr_nb_common_lib \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
#    libstlport 
#    libOpenSLES

LOCAL_LDLIBS += -llog -lGLESv2 
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
# ssl
#  LOCAL_LDLIBS += -lvpx_armv7a -lcpufeatures -lx264 -lavcodec -lavutil -lsilk_armv7a  -lz -lsrtp -lssl -lcrypto
   LOCAL_LDLIBS += -lvpx_armv7a -lcpufeatures -lx264 -lavcodec -lavutil -lsilk_armv7a  -lz -lsrtp -lcrypto -lVoipCpt
#  LOCAL_LDLIBS += -lvpx_armv7a -lcpufeatures -lx264 -lavcodec -lavutil -lsilk_armv7a  -lz -lsrtp
else
# ssl
#  LOCAL_LDLIBS += -lvpx -lcpufeatures -lx264 -lavcodec -lavutil -lsilk -lz  -lsrtp -lssl -lcrypto
   LOCAL_LDLIBS += -lvpx -lcpufeatures -lx264 -lavcodec -lavutil -lsilk -lz  -lsrtp -lcrypto -lVoipCpt
#  LOCAL_LDLIBS += -lvpx -lcpufeatures -lx264 -lavcodec -lavutil -lsilk -lz  -lsrtp
endif


LOCAL_PRELINK_MODULE := false

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
