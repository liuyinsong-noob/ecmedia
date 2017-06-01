LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../../android-webrtc.mk


LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libSoundTouch
LOCAL_MODULE_TAGS := optional
				 

LOCAL_SRC_FILES := \
	AAFilter.cpp \
	BPMDetect.cpp \
	FIFOSampleBuffer.cpp \
	FIRFilter.cpp \
	PeakFinder.cpp \
	RateTransposer.cpp \
	SoundTouch.cpp \
	TDStretch.cpp \
	cpu_detect_x86.cpp \
	mmx_optimized.cpp \
	sse_optimized.cpp \


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/

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


