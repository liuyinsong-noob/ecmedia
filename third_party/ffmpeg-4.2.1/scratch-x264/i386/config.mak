SRCPATH=../../x264
prefix=/Users/yukening/Downloads/ecmedia/third_party/ffmpeg_4.2.1/thin-x264/i386
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
SYS_ARCH=X86
SYS=MACOSX
CC=xcrun -sdk iphonesimulator clang
CFLAGS=-Wshadow -O3 -ffast-math -m32  -Wall -I. -I$(SRCPATH) -arch i386 -mios-simulator-version-min=5.0 -march=i686 -mfpmath=sse -msse -msse2 -std=gnu99 -D_GNU_SOURCE -mstack-alignment=64 -fPIC -fomit-frame-pointer -fno-tree-vectorize -fvisibility=hidden
CFLAGSSO=
CFLAGSCLI=
COMPILER=GNU
COMPILER_STYLE=GNU
DEPMM=-MM -g0
DEPMT=-MT
LD=xcrun -sdk iphonesimulator clang -o 
LDFLAGS=-m32  -arch i386 -mios-simulator-version-min=5.0 -lm -lpthread -ldl
LDFLAGSCLI=
LIBX264=libx264.a
CLI_LIBX264=$(LIBX264)
AR=ar rc 
RANLIB=ranlib
STRIP=strip
INSTALL=install
AS=nasm
ASFLAGS= -I. -I$(SRCPATH)  -DARCH_X86_64=0 -I$(SRCPATH)/common/x86/ -f macho32 -DPREFIX -DSTACK_ALIGNMENT=64 -DPIC
RC=
RCFLAGS=
EXE=
HAVE_GETOPT_LONG=1
DEVNULL=/dev/null
PROF_GEN_CC=-fprofile-generate
PROF_GEN_LD=-fprofile-generate
PROF_USE_CC=-fprofile-use
PROF_USE_LD=-fprofile-use
HAVE_OPENCL=yes
CC_O=-o $@
default: lib-static
install: install-lib-static
