# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../android-webrtc.mk

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_rtp_rtcp
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_GENERATED_SOURCES :=
LOCAL_SRC_FILES := \
        bitrate.cc \
        dtmf_queue.cc \
        flexfec_header_reader_writer.cc \
        flexfec_receiver.cc \
        flexfec_sender.cc \
        forward_error_correction.cc \
        forward_error_correction_internal.cc \
        packet_loss_stats.cc \
        playout_delay_oracle.cc \
        receive_statistics_impl.cc \
        remote_ntp_time_estimator.cc \
        rtcp_nack_stats.cc \
        rtcp_packet.cc \
        rtcp_receiver.cc \
        rtcp_receiver_help.cc \
        rtcp_sender.cc \
        rtcp_utility.cc \
        rtp_format.cc \
        rtp_format_h264.cc \
        rtp_format_video_generic.cc \
        rtp_format_vp8.cc \
        rtp_format_vp9.cc \
        rtp_header_extension.cc \
        rtp_header_extensions.cc \
        rtp_header_parser.cc \
        rtp_packet.cc \
        rtp_packet_history.cc \
        rtp_payload_registry.cc \
        rtp_receiver_audio.cc \
        rtp_receiver_impl.cc \
        rtp_receiver_strategy.cc \
        rtp_receiver_video.cc \
        rtp_rtcp_impl.cc \
        rtp_sender.cc \
        rtp_sender_audio.cc \
        rtp_sender_video.cc \
        rtp_utility.cc \
        ssrc_database.cc \
        time_util.cc \
        tmmbr_help.cc \
        ulpfec_generator.cc \
        ulpfec_header_reader_writer.cc \
        ulpfec_receiver_impl.cc \
        vp8_partition_aggregator.cc \
        rtcp_packet/app.cc \
		rtcp_packet/bye.cc \
		rtcp_packet/common_header.cc \
		rtcp_packet/compound_packet.cc \
		rtcp_packet/dlrr.cc \
		rtcp_packet/extended_jitter_report.cc \
		rtcp_packet/extended_reports.cc \
		rtcp_packet/fir.cc \
		rtcp_packet/nack.cc \
		rtcp_packet/pli.cc \
		rtcp_packet/psfb.cc \
		rtcp_packet/rapid_resync_request.cc \
		rtcp_packet/receiver_report.cc \
		rtcp_packet/remb.cc \
		rtcp_packet/report_block.cc \
		rtcp_packet/rpsi.cc \
		rtcp_packet/rrtr.cc \
		rtcp_packet/rtpfb.cc \
		rtcp_packet/sdes.cc \
		rtcp_packet/sender_report.cc \
		rtcp_packet/sli.cc \
		rtcp_packet/target_bitrate.cc \
		rtcp_packet/tmmb_item.cc \
		rtcp_packet/tmmbn.cc \
		rtcp_packet/tmmbr.cc \
		rtcp_packet/transport_feedback.cc \
		rtcp_packet/voip_metric.cc


# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../.. \
    $(LOCAL_PATH)/../../interface \
    $(LOCAL_PATH)/../../../ \
    $(LOCAL_PATH)/../../../system_wrappers/interface \
    $(LOCAL_PATH)/../../../system_wrappers/source \
    $(LOCAL_PATH)/../../remote_bitrate_estimator/include \
    $(LOCAL_PATH)/../../pacing/include

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
