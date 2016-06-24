TOOLSET := target
TARGET := osip

OUTPUT:= ../../$(OBJDIR)/$(TARGET)
INPUT := ./src
MYLIB = ../../$(OUTLIB)

DEFS_Release := '-D_FILE_OFFSET_BITS=64' \
	'-DOSIP_MT' \
	'-DHAVE_PTHREAD' \
	'-DHAVE_SEMAPHORE_H'  \
	'-DHAVE_FCNTL_H' \
	'-DHAVE_SYS_TIME_H'  \
	'-DHAVE_STRUCT_TIMEVAL' \
	'-DHAVE_SYS_SELECT_H'  \
	'-D__linux'

# Flags passed to all source files.
CFLAGS_Release :=-fno-exceptions \
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

INCS_Release := -I./include \
     -I./src

OSIP2OBJS := $(OUTPUT)/ict_fsm.o \
          $(OUTPUT)/ist_fsm.o    \
          $(OUTPUT)/nict_fsm.o  \
          $(OUTPUT)/nist_fsm.o   \
          $(OUTPUT)/ict.o            \
          $(OUTPUT)/ist.o            \
          $(OUTPUT)/nict.o          \
          $(OUTPUT)/nist.o          \
          $(OUTPUT)/fsm_misc.o  \
          $(OUTPUT)/osip.o          \
          $(OUTPUT)/osip_transaction.o    \
          $(OUTPUT)/osip_event.o  \
          $(OUTPUT)/port_fifo.o    \
          $(OUTPUT)/osip_dialog.o  \
          $(OUTPUT)/osip_time.o \
          $(OUTPUT)/port_sema.o \
          $(OUTPUT)/port_thread.o \
          $(OUTPUT)/port_condv.o

OSIP2PARSEROBJS := $(OUTPUT)/osip_proxy_authorization.o \
           $(OUTPUT)/osip_cseq.o             \
           $(OUTPUT)/osip_record_route.o        \
           $(OUTPUT)/osip_route.o            \
           $(OUTPUT)/osip_to.o                  \
           $(OUTPUT)/osip_from.o              \
           $(OUTPUT)/osip_uri.o                 \
           $(OUTPUT)/osip_authorization.o  \
           $(OUTPUT)/osip_header.o              \
           $(OUTPUT)/osip_www_authenticate.o   \
           $(OUTPUT)/osip_via.o                 \
           $(OUTPUT)/osip_body.o              \
           $(OUTPUT)/osip_md5c.o                \
           $(OUTPUT)/osip_message.o           \
           $(OUTPUT)/osip_list.o                \
           $(OUTPUT)/osip_call_id.o           \
           $(OUTPUT)/osip_message_parse.o       \
           $(OUTPUT)/osip_contact.o           \
           $(OUTPUT)/osip_message_to_str.o      \
           $(OUTPUT)/osip_content_length.o     \
           $(OUTPUT)/osip_parser_cfg.o          \
           $(OUTPUT)/osip_content_type.o      \
           $(OUTPUT)/osip_proxy_authenticate.o  \
           $(OUTPUT)/osip_mime_version.o       \
           $(OUTPUT)/osip_port.o                     \
           $(OUTPUT)/osip_accept_encoding.o   \
           $(OUTPUT)/osip_content_encoding.o \
           $(OUTPUT)/osip_authentication_info.o   \
           $(OUTPUT)/osip_proxy_authentication_info.o \
           $(OUTPUT)/osip_accept_language.o     \
           $(OUTPUT)/osip_accept.o                    \
           $(OUTPUT)/osip_alert_info.o         \
           $(OUTPUT)/osip_error_info.o                \
           $(OUTPUT)/osip_allow.o              \
           $(OUTPUT)/osip_content_disposition.o       \
           $(OUTPUT)/sdp_accessor.o            \
           $(OUTPUT)/sdp_message.o                    \
           $(OUTPUT)/osip_call_info.o

$(MYLIB)/libosip.a:$(OSIP2OBJS) $(OSIP2PARSEROBJS)
	mkdir -p $(MYLIB)
	 $(AR) $(ARFLAGS) $@ $(OSIP2OBJS) $(OSIP2PARSEROBJS)

$(OUTPUT)/%.o:$(INPUT)/osip2/%.c
	mkdir -p $(OUTPUT)
	$(CC) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_C_Release) $(INCS_Release) $<

$(OUTPUT)/%.o:$(INPUT)/osipparser2/%.c
	mkdir -p $(OUTPUT)
	$(CC) -c -o $@ $(CFLAGS_Release) $(DEFS_Release)  $(CFLAGS_C_Release) $(INCS_Release) $<
