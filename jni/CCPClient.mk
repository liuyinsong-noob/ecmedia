TOOLSET := target
TARGET := ccpclient

OUTPUT= ../$(OBJDIR)/$(TARGET)
INPUT = ./
MYLIB = ../$(OUTLIB)

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

INCS_Release :=  \
        -I../servicecore/include \
        -I../servicecore/interface \
        -I../system_wrappers/interface \
	-I$(ANDROID_INCLUDE)
LIBS_LINK := \
		$(MYLIB)/libservicecore.a \
		$(MYLIB)/libvoice_engine_core.a \
		$(MYLIB)/libamrnb.a \
		$(MYLIB)/libaudio_coding_module.a \
		$(MYLIB)/libaudio_conference_mixer.a \
		$(MYLIB)/libaudio_device.a \
		$(MYLIB)/libaudioprocess.a \
		$(MYLIB)/libbitrate_controller.a \
		$(MYLIB)/libCNG.a \
		$(MYLIB)/libG711.a \
		$(MYLIB)/libjpeg.a \
		$(MYLIB)/libmedia_file.a \
		$(MYLIB)/libNetEq.a \
            	$(MYLIB)/libexosip.a \
		$(MYLIB)/libosip.a \
		$(MYLIB)/libresampler.a \
		$(MYLIB)/librtp_rtcp.a \
		$(MYLIB)/libsignalprocess.a \
		$(MYLIB)/libudp_transport.a \
		$(MYLIB)/libwebrtc_utility.a \
		$(MYLIB)/libyuv.a \
		$(MYLIB)/libsystem_wrappers.a \
		$(STLPORT_LIBDIR)/libstlport_static.a

OBJS :=	$(OUTPUT)/com_hisunsray_phone_SerPhone.o \
	 $(OUTPUT)/servicephone.o 

$(MYLIB)/$(TARGET).so:$(OBJS)
	$(LINK) -Wl,-soname,libserphone.so  \
		-Wl,--no-undefined \
		-Wl,-z,noexecstack \
		-Wl,-z,relro \
		-Wl,-z,now \
		-lOpenSLES -llog -lc -lm -shared -fPIC -L$(STLPORT_LIBDIR) -o $@ $(OBJS) $(LIBS_LINK)

	$(STRIP) --strip-unneeded $@

$(OBJS):$(OUTPUT)/%.o:$(INPUT)/%.cpp
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_C_Release) $(INCS_Release) $<
