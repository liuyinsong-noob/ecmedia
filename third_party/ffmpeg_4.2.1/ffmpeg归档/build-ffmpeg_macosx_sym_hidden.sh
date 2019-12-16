#!/bin/sh

# directories
SOURCE="ffmpeg-3.0"
FAT="FFmpeg-iOS"

SCRATCH="scratch"
# must be an absolute path
THIN=`pwd`/"thin"

# absolute path to x264 library
X264=`pwd`/fat-x264-macosx
echo ${X264}
FAAC=`pwd`/fat-faac-mac
echo ${FAAC}
#FDK_AAC=`pwd`/fdk-aac/fdk-aac-ios
FREETYPE=`pwd`/fat-freetype-mac
echo ${FREETYPE}

SYSROOT=`xcrun --sdk macosx --show-sdk-path`

CONFIGURE_FLAGS="--disable-everything --disable-debug --disable-programs --disable-doc --enable-pic --disable-iconv --enable-decoder=h264 --enable-parser=png --enable-filter=drawtext --enable-muxer=mp4 --enable-protocol=file --disable-programs --disable-filters --enable-filter=movie --enable-filter=overlay --enable-gpl --enable-avformat --enable-filter=scale --enable-nonfree --enable-avfilter --enable-demuxer=image2  --enable-decoder=png  --enable-avcodec --enable-avutil  --enable-filter=framepack   --enable-filter=framerate --enable-swscale --enable-parser=h264 --enable-muxer=mp4  --enable-muxer=adts --enable-filter=framestep --enable-filter=drawtext"

if [ "$X264" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-gpl --enable-libx264"
fi

if [ "$FDK_AAC" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfdk-aac"
fi


if [ "$FAAC" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-parser=aac --enable-libfaac --enable-encoder=libfaac"
fi


if [ "$FREETYPE" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfreetype"
fi

# avresample
#CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-avresample"

ARCHS="x86_64 i386"

COMPILE="y"
LIPO="y"

DEPLOYMENT_TARGET="10.7"

if [ "$*" ]
then
	if [ "$*" = "lipo" ]
	then
		# skip compile
		COMPILE=
	else
		ARCHS="$*"
		if [ $# -eq 1 ]
		then
			# skip lipo
			LIPO=
		fi
	fi
fi

if [ "$COMPILE" ]
then
	if [ ! `which yasm` ]
	then
		echo 'Yasm not found'
		if [ ! `which brew` ]
		then
			echo 'Homebrew not found. Trying to install...'
                        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" \
				|| exit 1
		fi
		echo 'Trying to install Yasm...'
		brew install yasm || exit 1
	fi
	if [ ! `which gas-preprocessor.pl` ]
	then
		echo 'gas-preprocessor.pl not found. Trying to install...'
		(curl -L https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl \
			-o /usr/local/bin/gas-preprocessor.pl \
			&& chmod +x /usr/local/bin/gas-preprocessor.pl) \
			|| exit 1
	fi

	if [ ! -r $SOURCE ]
	then
		echo 'FFmpeg source not found. Trying to download...'
		curl http://www.ffmpeg.org/releases/$SOURCE.tar.bz2 | tar xj \
			|| exit 1
	fi

	CWD=`pwd`
	for ARCH in $ARCHS
	do
		echo "building $ARCH..."
		mkdir -p "$SCRATCH/$ARCH"
		cd "$SCRATCH/$ARCH"

		CFLAGS="-arch $ARCH -fvisibility=hidden"
		if [ "$ARCH" = "i386" -o "$ARCH" = "x86_64" ]
		then
		    PLATFORM="macosx"
		    CFLAGS="$CFLAGS -mmacosx-version-min=$DEPLOYMENT_TARGET"
		fi

		XCRUN_SDK=`echo $PLATFORM | tr '[:upper:]' '[:lower:]'`
		CC="xcrun -sdk $XCRUN_SDK clang"
		CXXFLAGS="$CFLAGS"
		LDFLAGS="$CFLAGS"
		if [ "$X264" ]
		then
			CFLAGS="$CFLAGS -I$X264/include"
			LDFLAGS="$LDFLAGS -L$X264/lib"
		fi
		if [ "$FAAC" ]
		then
			CFLAGS="$CFLAGS -I$FAAC/include"
			LDFLAGS="$LDFLAGS -L$FAAC/lib"
		fi
		echo "CFLAGS $CFLAGS"
		echo "LDFLAGS $LDFLAGS"
		TMPDIR=${TMPDIR/%\/} sh -x $CWD/$SOURCE/configure \
		    --target-os=darwin \
		    --arch=$ARCH \
		    --cc="$CC" \
		    --sysroot="$SYSROOT" \
		    $CONFIGURE_FLAGS \
		    --extra-cflags="$CFLAGS" \
		    --extra-ldflags="$LDFLAGS" \
		    --prefix="$THIN/$ARCH" \
		|| exit 1

		make -j3 install $EXPORT || exit 1
		cd $CWD
	done
fi

if [ "$LIPO" ]
then
	echo "building fat binaries..."
	mkdir -p $FAT/lib
	set - $ARCHS
	CWD=`pwd`
	cd $THIN/$1/lib
	for LIB in *.a
	do
		cd $CWD
		echo lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB 1>&2
		lipo -create `find $THIN/i386 $THIN/x86_64 -name $LIB` -output $FAT/lib/$LIB || exit 1
	done

	cd $CWD
	cp -rf $THIN/$1/include $FAT
fi

echo Done
