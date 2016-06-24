TOOLSET := target
TARGET := servicecore

OUTPUT:= ../$(OBJDIR)/$(TARGET)
INPUT := ./source
MYLIB = ../$(OUTLIB)

DEFS_Release := '-DOSIP_MT' \
	'-DNDEBUG' 

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

INCS_Release := -I../.  \
    -I../module/interface \
    -I../module/. \
    -I../module/exosip/include \
    -I../module/exosip/source \
    -I../module/osip/include \
    -I../voice_engine/main/include \
	-I../system_wrappers/interface \
	-I./include \
	-I./interface \
	-I./source \
	-I../module/video_coding/codecs/interface

OBJS := $(OUTPUT)/address.o \
	$(OUTPUT)/callbacks.o \
	$(OUTPUT)/chat.o \
	$(OUTPUT)/enum.o \
	$(OUTPUT)/friends.o \
	$(OUTPUT)/lpconfig.o \
	$(OUTPUT)/mediaprocess.o \
	$(OUTPUT)/offeranswer.o \
	$(OUTPUT)/prensence.o \
	$(OUTPUT)/proxy.o \
	$(OUTPUT)/sal_eXosip2.o \
	$(OUTPUT)/sal_eXosip2_presence.o \
	$(OUTPUT)/sal_eXosip2_sdp.o \
	$(OUTPUT)/salpr.o \
	$(OUTPUT)/serphonecall.o \
	$(OUTPUT)/serphoneinterface.o \
	$(OUTPUT)/servicecore.o \
	$(OUTPUT)/siplogin.o \
	$(OUTPUT)/sipsetup.o \
	$(OUTPUT)/sometools.o \
	$(OUTPUT)/CaptureScreen.o

$(MYLIB)/libservicecore.a:$(OBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OBJS)

$(OUTPUT)/%.o:$(INPUT)/%.cpp
	mkdir -p $(OUTPUT)
	$(CXX) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_CC_Release) $(INCS_Release) $<
