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
        BuildBase.__init__(self, buildType, platform, projectPath)
        
    def build(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('devenv.com MyWebRtc.sln /build "Release|Win32" /Project ECMedia')
        else:
            print'%s are not exist!'%self.CompilePath

    def collectLibFiles(self):
        if os.path.exists(self.RarLibsPath):
           pass
        else:
           os.mkdir(self.RarLibsPath)

        ecmediaTargetFile = os.path.join(self.LibFilesPath, 'Ecmedia.*')
        x264TargetFile = os.path.join(self.LibFilesPath, 'libx264-148.dll')
        print os.system('copy ' + ecmediaTargetFile + ' ' + self.RarLibsPath)
        print os.system('copy ' + x264TargetFile + ' ' + self.RarLibsPath)
        
if __name__=='__main__' :
    buildType = 'release'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
    buildWindows = BuildWindows(buildType)
    buildWindows.run()