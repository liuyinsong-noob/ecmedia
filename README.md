##  ECMedia  

[![build status](http://git.yuntongxun.com/platform_sdk/ecmedia/badges/master/build.svg)](http://git.yuntongxun.com/platform-sdk/ecmedia/commits/master)



### Overview 

ECMedia 提供跨平台媒体传输处理能力，支持的平台包括 Windows,  Android， iOS 等 





### Build

Ecmedia 工程目录结构

```shell
├── ECMedia 						# ecmedia 接口和逻辑抽象目录
├── README.md
├── ReleaseNotes.txt
├── android-webrtc.mk
├── base
├── build
├── change.txt
├── common_video
├── config.cc
├── config.h
├── demo_Android # Android demo 程序
├── demo_Mac	
├── demo_Win	# windows demo 程序
├── demo_iOS # ios demo 程序
├── demo_linux
├── jni			# ecmedia Android 平台编译目录
├── libs 		# ecmedia Android 动态库生成目录
├── logging
├── module
├── servicecore
├── system_wrappers
├── third_party
├── video_engine
└── voice_engine
```



#### Android

系统配置好 NDK 编译工具后

终端中，cd 进入 ecmedia 项目根目录下的 `jni` 目录，执行 `ndk-build` 命令编译 ecmedia 库，生成的动态库，在上层路径下的 `libs` 目录

#### iOS 

进入 ecmedia 项目根目录下的 `ECMedia` 目录，下面还有一个 `ECMedia` 目录，使用  Xcode `ECMedia.xcodeproj` 打开该工程，编译出的即为 `ECMedia` iOS平台媒体库

#### Window

进入 ecmedia 项目根目录下的 `demo_Win` 目录, 使用 vs 2015 打开，执行生成解决方案，即可生成对应的 ecmedia 动态库



### contact

@[Audio Video R&D Team](http://yuntongxun.com) 

 




