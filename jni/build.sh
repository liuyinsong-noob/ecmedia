#!/bin/sh
	ndk-build clean
if [[ "$1" =  "haiyuntong" ]] 
then
	cp Android_Haiyuntong.mk Android.mk
elif [[ "$1" = "xinwei" ]] 
then
	cp Android_video_xinwei.mk Android.mk
elif [[ "$1" = "video" ]] 
then
	cp Android_video.mk Android.mk
elif [[ "$1" = "voice" ]] 
then
	cp Android_Voice.mk Android.mk
elif [[ "$1" = "message" ]] 
then
	cp Android_no_voip.mk Android.mk
else
	echo usage "$0 haiyuntong/video/voice/message"
	exit
fi
	ndk-build	
