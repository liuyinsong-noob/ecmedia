ARCH = 
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC -fpermissive
OPTI_OPTS= -std=c++11 -DWEBRTC_POSIX

INC_DIR = ../include
INC_DIR += ../..
INC_DIR += ../../../system_wrappers/interface
INC_DIR += ../../../system_wrappers/source
INC_DIR += ../../audioprocess/source

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/common_audio/source

SRC = audio_util.cc \
	blocker.cc \
	fir_filter.cc \
	wav_file.cc \
	wav_header.cc 
	
SRC_C = ring_buffer.c

OBJS = $(patsubst %.cc,$(DEBUG_DIR)/%.o,$(SRC))
OBJS_C = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC_C))

all:$(OBJS) $(OBJS_C)
$(DEBUG_DIR)/%.o : %.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
$(DEBUG_DIR)/%.o : %.c
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS) $(OBJS_C)


