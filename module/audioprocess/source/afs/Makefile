ARCH = 
CC = $(ARCH)gcc
CC_OPTS=-c -fPIC


INC_DIR = .
INC_DIR += ../utility
INC_DIR += ../../..
INC_DIR += ../../../interface
INC_DIR += ../../../../system_wrappers/interface
INC_DIR += ../../../common_audio/source
INC_DIR += ../../../common_audio/source/signal_processing/include

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/audioprocess

SRC = afs_core.c \
	howling_control.c \
	howlingfilter_core.c 

	
OBJS = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.c
	$(CC) $(CC_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


