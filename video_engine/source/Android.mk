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
LOCAL_MODULE := libwebrtc_vie_core
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    call_stats.cc \
    encoder_state_feedback.cc \
    overuse_frame_detector.cc \
    receive_statistics_proxy.cc \
    report_block_stats.cc \
    send_statistics_proxy.cc \
    stream_synchronization.cc \
    vie_base_impl.cc \
    vie_capturer.cc \
    vie_capture_impl.cc \
    vie_channel.cc \
    vie_channel_group.cc \
    vie_channel_manager.cc \
    vie_codec_impl.cc \
    vie_desktop_share_impl.cpp \
	vie_desktop_share_manager.cc \
	vie_desktop_capture.cpp \
    vie_encoder.cc \
    vie_external_codec_impl.cc \
    vie_file_image.cc \
    vie_file_impl.cc \
    vie_file_player.cc \
    vie_file_recorder.cc \
    vie_frame_provider_base.cc \
    vie_image_process_impl.cc \
    vie_impl.cc \
    vie_input_manager.cc \
    vie_manager_base.cc \
    vie_network_impl.cc \
    vie_receiver.cc \
    vie_ref_count.cc \
    vie_remb.cc \
    vie_renderer.cc \
    vie_render_impl.cc \
    vie_render_manager.cc \
    vie_rtp_rtcp_impl.cc \
    vie_sender.cc \
    vie_shared_data.cc \
	vie_sync_module.cc \
	vie_encryption_impl.cc \
	video_send_stream.cc \
	video/encoder_rtcp_feedback.cc \
    vie_watermark.cc \
    vie_file_capturer.cc  


	
	
# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    '-DVIDEOCODEC_H264' \
    '-DVIDEOCODEC_H264_HIGH' \
	'-DVIDEOCODEC_VP8' \
	'-DWEBRTC_SRTP' \
	'-DVIDEO_ENABLED' \
	'-DENABLE_GCC' \
	'-DENABLE_SCREEN_SHARE' \
    '-DWEBRTC_VIDEO_ENGINE_DESKTOP_SHARE_API'

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/. \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/.. \
    $(LOCAL_PATH)/../../module \
    $(LOCAL_PATH)/../../base \
    $(LOCAL_PATH)/../common_video/interface \
    $(LOCAL_PATH)/../../common_video/include \
    $(LOCAL_PATH)/../common_video/jpeg/main/interface \
    $(LOCAL_PATH)/../common_video/vplib/main/interface \
    $(LOCAL_PATH)/../../module/interface \
    $(LOCAL_PATH)/../../module/videojpegyuv/include \
    $(LOCAL_PATH)/../../module/audio_coding/main/include \
    $(LOCAL_PATH)/../../module/bitrate_controller/include \
    $(LOCAL_PATH)/../../module/media_file/include \
    $(LOCAL_PATH)/../../module/rtp_rtcp/include \
    $(LOCAL_PATH)/../../module/rtp_rtcp/source \
    $(LOCAL_PATH)/../../module/udp_transport/include \
    $(LOCAL_PATH)/../../module/utility/include \
    $(LOCAL_PATH)/../../module/video_capture/main/include \
    $(LOCAL_PATH)/../../module/video_capture/main/source \
    $(LOCAL_PATH)/../../module/video_capture/main/source/Android \
    $(LOCAL_PATH)/../../module/video_coding/codecs/interface \
    $(LOCAL_PATH)/../../module/video_coding/main/include \
    $(LOCAL_PATH)/../../module/video_mixer/main/interface \
    $(LOCAL_PATH)/../../module/video_processing/main/include \
	$(LOCAL_PATH)/../../module/video_processing/main/source/beauty_filter \
    $(LOCAL_PATH)/../../module/video_render/main/include \
    $(LOCAL_PATH)/../../module/common_video/source \
    $(LOCAL_PATH)/../../module/common_video/interface \
    $(LOCAL_PATH)/../../module/remote_bitrate_estimator/source \
    $(LOCAL_PATH)/../../module/remote_bitrate_estimator/include \
    $(LOCAL_PATH)/../../common_video/source/libyuv/include \
    $(LOCAL_PATH)/../../module/pacing/include \
    $(LOCAL_PATH)/../../module/video_coding/main/source \
    $(LOCAL_PATH)/../../module/desktop_capture/source \
    $(LOCAL_PATH)/../../system_wrappers/include \
	$(LOCAL_PATH)/../../system_wrappers/source \
    $(LOCAL_PATH)/../../voice_engine/main/include \
	$(LOCAL_PATH)/../../voice_engine/main/source \
	$(LOCAL_PATH)/../../servicecore/include \
	$(LOCAL_PATH)/../../third_party/oRTP/include/ortp \
	$(LOCAL_PATH)/../../third_party/oRTP/include \
    $(LOCAL_PATH)/../../third_party/libyuv-android/include \
    $(LOCAL_PATH)/../../third_party/ffmpeg/ffmpeg-android-bin/include \
    
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

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
