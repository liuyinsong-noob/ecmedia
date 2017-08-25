# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_system_wrappers
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    android/cpu-features.c \
    cpu_features_android.c \
    map.cc \
    sort.cc \
    aligned_malloc.cc \
    atomic32_non_darwin_unix.cc \
    clock.cc\
    condition_variable.cc \
    cpu_features.cc \
    cpu_info.cc \
    critical_section.cc \
    event_timer_posix.cc \
    event_wrapper_impl.cc \
    file_impl.cc \
    list_no_stl.cc \
    rw_lock.cc \
    thread.cc \
    trace_impl.cc \
    condition_variable_posix.cc \
    cpu_linux.cc \
    critical_section_posix.cc \
    thread_posix.cc \
    trace_posix.cc \
    rw_lock_posix.cc \
    stats_types.cc \
    field_trial_default.cc \
    logging.cc \
    metrics_default.cc \
    platform_file.cc \
    rtp_to_ntp_estimator.cc \
    sleep.cc \
    data_log_c.cc \
    cpu.cc \
    data_log_no_op.cc \
    timestamp_extrapolator.cc \
    ../../base/numerics/exp_filter.cc \
    ../../base/asyncpacketsocket.cc \
    ../../base/asyncresolverinterface.cc \
    ../../base/asyncsocket.cc \
    ../../base/bitbuffer.cc \
    ../../base/checks.cc \
    ../../base/copyonwritebuffer.cc\
    ../../base/criticalsection.cc \
    ../../base/event.cc \
    ../../base/event_tracer.cc \
    ../../base/ipaddress.cc \
    ../../base/location.cc \
    ../../base/messagehandler.cc \
    ../../base/messagequeue.cc \
    ../../base/nethelpers.cc \
    ../../base/nullsocketserver.cc \
    ../../base/platform_thread.cc \
    ../../base/race_checker.cc \
    ../../base/random.cc \
    ../../base/rate_limiter.cc \
    ../../base/rate_statistics.cc \
    ../../base/sharedexclusivelock.cc \
    ../../base/signalthread.cc \
    ../../base/sigslot.cc \
    ../../base/socketaddress.cc \
    ../../base/stringencode.cc \
    ../../base/stringutils.cc \
    ../../base/thread_checker_impl.cc \
    ../../base/threads.cc \
    ../../base/timeutils.cc \
    ../../base/ifaddrs-android.cc \
    ../../base/sequenced_task_checker_impl.cc \
    ../../base/physicalsocketserver.cc \
    ../../base/networkmonitor.cc \
	../../base/task_queue_posix.cc \
    ../../base/task_queue_libevent.cc

LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DWEBRTC_POSIX'

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH) \
		$(LOCAL_PATH)/../include \
		$(LOCAL_PATH)/../.. \
		$(LOCAL_PATH)/../../module \
    $(LOCAL_PATH)/../../module/include \
    $(LOCAL_PATH)/../../third_party \
    $(LOCAL_PATH)/../../third_party/libevent \
	$(LOCAL_PATH)/../../third_party/libevent/android \
	$(LOCAL_PATH)/../../third_party/libevent/include \
	$(LOCAL_PATH)/android \
    $(LOCAL_PATH)/spreadsortlib

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
