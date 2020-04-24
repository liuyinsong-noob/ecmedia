cmd /k gn gen out/Default --args="is_debug=false target_cpu=\"x86\" rtc_use_h264=true proprietary_codecs=true ffmpeg_branding = \"Chrome\"" --ide=vs2017
dir
