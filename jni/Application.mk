#APP_BUILD_SCRIPT := Android.mk 
NDK_TOOLCHAIN_VERSION = 4.8
APP_PLATFORM := android-9
APP_STL := stlport_static
#APP_ABI := armeabi-v7a
APP_ABI := armeabi # armeabi-v7a
APP_CFLAGS := -fno-exceptions \
	-Wno-psabi \
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
	-mthumb \
	-mthumb-interwork \
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
	
TEST := 	-march=armv5te \
	-mtune=cortex-a8 \
	-mfloat-abi=softfp \
	-mfpu=vfpv3-d16 \
	-mthumb-interwork  \
	-mthumb \
	-mfpu=neon \
	-mcpu=cortex-a8 \
	-mfloat-abi=softfp
