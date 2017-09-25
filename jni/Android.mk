# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

MY_WEBRTC_ROOT_PATH := $(call my-dir)/..
JNI_PATH := $(call my-dir)

include $(MY_WEBRTC_ROOT_PATH)/module/osip/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/module/exosip/Android.mk
include $(MY_WEBRTC_ROOT_PATH)/servicecore/source/Android_video.mk
include $(MY_WEBRTC_ROOT_PATH)/jni/BuildECMedia.mk

# build .so
LOCAL_PATH := $(JNI_PATH)
include $(CLEAR_VARS)

include $(JNI_PATH)/../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libserphone
LOCAL_MODULE_TAGS := optional
LOCAL_LDLIBS := -L$(JNI_PATH)

LOCAL_SRC_FILES := \
		callback.cpp \
		com_CCP_phone_NativeInterface.cpp
		
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/../servicecore/include \
		$(LOCAL_PATH)/../servicecore/source/Utility \
		$(LOCAL_PATH)/../servicecore/interface \
		$(LOCAL_PATH)/../system_wrappers/interface \
		$(LOCAL_PATH)/../module \
		$(LOCAL_PATH)/../base \
		$(LOCAL_PATH)/../third_party/ffmpeg \
		$(LOCAL_PATH)/../ECMedia/interface \
		$(LOCAL_PATH)/../third_party/protobuf/src \
		$(LOCAL_PATH)/../third_party/protobuf/src/google/protobuf

LOCAL_STATIC_LIBRARIES := \
	libserphone_service_core \
	libwebrtc_exosip \
	libwebrtc_osip \
	libortp 
	
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
	libECMedia

LOCAL_LDLIBS += -llog -lGLESv2

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true


LOCAL_PRELINK_MODULE := false
APP_ALLOW_MISSING_DEPS = true

include $(BUILD_SHARED_LIBRARY)

