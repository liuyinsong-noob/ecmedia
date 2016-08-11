# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libjpeg_turbo
LOCAL_MODULE_TAGS := optional
#LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	jidctflt.c \
    jdmaster.c \
    jcapimin.c \
    jfdctint.c \
    jcomapi.c \
    jmemnobs.c \
    jcprepct.c \
    jcsample.c \
    jcmainct.c \
    jdatadst.c \
    jcinit.c \
    jddctmgr.c \
    jdinput.c \
    jcmarker.c \
    jdmarker.c \
    jquant2.c \
    jcapistd.c \
    jdapistd.c \
    jdmerge.c \
    jchuff.c \
    jfdctfst.c \
    jccoefct.c \
    jerror.c \
    jmemmgr.c \
    jdhuff.c \
    jcdctmgr.c \
    jfdctflt.c \
    jdcoefct.c \
    jdatasrc.c \
    jidctred.c \
    jccolor.c \
    jcparam.c \
    jidctint.c \
    jcphuff.c \
    jdmainct.c \
    jdcolor.c \
    jcmaster.c \
    jutils.c \
    jdphuff.c \
    jquant1.c \
    jdpostct.c \
    jdapimin.c \
    jidctfst.c \
    jdsample.c

ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_SRC_FILES += \
	./simd/jsimd_arm.c \
	./simd/jsimd_arm_neon.S
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES += \
	./simd/jsimd_arm.c \
	./simd/jsimd_arm_neon.S
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES += \
	./simd/jsimd_arm64.c \
	./simd/jsimd_arm64_neon.S
endif

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_SRC_FILES += \
LOCAL_SRC_FILES += \
	./simd/jsimd_i386.c \
	./simd/jccolss2.asm \
	./simd/jcgrass2.asm \
	./simd/jcqnt3dn.asm \
	./simd/jcqnts2f.asm \
	./simd/jcqnts2i.asm \
	./simd/jcqntsse.asm \
	./simd/jcsamss2.asm \
	./simd/jdcolss2.asm \
	./simd/jdmerss2.asm \
	./simd/jdsamss2.asm \
	./simd/jf3dnflt.asm \
	./simd/jfss2fst.asm \
	./simd/jfss2int.asm \
	./simd/jfsseflt.asm \
	./simd/ji3dnflt.asm \
	./simd/jiss2flt.asm \
	./simd/jiss2fst.asm \
	./simd/jiss2int.asm \
	./simd/jiss2red.asm \
	./simd/jisseflt.asm \
	./simd/jsimdcpu.asm
endif

ifeq ($(TARGET_ARCH_ABI),x86_64)
LOCAL_SRC_FILES += \
	./simd/jsimd_x86_64.c \
	./simd/jsimdcpu.asm
endif
	
#    ./simd/jsimd_arm.c \
#    ./simd/jsimd_arm_neon.S
	

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
  $(MY_WEBRTC_COMMON_DEFS)
 
# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/. \
	$(LOCAL_PATH)/android \
	$(LOCAL_PATH)/simd

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)