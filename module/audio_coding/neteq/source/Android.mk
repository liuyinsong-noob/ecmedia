# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
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
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_neteq
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    accelerate.cc \
    audio_decoder_impl.cc \
    audio_multi_vector.cc \
    audio_vector.cc \
    background_noise.cc \
    buffer_level_filter.cc \
    comfort_noise.cc \
    decision_logic.cc \
    decision_logic_fax.cc \
    decision_logic_normal.cc \
    decoder_database.cc \
    delay_manager.cc \
    delay_peak_detector.cc \
    dsp_helper.cc \
    dtmf_buffer.cc \
    dtmf_tone_generator.cc \
    expand.cc \
    merge.cc \
    neteq_impl.cc \
    neteq.cc \
    statistics_calculator.cc \
    normal.cc \
    packet_buffer.cc \
    payload_splitter.cc \
    post_decode_vad.cc \
    preemptive_expand.cc \
    random_vector.cc \
    rtcp.cc \
    sync_buffer.cc \
    timestamp_scaler.cc \
    time_stretch.cc \

#    audio_classifier.cc \
# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DNETEQ_VOICEENGINE_CODECS' \
    '-DWEBRTC_CODEC_OPUS' \
	'-DWEBRTC_CODEC_G729'

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
		$(LOCAL_PATH)/../../../common_audio/source/signal_processing/include \
		$(LOCAL_PATH)/../../../common_audio/source/vad/include \
		$(LOCAL_PATH)/../../../audio_coding/codecs \
		$(LOCAL_PATH)/../../../audio_coding/codecs/cng/include \
		$(LOCAL_PATH)/../../../audio_coding/codecs/g711/include \
		$(LOCAL_PATH)/../../../audio_coding/codecs/opus/interface \
		$(LOCAL_PATH)/../../../audio_coding/codecs/g729/source \
		$(LOCAL_PATH)/../../../audio_coding/codecs/g729/interface \
		$(LOCAL_PATH)/../../../../system_wrappers/interface \
		$(LOCAL_PATH)/../../../../system_wrappers/source \
		$(LOCAL_PATH)/../../.. \
		$(LOCAL_PATH)/../../../interface \
		$(LOCAL_PATH)/../../../../third_party/opus/src/celt \
		$(LOCAL_PATH)/../../../../third_party/opus/src/include \
		$(LOCAL_PATH)/../../../../third_party/opus/src/src \
		$(LOCAL_PATH)/../../../../voice_engine/main/source
		

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
