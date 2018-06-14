# -*- coding:utf-8 -*-

import os
import sys


from build_base import BuildBase
from build_base import *
    
class BuildAndroid(BuildBase):
    def __init__(self, version):
        platform = 'android'
        projectPath = os.path.join(os.getcwd(), '..', '..')
        self.CompilePath = os.path.join(projectPath, 'jni')
        self.Lib32FilesPath = os.path.join(projectPath, 'libs', 'armeabi')
        self.Lib64FilesPath = os.path.join(projectPath, 'libs', 'arm64-v8a')
        BuildBase.__init__(self, version, platform, projectPath)
        
    def build(self):
        if self.build_config.get('build_setting', self.platform + '_libs_type') == 'audio_only' :
            return 0
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('cp -r ' + "BuildECMedia.mk " + "Android.mk")
            if self.build_config.get('build_state', self.platform + '_video_state') == 'rebuild' :
                print os.system('ndk-build clean')
            ret = os.system('ndk-build -j 4')
            print os.system('cp -r ../obj/local/armeabi ../build')
            return ret
        else:
            print'%s are not exist!'%self.CompilePath
            return -1
            
    def buildAudioOnly(self):
        if self.build_config.get('build_setting', self.platform + '_libs_type') == 'video_only' :
            return 0
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            if self.build_config.get('build_state', self.platform + '_audio_state') == 'rebuild' :
                print os.system('ndk-build clean')
            print os.system('cp -r ' + "BuildECMedia_Voice.mk " + "Android.mk")
            return os.system('ndk-build -j 4')
        else:
            print'%s are not exist!'%self.CompilePath
            return -1

    def collectLibFiles(self):
        if os.path.exists(self.RarX32LibsPath):
           pass
        else:
           os.mkdir(self.RarX32LibsPath)
           
        if os.path.exists(self.RarX64LibsPath):
           pass
        else:
           os.mkdir(self.RarX64LibsPath)

        sourceFile = os.path.join(self.Lib32FilesPath, 'libECMedia.so')
        destFile = os.path.join(self.RarX32LibsPath, 'libECMedia.so')
        print os.system('cp -r ' + sourceFile + ' ' + destFile)
        
        sourceFile = os.path.join(self.Lib64FilesPath, 'libECMedia.so')
        destFile = os.path.join(self.RarX64LibsPath, 'libECMedia.so')
        print os.system('cp -r ' + sourceFile + ' ' + destFile)
        
    def collectLibAudioOnlyFiles(self):
        if os.path.exists(self.RarX32LibsPath):
           pass
        else:
           os.mkdir(self.RarX32LibsPath)
           
        if os.path.exists(self.RarX64LibsPath):
           pass
        else:
           os.mkdir(self.RarX64LibsPath)

        sourceFile = os.path.join(self.Lib32FilesPath, 'libECMedia_Voice.so')
        destFile = os.path.join(self.RarX32LibsPath, 'libECMedia_Voice.so')
        print os.system('cp -r ' + sourceFile + ' ' + destFile)
        
        sourceFile = os.path.join(self.Lib64FilesPath, 'libECMedia_Voice.so')
        destFile = os.path.join(self.RarX64LibsPath, 'libECMedia_Voice.so')
        print os.system('cp -r ' + sourceFile + ' ' + destFile)
    
    def collectHeaderFiles(self):
        if os.path.exists(self.RarIncludePath):
           pass
        else:
           os.mkdir(self.RarIncludePath)

        print os.system('cp -r ' + self.EcmediaHeader + ' ' + self.RarIncludePath)
        print os.system('cp -r ' + self.CommonTypesHeader + ' ' + self.RarIncludePath)
        print os.system('cp -r ' + self.SdkCommonHeader + ' ' + self.RarIncludePath)
        print os.system('cp -r ' + self.TypesDefsHeader + ' ' + self.RarIncludePath)
    
if __name__=='__main__' :
    newVersion = ''
    if len(sys.argv) == 2:
        newVersion = sys.argv[1]

    buildAndroid = BuildAndroid(newVersion)
    buildAndroid.run()
