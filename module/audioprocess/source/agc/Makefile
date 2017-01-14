ARCH = 
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11 -DWEBRTC_POSIX

INC_DIR = .
INC_DIR = ..
INC_DIR += ../utility
INC_DIR += ../../include
INC_DIR += ../../..
INC_DIR += ../../../interface
INC_DIR += ../../../utility/include
INC_DIR += ../../../../system_wrappers/interface
INC_DIR += ../../../../system_wrappers/source
INC_DIR += ../../../common_audio/source
INC_DIR += ../../../common_audio/source/signal_processing/include
INC_DIR += ../../../common_audio/source/resampler/include
INC_DIR += ../../../common_audio/source/vad/include
INC_DIR += ../../../audio_coding/main/source/codecs/isac/main/source
INC_DIR += ../../../audio_coding/main/source/codecs/isac/main/interface

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/audioprocess

SRC = agc.cc \
    agc_audio_proc.cc \
    agc_manager_direct.cc \
    circular_buffer.cc \
    gmm.cc \
    histogram.cc \
    pitch_based_vad.cc \
    pitch_internal.cc \
    pole_zero_filter.cc \
    standalone_vad.cc \
    utility.cc 

	
OBJS = $(patsubst %.cc,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


