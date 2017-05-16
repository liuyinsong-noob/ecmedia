# -*- coding:utf-8 -*-

import os
import sys

from build_base import BuildBase
from build_base import *

CompilePath= ProjectPath + '\\jni'
LibFilesPath = ProjectPath + '\\libs\\armeabi'
    
class BuildAndroid(BuildBase):
    def __init__(self):
        pass
        
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
    buildAndroid = BuildAndroid()
    buildAndroid.run()