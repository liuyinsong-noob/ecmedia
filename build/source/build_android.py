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
            print os.system('ndk-build')
        else:
            print'%s are not exist!'%self.CompilePath

    def collectLibFiles(self):
        if os.path.exists(self.RarLibsPath):
           pass
        else:
           os.mkdir(self.RarLibsPath)

        sourceFile = os.path.join(self.Lib32FilesPath, 'libECMedia.so')
        destFile = os.path.join(self.RarLibsPath, 'libECMedia-32.so')
        print os.system('cp ' + sourceFile + ' ' + destFile)
        
        sourceFile = os.path.join(self.Lib64FilesPath, 'libECMedia.so')
        destFile = os.path.join(self.RarLibsPath, 'libECMedia-64.so')
        print os.system('cp ' + sourceFile + ' ' + destFile)
        
if __name__=='__main__' :
    buildType = 'release'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
        
    buildAndroid = BuildAndroid(buildType)
    buildAndroid.run()