APP_PLATFORM := android-9
APP_STL := stlport_static
APP_MODULES	 += silk
APP_ABI := armeabi armeabi-v7a
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
  -Os \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer \
		-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-Wno-error=non-virtual-dtor \
  -ffunction-sections \
	-funwind-tables