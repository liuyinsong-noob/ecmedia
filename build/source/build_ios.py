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
            print os.system('xcodebuild -project ECMedia.xcodeproj')
            print os.system('xcodebuild clean build -sdk iphonesimulator10.3 ONLY_ACTIVE_ARCH=NO VALID_ARCHS="i386 x86_64" -project ECMedia.xcodeproj')         
        else:
            print'%s are not exist!'%self.CompilePath
    
    def buildAudioOnly(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('xcodebuild -project ECMediaAudio.xcodeproj')
            print os.system('xcodebuild clean build -sdk iphonesimulator10.3 ONLY_ACTIVE_ARCH=NO VALID_ARCHS="i386 x86_64" -project ECMediaAudio.xcodeproj')            
        else:
            print'%s are not exist!'%self.CompilePath

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
        sourceFile = os.path.join(self.BuildPath, self.buildType)
        print os.system('zip -jr ' + targetFile + ' ' + sourceFile)
        
if __name__=='__main__' :
    buildType = 'release'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
        
    buildIos = BuildIos(buildType)
    buildIos.run()
