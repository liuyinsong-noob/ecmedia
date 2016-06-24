TOOLSET := target
TARGET := video_engine_core

OUTPUT:= ../$(OBJDIR)/$(TARGET)
INPUT := ./source
MYLIB = ../$(OUTLIB)

DEFS_Release := '-DWEBRTC_SVNREVISION="n/a"' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_GPU=1' \
	'-DUSE_OPENSSL=1' \
	'-DENABLE_EGLIMAGE=1' \
	'-DUSE_SKIA=1' \
	'-DWEBRTC_LINUX' \
	'-DWEBRTC_ANDROID' \
	'-DWEBRTC_ARCH_ARM' \
	'-DWEBRTC_DETECT_ARM_NEON' \
	'-DWEBRTC_CLOCK_TYPE_REALTIME' \
	'-DWEBRTC_THREAD_RR' \
	'-DWEBRTC_ANDROID_OPENSLES' \
	'-D__STDC_FORMAT_MACROS' \
	'-DANDROID' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DHAVE_OFF64_T' \
	'-DHAVE_SYS_UIO_H' \
	'-DANDROID_BINSIZE_HACK' \
	'-DANDROID_UPSTREAM_BRINGUP=1' \
	'-DNDEBUG' \
	'-DNVALGRIND' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=0'

# Flags passed to all source files.
CFLAGS_Release := -Werror \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-mthumb \
	-march=armv7-a \
	-mtune=cortex-a8 \
	-mfloat-abi=softfp \
	-mfpu=vfpv3-d16 \
	-fno-tree-sra \
	-Wno-psabi \
	-mthumb-interwork \
	-U__linux__ \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-Wno-error=non-virtual-dtor \
	-I$(ANDROID_INCLUDE) \
	-I$(STLPORT_DIR) \
	-Os \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer

# Flags passed to only C files.
CFLAGS_C_Release := 

# Flags passed to only C++ files.
CFLAGS_CC_Release := -fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Woverloaded-virtual \
	-Wno-abi

INCS_Release :=  -I../. \
	-I../module/. \
	-I../module/interface/. \
	-I./include \
	-I../module/video_capture/main/include \
	-I../module/video_render/main/include \
	-I../module/videojpegyuv/include \
	-I../module/media_file/include \
	-I../module/rtp_rtcp/include \
	-I../module/rtp_rtcp/source \
	-I../module/udp_transport/include \
	-I../module/bitrate_controller/include \
	-I../module/utility/include \
	-I../system_wrappers/interface \
	-I../module/audio_coding/main/include \
	-I../module/video_coding/main/include \
	-I../module/video_coding/codecs/interface \
	-I../module/video_processing/main/include \
	-I../voice_engine/main/include

OBJS := $(OUTPUT)/vie_base_impl.o \
	$(OUTPUT)/vie_capture_impl.o \
	$(OUTPUT)/vie_codec_impl.o \
	$(OUTPUT)/vie_encryption_impl.o \
	$(OUTPUT)/vie_external_codec_impl.o \
	$(OUTPUT)/vie_file_impl.o \
	$(OUTPUT)/vie_image_process_impl.o \
	$(OUTPUT)/vie_impl.o \
	$(OUTPUT)/vie_network_impl.o \
	$(OUTPUT)/vie_ref_count.o \
	$(OUTPUT)/vie_render_impl.o \
	$(OUTPUT)/vie_rtp_rtcp_impl.o \
	$(OUTPUT)/vie_shared_data.o \
	$(OUTPUT)/vie_capturer.o \
	$(OUTPUT)/vie_channel.o \
	$(OUTPUT)/vie_channel_group.o \
	$(OUTPUT)/vie_channel_manager.o \
	$(OUTPUT)/vie_encoder.o \
	$(OUTPUT)/vie_file_image.o \
	$(OUTPUT)/vie_file_player.o \
	$(OUTPUT)/vie_file_recorder.o \
	$(OUTPUT)/vie_frame_provider_base.o \
	$(OUTPUT)/vie_input_manager.o \
	$(OUTPUT)/vie_manager_base.o \
	$(OUTPUT)/vie_performance_monitor.o \
	$(OUTPUT)/vie_receiver.o \
	$(OUTPUT)/vie_remb.o \
	$(OUTPUT)/vie_renderer.o \
	$(OUTPUT)/vie_render_manager.o \
	$(OUTPUT)/vie_sender.o \
	$(OUTPUT)/vie_sync_module.o

$(MYLIB)/libvideo_engine_core.a:$(OBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJS)

$(OUTPUT)/%.o:$(INPUT)/%.cc
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<
