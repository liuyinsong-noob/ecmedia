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
        self.LibFilesPath = os.path.join(projectPath, 'ECMedia', 'ECMedia', 'build', 'Release-iphoneos')
        BuildBase.__init__(self, buildType, platform, projectPath)
        
    def build(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('xcodebuild -project ECMedia.xcodeproj')
        else:
            print'%s are not exist!'%self.CompilePath

    def collectLibFiles(self):
        if os.path.exists(self.RarLibsPath):
           pass
        else:
           os.mkdir(self.RarLibsPath)

        sourceFile = os.path.join(self.LibFilesPath, 'libECMedia.a')
        print os.system('cp ' + sourceFile + ' ' + self.RarLibsPath)
        
if __name__=='__main__' :
    buildType = 'release'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
        
    buildIos = BuildIos(buildType)
    buildIos.run()