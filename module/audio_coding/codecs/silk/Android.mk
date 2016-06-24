LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE    := webrtc_silk
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES :=  silk_interface.cc

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS = -O3 
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
LOCAL_C_INCLUDES += $(LOCAL_PATH)/  \
				$(LOCAL_PATH)/silk_android/src \
				$(LOCAL_PATH)/silk_android/interface \
				$(LOCAL_PATH)/../../../  \
				$(LOCAL_PATH)/../../../../system_wrappers/interface

	# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)
    
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport
    
ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif


include $(BUILD_STATIC_LIBRARY)