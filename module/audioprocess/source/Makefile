ARCH = 
CPP = $(ARCH)g++
CC_OPTS=-c -fPIC
OPTI_OPTS= -std=c++11 -DWEBRTC_POSIX -DWEBRTC_NS_FLOAT

INC_DIR = .
INC_DIR += ./aec
INC_DIR += ./aec/include
INC_DIR += ./aecm/include
INC_DIR += ./agc
INC_DIR += ./agc/legacy
INC_DIR += ./afs
INC_DIR += ./ns
INC_DIR += ./ns/include
INC_DIR += ./beamformer
INC_DIR += ./transient
INC_DIR += ../include
INC_DIR += ../..
INC_DIR += ../../interface
INC_DIR += ../../common_audio/include
INC_DIR += ../../common_audio/source
INC_DIR += ../../common_audio/source/resampler
INC_DIR += ../../common_audio/source/vad/include
INC_DIR += ../../common_audio/source/signal_processing/include
INC_DIR += ../../../system_wrappers/interface
INC_DIR += ../../../system_wrappers/source

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/audioprocess

SRC = audio_buffer.cc \
    audio_processing_impl.cc \
	channel_buffer.cc \
    echo_cancellation_impl.cc \
    echo_control_mobile_impl.cc \
    gain_control_impl.cc \
    high_pass_filter_impl.cc \
	howling_control_impl.cc \
    level_estimator_impl.cc \
    noise_suppression_impl.cc \
	processing_component.cc \
    rms_level.cc \
    splitting_filter.cc \
	typing_detection.cc \
	voice_detection_impl.cc 
	

OBJS = $(patsubst %.cc,$(DEBUG_DIR)/audioprocess_%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/audioprocess_%.o : %.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


