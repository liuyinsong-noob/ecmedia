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
LOCAL_MODULE := libwebrtc_exosip
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
		source/eXcall_api.c \
		source/eXconf.c \
		source/eXinsubscription_api.c \
		source/eXmessage_api.c \
		source/eXoptions_api.c \
		source/eXosip.c \
		source/eXosip_transport_hook.c \
		source/eXpublish_api.c \
		source/eXrefer_api.c \
		source/eXregister_api.c \
		source/eXsubscription_api.c \
		source/eXtl.c \
		source/eXtl_dtls.c \
		source/eXtl_tcp.c \
		source/eXtl_tls.c \
		source/eXtl_udp.c \
		source/eXtransport.c \
		source/eXutils.c \
		source/inet_ntop.c \
		source/jauth.c \
		source/jcall.c \
		source/jcallback.c \
		source/jdialog.c \
		source/jevents.c \
		source/jnotify.c \
		source/jpipe.c \
		source/jpublish.c \
		source/jreg.c \
		source/jrequest.c \
		source/jresponse.c \
		source/jsubscribe.c \
		source/milenage.c \
		source/misc.c \
		source/rijndael.c \
		source/sdp_offans.c \
		source/udp.c 

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
		'-DOSIP_MT' \
		'-DHAVE_TIME_H' \
		'-DHAVE_SYS_SELECT_H' \
		'-DENABLE_TRACE' \
		$(MY_WEBRTC_COMMON_DEFS)
	
LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/source	\
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/../osip/include \
    $(LOCAL_PATH)/../../third_party/oRTP/include/ortp \
    $(LOCAL_PATH)/../../third_party/oRTP/include
    
    
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
