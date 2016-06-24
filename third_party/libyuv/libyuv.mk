TOOLSET := target
TARGET := libyuv

OUTPUT:= ../../$(OBJDIR)/$(TARGET)
INPUT := ./source
MYLIB = ../../$(OUTLIB)

DEFS_Release := '-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_GPU=1' \
	'-DUSE_OPENSSL=1' \
	'-DENABLE_EGLIMAGE=1' \
	'-DUSE_SKIA=1' \
	'-DHAVE_JPEG' \
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
CFLAGS_Release := -fno-exceptions \
	-fno-strict-aliasing \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wno-format \
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
	-Wno-deprecated \
	-Wno-abi

INCS_Release := -I./include \
	-I./. \
	-I..//libjpeg_turbo

OBJS := $(OUTPUT)/compare.o \
	$(OUTPUT)/convert.o \
	$(OUTPUT)/convert_from.o \
	$(OUTPUT)/cpu_id.o \
	$(OUTPUT)/format_conversion.o \
	$(OUTPUT)/mjpeg_decoder.o \
	$(OUTPUT)/planar_functions.o \
	$(OUTPUT)/rotate.o \
	$(OUTPUT)/rotate_neon.o \
	$(OUTPUT)/row_common.o \
	$(OUTPUT)/row_neon.o \
	$(OUTPUT)/row_posix.o \
	$(OUTPUT)/row_win.o \
	$(OUTPUT)/scale.o \
	$(OUTPUT)/video_common.o

$(MYLIB)/libyuv.a:$(OBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJS)

$(OUTPUT)/%.o:$(INPUT)/%.cc
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<
