# -*- coding:utf-8 -*-

import os
import sys
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
def build_src():
    #下载webrtc的第三方依赖库
    webrtc_dep_url = "http://192.168.182.122/chfs/shared/webrtc/webrtc_deps.zip"
    webrtc_dep_android_url = "http://192.168.182.122/chfs/shared/webrtc/webrtc_deps_android.tar.gz"
    webrtc_cipd_url = "http://192.168.182.122/chfs/shared/webrtc/cipd.zip"
    if os.path.exists("resources") == False:
        download_extract(webrtc_dep_url,"./")
    if  platform.system() == "Linux":
        if os.path.exists("third_party/android_ndk") == False:
           download_extract(webrtc_dep_android_url,"./")
        if os.path.exists("../.cipd") == False:
           download_extract(webrtc_cipd_url,"../")
    #gn 生成可编译的工程
    gn_param = []
    gn_param.append('is_debug=false')

    if  platform.system() == "Windows":
        gn_param.append('rtc_use_h264=true')
        gn_param.append('proprietary_codecs=true')
        gn_param.append('target_cpu=\\\"x86\\\"')
        gn_param.append('ffmpeg_branding=\\\"Chrome\\\"')
    elif platform.system() == "Darwin":
        gn_param.append('target_cpu=\\\"arm64\\\"')
        gn_param.append('target_os=\\\"ios\\\"')
        gn_param.append('rtc_include_tests=false')
        gn_param.append('ios_enable_code_signing=false')
    else:
        gn_param.append('target_cpu=\\\"arm64\\\"')
        gn_param.append('target_os=\\\"android\\\"')
        gn_param.append('rtc_include_tests=false')
        
    if  platform.system() == "Windows":
        gn_cmd = 'buildtools\win\gn gen out/default --args="' +" ".join(gn_param)+'"'
    elif platform.system() == "Darwin":
        gn_cmd = 'buildtools/mac/gn gen out/default --args="' +" ".join(gn_param)+'"'
    else: 
        gn_cmd = 'buildtools/linux64/gn gen out/default --args="' +" ".join(gn_param)+'"'
    print "Excute gn command: ",gn_cmd
    if os.system(gn_cmd) <> 0:
      sys.exit(-1)
    #ninja 编译工程
    print "Excute ninjia compile source "
    if os.system(os.path.normpath('third_party/depot_tools/ninja') +' -C out/default ecmedia') <> 0:
      sys.exit(-1)
               
if __name__=='__main__' :
    build_src()
