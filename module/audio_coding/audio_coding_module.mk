TOOLSET := target
TARGET := audio_coding_module

OUTPUT:= ../../$(OBJDIR)/$(TARGET)
INPUT := ./main/source
MYLIB = ../../$(OUTLIB)

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

INCS_Release :=  -I../../.  \
    -I../interface \
    -I../ \
	-I../../system_wrappers/interface \
	-I./main/include \
	-I./main/source \
	-I./codecs/cng/include \
	-I./codecs/g711/include \
	-I./codecs/opencore-amr/amrnb \
	-I./neteq/include \
	-I../resampler/include \
	-I../signalprocess/include

OBJS :=  $(OUTPUT)/acm_amr.o \
	$(OUTPUT)/acm_amrwb.o \
	$(OUTPUT)/acm_celt.o \
	$(OUTPUT)/acm_cng.o \
	$(OUTPUT)/acm_codec_database.o \
	$(OUTPUT)/acm_dtmf_detection.o \
	$(OUTPUT)/acm_dtmf_playout.o \
	$(OUTPUT)/acm_g722.o \
	$(OUTPUT)/acm_g7221.o \
	$(OUTPUT)/acm_g7221c.o \
	$(OUTPUT)/acm_g729.o \
	$(OUTPUT)/acm_g7291.o \
	$(OUTPUT)/acm_generic_codec.o \
	$(OUTPUT)/acm_gsmfr.o \
	$(OUTPUT)/acm_ilbc.o \
	$(OUTPUT)/acm_isac.o \
	$(OUTPUT)/acm_neteq.o \
	$(OUTPUT)/acm_opus.o \
	$(OUTPUT)/acm_speex.o \
	$(OUTPUT)/acm_pcm16b.o \
	$(OUTPUT)/acm_pcma.o \
	$(OUTPUT)/acm_pcmu.o \
	$(OUTPUT)/acm_red.o \
	$(OUTPUT)/acm_resampler.o \
	$(OUTPUT)/audio_coding_module.o \
	$(OUTPUT)/audio_coding_module_impl.o

$(MYLIB)/libaudio_coding_module.a:$(OBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJS)

$(OUTPUT)/%.o:$(INPUT)/%.cc
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<
