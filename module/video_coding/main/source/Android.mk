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
LOCAL_MODULE := libwebrtc_video_coding
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
   	./utility/frame_dropper.cc \
   	codec_database.cc \
   	codec_timer.cc \
   	content_metrics_processing.cc \
   	decoding_state.cc \
   	encoded_frame.cc \
   	frame_buffer.cc \
   	generic_decoder.cc \
   	generic_encoder.cc \
   	inter_frame_delay.cc \
   	jitter_buffer.cc \
   	jitter_estimator.cc \
   	media_opt_util.cc \
   	media_optimization.cc \
   	packet.cc \
   	qm_select.cc \
   	receiver.cc \
   	rtt_filter.cc \
   	session_info.cc \
   	timestamp_map.cc \
   	timing.cc \
   	video_coding_impl.cc \
   	video_sender.cc \
   	video_receiver.cc
   	
#./utility/quality_scaler.cc \
#    ./utility/qp_parser.cc \
# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
		'-DVIDEOCODEC_H264' \
		'-DVIDEOCODEC_VP8' \
		'-D__STDC_CONSTANT_MACROS'
			

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../../codecs/interface \
    $(LOCAL_PATH)/../../codecs/i420/main/interface \
    $(LOCAL_PATH)/../../codecs/vp8/main/interface \
    $(LOCAL_PATH)/../../codecs/h264 \
    $(LOCAL_PATH)/../../../interface \
    $(LOCAL_PATH)/../../.. \
    $(LOCAL_PATH)/../../../videojpegyuv/include \
    $(LOCAL_PATH)/../../../common_video/source \
    $(LOCAL_PATH)/../../../common_video/interface \
    $(LOCAL_PATH)/../../../video_coding/main/source/utility/include \
    $(LOCAL_PATH)/../../../common_video/source/libyuv/include \
    $(LOCAL_PATH)/../../../.. \
    $(LOCAL_PATH)/../../../../common_video/vplib/main/interface \
    $(LOCAL_PATH)/../../../../common_video/interface \
    $(LOCAL_PATH)/../../../../system_wrappers/interface \
    $(LOCAL_PATH)/../../../../system_wrappers/source \
	$(LOCAL_PATH)/../../../../third_party/libx264/libx264_android/include \
    $(LOCAL_PATH)/../../../../third_party/ffmpeg/ffmpeg-android-bin/include
	
	
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
