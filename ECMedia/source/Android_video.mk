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
LOCAL_MODULE := libMedia
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		base64.c \
		ECMedia.cpp \
		RecordVoip.cpp \
		statsCollector.cc \
		VoeObserver.cpp \
		ECLiveStream_rtmp.cpp \
		ECLiveStream.cpp \
		faaccodec.cpp

LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
	'-DVIDEO_ENABLED'

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../../system_wrappers/interface \
	$(LOCAL_PATH)/../../voice_engine/main/include \
	$(LOCAL_PATH)/../../voice_engine/main/source \
	$(LOCAL_PATH)/../../video_engine/include \
	$(LOCAL_PATH)/../../video_engine/source \
	$(LOCAL_PATH)/../../video_engine/source/video \
	$(LOCAL_PATH)/../interface \
	$(LOCAL_PATH)/../.. \
	$(LOCAL_PATH)/../../module \
	$(LOCAL_PATH)/../../module/interface \
	$(LOCAL_PATH)/../../module/videojpegyuv/include \
	$(LOCAL_PATH)/../../third_party/oRTP/include \
	$(LOCAL_PATH)/../../third_party/oRTP/include/ortp \
	$(LOCAL_PATH)/../../module/rtp_rtcp/source/oRTP/include/ortp/ \
	$(LOCAL_PATH)/../../third_party/srtp \
	$(LOCAL_PATH)/../../third_party/srtp/crypto/include \
	$(LOCAL_PATH)/../../third_party/gsm/inc/ \
	$(LOCAL_PATH)/../../third_party/openssl \
	$(LOCAL_PATH)/../../third_party/librtmp/include \
	$(LOCAL_PATH)/../../module/video_coding/codecs/interface \
	$(LOCAL_PATH)/../../module/video_coding/codecs/h264 \
	$(LOCAL_PATH)/../../module/video_coding/main/include \
	$(LOCAL_PATH)/../../module/common_video/interface \
	$(LOCAL_PATH)/../../module/common_video/source \
	$(LOCAL_PATH)/../../module/rtp_rtcp/include \
	$(LOCAL_PATH)/../../module/common_video/source/libyuv/include \
	$(LOCAL_PATH)/../../module/common_audio/source/resampler/include \
	$(LOCAL_PATH)/../../module/bitrate_controller/include \
	$(LOCAL_PATH)/../../module/remote_bitrate_estimator/include \
	$(LOCAL_PATH)/../../module/pacing/include \
	$(LOCAL_PATH)/../../module/remote_bitrate_estimator/source \
	$(LOCAL_PATH)/../../module/video_render/main/include \
	$(LOCAL_PATH)/../../module/audio_coding/codecs/opencore-amr/amrnb \
	$(LOCAL_PATH)/../../third_party/libfaad/faad2-2.7/include \
	$(LOCAL_PATH)/../../third_party/libfaac/include \
	$(LOCAL_PATH)/../../third_party/ffmpeg/ffmpeg-android-bin/include
	
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
