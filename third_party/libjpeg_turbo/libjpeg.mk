TOOLSET := target
TARGET := libjpeg

OUTPUT:= ../../$(OBJDIR)/$(TARGET)
INPUT := ./
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
	'-DWITH_SIMD' \
	'-DMOTION_JPEG_SUPPORTED' \
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

INCS_Release := -I./

OBJS := $(OUTPUT)/jcapimin.o \
	$(OUTPUT)/jcapistd.o \
	$(OUTPUT)/jccoefct.o \
	$(OUTPUT)/jccolor.o \
	$(OUTPUT)/jcdctmgr.o \
	$(OUTPUT)/jchuff.o \
	$(OUTPUT)/jcinit.o \
	$(OUTPUT)/jcmainct.o \
	$(OUTPUT)/jcmarker.o \
	$(OUTPUT)/jcmaster.o \
	$(OUTPUT)/jcomapi.o \
	$(OUTPUT)/jcparam.o \
	$(OUTPUT)/jcphuff.o \
	$(OUTPUT)/jcprepct.o \
	$(OUTPUT)/jcsample.o \
	$(OUTPUT)/jdapimin.o \
	$(OUTPUT)/jdapistd.o \
	$(OUTPUT)/jdatadst.o \
	$(OUTPUT)/jdatasrc.o \
	$(OUTPUT)/jdcoefct.o \
	$(OUTPUT)/jdcolor.o \
	$(OUTPUT)/jddctmgr.o \
	$(OUTPUT)/jdhuff.o \
	$(OUTPUT)/jdinput.o \
	$(OUTPUT)/jdmainct.o \
	$(OUTPUT)/jdmarker.o \
	$(OUTPUT)/jdmaster.o \
	$(OUTPUT)/jdmerge.o \
	$(OUTPUT)/jdphuff.o \
	$(OUTPUT)/jdpostct.o \
	$(OUTPUT)/jdsample.o \
	$(OUTPUT)/jerror.o \
	$(OUTPUT)/jfdctflt.o \
	$(OUTPUT)/jfdctfst.o \
	$(OUTPUT)/jfdctint.o \
	$(OUTPUT)/jidctflt.o \
	$(OUTPUT)/jidctfst.o \
	$(OUTPUT)/jidctint.o \
	$(OUTPUT)/jidctred.o \
	$(OUTPUT)/jmemmgr.o \
	$(OUTPUT)/jmemnobs.o \
	$(OUTPUT)/jquant1.o \
	$(OUTPUT)/jquant2.o \
	$(OUTPUT)/jutils.o \
	$(OUTPUT)/jsimd_none.o

$(MYLIB)/libjpeg.a:$(OBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJS)

$(OUTPUT)/%.o:$(INPUT)/%.c
	mkdir -p $(OUTPUT)
	$(CC) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_C_Release) $(INCS_Release) $<
