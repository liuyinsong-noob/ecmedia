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
        self.LibFilesPath = os.path.join(projectPath, 'libs', 'armeabi')
        BuildBase.__init__(self, buildType, platform, projectPath)
        
    def build(self):
        if os.path.exists(self.CompilePath):
            os.chdir(self.CompilePath)
            print os.system('ndk-build')
        else:
            print'%s are not exist!'%self.CompilePath

    def collectLibFiles(self):
        if os.path.exists(self.RarLibsPath):
           pass
        else:
           os.mkdir(self.RarLibsPath)

        sourceFile = os.path.join(self.LibFilesPath, 'libECMedia.so')
        print os.system('cp ' + sourceFile + ' ' + self.RarLibsPath)
        
if __name__=='__main__' :
    buildType = 'release'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
        
    buildAndroid = BuildAndroid(buildType)
    buildAndroid.run()