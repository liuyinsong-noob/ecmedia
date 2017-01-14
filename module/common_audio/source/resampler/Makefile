ARCH = 
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11

INC_DIR = .
INC_DIR += ./include
INC_DIR += ../../..
INC_DIR += ../../../common_audio/include
INC_DIR += ../../../common_audio/source/signal_processing/include
INC_DIR += ../../../../system_wrappers/interface
INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/common_audio/resampler

SRC = push_resampler.cc \
	push_sinc_resampler.cc \
	resampler.cc \
	sinc_resampler.cc

OBJS = $(patsubst %.cc,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


