# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

################## Build ProtoBuf ################
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_CPPFLAGS += -frtti
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE    := libProtobuf_lite
LOCAL_MODULE_TAGS := optional
#LOCAL_CPP_EXTENSION :=.cc.cpp
LOCAL_SRC_FILES := \
			 google/protobuf/generated_message_util.cc         \
			 google/protobuf/message_lite.cc                   \
			 google/protobuf/repeated_field.cc                 \
			 google/protobuf/wire_format_lite.cc               \
			 google/protobuf/extension_set.cc   \
             google/protobuf/stubs/atomicops_internals_x86_msvc.cc \
			 google/protobuf/stubs/common.cc                   \
			 google/protobuf/stubs/stringprintf.cc             \
			 google/protobuf/stubs/once.cc                     \
			 google/protobuf/io/zero_copy_stream.cc            \
			 google/protobuf/io/zero_copy_stream_impl_lite.cc   \
             google/protobuf/io/coded_stream.cc                         

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)	\
		$(LOCAL_PATH)/google/protobuf/stubs \
		$(LOCAL_PATH)/google/
			
LOCAL_LDLIBS := -llog -pthread \


ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)
