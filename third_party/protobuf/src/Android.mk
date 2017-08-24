LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE    := libProtobuf_lite
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  ./google/protobuf/stubs/atomicops_internals_x86_gcc.cc         \
  ./google/protobuf/stubs/atomicops_internals_x86_msvc.cc        \
  ./google/protobuf/stubs/bytestream.cc                          \
  ./google/protobuf/stubs/bytestream.h                           \
  ./google/protobuf/stubs/common.cc                              \
  ./google/protobuf/stubs/hash.h                                 \
  ./google/protobuf/stubs/int128.cc                              \
  ./google/protobuf/stubs/int128.h                               \
  ./google/protobuf/stubs/map_util.h                             \
  ./google/protobuf/stubs/mathutil.h                             \
  ./google/protobuf/stubs/once.cc                                \
  ./google/protobuf/stubs/shared_ptr.h                           \
  ./google/protobuf/stubs/status.cc                              \
  ./google/protobuf/stubs/status.h                               \
  ./google/protobuf/stubs/status_macros.h                        \
  ./google/protobuf/stubs/statusor.cc                            \
  ./google/protobuf/stubs/statusor.h                             \
  ./google/protobuf/stubs/stringpiece.cc                         \
  ./google/protobuf/stubs/stringpiece.h                          \
  ./google/protobuf/stubs/stringprintf.cc                        \
  ./google/protobuf/stubs/stringprintf.h                         \
  ./google/protobuf/stubs/structurally_valid.cc                  \
  ./google/protobuf/stubs/strutil.cc                             \
  ./google/protobuf/stubs/strutil.h                              \
  ./google/protobuf/stubs/time.cc                                \
  ./google/protobuf/stubs/time.h                                 \
  ./google/protobuf/arena.cc                                     \
  ./google/protobuf/arenastring.cc                               \
  ./google/protobuf/extension_set.cc                             \
  ./google/protobuf/generated_message_util.cc                    \
  ./google/protobuf/message_lite.cc                              \
  ./google/protobuf/repeated_field.cc                            \
  ./google/protobuf/wire_format_lite.cc                          \
  ./google/protobuf/io/coded_stream.cc                           \
  ./google/protobuf/io/coded_stream_inl.h                        \
  ./google/protobuf/io/zero_copy_stream.cc                       \
  ./google/protobuf/io/zero_copy_stream_impl_lite.cc
 
LOCAL_EXPORT_C_INCLUDES :=  
LOCAL_EXPORT_LDLIBS :=

LOCAL_CFLAGS := \
	'-DGOOGLE_PROTOBUF_NO_RTTI' \
        '-DHAVE_PTHREAD=1'  

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
$(LOCAL_PATH)/src

LOCAL_LDLIBS := -llog -lz

include $(BUILD_STATIC_LIBRARY)
