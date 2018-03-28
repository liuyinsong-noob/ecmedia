# -*- coding:utf-8 -*-

import os
import sys

from build_base import BuildBase
from build_base import *
    
class BuildIos(BuildBase):
    def __init__(self, buildType):
        platform = 'ios'
        projectPath = os.path.join(os.getcwd(), '..', '..')
        self.CompilePath = os.path.join(projectPath, 'ECMedia', 'ECMedia')
        self.LibReleaseFilesPath = os.path.join(projectPath, 'ECMedia', 'ECMedia', 'build', 'Release-iphoneos')
        self.LibSimulatorFilesPath = os.path.join(projectPath, 'ECMedia', 'ECMedia', 'build', 'Release-iphonesimulator')
        BuildBase.__init__(self, buildType, platform, projectPath)
        
    def build(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            video_need_rebuild =  self.build_config.get('build_state', 'video_libs_state') == 'rebuild' :
            if video_need_rebuild :
                os.system('xcodebuild clean -project ECMedia.xcodeproj')
            ret =  os.system('xcodebuild -project ECMedia.xcodeproj')
            if ret != 0:
                return ret
            if video_need_rebuild :
                return os.system('xcodebuild clean build -sdk iphonesimulator10.3 ONLY_ACTIVE_ARCH=NO VALID_ARCHS="i386 x86_64" -project ECMedia.xcodeproj')
            else :
                return os.system('xcodebuild build -sdk iphonesimulator10.3 ONLY_ACTIVE_ARCH=NO VALID_ARCHS="i386 x86_64" -project ECMedia.xcodeproj')
        else:
            print'%s are not exist!'%self.CompilePath
            return -1
    
    def buildAudioOnly(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            audio_need_rebuild =  self.build_config.get('build_state', 'audio_libs_state') == 'rebuild' :
            if audio_need_rebuild :
                os.system('xcodebuild clean -project ECMediaAudio.xcodeproj')
            ret =  os.system('xcodebuild -project ECMediaAudio.xcodeproj')
            if ret != 0:
                return ret
            if audio_need_rebuild:
                return os.system('xcodebuild clean build -sdk iphonesimulator10.3 ONLY_ACTIVE_ARCH=NO VALID_ARCHS="i386 x86_64" -project ECMediaAudio.xcodeproj')
            else: 
                return os.system('xcodebuild build -sdk iphonesimulator10.3 ONLY_ACTIVE_ARCH=NO VALID_ARCHS="i386 x86_64" -project ECMediaAudio.xcodeproj')
        else:
            print'%s are not exist!'%self.CompilePath
            return -1

    def collectLibFiles(self):
        sourceReleaseFile = os.path.join(self.LibReleaseFilesPath, 'libECMedia.a')
        sourceSimulatorFile = os.path.join(self.LibSimulatorFilesPath, 'libECMedia.a')
        destinationFile = os.path.join(self.RarLibsPath, 'libECMedia.a')
        print os.system('lipo -c ' + sourceReleaseFile + ' ' + sourceSimulatorFile + ' -o ' + destinationFile)
        
    def collectLibAudioOnlyFiles(self):
        sourceReleaseFile = os.path.join(self.LibReleaseFilesPath, 'libECMediaAudio.a')
        sourceSimulatorFile = os.path.join(self.LibSimulatorFilesPath, 'libECMediaAudio.a')
        destinationFile = os.path.join(self.RarLibsPath, 'libECMediaAudio.a')
        print os.system('lipo -c ' + sourceReleaseFile + ' ' + sourceSimulatorFile + ' -o ' + destinationFile)
        
    def rarFiles(self):
        os.chdir(self.BuildPath)
        targetFile = os.path.join(self.BuildPath, self.rarFileName)
        sourceFile = self.buildType
        print os.system('zip -r -m ' + targetFile + ' ' + sourceFile)
        
if __name__=='__main__' :
    buildType = 'ios_target_libs'

    buildIos = BuildIos(buildType)
    buildIos.run()
