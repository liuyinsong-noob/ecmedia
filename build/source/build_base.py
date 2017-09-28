# -*- coding:utf-8 -*-

import os
import sys
import time

class BuildBase:
    def __init__(self, buildType, platform, projectPath):
        self.buildType = buildType
        self.platform = platform
        
        self.ProjectPath = projectPath
        self.EcmediaCpp = os.path.join(self.ProjectPath, 'ECMedia', 'source', 'ECMedia.cpp')
        self.EcmediaHeader = os.path.join(self.ProjectPath, 'ECMedia', 'interface', 'ECMedia.h')
        self.CommonTypesHeader = os.path.join(self.ProjectPath, 'module', 'common_types.h')
        self.SdkCommonHeader = os.path.join(self.ProjectPath, 'module', 'sdk_common.h')
        self.TypesDefsHeader = os.path.join(self.ProjectPath, 'module', 'typedefs.h')
        self.BaseHeader = os.path.join(self.projectPath, 'base', 'checks.h')
        self.SafeCompareHeader = os.path.join(self.projectPath, 'base', 'safe_compare.h')
        self.ReleaseNoteFile = os.path.join(self.ProjectPath, 'ReleaseNotes.txt')

        self.BuildPath = os.path.join(self.ProjectPath, 'build')
        self.RarPath = os.path.join(self.ProjectPath, 'build', self.buildType)
        self.RarIncludePath = os.path.join(self.RarPath, 'include')
        self.RarLibsPath = os.path.join(self.RarPath, 'libs')
        self.RarX32LibsPath = os.path.join(self.RarPath, 'libs', 'x32')
        self.RarX64LibsPath = os.path.join(self.RarPath, 'libs', 'x64')
        timestamp = time.strftime('%Y-%m-%d-%H-%M-%S',time.localtime(time.time()))
        self.rarFileName = self.buildType + '-' + self.getEcmediaVersion() + '-' + timestamp + '.zip'
    
    def run(self):
        self.build()
        self.collectFiles()
        self.buildAudioOnly()
        self.collectAudioOnlyFiles()
        version, timestamp, sha = self.getLastCommitInfo()
        timestamp = str(int(timestamp) + 1)
        if self.buildType == 'release':
            if self.getEcmediaVersion() != version:
                self.writeReleaseNote(timestamp, sha)
                self.updateReleaseNote()
        self.rarFiles()
        self.copyToRemote(self.platform)

    def build():
        pass
    
    def buildAudioOnly():
        pass
    
    def collectLibFiles(self):
        pass
    
    def collectLibAudioOnlyFiles(self):
        pass
      
    def collectHeaderFiles(self):
        if os.path.exists(self.RarIncludePath):
           pass
        else:
           os.mkdir(self.RarIncludePath)

        print os.system('cp ' + self.EcmediaHeader + ' ' + self.RarIncludePath)
        print os.system('cp ' + self.CommonTypesHeader + ' ' + self.RarIncludePath)
        print os.system('cp ' + self.SdkCommonHeader + ' ' + self.RarIncludePath)
        print os.system('cp ' + self.TypesDefsHeader + ' ' + self.RarIncludePath)
		print os.system('cp ' + self.BaseHeader + ' ' + self.RarIncludePath)
        print os.system('cp ' + self.SafeCompareHeader + ' ' + self.RarIncludePath)   
        
    def collectFiles(self):
        if os.path.exists(self.RarPath):
           pass
        else:
           os.mkdir(self.RarPath)
           
        if os.path.exists(self.RarLibsPath):
           pass
        else:
           os.mkdir(self.RarLibsPath)

        self.collectLibFiles()
        self.collectHeaderFiles()
        
    def collectAudioOnlyFiles(self):
        if os.path.exists(self.RarPath):
           pass
        else:
           os.mkdir(self.RarPath)

        self.collectLibAudioOnlyFiles()

    def rarFiles(self):
        os.chdir(self.BuildPath)
        targetFile = os.path.join(self.BuildPath, self.rarFileName)
        sourceFile = os.path.join(self.BuildPath, self.buildType)
        print os.system('7z a -tzip ' + targetFile + ' ' + sourceFile)

    def getEcmediaVersion(self):
        fd = open(self.EcmediaCpp)
        version = None
        for line in fd.readlines():
            if line.startswith('#define ECMEDIA_VERSION'):
                version = line.split(' ')[-1]
        return version.strip('\r\n').strip('\n').strip('"')

    def getLatestSHA(self, timestamp):
        if os.path.exists(self.ProjectPath):                 
            os.chdir(self.ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        outputStr = output.read().split('\n')
        for line in outputStr:
            if line.startswith('logstart'):
                latestSHA = line.split('-')[1]
                return latestSHA

    def getLatestTimestamp(self, timestamp):
        if os.path.exists(self.ProjectPath):                 
            os.chdir(self.ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        outputStr = output.read().split('\n')
        for line in outputStr:
            if line.startswith('logstart'):
                latestTimestamp = line.split('-')[2]
                return latestTimestamp
                
    def getCommitLogs(self, timestamp):
        if os.path.exists(self.ProjectPath):                 
            os.chdir(self.ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        outputStr = output.read().split('\n')
        commitLogs = []
        for line in outputStr:
            if line.startswith('logstart'):
                commitLog = ''.join(line.split('-')[4:])
                commitLogs.append(commitLog.decode('utf-8'))
        return commitLogs
        
    def isEcmediaHeaderChanged(self, timestamp):
        if os.path.exists(self.ProjectPath):                 
            os.chdir(self.ProjectPath)
            output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
        
        bEcmediaHeaderChanged = False
        if "ECMedia.h" in output:
            bEcmediaHeaderChanged = True
            
        return bEcmediaHeaderChanged
        
    def getLastCommitInfo(self):
        fdOrig = open(os.path.join(self.ProjectPath, 'ReleaseNotes.txt'))
        for line in fdOrig.read().split('\n'):
            if line.startswith('Version'):
                if len(line.split(' ')) == 4:
                    version = line.split(' ')[1]
                    timestamp = line.split(' ')[2]
                    sha = line.split(' ')[3]
                    return (version, timestamp, sha)
                else:
                    return (None, None, None)
        
    def writeReleaseNote(self, timestamp, sha):
        fdOrig = open(os.path.join(self.ProjectPath, 'ReleaseNotes.txt'), 'r')
        origContent = fdOrig.read()
        fdOrig.close()
        fdOrig = open(os.path.join(self.ProjectPath, 'ReleaseNotes.txt'), 'wb')

        fdNew = open(os.path.join(self.RarPath, 'ReleaseNotes.txt'), 'wb')
        
        ecmediaVersion = self.getEcmediaVersion()
        latestSHA = self.getLatestSHA(timestamp)
        latestTimestamp = self.getLatestTimestamp(timestamp)
        commitLogs = self.getCommitLogs(timestamp)
        
        insertContent = 'Version' + ' ' + ecmediaVersion
        insertContent = insertContent + ' ' + latestTimestamp + ' ' + latestSHA
        insertContent = insertContent + '\n'

        index = 1
        if self.isEcmediaHeaderChanged(timestamp):
            insertContent = insertContent + (str(index) + '. ' + '接口改变').decode('utf-8').encode('gbk')
            insertContent = insertContent + '\n'
            index = index + 1
        else:
            insertContent = insertContent + (str(index) + '. ' + '接口不变').decode('utf-8').encode('gbk')
            insertContent = insertContent + '\n'
            index = index + 1
        for commitLog in commitLogs:
            if 'bug fix' in commitLog or 'feature' in commitLog:
                insertContent = insertContent + str(index) + '. ' + commitLog.encode('gbk')
                insertContent = insertContent + '\n'
                index = index + 1
            
        insertContent = insertContent + '\n'
        
        fdOrig.write(insertContent)
        fdOrig.write(origContent)
        
        fdNew.write(insertContent)
        fdNew.write(origContent)
    
    def copyToRemote(self, platform):
        os.chdir(self.BuildPath)
        print os.system('scp -i id_rsa.jenkins ' + self.rarFileName + ' jenkins@192.168.179.129:/app/userhome/jenkins/' + self.buildType + '/ecmedia/' + platform)
        
    def updateReleaseNote(self):
        print os.system('git commit -a -m ' + '"updste release note for %s"'%(self.getEcmediaVersion()))
        print os.system('git push')
        
        
