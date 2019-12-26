#!/bin/sh

# directories
SOURCE="ffmpeg-3.4.2"
FAT="FFmpeg-iOS"

SCRATCH="scratch"
# must be an absolute path
THIN=`pwd`/"thin"

# absolute path to x264 library
X264=`pwd`/fat-x264
echo ${X264}
# FAAC=`pwd`/fat-faac
# echo ${FAAC}
FDK_AAC=`pwd`/fdk-aac/fdk-aac-ios
FREETYPE=`pwd`/fat-freetype
echo ${FREETYPE}

CONFIGURE_FLAGS="--disable-everything --enable-cross-compile --disable-debug --disable-programs --disable-doc --enable-pic --disable-iconv --enable-decoder=h264 --enable-parser=png --enable-filter=drawtext --enable-muxer=mp4 --enable-protocol=file --disable-programs --disable-filters --enable-filter=movie --enable-filter=overlay --enable-gpl --enable-avformat --enable-filter=scale --enable-nonfree --enable-avfilter --enable-demuxer=image2  --enable-decoder=png  --enable-avcodec --enable-avutil  --enable-filter=framepack   --enable-filter=framerate --enable-encoder=png --enable-swscale --enable-parser=h264 --enable-muxer=mp4  --enable-muxer=adts --enable-filter=framestep --enable-filter=drawtext --enable-decoder=aac --enable-decoder=aac_latm --enable-decoder=flv --enable-decoder=mp3* --enable-decoder=vp6f --enable-decoder=flac --enable-decoder=hevc --enable-decoder=vp8 --enable-decoder=vp9 --enable-demuxer=aac --enable-demuxer=concat --enable-demuxer=data --enable-demuxer=flv --enable-demuxer=hls --enable-demuxer=live_flv --enable-demuxer=mov --enable-demuxer=mp3 --enable-demuxer=mpegps --enable-demuxer=mpegts --enable-demuxer=mpegvideo --enable-demuxer=flac --enable-demuxer=hevc --enable-demuxer=webm_dash_manifest --enable-parser=aac --enable-parser=aac_latm --enable-parser=h264 --enable-parser=flac --enable-parser=hevc --enable-bsfs --enable-protocols --enable-protocol=async --enable-protocol=ffrtmphttp --enable-protocol=rtmpt --enable-protocol=rtmp --enable-static --enable-neon --enable-optimizations  --enable-small"

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

ARCHS="armv7"

COMPILE='y'
LIPO="y"

DEPLOYMENT_TARGET="6.0"

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
		    PLATFORM="iPhoneSimulator"
		    CFLAGS="$CFLAGS -mios-simulator-version-min=$DEPLOYMENT_TARGET"
		else
		    PLATFORM="iPhoneOS"
		    CFLAGS="$CFLAGS -mios-version-min=$DEPLOYMENT_TARGET"
		    if [ "$ARCH" = "arm64" ]
		    then
		        EXPORT="GASPP_FIX_XCODE5=1"
		    fi
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
		if [ "$FDK_AAC" ]
		then
			CFLAGS="$CFLAGS -I$FDK_AAC/include"
			LDFLAGS="$LDFLAGS -L$FDK_AAC/lib"
		fi
		if [ "$FAAC" ]
		then
			CFLAGS="$CFLAGS -I$FAAC/include"
			LDFLAGS="$LDFLAGS -L$FAAC/lib"
		fi
		if [ "$FREETYPE" ]
		then
			CFLAGS="$CFLAGS -I$FREETYPE/include"
			LDFLAGS="$LDFLAGS -L$FREETYPE/lib"
		fi
		echo "CFLAGS $CFLAGS"
		echo "LDFLAGS $LDFLAGS"
		TMPDIR=${TMPDIR/%\/} $CWD/$SOURCE/configure \
		    --target-os=darwin \
		    --arch=$ARCH \
		    --cc="$CC" \
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
		lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB || exit 1
	done

	cd $CWD
	cp -rf $THIN/$1/include $FAT
fi

echo Done
