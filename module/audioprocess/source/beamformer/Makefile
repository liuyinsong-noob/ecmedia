ARCH = 
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11

INC_DIR = .
INC_DIR += ..
INC_DIR += ../../..
INC_DIR += ../../../../system_wrappers/interface
INC_DIR += ../../../common_audio/source
INC_DIR += ../../../common_audio/include

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/audioprocess

SRC = covariance_matrix_generator.cc

	
OBJS = $(patsubst %.cc,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


