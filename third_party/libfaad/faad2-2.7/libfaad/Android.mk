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
LOCAL_MODULE := libfaad
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        bits.c      \
        cfft.c      \
        decoder.c   \
        drc.c       \
        drm_dec.c   \
        error.c     \
        filtbank.c  \
        ic_predict.c    \
        is.c        \
        lt_predict.c    \
        mdct.c      \
        mp4.c       \
        ms.c        \
        output.c    \
        pns.c       \
        ps_dec.c    \
        ps_syntax.c     \
        pulse.c     \
        specrec.c   \
        syntax.c    \
        tns.c       \
        hcr.c       \
        huffman.c   \
        rvlc.c      \
        ssr.c       \
        ssr_fb.c    \
        ssr_ipqf.c  \
        common.c    \
        sbr_dct.c   \
        sbr_e_nf.c  \
        sbr_fbt.c   \
        sbr_hfadj.c     \
        sbr_hfgen.c     \
        sbr_huff.c  \
        sbr_qmf.c   \
        sbr_syntax.c    \
        sbr_tf_grid.c   \
        sbr_dec.c
		

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
   -DHAVE_CONFIG_H
   
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/./ \
	$(LOCAL_PATH)/codebook \
	$(LOCAL_PATH)/../include

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
