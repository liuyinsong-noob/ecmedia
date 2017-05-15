# -*- coding:utf-8 -*-

import os
import sys

ProjectPath = os.getcwd() + '\..'
CompilePath= ProjectPath + '\\demo_Win'
EcmediaCpp = ProjectPath + '\\ECMedia\\source\\ECMedia.cpp'

CommonTypesHeader = ProjectPath + '\\module\\common_types.h'
SdkCommonHeader = ProjectPath + '\\module\\sdk_common.h'
TypesDefsHeader = ProjectPath + '\\module\\typedefs.h'

ReleaseNoteFile = ProjectPath + '\\ReleaseNotes.txt'

ReleaseFiles = ProjectPath + '\\demo_Win\\build\\Win32\\Release'
RarPath = ProjectPath + '\\build\\release' 
BuildPath = ProjectPath + '\\build'
    
def build():
	if os.path.exists(CompilePath):
		os.chdir(CompilePath)
		print os.system('devenv.com MyWebRtc.sln /Project ECMedia /Build')
	else:
		print'%s are not exist!'%CompilePath
        
def copyFiles():
    if os.path.exists(RarPath):
       pass
    else:
       os.mkdir(RarPath)
    print os.system('copy ' + ReleaseFiles + '\Ecmedia.* ' + RarPath)
    print os.system('copy ' + ReleaseFiles + '\libx264-148.dll ' + RarPath)
    print os.system('copy ' + CommonTypesHeader + ' ' + RarPath)
    print os.system('copy ' + SdkCommonHeader + ' ' + RarPath)
    print os.system('copy ' + TypesDefsHeader + ' ' + RarPath)

def rarFiles():
    os.chdir(BuildPath)
    print os.system('7z a -tzip release.zip release')

def getEcmediaVersion():
    fd = open(EcmediaCpp)
    version = None
    for line in fd.readlines():
        if line.startswith('#define ECMEDIA_VERSION'):
	    version = line.split(' ')[-1]
    return version.strip('\n').strip('"')

def getLatestSHA(timestamp):
    if os.path.exists(ProjectPath):                 
        os.chdir(ProjectPath)
        output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
    outputStr = output.read().split('\n')
    for line in outputStr:
        if line.startswith('logstart'):
            latestSHA = line.split('-')[1]
            return latestSHA

def getLatestTimestamp(timestamp):
    if os.path.exists(ProjectPath):                 
        os.chdir(ProjectPath)
        output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
    outputStr = output.read().split('\n')
    for line in outputStr:
        if line.startswith('logstart'):
            latestTimestamp = line.split('-')[2]
            return latestTimestamp
			
def getCommitLogs(timestamp):
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
	
def isEcmediaHeaderChanged(timestamp):
    if os.path.exists(ProjectPath):                 
        os.chdir(ProjectPath)
        output = os.popen('git log --after=' + timestamp + ' --pretty=format:"logstart-"%H-%ct-%cn-%s --no-merges --stat')
	
    bEcmediaHeaderChanged = False
    if "ECMedia.h" in output:
        bEcmediaHeaderChanged = True
		
    return bEcmediaHeaderChanged
	
def getLastCommitInfo():
    fdOrig = open(ProjectPath + '\\ReleaseNotes.txt')
    for line in fdOrig.read().split('\n'):
        if line.startswith('Version'):
            if len(line.split(' ')) == 4:
                timestamp = line.split(' ')[2]
                sha = line.split(' ')[3]
                return (timestamp, sha)
            else:
                return (None, None)
	
def writeReleaseNote(timestamp, sha):
    fdOrig = open(ProjectPath + '\\ReleaseNotes.txt')
    origContent = fdOrig.read()

    ecmediaVersion = getEcmediaVersion()
    latestSHA = getLatestSHA(timestamp)
    latestTimestamp = getLatestTimestamp(timestamp)
    commitLogs = getCommitLogs(timestamp)

    fdNew = open(RarPath + '\ReleaseNotes.txt', 'wb')
    fdNew.write('Version' + ' ' + ecmediaVersion)
    fdNew.write(' ' + latestTimestamp + ' ' + latestSHA)
    fdNew.write('\n')

    index = 1
    if isEcmediaHeaderChanged(timestamp):
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
		
if __name__=='__main__' :
    build()
    copyFiles()
    timestamp, sha = getLastCommitInfo()
    timestamp = str(int(timestamp) + 1)
    writeReleaseNote(timestamp, sha)
    rarFiles()
	