# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_base
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    numerics/exp_filter.cc \
    asyncpacketsocket.cc \
    asyncresolverinterface.cc \
    asyncsocket.cc \
    bitbuffer.cc \
    checks.cc \
    copyonwritebuffer.cc\
    criticalsection.cc \
    event.cc \
    event_tracer.cc \
    ipaddress.cc \
    location.cc \
    messagehandler.cc \
    messagequeue.cc \
    nethelpers.cc \
    nullsocketserver.cc \
    platform_thread.cc \
    race_checker.cc \
    random.cc \
    rate_limiter.cc \
    rate_statistics.cc \
#    sequenced_task_checker_impl.cc \
    sharedexclusivelock.cc \
    signalthread.cc \
    sigslot.cc \
    socketaddress.cc \
    stringencode.cc \
    stringutils.cc \
    task_queue_gcd.cc \
    task_queue_libevent.cc \
    task_queue_posix.cc \
    thread_checker_impl.cc \
    threads.cc \
    timeutils.cc \
    ifaddrs-android.cc
		
#		checks.cc \
#   android/cpu-features.c \
#    cpu_no_op.cc \

LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DWEBRTC_POSIX' \
    '-DWEBRTC_ANDROID'

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH) \
		$(LOCAL_PATH)/../interface \
		$(LOCAL_PATH)/../.. \
		$(LOCAL_PATH)/../../module \
    $(LOCAL_PATH)/../../module/interface \
    $(LOCAL_PATH)/spreadsortlib

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
