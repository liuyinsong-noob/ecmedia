#APP_BUILD_SCRIPT := Android.mk
NDK_TOOLCHAIN_VERSION = 4.9
APP_PLATFORM := android-19
APP_STL := gnustl_static #stlport_static #gnustl_static#
#APP_ABI := armeabi-v7a
#APP_ABI := armeabi arm64-v8a x86_64
APP_ABI :=armeabi arm64-v8a #armeabi # arm64-v8a x86 x86_64
APP_CPPFLAGS :=  -std=c++11 -fexceptions 
APP_CFLAGS :=-Wno-psabi \
	-fno-exceptions \
	-Wno-missing-field-initializers \
	-fno-strict-aliasing \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wextra \
	-fno-tree-sra \
	-D__linux__ \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-Wno-error=non-virtual-dtor \
	-O3 \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer 
