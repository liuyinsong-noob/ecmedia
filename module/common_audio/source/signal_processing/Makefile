ARCH = 
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC -fpermissive
OPTI_OPTS= -std=c++11 -DWEBRTC_POSIX

INC_DIR = ./include
INC_DIR += ../../..
INC_DIR += ../../../../system_wrappers/interface

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/common_audio/signal_processing

SRC = auto_correlation.c \
	complex_bit_reverse.c \
	complex_fft.c \
	copy_set_operations.c \
	cross_correlation.c \
	division_operations.c \
	dot_product_with_scale.c \
	downsample_fast.c \
	energy.c \
	filter_ar.c \
	filter_ar_fast_q12.c \
	filter_ma_fast_q12.c \
	get_hanning_window.c \
	get_scaling_square.c \
	ilbc_specific_functions.c \
	levinson_durbin.c \
	min_max_operations.c \
	randomization_functions.c \
	real_fft.c \
	resample.c \
	resample_48khz.c \
	resample_by_2.c \
	resample_by_2_internal.c \
	resample_fractional.c \
	spl_init.c \
	spl_sqrt.c \
	spl_sqrt_floor.c \
	splitting_filter.c \
	vector_scaling_operations.c 
	
OBJS = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.c
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


