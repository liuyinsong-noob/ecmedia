ARCH = 
CC = $(ARCH)gcc
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11

INC_DIR = .
INC_DIR += ..
INC_DIR += ../../..
INC_DIR += ../../../../system_wrappers/interface

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/audioprocess

SRC = delay_estimator.c \
    delay_estimator_wrapper.c \
    fft4g.c
	
OBJS = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.c
	$(CC) $(CC_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


