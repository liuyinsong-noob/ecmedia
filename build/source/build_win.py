# -*- coding:utf-8 -*-

import os
import sys

from build_base import BuildBase
from build_base import *

CompilePath= ProjectPath + '\\demo_Win'
LibFilesPath = ProjectPath + '\\demo_Win\\build\\Win32\\Release'
    
class BuildWindows(BuildBase):
    def __init__(self):
        pass
        
    def build(self):
        if os.path.exists(CompilePath):
            os.chdir(CompilePath)
            print os.system('devenv.com MyWebRtc.sln /build "Release|Win32" /Project ECMedia')
        else:
            print'%s are not exist!'%CompilePath

    def collectLibFiles(self):
        if os.path.exists(RarLibsPath):
           pass
        else:
           os.mkdir(RarLibsPath)

        print os.system('copy ' + LibFilesPath + '\Ecmedia.* ' + RarLibsPath)
        print os.system('copy ' + LibFilesPath + '\libx264-148.dll ' + RarLibsPath)
        
if __name__=='__main__' :
    buildWindows = BuildWindows()
    buildWindows.run()