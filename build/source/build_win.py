# -*- coding:utf-8 -*-

import os
import sys

from build_base import BuildBase
from build_base import *
    
class BuildWindows(BuildBase):
    def __init__(self, buildType):
        platform = 'windows'
        projectPath = os.path.join(os.getcwd(), '..', '..')
        self.CompilePath= os.path.join(projectPath, 'demo_Win')
        self.LibFilesPath = os.path.join(projectPath, 'demo_Win', 'build', 'Win32', 'Release')
        self.Libx64FilesPath = os.path.join(projectPath, 'demo_Win', 'build', 'x64', 'Release')
        self.LibAudioFilesFilesPath = os.path.join(projectPath, 'demo_Win', 'build', 'Win32', 'audioRelease')
        self.Libx64AudioFilesFilesPath = os.path.join(projectPath, 'demo_Win', 'build', 'x64', 'audioRelease')
        BuildBase.__init__(self, buildType, platform, projectPath)
        
    def build(self):
        if self.build_config.get('build_setting', self.platform + '_libs_type') == 'audio_only' :
            return 0
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            ret = -1
            if self.build_config.get('build_state', self.platform + '_video_state') == 'rebuild' :
                ret =  os.system('devenv.com MyWebRtc.sln /rebuild "Release|Win32" /Project ECMedia')
            else :
                ret =  os.system('devenv.com MyWebRtc.sln /build "Release|Win32" /Project ECMedia')
            if ret != 0:
                return ret
            if self.build_config.get('build_setting', 'windows_arch') == 'x32_only' :
                return 0
            if self.build_config.get('build_state', self.platform + '_video_state') == 'rebuild' :
                return os.system('devenv.com MyWebRtc.sln /rebuild "Release|x64" /Project ECMedia')
            else :
                return os.system('devenv.com MyWebRtc.sln /build "Release|x64" /Project ECMedia')
        else:
            print'%s are not exist!'%self.CompilePath
            return -1
            
    def buildAudioOnly(self):
        if self.build_config.get('build_setting', self.platform + '_libs_type') == 'video_only' :
            return 0
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            ret = -1
            if self.build_config.get('build_state', self.platform + '_audio_state') == 'rebuild' :
                ret = os.system('devenv.com MyWebRtc.sln /rebuild "audioRelease|Win32" /Project ECMediaVOICE')
            else :
                ret = os.system('devenv.com MyWebRtc.sln /build "audioRelease|Win32" /Project ECMediaVOICE')
            if ret != 0:
                return ret
            if self.build_config.get('build_setting', 'windows_arch') == 'x32_only' :
                return 0
            if self.build_config.get('build_state', self.platform + '_audio_state') == 'rebuild' :
                return os.system('devenv.com MyWebRtc.sln /rebuild "audioRelease|x64" /Project ECMediaVOICE')
            else :
                return os.system('devenv.com MyWebRtc.sln /build "audioRelease|x64" /Project ECMediaVOICE')
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

        ecmediaTargetFile = os.path.join(self.LibFilesPath, 'Ecmedia.*')
        x264TargetFile = os.path.join(self.LibFilesPath, 'libx264-148.dll')
        print os.system('copy ' + ecmediaTargetFile + ' ' + self.RarX32LibsPath)
        print os.system('copy ' + x264TargetFile + ' ' + self.RarX32LibsPath)
        
        ecmediaTargetFile = os.path.join(self.Libx64FilesPath, 'Ecmedia.*')
        x264TargetFile = os.path.join(self.Libx64FilesPath, 'libx264-148.dll')
        print os.system('copy ' + ecmediaTargetFile + ' ' + self.RarX64LibsPath)
        print os.system('copy ' + x264TargetFile + ' ' + self.RarX64LibsPath)
        
    def collectLibAudioOnlyFiles(self):
        if os.path.exists(self.RarX32LibsPath):
           pass
        else:
           os.mkdir(self.RarX32LibsPath)
           
        if os.path.exists(self.RarX64LibsPath):
           pass
        else:
           os.mkdir(self.RarX64LibsPath)

        ecmediaTargetFile = os.path.join(self.LibAudioFilesFilesPath, 'EcmediaVOICE.*')
        print os.system('copy ' + ecmediaTargetFile + ' ' + self.RarX32LibsPath)
        
        ecmediaTargetFile = os.path.join(self.Libx64AudioFilesFilesPath, 'EcmediaVOICE.*')
        print os.system('copy ' + ecmediaTargetFile + ' ' + self.RarX64LibsPath)

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
    buildType = 'win_target_libs'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
    buildWindows = BuildWindows(buildType)
    buildWindows.run()
