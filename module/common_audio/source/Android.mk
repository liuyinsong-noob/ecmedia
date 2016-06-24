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
LOCAL_MODULE := libwebrtc_common_audio
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    audio_converter.cc \
    audio_util.cc \
    blocker.cc \
    fir_filter.cc \
    resampler/push_resampler.cc \
    resampler/push_sinc_resampler.cc \
    resampler/resampler.cc \
    resampler/sinc_resampler.cc \
    ring_buffer.c \
    signal_processing/auto_correlation.c \
    signal_processing/auto_corr_to_refl_coef.c \
    signal_processing/complex_bit_reverse.c \
    signal_processing/complex_fft.c \
    signal_processing/copy_set_operations.c \
    signal_processing/cross_correlation.c \
    signal_processing/division_operations.c \
    signal_processing/dot_product_with_scale.c \
    signal_processing/downsample_fast.c \
    signal_processing/energy.c \
    signal_processing/filter_ar.c \
    signal_processing/filter_ar_fast_q12.c \
    signal_processing/filter_ma_fast_q12.c \
    signal_processing/get_hanning_window.c \
    signal_processing/get_scaling_square.c \
    signal_processing/ilbc_specific_functions.c \
    signal_processing/levinson_durbin.c \
    signal_processing/lpc_to_refl_coef.c \
    signal_processing/min_max_operations.c \
    signal_processing/randomization_functions.c \
    signal_processing/real_fft.c \
    signal_processing/refl_coef_to_lpc.c \
    signal_processing/resample.c \
    signal_processing/resample_48khz.c \
    signal_processing/resample_by_2.c \
    signal_processing/resample_by_2_internal.c \
    signal_processing/resample_fractional.c \
    signal_processing/splitting_filter.c \
    signal_processing/spl_init.c \
    signal_processing/spl_sqrt.c \
    signal_processing/spl_sqrt_floor.c \
    signal_processing/sqrt_of_one_minus_x_squared.c \
    signal_processing/vector_scaling_operations.c \
    vad/vad.cc \
    vad/vad_core.c \
    vad/vad_filterbank.c \
    vad/vad_gmm.c \
    vad/vad_sp.c \
    vad/webrtc_vad.c \
    wav_file.cc \
    wav_header.cc \
    window_generator.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DWEBRTC_POSIX'

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/../../../system_wrappers/source \
		$(LOCAL_PATH)/../../../system_wrappers/interface \
		$(LOCAL_PATH)/../../audioprocess/source \
		$(LOCAL_PATH)/../.. \
		$(LOCAL_PATH)/signal_processing/include \
		$(LOCAL_PATH)/resampler \
		$(LOCAL_PATH)/resampler/include \
		$(LOCAL_PATH)/vad/include \
		$(LOCAL_PATH)/../include

    
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)

