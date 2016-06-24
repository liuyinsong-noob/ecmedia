# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_osip
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		src/osip2/fsm_misc.c \
		src/osip2/ict.c \
		src/osip2/ict_fsm.c \
		src/osip2/ist.c \
		src/osip2/ist_fsm.c \
		src/osip2/nict.c \
		src/osip2/nict_fsm.c \
		src/osip2/nist.c \
		src/osip2/nist_fsm.c \
		src/osip2/osip.c \
		src/osip2/osip_dialog.c \
		src/osip2/osip_event.c \
		src/osip2/osip_time.c \
		src/osip2/osip_transaction.c \
		src/osip2/port_condv.c \
		src/osip2/port_fifo.c \
		src/osip2/port_sema.c \
		src/osip2/port_thread.c \
		src/osipparser2/osip_accept.c \
		src/osipparser2/osip_accept_encoding.c \
		src/osipparser2/osip_accept_language.c \
		src/osipparser2/osip_alert_info.c \
		src/osipparser2/osip_allow.c \
		src/osipparser2/osip_authentication_info.c \
		src/osipparser2/osip_authorization.c \
		src/osipparser2/osip_body.c \
		src/osipparser2/osip_call_id.c \
		src/osipparser2/osip_call_info.c \
		src/osipparser2/osip_contact.c \
		src/osipparser2/osip_content_disposition.c \
		src/osipparser2/osip_content_encoding.c \
		src/osipparser2/osip_content_length.c \
		src/osipparser2/osip_content_type.c \
		src/osipparser2/osip_cseq.c \
		src/osipparser2/osip_error_info.c \
		src/osipparser2/osip_from.c \
		src/osipparser2/osip_header.c \
		src/osipparser2/osip_list.c \
		src/osipparser2/osip_md5c.c \
		src/osipparser2/osip_message.c \
		src/osipparser2/osip_message_parse.c \
		src/osipparser2/osip_message_to_str.c \
		src/osipparser2/osip_mime_version.c \
		src/osipparser2/osip_parser_cfg.c \
		src/osipparser2/osip_port.c \
		src/osipparser2/osip_proxy_authenticate.c \
		src/osipparser2/osip_proxy_authentication_info.c \
		src/osipparser2/osip_proxy_authorization.c \
		src/osipparser2/osip_record_route.c \
		src/osipparser2/osip_route.c \
		src/osipparser2/osip_to.c \
		src/osipparser2/osip_uri.c \
		src/osipparser2/osip_via.c \
		src/osipparser2/osip_www_authenticate.c \
		src/osipparser2/sdp_accessor.c \
		src/osipparser2/sdp_message.c


# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
	  '-D_FILE_OFFSET_BITS=64' \
		'-DOSIP_MT' \
		'-DHAVE_PTHREAD' \
		'-DHAVE_SEMAPHORE_H'  \
		'-DHAVE_FCNTL_H' \
		'-DHAVE_SYS_TIME_H'  \
		'-DHAVE_STRUCT_TIMEVAL' \
		'-DHAVE_SYS_SELECT_H'  \
		'-D__linux' \
		'-DENABLE_TRACE' \
		$(MY_WEBRTC_COMMON_DEFS)
	
LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)	\
    $(LOCAL_PATH)/include 

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
