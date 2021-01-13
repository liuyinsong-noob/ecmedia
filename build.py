# -*- coding:utf-8 -*-

import os
import sys
import getopt
import tarfile
import platform
import requests,zipfile 
ZIP_UNIX_SYSTEM = 3                 
def download_file(url, save_path, chunk_size=128):
    r = requests.get(url, stream=True)
    if r.status_code <> 200:
        return False
    with open(save_path, 'wb') as fd:
        for chunk in r.iter_content(chunk_size=chunk_size):
            fd.write(chunk)
    return True           

#dos_path 最长260字节，
def winapi_path(dos_path):
    if platform.system() <> "Windows":
        return dos_path
    path = os.path.abspath(dos_path)
    if path.startswith("\\\\"):
        path = "\\\\?\\UNC\\" + path[2:]
    else:
        path = "\\\\?\\" + path 

    return path  
   

def extract_all_with_permission(zf, target_dir):
  for info in zf.infolist():
    extracted_path = zf.extract(info, target_dir)
    #print "extract ", info.filename
    if platform.system() == "Windows":
        continue
    if info.create_system == ZIP_UNIX_SYSTEM:
      unix_attributes = info.external_attr >> 16
      if unix_attributes:
        os.chmod(extracted_path, unix_attributes)    
    
def unzip_file(zip_filename, path):
    path = winapi_path(path)
    print "path is ",path
    if zipfile.is_zipfile(zip_filename): 
        with zipfile.ZipFile(zip_filename, 'r') as zipf:
            extract_all_with_permission(zipf,path)
            #zipf.extractall(path)
            '''
            for name in zipf.namelist():
                mylist = name.split('/')
                mylist.pop(0)
                tmp_dir = "/".join(mylist)
         
                f_handle = open( +name,"wb")
                f_handle.write(myzip.read(name))
            f_handle.close()
         '''
    else:
         print "error open zipfile",zip_filename
         sys.exit(-1)

def untarzip_file(targz_file,path):
    tf = tarfile.open(targz_file)
    tf.extractall(path)
    tf.close()

def download_extract(url,target_path):
    filename = os.path.basename(url)
    print "Start download ",url
    if os.path.exists(filename) == False:
         if download_file(url,filename) == False:
            print "Download failure"
            sys.exit(-1)
    else:
        print "Found local file ", filename, " don't download"
    print "Unzip ...",filename
    if filename.endswith("tar.gz"):
       untarzip_file(filename,target_path)
    else:
       unzip_file(filename,target_path)

def download_deps(target_os):
    #下载webrtc的第三方依赖库
    download_root_url = "http://192.168.182.122/chfs/shared/webrtc/"
    webrtc_dep_url = download_root_url + "webrtc_deps.zip"
    webrtc_dep_android_url = download_root_url + "webrtc_deps_android.tar.gz"
    webrtc_cipd_url = download_root_url + "cipd.zip"
    if os.path.exists("resources") == False:
        download_extract(webrtc_dep_url,"./")
    if  target_os == "android" or target_os == "linux":
        if os.path.exists("third_party/android_ndk") == False:
           download_extract(webrtc_dep_android_url,"./")
        if os.path.exists("../.cipd") == False:
           download_extract(webrtc_cipd_url,"../")
           
def gn_project(target_os,target_cpu):
    download_deps(target_os)
    gn_param = []
    gn_param.append('is_debug=false')
    gn_param.append('rtc_include_tests=false')
    gn_param.append('target_cpu=\\\"'+target_cpu+'\\\"')
    gn_param.append('target_os=\\\"'+target_os+'\\\"')
    if  target_os == "win" or target_os == "linux":
        gn_param.append('rtc_use_h264=true')
        gn_param.append('proprietary_codecs=true')
        gn_param.append('ffmpeg_branding=\\\"Chrome\\\"')
    elif target_os == "ios":
        gn_param.append('ios_enable_code_signing=false')
        
    if  platform.system() == "Windows":
        gn_cmd = 'buildtools\win\gn gen out/release --ide=vs2017 --args="' +" ".join(gn_param)+'"'
    elif platform.system() == "Darwin":
        gn_cmd = 'buildtools/mac/gn gen out/default --args="' +" ".join(gn_param)+'"'
    else: 
        gn_cmd = 'buildtools/linux64/gn gen out/default --args="' +" ".join(gn_param)+'"'
    print "Excute gn command: ",gn_cmd
    if os.system(gn_cmd) <> 0:
      sys.exit(-1)
           

def build_project():
   #gn 生成可编译的工程
    #ninja 编译工程
    ninja_cmd = os.path.normpath('third_party/depot_tools/ninja') +' -C out/release ECMedia'
    print "Excute ninjia command: ",ninja_cmd
    if os.system(ninja_cmd) <> 0:
      sys.exit(-1)
               
if __name__=='__main__' :
    target_os = ''
    target_cpu = ''
    build_only = False
    try:
      opts, args = getopt.getopt(sys.argv[1:],"hb",["help","target_os=","target_cpu="])
    except getopt.GetoptError:
      print sys.argv[0],' --target_ios ["android","ios","win","mac","linux"] --target_cpu ["x86","x64","arm64"]'
      sys.exit(2)
    for opt, arg in opts:
      if opt in('-h',"--help"):
         print sys.argv[0],' --target_ios ["android","ios","win","mac","linux"] --target_cpu ["x86","x64","arm64"]'
         sys.exit()
      elif opt == '-b':
         build_only = True
      elif opt == "--target_os":
         target_os = arg
      elif opt == "--target_cpu":
         target_cpu = arg

    if build_only:
       build_project()
       sys.exit(0)
   
    if target_os == "" :
       if  platform.system() == "Windows":
           target_os = "win"
       elif platform.system() == "Darwin":
           target_os = "ios"
       elif platform.system() == "Linux":            
           target_os = "android"
           
    if target_cpu == "":
       if target_os in("win","linux") :
            target_cpu = "x86"
       elif target_os in ("android","ios"):
            target_cpu = "arm64"               
    print "target_os is ",target_os, " target_cpu is ",target_cpu    
    gn_project(target_os,target_cpu)
    build_project()
