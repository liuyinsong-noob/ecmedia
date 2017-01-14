ARCH = 
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11

INC_DIR = .
INC_DIR += ./include
INC_DIR += ../../..
INC_DIR += ../../../common_audio/source/signal_processing/include

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/common_audio/vad

SRC = vad_core.c \
	vad_filterbank.c \
	vad_gmm.c \
	vad_sp.c \
	webrtc_vad.c 


OBJS = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.c
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


