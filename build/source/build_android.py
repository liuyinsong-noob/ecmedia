# -*- coding:utf-8 -*-

import os
import sys


from build_base import BuildBase
from build_base import *
    
class BuildAndroid(BuildBase):
    def __init__(self, buildType, onlyAudio, needClean):
        platform = 'android'
        self.onlyAudio = onlyAudio
        self.needClean = needClean
        projectPath = os.path.join(os.getcwd(), '..', '..')
        self.CompilePath = os.path.join(projectPath, 'jni')
        self.Lib32FilesPath = os.path.join(projectPath, 'libs', 'armeabi')
        self.Lib64FilesPath = os.path.join(projectPath, 'libs', 'arm64-v8a')
        BuildBase.__init__(self, buildType, platform, projectPath)
        
    def build(self):
        if os.path.exists(self.CompilePath):
            if self.onlyAudio :
                return 0
            os.chdir(self.CompilePath)
            # print os.system('ndk-build clean')
            print os.system('cp -r ' + "BuildECMedia.mk " + "Android.mk")
            print os.system('ndk-build -j 4')
        else:
            print'%s are not exist!'%self.CompilePath
            return -1
            
    def buildAudioOnly(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('ndk-build clean')
            print os.system('cp -r ' + "BuildECMedia_Voice.mk " + "Android.mk")
            print os.system('ndk-build -j 4')
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
    buildType = 'release'
    onlyAudio = False
    needClean = False
    argvLen =  sys.argv.__len__();
    if argvLen != 1:
        args = sys.argv[1: argvLen]
        for item in  args:
            if item.startswith('--') :
                pass
            else :
                print('bad input commond argument, please check again! argument is: %s' %(item))
                quit()

            if item == '--release' :
                print('[build info]: build type: release !')
                buildType = item

            if item == '--audio' :
                onlyAudio = True

            if item == '--clean' :
                print('[build info]: need clean build targets!')
                needClean = True

        if onlyAudio :
            print('[build info]: only build ecmedia audio libs!')
        else :
            print('[build info]: build full ecmedia libs!')

    buildAndroid = BuildAndroid(buildType, onlyAudio, needClean)
    buildAndroid.run()
