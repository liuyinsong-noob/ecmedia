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
LOCAL_MODULE := libwebrtc_apm
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    aecm/aecm_core.c \
    aecm/aecm_core_c.c \
    aecm/echo_control_mobile.c \
    aec/aec_core.c \
    aec/aec_rdft.c \
    aec/aec_resampler.c \
    aec/echo_cancellation.c \
    agc/agc.cc \
    agc/agc_audio_proc.cc \
    agc/agc_manager_direct.cc \
    agc/circular_buffer.cc \
    agc/gmm.cc \
    agc/histogram.cc \
    agc/legacy/analog_agc.c \
    agc/legacy/digital_agc.c \
    agc/pitch_based_vad.cc \
    agc/pitch_internal.cc \
    agc/pole_zero_filter.cc \
    agc/standalone_vad.cc \
    agc/utility.cc \
	afs/afs_core.c \
	afs/howling_control.c \
	afs/howlingfilter_core.c \
    audio_buffer.cc \
    audio_processing_impl.cc \
    beamformer/covariance_matrix_generator.cc \
    channel_buffer.cc \
    echo_cancellation_impl.cc \
    echo_control_mobile_impl.cc \
    gain_control_impl.cc \
    high_pass_filter_impl.cc \
    level_estimator_impl.cc \
    noise_suppression_impl.cc \
    ns/noise_suppression.c \
    ns/ns_core.c \
    processing_component.cc \
    rms_level.cc \
    splitting_filter.cc \
    transient/moving_moments.cc \
    transient/transient_detector.cc \
    transient/transient_suppressor.cc \
    transient/wpd_node.cc \
    transient/wpd_tree.cc \
    typing_detection.cc \
    utility/delay_estimator.c \
    utility/delay_estimator_wrapper.c \
    utility/fft4g.c \
    voice_detection_impl.cc \
    howling_control_impl.cc
    
# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DWEBRTC_NS_FLOAT' \
    '-DWEBRTC_ANDROID_PLATFORM_BUILD' \
    '-DWEBRTC_POSIX' 
#   '-DWEBRTC_AUDIOPROC_DEBUG_DUMP'
#   floating point
#   -DWEBRTC_NS_FLOAT'
#		'-DWEBRTC_NS_FIXED' \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
	  $(LOCAL_PATH)/./aecm/include \
		$(LOCAL_PATH)/./aec/include \
		$(LOCAL_PATH)/./aec \
		$(LOCAL_PATH)/./agc/legacy \
		$(LOCAL_PATH)/./agc \
		$(LOCAL_PATH)/./beamformer \
		$(LOCAL_PATH)/./ns \
		$(LOCAL_PATH)/./ns/include \
		$(LOCAL_PATH)/./utility \
		$(LOCAL_PATH)/./utility/include \
		$(LOCAL_PATH)/./transient \
		$(LOCAL_PATH)/./afs \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../.. \
    $(LOCAL_PATH)/../../interface \
    $(LOCAL_PATH)/../../signalprocess/include \
    $(LOCAL_PATH)/../../../system_wrappers/interface \
    $(LOCAL_PATH)/../../../system_wrappers\interface \
		$(LOCAL_PATH)/../../../system_wrappers/source \
		$(LOCAL_PATH)/../../interface \
		$(LOCAL_PATH)/../../signalprocess/include \
		$(LOCAL_PATH)/../../common_audio/source \
		$(LOCAL_PATH)/../../common_audio/source/signal_processing/include \
		$(LOCAL_PATH)/../../common_audio/source/vad/include \
		$(LOCAL_PATH)/../../common_audio/source/resampler/include \
		$(LOCAL_PATH)/../../common_audio/source/resampler \
		$(LOCAL_PATH)/../../common_audio/include \
		$(LOCAL_PATH)/../../utility/include \
		$(LOCAL_PATH)/../../audio_coding/main/source/codecs/isac/main/source \
		$(LOCAL_PATH)/../../audio_coding/main/source/codecs/isac/main/interface

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)

