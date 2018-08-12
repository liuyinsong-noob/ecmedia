//
//  ECLiveStream.cpp
//  ECMedia
//
//  Created by james on 16/9/20.
//  Copyright © 2016年 Cloopen. All rights reserved.
//

#include <stdio.h>
#include "ECMedia.h"
#include "ECLiveStream.h"
#include "librtmp/rtmp.h"
//#include "rtmp_sys.h"
#include "librtmp/log.h"
#include <string>
#include <ctype.h>
#include "ECMedia.h"
#include "voe_base.h"
//#include "sometools.h"
//#include "serphoneinterface.h"
#include "voe_volume_control.h"
#include "../system_wrappers/include/trace.h"
#include "voe_file.h"
#include "voe_encryption.h"
#include "voe_network.h"
#include "voe_audio_processing.h"
#include "voe_codec.h"
#include "voe_rtp_rtcp.h"
#include "voe_hardware.h"
#include "voe_dtmf.h"

#ifdef WIN32
#include "codingHelper.h"
#endif
#ifdef VIDEO_ENABLED
#include "vie_network.h"
#include "vie_base.h"
#include "vie_capture.h"
#include "vie_file.h"
#include "vie_render.h"
#include "vie_codec.h"
#include "vie_rtp_rtcp.h"
#include "RecordVoip.h"
#include "../common_video/source/libyuv/include/webrtc_libyuv.h"
#endif

#include "rtp_rtcp_defines.h"
#include "sdk_common.h"
//#include "base64.h"
using namespace yuntongxunwebrtc;
extern yuntongxunwebrtc::VoiceEngine* m_voe;

#ifdef VIDEO_ENABLED
extern yuntongxunwebrtc::VideoEngine* m_vie;
#endif




/*
* 当libjpeg-turbo为vs2010编译时，vs2015下静态链接libjpeg-turbo会链接出错:找不到__iob_func,
* 增加__iob_func到__acrt_iob_func的转换函数解决此问题,
* 当libjpeg-turbo用vs2015编译时，不需要此补丁文件
*/
#if _MSC_VER>=1900
#include "stdio.h" 
_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned);
#ifdef __cplusplus 
extern "C"
#endif 
FILE* __cdecl __iob_func(unsigned i) {
	return __acrt_iob_func(i);
}
#endif /* _MSC_VER>=1900 */



namespace yuntongxunwebrtc {
    
    
    ECMedia_LiveStream::ECMedia_LiveStream()
    {

    }
    
    int ECMedia_LiveStream::Init()
    {        
        return 0;
    }
    
    RTMPLiveSession* ECMedia_LiveStream::CreateLiveStream(int type)
    {
#ifndef _WIN32
		signal(SIGPIPE, SIG_IGN);
#endif
		if (type != 0)
			return NULL;

        if(!m_voe) {
            ECMedia_init_audio();
        }
        if(!m_vie) {
            ECMedia_init_video();
        }
        
        RTMPLiveSession* pLiveSession = RTMPLiveSession::CreateRTMPSession(m_voe, m_vie);
        return pLiveSession;
    }
}



