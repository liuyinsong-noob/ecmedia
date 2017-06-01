# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_audio_coding
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    $(call all-proto-files-under, .) \
    acm_amr.cc \
    acm_amrwb.cc \
    acm_cng.cc \
    acm_codec_database.cc \
    acm_dtmf_playout.cc \
    acm_g722.cc \
    acm_generic_codec.cc \
    acm_ilbc.cc \
    acm_g729.cc \
    acm_g7291.cc \
    acm_isac.cc \
    acm_opus.cc \
    acm_pcm16b.cc \
    acm_pcma.cc \
    acm_pcmu.cc \
    acm_red.cc \
    acm_resampler.cc \
    acm_speex.cc \
    audio_coding_module.cc \
    audio_coding_module_impl.cc \
    acm_receiver.cc \
    call_statistics.cc \
    nack.cc \
    initial_delay_manager.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    -DWEBRTC_CODEC_G729 \
    -DWEBRTC_CODEC_AMR \
	-DWEBRTC_CODEC_RED \
	-DWEBRTC_CODEC_OPUS \
	-DWEBRTC_CODEC_AVT \
	-DWEBRTC_CODEC_PCM16

#   -DWEBRTC_CODEC_ILBC \
# 	-DWEBRTC_CODEC_SILK \
#   -DWEBRTC_CODEC_AVT \

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/.	\
		$(LOCAL_PATH)/../../../. \
		$(LOCAL_PATH)/../../main/include \
		$(LOCAL_PATH)/../../main/source \
		$(LOCAL_PATH)/../../../interface \
		$(LOCAL_PATH)/../../neteq/source \
		$(LOCAL_PATH)/../../neteq/include \
		$(LOCAL_PATH)/../../../../system_wrappers/interface \
		$(LOCAL_PATH)/../../../../system_wrappers/source \
		$(LOCAL_PATH)/../../../resampler/include \
		$(LOCAL_PATH)/../../../signalprocess/include \
		$(LOCAL_PATH)/../../codecs \
		$(LOCAL_PATH)/../../codecs/g711/include \
		$(LOCAL_PATH)/../../codecs/cng/include \
		$(LOCAL_PATH)/../../codecs/opencore-amr/amrnb \
		$(LOCAL_PATH)/../../codecs/ilbc/interface \
		$(LOCAL_PATH)/../../codecs/g729/interface \
		$(LOCAL_PATH)/../../codecs/g729/source \
		$(LOCAL_PATH)/../../codecs/silk \
		$(LOCAL_PATH)/../../codecs/opus/interface \
		$(LOCAL_PATH)/../../../common_video \
		$(LOCAL_PATH)/../../../common_audio/source/vad/include \
		$(LOCAL_PATH)/../../../common_audio/source/resampler/include \
		$(LOCAL_PATH)/../../../common_audio/source/signal_processing/include \
		$(LOCAL_PATH)/../../../../voice_engine/main/source \
		$(LOCAL_PATH)/../../../../third_party/opus/src \
		$(LOCAL_PATH)/../../codecs/pcm16b/include \
		$(LOCAL_PATH)/../../../../third_party/SoundTouch/SoundTouch \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
