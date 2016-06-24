# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_g729
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		adaptativeCodebookSearch.c \
		codebooks.c \
		computeAdaptativeCodebookGain.c \
		computeLP.c \
		computeWeightedSpeech.c \
		decodeAdaptativeCodeVector.c \
		decodeFixedCodeVector.c \
		decodeGains.c \
		decodeLSP.c \
		decoder.c \
		encoder.c \
		findOpenLoopPitchDelay.c \
		fixedCodebookSearch.c \
		gainQuantization.c \
		g729_interface.cpp \
		interpolateqLSP.c \
		LP2LSPConversion.c \
		LPSynthesisFilter.c \
		LSPQuantization.c \
		postFilter.c \
		postProcessing.c \
		preProcessing.c \
		qLSP2LP.c \
		utils.c

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../interface \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../include/bcg729 \
    $(LOCAL_PATH)/../../../.. \
    $(LOCAL_PATH)/../../../../signalprocess/include 

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
