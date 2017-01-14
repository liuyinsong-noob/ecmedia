ARCH = 
CC = $(ARCH)gcc
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11 

INC_DIR = .
INC_DIR += ./include
INC_DIR += ../utility
INC_DIR += ../../..
//INC_DIR += ../../../interface
//INC_DIR += ../../../../system_wrappers/interface
//INC_DIR += ../../../common_audio/source
INC_DIR += ../../../common_audio/source/signal_processing/include

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/audioprocess

SRC = noise_suppression.c \
    ns_core.c

	
OBJS = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.c
	$(CC) $(CC_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


