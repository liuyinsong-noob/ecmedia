# -*- coding:utf-8 -*-

import os
import sys

from build_base import BuildBase
from build_base import *

CompilePath= ProjectPath + '\\jni'
LibFilesPath = ProjectPath + '\\libs\\armeabi'
    
class BuildAndroid(BuildBase):
    def __init__(self, buildType):
        platform = 'android'
        BuildBase.__init__(self, buildType, platform)
        
    def build(self):
        if os.path.exists(CompilePath):
            os.chdir(CompilePath)
            print os.system('ndk-build')
        else:
            print'%s are not exist!'%CompilePath

    def collectLibFiles(self):
        if os.path.exists(RarLibsPath):
           pass
        else:
           os.mkdir(RarLibsPath)

        print os.system('copy ' + LibFilesPath + '\libECMedia.so ' + RarLibsPath)
        
if __name__=='__main__' :
    buildType = 'release'
    if len(sys.argv) != 1:
        buildType = sys.argv[1]
        
    buildAndroid = BuildAndroid(buildType)
    buildAndroid.run()