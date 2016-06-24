PLATFORM ?= iPhoneOS
SDKVER ?= 5.0

CC=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/arm-apple-darwin10-llvm-g++-4.2
#CC=/Developer/Platforms/$(PLATFORM).platform/Developer/usr/bin/gcc
CFLAGS= -arch armv6 -isysroot /Developer/Platforms/$(PLATFORM).platform/Developer/SDKs/$(PLATFORM)$(SDKVER).sdk
CPP=/Developer/Platforms/$(PLATFORM).platform/Developer/usr/bin/cpp

OUTLIB = out/$(PLATFORM)/Release/lib
OBJDIR = out/$(PLATFORM)/Release/obj
export CC CFLAGS CPP OUTLIB OBJDIR
#export OUTLIB OBJDIR

.PHONY: All
All:
#	cd module/rtp_rtcp; $(MAKE) -f rtp_rtcp_ios.mk
#	cd module/udp_transport; $(MAKE) -f udp_transport_ios.mk
#	cd module/utility;  $(MAKE) -f webrtc_utility_ios.mk
#	cd module/resampler; $(MAKE) -f resampler_ios.mk
#	cd module/signalprocess; $(MAKE) -f signalprocess_ios.mk
#	cd module/bitrate_controller;  $(MAKE) -f bitrate_controller_ios.mk
#	cd module/audio_coding;  $(MAKE) -f NetEq_ios.mk
#	cd module/audio_coding;  $(MAKE) -f CNG_ios.mk
	cd module/audio_coding;  $(MAKE) -f G711_ios.mk
#	cd module/audio_coding;  $(MAKE) -f audio_coding_module_ios.mk
#	cd module/audio_coding;  $(MAKE) -f amrnb_ios.mk
#	cd module/audio_device;  $(MAKE) -f audio_device_ios.mk
#	cd module/audio_conference_mixer;  $(MAKE) -f audio_conference_mixer_ios.mk
#	cd module/osip;  $(MAKE) -f osip_ios.mk
#	cd module/exosip;  $(MAKE) -f exosip_ios.mk
#	cd module/video_capture;  $(MAKE) -f video_capture_module_ios.mk
#	cd module/video_processing;  $(MAKE) -f video_processing_ios.mk
#	cd module/video_render;  $(MAKE) -f video_render_module_ios.mk
#	cd module/videojpegyuv;  $(MAKE) -f videojpegyuv_ios.mk
#	cd module/media_file;  $(MAKE) -f media_file_ios.mk
#	cd module/video_coding;  $(MAKE) -f webrtc_video_coding_ios.mk
#	cd module/audioprocess;  $(MAKE) -f audioprocess_ios.mk
#	cd third_party/libjpeg_turbo;  $(MAKE) -f libjpeg_ios.mk
#	cd third_party/libyuv;  $(MAKE) -f libyuv_ios.mk
#	cd system_wrappers/source;  $(MAKE) -f system_wrappers_ios.mk
#	cd voice_engine;  $(MAKE) -f voice_engine_core_ios.mk
#	cd video_engine;  $(MAKE) -f video_engine_core_ios.mk
#	cd servicecore;  $(MAKE) -f servicecore_ios.mk

#createlib:
#	lipo -create build/Debug-iphoneos/libname.a build/Debug-iphonesimulator/libname.a -output libname.a

.PHONY: clean
clean:
	rm -rdf out

