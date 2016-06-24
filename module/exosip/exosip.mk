TOOLSET := target
TARGET := exosip

OUTPUT:= ../../$(OBJDIR)/$(TARGET)
INPUT := ./source
MYLIB = ../../$(OUTLIB)

DEFS_Release := '-DOSIP_MT'   '-DHAVE_TIME_H'    '-DHAVE_SYS_SELECT_H'

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
CFLAGS_C_Release := 

# Flags passed to only C++ files.
CFLAGS_CC_Release := -fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Woverloaded-virtual \
	-Wno-abi

INCS_Release := -I./include  \
	-I./source \
    -I../osip/include
 
OBJS :=$(OUTPUT)/eXosip.o \
	$(OUTPUT)/eXconf.o \
	$(OUTPUT)/eXregister_api.o    \
	$(OUTPUT)/eXcall_api.o     \
	$(OUTPUT)/eXmessage_api.o   \
	$(OUTPUT)/eXtransport.o    \
	$(OUTPUT)/jrequest.o        \
	$(OUTPUT)/jresponse.o      \
	$(OUTPUT)/jcallback.o     \
	$(OUTPUT)/jdialog.o        \
	$(OUTPUT)/udp.o          \
	$(OUTPUT)/jcall.o          \
	$(OUTPUT)/jreg.o           \
	$(OUTPUT)/eXutils.o        \
	$(OUTPUT)/jevents.o        \
	$(OUTPUT)/misc.o           \
	$(OUTPUT)/jauth.o          \
	$(OUTPUT)/eXosip_transport_hook.o  \
	$(OUTPUT)/eXtl.o \
	$(OUTPUT)/eXtl_udp.o \
	$(OUTPUT)/eXtl_tcp.o \
	$(OUTPUT)/eXtl_dtls.o \
	$(OUTPUT)/eXtl_tls.o  \
	$(OUTPUT)/milenage.o \
	$(OUTPUT)/rijndael.o \
	$(OUTPUT)/eXsubscription_api.o   \
	$(OUTPUT)/eXoptions_api.o    \
	$(OUTPUT)/eXinsubscription_api.o   \
	$(OUTPUT)/eXpublish_api.o    \
	$(OUTPUT)/jnotify.o              \
	$(OUTPUT)/jsubscribe.o       \
	$(OUTPUT)/inet_ntop.o         \
	$(OUTPUT)/jpipe.o             \
	$(OUTPUT)/eXrefer_api.o       \
	$(OUTPUT)/jpublish.o         \
	$(OUTPUT)/sdp_offans.o

$(MYLIB)/libexosip.a:$(OBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJS)

$(OUTPUT)/%.o:$(INPUT)/%.c
	mkdir -p $(OUTPUT)
	$(CC) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_C_Release) $(INCS_Release) $<
