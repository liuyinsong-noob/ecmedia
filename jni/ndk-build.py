#!/usr/bin/python

import os
import sys
import time

class BuildBase:
    def __init__(self):
        self.libsPath = os.path.join(os.getcwd(), '..', 'libs/armeabi')
        self.appLibPath = os.path.join(os.getcwd(), '..', 'demo_Android/CCP_Demo_v3.6.4r/libs/')
        print self.libsPath
        print self.appLibPath

    def run(self):
        self.build()
        self.copyLibsToApp()

    def build(self):
        print os.system('ndk-build -j 4')
    
    def copyLibsToApp(self):
        print self.libsPath
        print os.system('cp -r ' + self.libsPath + ' ' + self.appLibPath)

if __name__=='__main__' :
    buildAndroid = BuildBase()
    buildAndroid.run()