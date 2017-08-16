# -*- coding:utf-8 -*-

import os
import sys

from build_base import BuildBase
from build_base import *
    
class BuildAndroid(BuildBase):
    def __init__(self, buildType):
        platform = 'android'
        projectPath = os.path.join(os.getcwd(), '..', '..')
        self.CompilePath = os.path.join(projectPath, 'jni')
        self.Lib32FilesPath = os.path.join(projectPath, 'libs', 'armeabi')
        self.Lib64FilesPath = os.path.join(projectPath, 'libs', 'arm64-v8a')
        BuildBase.__init__(self, buildType, platform, projectPath)
        
    def build(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('ndk-build clean')
            print os.system('copy ' + "BuildECMedia.mk " + "Android.mk")
            print os.system('ndk-build')
        else:
            print'%s are not exist!'%self.CompilePath
            
    def buildAudioOnly(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('ndk-build clean')
            print os.system('copy ' + "BuildECMedia_Voice.mk " + "Android.mk")
            print os.system('ndk-build')
        else:
            print'%s are not exist!'%self.CompilePath

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
        print os.system('copy ' + sourceFile + ' ' + destFile)
        
        sourceFile = os.path.join(self.Lib64FilesPath, 'libECMedia.so')
        destFile = os.path.join(self.RarX64LibsPath, 'libECMedia.so')
        print os.system('copy ' + sourceFile + ' ' + destFile)
        
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
        print os.system('copy ' + sourceFile + ' ' + destFile)
        
        sourceFile = os.path.join(self.Lib64FilesPath, 'libECMedia_Voice.so')
        destFile = os.path.join(self.RarX64LibsPath, 'libECMedia_Voice.so')
        print os.system('copy ' + sourceFile + ' ' + destFile)
    
    def collectHeaderFiles(self):
        if os.path.exists(self.RarIncludePath):
           pass
        else:
           os.mkdir(self.RarIncludePath)

        print os.system('copy ' + self.EcmediaHeader + ' ' + self.RarIncludePath)
        print os.system('copy ' + self.CommonTypesHeader + ' ' + self.RarIncludePath)
        print os.system('copy ' + self.SdkCommonHeader + ' ' + self.RarIncludePath)
        print os.system('copy ' + self.TypesDefsHeader + ' ' + self.RarIncludePath)
    
if __name__=='__main__' :
    buildType = 'release'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
        
    buildAndroid = BuildAndroid(buildType)
    buildAndroid.run()