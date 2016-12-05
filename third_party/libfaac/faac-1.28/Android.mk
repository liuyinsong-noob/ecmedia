LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:= faac
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := 			\
	$(LOCAL_PATH)				\
	$(LOCAL_PATH)/include		\
	$(LOCAL_PATH)/libfaac

LOCAL_SRC_FILES := 				\
	./libfaac/kiss_fft/kiss_fft.c \
	./libfaac/kiss_fft/kiss_fftr.c \
	./libfaac/aacquant.c \
	./libfaac/backpred.c \
	./libfaac/bitstream.c \
	./libfaac/channels.c \
	./libfaac/fft.c \
	./libfaac/filtbank.c \
	./libfaac/frame.c \
	./libfaac/huffman.c \
	./libfaac/ltp.c \
	./libfaac/midside.c \
	./libfaac/psychkni.c \
	./libfaac/tns.c \
	./libfaac/util.c
		

WITH_MP4V2_TRUE := false

LOCAL_CFLAGS:= -DHAVE_CONFIG_H
LOCAL_LDLIBS := -llog

include $(BUILD_STATIC_LIBRARY)
