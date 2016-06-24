TOOLSET := target
TARGET := voice_engine_core

OUTPUT:= ../$(OBJDIR)/$(TARGET)
INPUT := ./main/source
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
	'-DDYNAMIC_ANNOTATIONS_ENABLED=0' \
	'-DWEBRTC_CODEC_RED'

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

INCS_Release := -I../. \
	-I../module/. \
	-I../module/interface/. \
	-I./main/include \
	-I./main/source \
	-I../module/audio_device/main/source \
	-I../module/resampler/include \
	-I../module/signalprocess/include \
	-I../module/audio_coding/main/include \
	-I../module/audio_conference_mixer/include \
	-I../module/audio_device/main/include \
	-I../module/audioprocess/include \
	-I../module/media_file/include \
	-I../module/rtp_rtcp/include \
	-I../module/udp_transport/include \
	-I../module/utility/include \
	-I../system_wrappers/interface

OBJS := $(OUTPUT)/channel.o \
	$(OUTPUT)/channel_manager.o \
	$(OUTPUT)/channel_manager_base.o \
	$(OUTPUT)/dtmf_inband.o \
	$(OUTPUT)/dtmf_inband_queue.o \
	$(OUTPUT)/level_indicator.o \
	$(OUTPUT)/monitor_module.o \
	$(OUTPUT)/output_mixer.o \
	$(OUTPUT)/shared_data.o \
	$(OUTPUT)/statistics.o \
	$(OUTPUT)/transmit_mixer.o \
	$(OUTPUT)/utility.o \
	$(OUTPUT)/voe_audio_processing_impl.o \
	$(OUTPUT)/voe_base_impl.o \
	$(OUTPUT)/voe_call_report_impl.o \
	$(OUTPUT)/voe_codec_impl.o \
	$(OUTPUT)/voe_dtmf_impl.o \
	$(OUTPUT)/voe_encryption_impl.o \
	$(OUTPUT)/voe_external_media_impl.o \
	$(OUTPUT)/voe_file_impl.o \
	$(OUTPUT)/voe_hardware_impl.o \
	$(OUTPUT)/voe_neteq_stats_impl.o \
	$(OUTPUT)/voe_network_impl.o \
	$(OUTPUT)/voe_rtp_rtcp_impl.o \
	$(OUTPUT)/voe_video_sync_impl.o \
	$(OUTPUT)/voe_volume_control_impl.o \
	$(OUTPUT)/voice_engine_impl.o

$(MYLIB)/libvoice_engine_core.a:$(OBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJS)

$(OUTPUT)/%.o:$(INPUT)/%.cc
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<
