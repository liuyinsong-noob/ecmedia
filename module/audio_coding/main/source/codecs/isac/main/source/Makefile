ARCH = 
CC = $(ARCH)gcc
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11

INC_DIR = .
INC_DIR += ../interface 
INC_DIR += ../.. 
INC_DIR += ../../../.. 
INC_DIR += ../../../../../../codecs 
INC_DIR += ../../../../../../codecs/cng/include 
INC_DIR += ../../../../../../.. 
INC_DIR += ../../../../../../../common_audio/source/signal_processing/include 
INC_DIR += ../../../../../../../../system_wrappers/interface 
INC_DIR += ../../../../../../../../system_wrappers/source

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../../../../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/audio_coding/main/source/codes/isac/main/source

SRC_C = arith_routines.c \
    arith_routines_hist.c \
    arith_routines_logist.c \
    bandwidth_estimator.c \
    crc.c \
    decode.c \
    decode_bwe.c \
    encode.c \
    encode_lpc_swb.c \
    entropy_coding.c \
    fft.c \
    filterbanks.c \
    filterbank_tables.c \
    filter_functions.c \
    intialize.c \
    isac.c \
    lattice.c \
    lpc_analysis.c \
    lpc_gain_swb_tables.c \
    lpc_shape_swb12_tables.c \
    lpc_shape_swb16_tables.c \
    lpc_tables.c \
    pitch_estimator.c \
    pitch_filter.c \
    pitch_gain_tables.c \
    pitch_lag_tables.c \
    spectrum_ar_model_tables.c \
    transform.c

SRC_CC = audio_encoder_isac.cc 
	
OBJS_C = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC_C))
OBJS_CC = $(patsubst %.cc,$(DEBUG_DIR)/%.o,$(SRC_CC))

all:$(OBJS_C) $(OBJS_CC)
$(DEBUG_DIR)/%.o : %.c
	$(CC) $(CC_OPTS) $(INCLUDE) -o $@ $<

$(DEBUG_DIR)/%.o : %.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS_C) $(OBJS_CC)



