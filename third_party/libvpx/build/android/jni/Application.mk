#APP_BUILD_SCRIPT := Android.mk 
APP_PLATFORM := android-9
APP_ABI := armeabi-v7a
APP_STL := stlport_static
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
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-Wno-error=non-virtual-dtor \
	-llog \
	-Os \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer
