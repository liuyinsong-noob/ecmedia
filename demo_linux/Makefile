ARCH = 
CPP = $(ARCH)g++
LD = $(ARCH)g++
CC_OPTS=-c -w
OPTI_OPTS= -std=c++11 -DWEBRTC_POSIX

INC_DIR = ../module
INC_DIR += ../module/interface
INC_DIR += ../system_wrappers/interface
INC_DIR += ../system_wrappers/source
INC_DIR += ../module/audioprocess/include

INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

AUDIOPROCESS_OBJS = $(wildcard ./debug/audioprocess/*.o) 

SYSTEM_WRAPPERS_OBJS = $(wildcard ./debug/system_wrappers/*.o) 

COMMON_AUDIO_SOURCE_OBJS = $(wildcard ./debug/common_audio/source/*.o) 
SIGNAL_PROCESSING_OBJS = $(wildcard ./debug/common_audio/signal_processing/*.o) 
RESAMPLER_OBJS = $(wildcard ./debug/common_audio/resampler/*.o) 
VAD_OBJS = $(wildcard ./debug/common_audio/vad/*.o) 
COMMON_AUDIO_OBJS = $(COMMON_AUDIO_SOURCE_OBJS) $(SIGNAL_PROCESSING_OBJS) $(RESAMPLER_OBJS) $(VAD_OBJS)

AUDIO_CODING_MAIN_SOURCE_CODES_ISAC_MAIN_SOURCE_OBJS = $(wildcard ./debug/audio_coding/main/source/codes/isac/main/source/*.o) 

TARGET=test

all: libapm.a main.o
	$(LD) -o $(TARGET) main.o libapm.a -lpthread -lrt
	
main.o : main.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<

libapm.a: audioprocess system_wrappers common_audio audio_coding_main_source_codes_isac_main_source
	ar rc libapm.a $(AUDIOPROCESS_OBJS) $(SYSTEM_WRAPPERS_OBJS) $(COMMON_AUDIO_OBJS) $(AUDIO_CODING_MAIN_SOURCE_CODES_ISAC_MAIN_SOURCE_OBJS)

#/************************audioprocess********************************/
audioprocess:
	$(MAKE) -C../module/audioprocess/source
	$(MAKE) -C../module/audioprocess/source/aec
	$(MAKE) -C../module/audioprocess/source/aecm
	$(MAKE) -C../module/audioprocess/source/afs
	$(MAKE) -C../module/audioprocess/source/agc/legacy
	$(MAKE) -C../module/audioprocess/source/agc
	$(MAKE) -C../module/audioprocess/source/beamformer
	$(MAKE) -C../module/audioprocess/source/ns
	$(MAKE) -C../module/audioprocess/source/transient
	$(MAKE) -C../module/audioprocess/source/utility

audioprocess_clean:
	$(MAKE) -C../module/audioprocess/source clean
	$(MAKE) -C../module/audioprocess/source/aec clean
	$(MAKE) -C../module/audioprocess/source/aecm clean
	$(MAKE) -C../module/audioprocess/source/afs clean
	$(MAKE) -C../module/audioprocess/source/agc/legacy clean
	$(MAKE) -C../module/audioprocess/source/agc clean 
	$(MAKE) -C../module/audioprocess/source/beamformer clean
	$(MAKE) -C../module/audioprocess/source/ns clean 
	$(MAKE) -C../module/audioprocess/source/transient clean 
	$(MAKE) -C../module/audioprocess/source/utility clean

#/************************system_wrappers********************************/
system_wrappers:
	$(MAKE) -C../system_wrappers/source

system_wrappers_clean:
	$(MAKE) -C../system_wrappers/source clean

#/************************common_audio********************************/
common_audio: common_audio_source signal_processing resampler vad
common_audio_clean:common_audio_source_clean signal_processing_clean resampler_clean vad_clean

common_audio_source: 
	$(MAKE) -C../module/common_audio/source	
common_audio_source_clean:
	$(MAKE) -C../module/common_audio/source clean

signal_processing:
	$(MAKE) -C../module/common_audio/source/signal_processing
signal_processing_clean:
	$(MAKE) -C../module/common_audio/source/signal_processing clean

resampler:
	$(MAKE) -C../module/common_audio/source/resampler
resampler_clean:
	$(MAKE) -C../module/common_audio/source/resampler clean

vad:
	$(MAKE) -C../module/common_audio/source/vad
vad_clean:
	$(MAKE) -C../module/common_audio/source/vad clean


#/************************audio_coding********************************/
audio_coding:

audio_coding_clean:

audio_coding_main:

audio_coding_main_clean:

audio_coding_main_source:

audio_coding_main_source_clean:

audio_coding_main_source_codes_isac_main_source:
	$(MAKE) -C../module/audio_coding/main/source/codecs/isac/main/source

audio_coding_main_source_codes_isac_main_source_clean:
	$(MAKE) -C../module/audio_coding/main/source/codecs/isac/main/source clean

	
.PHONY:clean 
clean:
	-rm -rf $(TARGET) main.o libapm.a $(AUDIOPROCESS_OBJS) $(SYSTEM_WRAPPERS_OBJS) $(COMMON_AUDIO_OBJS) $(AUDIO_CODING_MAIN_SOURCE_CODES_ISAC_MAIN_SOURCE_OBJS)

