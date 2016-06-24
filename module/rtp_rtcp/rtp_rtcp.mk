TOOLSET := target
TARGET := rtp_rtcp

OUTPUT= ../../$(OBJDIR)/$(TARGET)
INPUT = ./source
MYLIB = ../../$(OUTLIB)

DEFS_Release := '-DWEBRTC_SVNREVISION="2518"' \
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
#	'-DWEBRTC_ANDROID' \
	'-DWEBRTC_ARCH_ARM' \
	'-DWEBRTC_DETECT_ARM_NEON' \
	'-DWEBRTC_CLOCK_TYPE_REALTIME' \
	'-DWEBRTC_THREAD_RR' \
	'-DWEBRTC_ANDROID_OPENSLES' \
	'-D__STDC_FORMAT_MACROS' \
#	'-DANDROID' \
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
CFLAGS_Release := -fno-exceptions \
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
CFLAGS_C := 

# Flags passed to only C++ files.
CFLAGS_CC_Release := -fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Woverloaded-virtual \
	-Wno-abi

INCS_Release := -I../../.  \
    -I../interface \
    -I../. \
	-I../../system_wrappers/interface \
	-I../bitrate_controller/include \
	-I./include \
	-I./source \
	-I./mock \

OBJ = $(OUTPUT)/bitrate.o 	$(OUTPUT)/rtp_rtcp_impl.o $(OUTPUT)/rtcp_receiver.o $(OUTPUT)/rtcp_receiver_help.o \
	$(OUTPUT)/rtcp_sender.o 	$(OUTPUT)/rtcp_utility.o $(OUTPUT)/rtp_header_extension.o \
	$(OUTPUT)/rtp_receiver.o \
	$(OUTPUT)/rtp_sender.o \
	$(OUTPUT)/rtp_utility.o \
	$(OUTPUT)/ssrc_database.o \
	$(OUTPUT)/tmmbr_help.o \
	$(OUTPUT)/dtmf_queue.o \
	$(OUTPUT)/rtp_receiver_audio.o \
	$(OUTPUT)/rtp_sender_audio.o \
	$(OUTPUT)/forward_error_correction.o \
	$(OUTPUT)/forward_error_correction_internal.o \
	$(OUTPUT)/producer_fec.o \
	$(OUTPUT)/rtp_packet_history.o \
	$(OUTPUT)/rtp_receiver_video.o \
	$(OUTPUT)/rtp_sender_video.o \
	$(OUTPUT)/receiver_fec.o \
	$(OUTPUT)/rtp_format_vp8.o \
	$(OUTPUT)/transmission_bucket.o \
	$(OUTPUT)/vp8_partition_aggregator.o \
	$(OUTPUT)/remote_bitrate_estimator.o \
	$(OUTPUT)/overuse_detector.o \
	$(OUTPUT)/remote_rate_control.o \
	$(OUTPUT)/bitrate_estimator.o

$(MYLIB)/librtp_rtcp.a:$(OBJ)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJ)

$(OUTPUT)/%.o:$(INPUT)/%.cc
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<

