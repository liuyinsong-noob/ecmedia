# -*- coding:utf-8 -*-

import os
import sys

ProjectPath = os.getcwd() + '\..\..'

EcmediaCpp = ProjectPath + '\\ECMedia\\source\\ECMedia.cpp'
EcmediaHeader = ProjectPath + '\\ECMedia\\interface\\ECMedia.h'
CommonTypesHeader = ProjectPath + '\\module\\common_types.h'
SdkCommonHeader = ProjectPath + '\\module\\sdk_common.h'
TypesDefsHeader = ProjectPath + '\\module\\typedefs.h'
ReleaseNoteFile = ProjectPath + '\\ReleaseNotes.txt'

BuildPath = ProjectPath + '\\build'
RarPath = ProjectPath + '\\build\\release'
RarIncludePath = RarPath + '\\include'
RarLibsPath = RarPath + '\\libs'

class BuildBase:
    def __init__(self, buildType, platform):
        self.buildType = buildType
        self.platform = platform
    
    def run(self):
        self.build()
        self.collectFiles()
        timestamp, sha = self.getLastCommitInfo()
        timestamp = str(int(timestamp) + 1)
        self.writeReleaseNote(timestamp, sha)
        self.rarFiles()
        self.copyToRemote(self.platform)

    def build():
        pass
    
    def collectLibFiles(self):
        pass
      
    def collectHeaderFiles(self):
        if os.path.exists(RarIncludePath):
           pass
        else:
           os.mkdir(RarIncludePath)

        print os.system('copy ' + EcmediaHeader + ' ' + RarIncludePath)
        print os.system('copy ' + CommonTypesHeader + ' ' + RarIncludePath)
        print os.system('copy ' + SdkCommonHeader + ' ' + RarIncludePath)
        print os.system('copy ' + TypesDefsHeader + ' ' + RarIncludePath)
           
        
    def collectFiles(self):
        if os.path.exists(RarPath):
           pass
        else:
           os.mkdir(RarPath)

        self.collectLibFiles()
        self.collectHeaderFiles()

    def rarFiles(self):
        os.chdir(BuildPath)
        rarFileName = 'release-' + self.getEcmediaVersion() + '.zip'
        print os.system('7z a -tzip ' + BuildPath + '\\' + rarFileName + ' ' + BuildPath + '\\release')

    def getEcmediaVersion(self):
        fd = open(EcmediaCpp)
        version = None
        for line in fd.readlines():
            if line.startswith('#define ECMEDIA_VERSION'):
                version = line.split(' ')[-1]
        return version.strip('\n').strip('"')

    def getLatestSHA(self, timestamp):
        if os.path.exists(ProjectPath):                 
            os.chdir(ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        outputStr = output.read().split('\n')
        for line in outputStr:
            if line.startswith('logstart'):
                latestSHA = line.split('-')[1]
                return latestSHA

    def getLatestTimestamp(self, timestamp):
        if os.path.exists(ProjectPath):                 
            os.chdir(ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        outputStr = output.read().split('\n')
        for line in outputStr:
            if line.startswith('logstart'):
                latestTimestamp = line.split('-')[2]
                return latestTimestamp
                
    def getCommitLogs(self, timestamp):
        if os.path.exists(ProjectPath):                 
            os.chdir(ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        outputStr = output.read().split('\n')
        commitLogs = []
        for line in outputStr:
            if line.startswith('logstart'):
                commitLog = ''.join(line.split('-')[4:])
                commitLogs.append(commitLog.decode('utf-8'))
        return commitLogs
        
    def isEcmediaHeaderChanged(self, timestamp):
        if os.path.exists(ProjectPath):                 
            os.chdir(ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        
        bEcmediaHeaderChanged = False
        if "ECMedia.h" in output:
            bEcmediaHeaderChanged = True
            
        return bEcmediaHeaderChanged
        
    def getLastCommitInfo(self):
        fdOrig = open(ProjectPath + '\\ReleaseNotes.txt')
        for line in fdOrig.read().split('\n'):
            if line.startswith('Version'):
                if len(line.split(' ')) == 4:
                    timestamp = line.split(' ')[2]
                    sha = line.split(' ')[3]
                    return (timestamp, sha)
                else:
                    return (None, None)
        
    def writeReleaseNote(self, timestamp, sha):
        fdOrig = open(ProjectPath + '\\ReleaseNotes.txt')
        origContent = fdOrig.read()

        ecmediaVersion = self.getEcmediaVersion()
        latestSHA = self.getLatestSHA(timestamp)
        latestTimestamp = self.getLatestTimestamp(timestamp)
        commitLogs = self.getCommitLogs(timestamp)

        fdNew = open(RarPath + '\ReleaseNotes.txt', 'wb')
        fdNew.write('Version' + ' ' + ecmediaVersion)
        fdNew.write(' ' + latestTimestamp + ' ' + latestSHA)
        fdNew.write('\n')

        index = 1
        if self.isEcmediaHeaderChanged(timestamp):
            fdNew.write((str(index) + '. ' + '接口改变').decode('utf-8').encode('gbk'))
            fdNew.write('\n')
            index = index + 1
        else:
            fdNew.write((str(index) + '. ' + '接口不变').decode('utf-8').encode('gbk'))
            fdNew.write('\n')
            index = index + 1
        for commitLog in commitLogs:
            fdNew.write(str(index) + '. ' + commitLog.encode('gbk'))
            fdNew.write('\n')
            index = index + 1
            
        fdNew.write('\n')
        fdNew.write(origContent)
    
    def copyToRemote(self, platform):
        os.chdir(BuildPath)
        rarFileName = 'release-' + self.getEcmediaVersion() + '.zip' 
        print os.system('scp -i id_rsa.jenkins ' + rarFileName + ' jenkins@192.168.179.129:/app/userhome/jenkins/release/ecmedia/' + platform)