# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_voe_core
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		channel.cc \
		channel_manager.cc \
		dtmf_inband.cc \
		dtmf_inband_queue.cc \
		level_indicator.cc \
		monitor_module.cc \
		network_predictor.cc \
		output_mixer.cc \
		shared_data.cc \
		statistics.cc \
		transmit_mixer.cc \
		utility.cc \
		voe_audio_processing_impl.cc \
		voe_base_impl.cc \
		voe_codec_impl.cc \
		voe_dtmf_impl.cc \
		voe_encryption_impl.cc \
		voe_external_media_impl.cc \
		voe_file_impl.cc \
		voe_hardware_impl.cc \
		voe_neteq_stats_impl.cc \
		voe_network_impl.cc \
		voe_rtp_rtcp_impl.cc \
		voe_video_sync_impl.cc \
		voe_volume_control_impl.cc \
		voice_engine_impl.cc \
		SrtpModule.cc \
		wavfile.cpp \
		audio_send_stream.cc \
		audio_receive_stream.cc
		
#		voe_encry_srtp.cc \
		
		

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DHAVE_SRTP_SHUTDOWN' \
	  '-DWEBRTC_CODEC_RED' \
	  '-DWEBRTC_POSIX'
	  
	#'-DWEBRTC_ANDROID_OPENSLES'
	#'-DWEBRTC_SRTP' \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../../.. \
    $(LOCAL_PATH)/../../../module \
    $(LOCAL_PATH)/../../../signalprocess/include \
    $(LOCAL_PATH)/../../../module/interface \
    $(LOCAL_PATH)/../../../module/audio_coding/main/include \
    $(LOCAL_PATH)/../../../module/audio_conference_mixer/include \
    $(LOCAL_PATH)/../../../module/audio_device/main/include \
    $(LOCAL_PATH)/../../../module/audio_device/main/source \
    $(LOCAL_PATH)/../../../module/audioprocess/include \
    $(LOCAL_PATH)/../../../module/signalprocess/include \
    $(LOCAL_PATH)/../../../module/media_file/include \
    $(LOCAL_PATH)/../../../module/rtp_rtcp/include \
    $(LOCAL_PATH)/../../../module/udp_transport/include \
    $(LOCAL_PATH)/../../../module/utility/include \
    $(LOCAL_PATH)/../../../module/resampler/include \
    $(LOCAL_PATH)/../../../module/common_audio/source/resampler/include \
    $(LOCAL_PATH)/../../../module/audio_coding/main/source \
    $(LOCAL_PATH)/../../../module/audio_coding/codecs \
    $(LOCAL_PATH)/../../../module/audio_coding/codecs/cng/include \
    $(LOCAL_PATH)/../../../module/audio_coding/neteq/include \
    $(LOCAL_PATH)/../../../module/audio_coding/neteq/source \
    $(LOCAL_PATH)/../../../module/audioprocess/source \
    $(LOCAL_PATH)/../../../module/bitrate_controller/include \
    $(LOCAL_PATH)/../../../system_wrappers/interface \
    $(LOCAL_PATH)/../../../system_wrappers/source \
    $(LOCAL_PATH)/../../../module/common_video/interface \
    $(LOCAL_PATH)/../../../module/video_coding/main/include \
    $(LOCAL_PATH)/../../../module/common_video/source \
    $(LOCAL_PATH)/../../../module/rtp_rtcp/source \
    $(LOCAL_PATH)/../../../module/common_audio/source/signal_processing/include \
    $(LOCAL_PATH)/../../../module/audio_device/main/source/android \
    $(LOCAL_PATH)/../../../video_engine/include \
	$(LOCAL_PATH)/../../../third_party/srtp/include \
	$(LOCAL_PATH)/../../../third_party/srtp/crypto/include \
	$(LOCAL_PATH)/../../../third_party/srtp \
	$(LOCAL_PATH)/../../../third_party/oRTP/include/ortp \
	$(LOCAL_PATH)/../../../third_party/oRTP/include \
	$(LOCAL_PATH)/../../../servicecore/include/

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport 

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl -lpthread
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
