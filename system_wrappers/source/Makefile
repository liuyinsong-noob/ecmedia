ARCH = 
CPP = $(ARCH)g++

CC_OPTS=-c -fpic
OPTI_OPTS= -std=c++11 -DWEBRTC_LINUX -DWEBRTC_POSIX

INC_DIR = .
INC_DIR += ../..
INC_DIR += ../interface
INC_DIR += ../../module
INCLUDE = $(foreach dir,$(INC_DIR),-I$(dir))

ROOT_DIR = ../../demo_linux
DEBUG_DIR = $(ROOT_DIR)/debug/system_wrappers


SRC = aligned_malloc.cc \
	checks.cc \
	clock.cc \
	condition_variable.cc \
	condition_variable_posix.cc \
	critical_section.cc \
	critical_section_posix.cc \
	event.cc \
	event_posix.cc \
	event_tracer.cc \
	file_impl.cc \
	list_no_stl.cc \
	logging.cc \
	map.cc \
	platform_file.cc \
	rtp_to_ntp.cc \
	rw_lock.cc \
	rw_lock_posix.cc \
	sleep.cc \
	timestamp_extrapolator.cc \
	thread.cc \
	thread_posix.cc \
	trace_impl.cc \
	trace_posix.cc


OBJS = $(patsubst %.cc,$(DEBUG_DIR)/%.o,$(SRC))

all:$(OBJS)
$(DEBUG_DIR)/%.o : %.cc
	$(CPP) $(CC_OPTS) $(OPTI_OPTS) $(INCLUDE) -o $@ $<
	
.PHONY:clean 
clean:
	-rm -rf $(OBJS)


