LOCAL_PATH := $(call my-dir)
LIBVPX_JNI_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)
LIBVPX_PATH := $(LOCAL_PATH)/../../../libvpx
include $(LOCAL_PATH)/../../../libvpx/build/make/Android.mk

LOCAL_PATH := $(LIBVPX_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := libvpx_dummy.c
LOCAL_MODULE := libvpx_dummy
LOCAL_STATIC_LIBRARIES := libvpx

include $(BUILD_SHARED_LIBRARY)
