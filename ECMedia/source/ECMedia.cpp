//
//  ECMedia.c
//  servicecoreVideo
//
//  Created by Sean Lee on 15/6/8.
//
//

#include <string>
#include <ctype.h>
#ifdef  WIN32      //for locating memory leak under windows platform added by zdm
#include   <stdlib.h>
#include   <crtdbg.h>
#endif

#ifdef WEBRTC_ANDROID
#include <android/log.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "ECMedia.h"
#include "voe_base.h"
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
#include "statsCollector.h"
#include "VoeObserver.h"
#include "amrnb_api.h"

#ifdef WIN32
#include "curl_post.h"
#include "codingHelper.h"
#include  <direct.h>
#endif

#ifdef VIDEO_ENABLED
#include "ec_live_engine.h"
#include "vie_network.h"
#include "vie_base.h"
#include "vie_capture.h"
#include "vie_file.h"
#include "vie_render.h"
#include "vie_codec.h"
#include "vie_rtp_rtcp.h"
#include "vie_desktop_share.h"
#include "RecordVoip.h"
#include "RecordLocal.h"
#include "../common_video/source/libyuv/include/webrtc_libyuv.h"
#include "vie_image_process.h"
#include "vie_encryption.h"
#endif

#include "../system_wrappers/include/critical_section_wrapper.h"
#include "base64.h"
#include "MediaStatisticsData.pb.h"

#ifdef ENABLE_LIB_CURL
#ifdef WIN32
CurlPost *g_curlpost = nullptr;
#endif
#endif


#define ECMEDIA_VERSION "ecmedia_version: v2.3.3.7"

enum {
    ERR_SDK_ALREADY_INIT = -1000,
    ERR_NO_MEMORY,
    ERR_ENGINE_UN_INIT,
    ERR_INVALID_PARAM,
    ERR_NOT_SUPPORT
};

#define AUDIO_ENGINE_UN_INITIAL_ERROR(ret) \
if(!m_voe) { \
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d m_voe is null, voice engine need be init first, ret = %d.",__FUNCTION__, __LINE__, ret); \
    return ret; \
}

#ifdef VIDEO_ENABLED
    #define VIDEO_ENGINE_UN_INITIAL_ERROR(ret) \
    if(!m_vie) { \
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d m_vie is null, video engine need be init first, ret = %d.",__FUNCTION__, __LINE__, ret);\
        return ret; \
    }
#endif

#ifdef VIDEO_ENABLED
class ECViECaptureObserver : public ViECaptureObserver
{
public:
    ECViECaptureObserver(onEcMediaNoCameraCaptureCb fp);
    virtual ~ECViECaptureObserver() {}
    // This method is called if a bright or dark captured image is detected.
    virtual void BrightnessAlarm(const int capture_id, const Brightness brightness) {}
    
    // This method is called periodically telling the capture device frame rate.
    virtual void CapturedFrameRate(const int capture_id, const unsigned char frame_rate) {}
    
    // This method is called if the capture device stops delivering images to VideoEngine.
    virtual void NoPictureAlarm(const int capture_id, const CaptureAlarm alarm);
    
private:
    bool m_firstStart;
    onEcMediaNoCameraCaptureCb m_callback;
    
};

typedef struct CameraCaptureCbAndObserver_
{
	int counter;
	onEcMediaNoCameraCaptureCb cb;
	ECViECaptureObserver* obsv;

	CameraCaptureCbAndObserver_()
		:counter(0)
		, cb(nullptr)
		, obsv(nullptr)
	{
	}
	CameraCaptureCbAndObserver_(const CameraCaptureCbAndObserver_& rOther) = delete;

	CameraCaptureCbAndObserver_& operator=(const CameraCaptureCbAndObserver_& rOther) = delete;

	CameraCaptureCbAndObserver_(CameraCaptureCbAndObserver_&& rOther)
		:counter(rOther.counter)
		, cb(std::move(rOther.cb))
		, obsv(std::move(rOther.obsv))
	{
		rOther.counter = 0;
		rOther.cb = nullptr;
		rOther.obsv = nullptr;
	}

	CameraCaptureCbAndObserver_ & operator = (CameraCaptureCbAndObserver_&& rOther)
	{
		if (this != &rOther)
		{
			counter = rOther.counter;
			rOther.counter = 0;

			cb = rOther.cb;
			rOther.cb = nullptr;

			if (obsv)
			{
				delete obsv;
				obsv = nullptr;
			}
			obsv = std::move(rOther.obsv);
			rOther.obsv = nullptr;
		}

		return *this;
	}

	~CameraCaptureCbAndObserver_()
	{
		cb = nullptr;
		if (obsv)
		{
			delete obsv;
			obsv = nullptr;
		}
	}

} CameraCaptureCbAndObserver;

class ECViECaptureObserverManager
{
public:
	static ECViECaptureObserverManager* getInst()
	{
		static ECViECaptureObserverManager inst;
		return &inst;
	}

	bool addCaptureObserver(int deviceId, onEcMediaNoCameraCaptureCb cb)
	{
		if (deviceId > 0 && cb != nullptr)
		{
			yuntongxunwebrtc::CritScope lock(&map_lock);
			ECViECaptureObserverMap::iterator it = mapObsv.find(deviceId);
			if (it == mapObsv.end())
			{
				CameraCaptureCbAndObserver cameraObsv;
				cameraObsv.counter++;
				cameraObsv.cb = cb;
				cameraObsv.obsv = new ECViECaptureObserver(cb);
				mapObsv[deviceId] = std::move(cameraObsv);
				return true;
			}
		}
		return false;
	}

	ECViECaptureObserver* requestCaptureObserver(int deviceId)
	{
		yuntongxunwebrtc::CritScope lock(&map_lock);
		ECViECaptureObserverMap::iterator it = mapObsv.find(deviceId);
		if (it != mapObsv.end())
		{
			it->second.counter++;
			return it->second.obsv;
		}
		return nullptr;
	}

	bool releaseCaptureObserver(int deviceId)
	{
		return removeCaptureObserver(deviceId);
	}

	bool removeCaptureObserver(int deviceId)
	{
		yuntongxunwebrtc::CritScope lock(&map_lock);
		ECViECaptureObserverMap::iterator it = mapObsv.find(deviceId);
		if (it != mapObsv.end())
		{
			it->second.counter--;
			if (it->second.counter == 0)
			{
				mapObsv.erase(it);
				return true;
			}
		}
		return false;
	}

	void clearCaptureObserver()
	{
		yuntongxunwebrtc::CritScope lock(&map_lock);
		ECViECaptureObserverMap::iterator it = mapObsv.begin();
		while (it != mapObsv.end())
		{
			mapObsv.erase(it++);
		}
	}

private:
	ECViECaptureObserverManager() {};
	~ECViECaptureObserverManager()
	{
		clearCaptureObserver();
	};
private:
	yuntongxunwebrtc::CriticalSection map_lock;
	typedef std::map<int, CameraCaptureCbAndObserver> ECViECaptureObserverMap;
	ECViECaptureObserverMap mapObsv;
};

ECViECaptureObserver::ECViECaptureObserver(onEcMediaNoCameraCaptureCb fp) :
                                                        m_firstStart(true),
                                                            m_callback(fp) {
    
}

void ECViECaptureObserver::NoPictureAlarm(const int capture_id, const CaptureAlarm alarm)
{
    if (!alarm && m_firstStart)//when start, the alarm=0
    {
        m_firstStart = false;
    }
    else
    {
        m_callback(capture_id, alarm);
    }
}
#endif

yuntongxunwebrtc::VoiceEngine* m_voe = NULL;
static StatsCollector *g_statsCollector = NULL;
static VoeObserver* g_VoeObserver = NULL;
static bool trace_log_header = true;
#ifdef WIN32
bool g_bGlobalAudioInDevice = false;
HWAVEIN g_hWaveIn = NULL;
#endif

#ifdef VIDEO_ENABLED
yuntongxunwebrtc::VideoEngine* m_vie = NULL;
static RecordVoip* g_recordVoip = NULL;
static RecordLocal* g_recordLocal = NULL;
static unsigned char* g_snapshotBuf = NULL;
static int g_CaptureDeviceId = -1;
ScreenList m_screenlist;
ScreenID *m_pScreenlist=NULL;
WindowList m_windowlist;
WindowShare *m_pWindowlist=NULL;
#endif


#ifdef ENABLE_LIB_CURL
char g_accountId[128]={'\0'};
#endif

static char gVersionString[1024]={'\0'};

using namespace yuntongxunwebrtc;
using namespace std;


bool g_media_TraceFlag                  = false;
const char * g_log_media_filename       = "./mediaConsole.log";
FILE *g_media_interface_fp              = NULL;

#define MAX_LOG_LINE    3000
#define MAX_LOG_SIZE    104857600   //100M Bytes
long long g_max_log_size = MAX_LOG_SIZE;

typedef void(*PrintConsoleHook_media)(int loglevel, const char *);
PrintConsoleHook_media gPrintConsoleHook_media = NULL;
yuntongxunwebrtc::CriticalSectionWrapper  *g_printConsole_lock;
static void media_init_print_log()
{
    if (!g_media_TraceFlag) {
        return;
    }
    g_printConsole_lock = yuntongxunwebrtc::CriticalSectionWrapper::CreateCriticalSection();
    if (NULL == g_media_interface_fp)
    {
        g_media_interface_fp = fopen(g_log_media_filename, "ab");
        //   g_media_interface_fp = fopen(g_log_media_filename,"wb");
        if (NULL == g_media_interface_fp)
        {
            g_media_interface_fp = fopen(g_log_media_filename, "wb");
        }
    }
}

static void media_uninit_print_log()
{
    if (!g_printConsole_lock) {
        return;
    }
    {
        yuntongxunwebrtc::CriticalSectionScoped lock(g_printConsole_lock);
        if (g_media_interface_fp)
            fclose(g_media_interface_fp);
        g_media_interface_fp = NULL;
    }
    
    if (g_printConsole_lock)
        delete g_printConsole_lock;
    g_printConsole_lock = NULL;
}

void WriteLogToFile(const char * fmt, ...)
{  
    if (!g_media_TraceFlag) {
        return;
    }
    char log_buffer[2048] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(log_buffer, 2047, fmt, ap);
    va_end(ap);
    
#ifdef WEBRTC_ANDROID
    __android_log_print(ANDROID_LOG_DEBUG, "console", "%s", log_buffer);
#else
    printf("%s", log_buffer);
#endif
    
    if (gPrintConsoleHook_media)
        gPrintConsoleHook_media(0, log_buffer);
    
    if (NULL == g_printConsole_lock) {
        return;
    }
    yuntongxunwebrtc::CriticalSectionScoped lock(g_printConsole_lock);
    if (g_media_interface_fp) {
        fprintf(g_media_interface_fp, "%s\n", log_buffer);
        if (ftell(g_media_interface_fp) >= g_max_log_size)//100M,100*1024*1024
        {
            fclose(g_media_interface_fp);
            g_media_interface_fp = NULL;
            
            char new_file[1024];
            strcpy(new_file, g_log_media_filename);
            char file_postfix[40] = { 0 };
        
            struct tm *pt = NULL;
            time_t curr_time;
            curr_time = time(NULL);
        
#ifdef WIN32
            pt = localtime(&curr_time);
#else
            struct tm t1;
            pt = localtime_r(&curr_time, &t1);
#endif
            if (!pt)
                return;
            sprintf(file_postfix, "-%04d-%02d-%02d_%02d-%02d-%02d.bak",
                    pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday,
                    pt->tm_hour, pt->tm_min, pt->tm_sec);
            strcat(new_file, file_postfix);
            rename(g_log_media_filename, new_file);
            
            g_media_interface_fp = fopen(g_log_media_filename, "wb");
        }
    }
}

const char* ECMedia_get_Version()
{
    if( strlen(gVersionString) <= 0 )
    {
        const char *platform ="unknow", *arch = "arm", *voice="voice=false", *video="video=false";
#if defined(WEBRTC_ANDROID)
        platform = "Android";
#elif defined(WIN32)
        platform = "Windows";
#elif defined(WEBRTC_IOS)
        platform ="iOS";
#elif defined(WEBRTC_MAC)
        platform ="Mac OS";
#elif defined(WEBRTC_LINUX)
        platform ="Linux";
#endif
        
#if defined WEBRTC_ARCH_ARM_V7A
        arch = "armv7a";
#elif defined WEBRTC_ARCH_ARM_V5
        arch = "armv5";
#elif defined WEBRTC_ARCH_ARM64_V8A
        arch = "armv8a";
#endif
        
#ifndef NO_VOIP_FUNCTION
        voice="voice=true";
#ifdef VIDEO_ENABLED
        video="video=true";
#endif
#endif
        sprintf(gVersionString,
                "%s#%s#%s#%s#%s#%s %s"
                ,ECMEDIA_VERSION,
                platform,
                arch,
                voice,
                video,
                __DATE__,
                __TIME__);
    }
    return gVersionString;
}

namespace yuntongxunwebrtc {
    class ECMediaTraceCallBack : public TraceCallback {
    public:
        virtual void Print(const TraceLevel level,
                           const char *traceString,
                           const int length)
        {
            WriteLogToFile("%s\n",traceString);
        }
    };
}

yuntongxunwebrtc::ECMediaTraceCallBack g_mediaTraceCallBack;

void ECMedia_trace_log_header() {
    if(trace_log_header) {
        trace_log_header = false;
        WEBRTC_TRACE(kTraceStateInfo, kTraceMediaApi, 0, "%s",
                     "************************************ ECMEDIA LOG BEGIN ************************************");
        WEBRTC_TRACE(kTraceStateInfo, kTraceMediaApi, 0, "%s",
                     "**                                                                                       **");
        WEBRTC_TRACE(kTraceStateInfo, kTraceMediaApi, 0, "**    %s    **",
                     ECMedia_get_Version());
        WEBRTC_TRACE(kTraceStateInfo, kTraceMediaApi, 0, "%s",
                     "**                                                                                       **");
        WEBRTC_TRACE(kTraceStateInfo, kTraceMediaApi, 0, "%s",
                     "*******************************************************************************************");
    }
}

void ECMedia_reset_state() {
    if(trace_log_header) {
        trace_log_header = true;
        if(g_media_interface_fp != nullptr) {
            fflush(g_media_interface_fp);
            fclose(g_media_interface_fp);
            g_media_interface_fp = nullptr;
        }
    }
}


int ECMedia_set_trace(const char *logFileName,void *printhoolk, int level, int lenMb)
{
    uint32_t filter = kTraceDefault;
    g_media_TraceFlag = true;
    if(nullptr != printhoolk) {
        gPrintConsoleHook_media=(PrintConsoleHook_media)printhoolk;
    }
    if(nullptr != logFileName)  {
        // todo: need str copy
        g_log_media_filename = logFileName;
    }
    if (lenMb > 0)
        g_max_log_size = lenMb * 1024 * 1024;
    
    media_init_print_log();
  
    Trace::CreateTrace();
    Trace::SetTraceCallback(&g_mediaTraceCallBack);
    
    switch(level) {
        case 20: {
            filter = kTraceCritical;
            break;
        }
        case 21: {
            filter = kTraceError | kTraceCritical;
            break;
        }
        case 22: {
            filter = kTraceError | kTraceCritical | kTraceWarning;
            break;
        }
        case 23: {
            filter = kTraceDefault;
            break;
        }
        case 24: {
            filter = kTraceDefault | kTraceInfo;
            break;
        }
        case 25: {
            filter = kTraceDefault | kTraceInfo | kTraceDebug;
            break;
        }
        default: {
            if(filter > 25) {
                filter = kTraceAll;
            }
            break;
        }
    }
    Trace::set_level_filter(filter);
    
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d, log_name:%s, hook:%p, log level:%d, file size limit:%d MB"
                 ,__FUNCTION__
                 , __LINE__
                 , logFileName
                 , printhoolk
                 , level
                 , lenMb);
    return 0;
}
int ECMedia_un_trace() {
    if (!g_media_TraceFlag) {
        return -1;
    }
    g_media_TraceFlag = false;
    media_uninit_print_log();
    Trace::ReturnTrace();
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void ECMedia_set_android_objects(void* javaVM, void* env, void* context)
{
     WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#if !defined(NO_VOIP_FUNCTION)
    yuntongxunwebrtc::VoiceEngine::SetAndroidObjects(javaVM,env,context);
#ifdef VIDEO_ENABLED
    yuntongxunwebrtc::VideoEngine::SetAndroidObjects(javaVM,env,context);
#endif
#endif
     WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end...", __FUNCTION__, __LINE__);
}

int ECMedia_init_video()
{
    ECMedia_trace_log_header();
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    m_pScreenlist = NULL;
    m_pWindowlist = NULL;
    
    if (m_vie)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d %s Video engine already create", __FUNCTION__, __LINE__);
        return 1;
    }
    

    //VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    m_vie = VideoEngine::Create();
    if ( NULL == m_vie)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d Create Video engine fail", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;;
    }
    ViEBase* videobase = ViEBase::GetInterface(m_vie);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d Init Video Engine...", __FUNCTION__, __LINE__);
    if(videobase->Init()!= 0) {
        int lastError = videobase->LastError(); //base init failed
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d Init Video Engine error, error code is %d", __FUNCTION__, __LINE__, lastError);
        videobase->Release();
        VideoEngine::Delete(m_vie);
        m_vie = NULL;
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return lastError;
    }
    else {
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d Init Video Engine...OK", __FUNCTION__, __LINE__);
    }
    if (m_voe) {
        videobase->SetVoiceEngine(m_voe);
    }
    videobase->Release();
    
    if (!g_statsCollector)
    {
        g_statsCollector = new StatsCollector();
    }
    g_statsCollector->SetVideoEngin(m_vie);
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_uninit_video()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...",__FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if (m_pScreenlist != NULL)
        delete[] m_pScreenlist;
    if (m_pWindowlist != NULL)
        delete[] m_pWindowlist;
    if(!m_vie)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d Video Engine is null",__FUNCTION__, __LINE__);
        return -99;
    }
    ViEBase* videobase = ViEBase::GetInterface(m_vie);
    if ( videobase)
    {
        videobase->Release();
    }
    VideoEngine::Delete(m_vie);
    m_vie= NULL;
    
    if(g_snapshotBuf) {
        free(g_snapshotBuf);
        g_snapshotBuf = NULL;
    }
    
#endif
    if (g_statsCollector)
    {
        delete g_statsCollector;
        g_statsCollector = nullptr;
    }
    ECMedia_reset_state();
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
    
}

int ECMedia_ring_stop(int& channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and Channel ID: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if ( channelid >=0 )
    {
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d Stop play ring, channelID=%d", __FUNCTION__, __LINE__, channelid);
        VoEBase* base = VoEBase::GetInterface(m_voe);
        VoEFile* file  = VoEFile::GetInterface(m_voe);
        if ( file->IsPlayingFileAsMicrophone(channelid) >=0)
        {
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d Stop play ring file locally, channelID=%d", __FUNCTION__, __LINE__, channelid);
            file->StopPlayingFileLocally( channelid );
        }
        //add by gyf to stop play ring file
        base->StopPlayout(channelid);
        base->DeleteChannel( channelid );
        file->Release();
        base->Release();
        channelid = -1;
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... ", __FUNCTION__, __LINE__);
    return 0;
}


//In this func, channel is created automatically. You should store this channel to use in func ring_stop.
int ECMedia_ring_start(int& channelid, const char *filename, bool loop)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d filename: %s loop: %s", __FUNCTION__, __LINE__, channelid, filename, loop?"true":"false");
#ifdef WIN32
    char buffer[1024];
    getcwd(buffer, 1024);
#endif
    FILE *fp  = fopen(filename, "r") ;
    if( fp == NULL ) {
        #ifdef WIN32
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d open file failed, current work path is: %s", __FUNCTION__, __LINE__, buffer);
        #endif
        return 0;
    }
    fclose(fp);
#ifdef WIN32
    ECMedia_init_audio();
#endif
    if (m_voe){
        VoEBase* base = VoEBase::GetInterface(m_voe);
        VoEFile* file  = VoEFile::GetInterface(m_voe);
        
        channelid = base->CreateChannel();
        string strFileName;
#ifdef WIN32
        strFileName = ASCII2UTF_8(filename?filename:"");
#else
        strFileName = filename?filename:"";
#endif
        int ret = -1;
        
        ret = file->StartPlayingFileLocally(channelid, strFileName.c_str(), loop);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d playfile is processing,channelID=%d,name:%s,ret:%d\n", __FUNCTION__, __LINE__, channelid, filename, ret);
        
        if (ret >=0)
        {
            base->StartPlayout(channelid);
        }
        base->Release();
        file->Release();
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

/*
 VoEBase functions
 */

int ECMedia_init_audio()
{
    ECMedia_trace_log_header();
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    if(m_voe)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d %s Voice engine already create", __FUNCTION__, __LINE__);
        return 1;
    }
    m_voe = VoiceEngine::Create();
    
    if ( NULL == m_voe)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d Create Voice engine fail", __FUNCTION__, __LINE__);
        return -99;
    }
    VoEBase* base = VoEBase::GetInterface(m_voe);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d Init Voice Engine...", __FUNCTION__, __LINE__);
    if( base->Init() != 0) {
        VoiceEngine::Delete(m_voe);
        m_voe = NULL;
        
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d Init Voice Engine Error, error code is %d", __FUNCTION__, __LINE__, base->LastError());
        return base->LastError(); //base init failed
    }
    else {
        VoEVolumeControl* volume = VoEVolumeControl::GetInterface(m_voe);
        if(volume){
            volume->SetMicVolume(255);
            volume->Release();
        }
    }
    base->Release();
#ifdef VIDEO_ENABLED
    if (m_vie) {
        ViEBase *viebase = (ViEBase*)ViEBase::GetInterface(m_vie);
        viebase->SetVoiceEngine(m_voe);
        viebase->Release();
    }
#endif
    if (!g_statsCollector)
    {
        g_statsCollector = new StatsCollector();
    }
    g_statsCollector->SetVoiceEngin(m_voe);
#ifdef VIDEO_ENABLED
    if (m_vie) {
        g_statsCollector->SetVideoEngin(m_vie);
    }
#endif
    
#ifdef ENABLE_LIB_CURL
#ifdef    WIN32
    if (!g_curlpost)
    {
        const char* url = "failover:(tcp://192.168.177.186:61616"")";
        std::string username("admin");
        std::string password("admin");
        g_curlpost = new CurlPost(url, username, password);
    }
#endif
#endif
    return 0;
}

int ECMedia_uninit_audio()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef ENABLE_LIB_CURL
#ifdef WIN32
    if (g_curlpost)
    {
        delete g_curlpost;
        g_curlpost = nullptr;
    }
#endif
#endif
    if(!m_voe)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d audio engine is null", __FUNCTION__, __LINE__);
        return -99;
    }
    VoEBase* base = VoEBase::GetInterface(m_voe);
    if ( base)
    {
        base->Terminate();
        base->Release();
    }
    
    VoiceEngine::Delete(m_voe);
    m_voe = NULL;
    
    if(g_VoeObserver) {
        delete g_VoeObserver;
        g_VoeObserver = NULL;
    }
    
    if (g_statsCollector)
    {
        delete g_statsCollector;
        g_statsCollector = nullptr;
    }
    ECMedia_reset_state();
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_audio_create_channel(int& channelid, bool is_video)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., create %s channel, channelid is: %d", __FUNCTION__, __LINE__, is_video?"video":"audio", channelid);
    if (!is_video) {
        AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        VoEBase *base = VoEBase::GetInterface(m_voe);
        if (base) {
            channelid = base->CreateChannel();
            base->Release();
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends with channelid %d just created...", __FUNCTION__, __LINE__, channelid);
            return 0;
        }
        else
        {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
            channelid = -1;
            return -99;
        }
    }
    else
    {
#ifdef VIDEO_ENABLED
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViEBase *base = ViEBase::GetInterface(m_vie);
        if (base) {
            base->CreateChannel(channelid);
            base->Release();
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends with video channelid %d just created...", __FUNCTION__, __LINE__, channelid);
            return 0;
        }
        else
        {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
            channelid = -1;
            return -99;
        }
#endif
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
}

bool ECMedia_get_recording_status()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        bool ret = base->GetRecordingIsInitialized();
        base->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends.. with code:%d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_delete_channel(int& channelid, bool is_video)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., delete %s channel, channelid:%d",__FUNCTION__, __LINE__, is_video?"video":"audio", channelid);
    if (!is_video) {
        AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        VoEBase *base = VoEBase::GetInterface(m_voe);
        if (base) {
            int ret = base->DeleteChannel(channelid);
            if (ret == 0) {
                channelid = -1;
            }
            else
            {
                WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to delete channel:%d, ret:%d",__FUNCTION__, __LINE__, channelid, ret);
            }
            base->Release();
            return ret;
        }
        else
        {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
            return -99;
        }
    } else {
#ifdef VIDEO_ENABLED
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViEBase *base = ViEBase::GetInterface(m_vie);
        if (base) {
            int ret = base->DeleteChannel(channelid);
            if (ret == 0) {
                channelid = -1;
            } else {
                WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%dfailed to delete channel:%d, ret:%d", __FUNCTION__, __LINE__, channelid, ret);
            }
            base->Release();
            return ret;
        } else {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
            return -99;
        }
#endif
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
}

int ECMedia_audio_set_ssrc(int channelid, unsigned int localssrc, unsigned int remotessrc)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid:%d, localssrc: %u, remotessrc %u", __FUNCTION__, __LINE__,channelid, localssrc, remotessrc);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp)
    {
        int localret = 0, remoteret = 0;
        if (localssrc != 0) {
            localret = rtp_rtcp->SetLocalSSRC(channelid, localssrc);
        }
        if (remotessrc != 0) {
            remoteret = rtp_rtcp->SetRemoteSSRC(channelid, remotessrc);
        }
        
        rtp_rtcp->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with localssrc set: %d, remotessrc set %d", __FUNCTION__, __LINE__, localret, remoteret);
        return (localret + remoteret);
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to set video ssrc", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_send_audiolevel_status(int channelid, bool enable, unsigned char id)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid:%d, enable: %d, id: %d", __FUNCTION__, __LINE__,channelid, enable, id);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp)
    {
        int ret = rtp_rtcp->SetSendAudioLevelIndicationStatus(channelid, enable, id);
        rtp_rtcp->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with ret:%d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to set send audio level", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_receive_audiolevel_status(int channelid, bool enable, unsigned char id)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid:%d, enable: %d, id: %d", __FUNCTION__, __LINE__,channelid, enable, id);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp)
    {
        int ret = rtp_rtcp->SetReceiveAudioLevelIndicationStatus(channelid, enable, id);
        rtp_rtcp->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with ret:%d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to set receive audio level", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_local_receiver(int channelid, int rtp_port, int rtcp_port, bool ipv6)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid:%d rtp_port:%d rtcp_port:%d, ipv6 %s",__FUNCTION__, __LINE__, channelid, rtp_port, rtcp_port, ipv6?"YES":"NO");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetLocalReceiver(channelid, rtp_port, rtcp_port, ipv6);
        base->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d %s failed to get VoEBase",__FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_set_send_destination(int channelid, int rtp_port, const char *rtp_addr, int source_port, int rtcp_port, const char *rtcp_ipaddr)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid:%d rtp_port:%d rtp_addr:%s source_port:%d rtcp_port:%d rtcp_ipaddr:%s",
                 __FUNCTION__, __LINE__, channelid, rtp_port, rtp_addr?"NULL":rtp_addr, source_port, rtcp_port, rtcp_ipaddr);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetSendDestination(channelid, rtp_port, rtp_addr, source_port, rtcp_port, rtcp_ipaddr);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set send destination", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else{
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_set_socks5_send_data(int channel_id, unsigned char *data, int length, bool isRTCP)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel_id);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetSocks5SendData(channel_id, data, length, isRTCP);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set socks5 send data", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else{
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

#ifdef VIDEO_ENABLED
int ECMedia_video_start_receive(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    g_statsCollector->AddVideoRecvStatsProxy(channelid);
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StartReceive(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video start receive", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_stop_receive(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (g_statsCollector) {
        g_statsCollector->DeleteVideoRecvStatsProxy(channelid);
    }
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StopReceive(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video stop receive", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_start_send(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StartSend(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video start send", __FUNCTION__, __LINE__);
        }
		else
			g_statsCollector->AddVideoSendStatsProxy(channelid);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_stop_send(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
   
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StopSend(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video stop send", __FUNCTION__, __LINE__);
        }
		else
		{
			if (g_statsCollector) {
				g_statsCollector->DeleteVideoSendStatsProxy(channelid);
			}
		}
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}
#endif
int ECMedia_audio_start_playout(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartPlayout(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio start playout", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_stop_playout(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopPlayout(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio stop playout", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_audio_start_record()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...",__FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartRecord();
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio start record", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase",__FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_stop_record()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopRecord();
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio stop record", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_soundcard_on_cb(onSoundCardOn soundcard_on_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->RegisterSoundCardOnCb(soundcard_on_cb);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set soundcard on cb", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_send_telephone_event_payload_type(int channelid, unsigned char type)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d type: %d", __FUNCTION__, __LINE__, channelid, type);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEDtmf* dtmf = VoEDtmf::GetInterface(m_voe);
    if (dtmf) {
        int ret = dtmf->SetSendTelephoneEventPayloadType(channelid, type);
        dtmf->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set send telephone event payload type", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_recv_telephone_event_payload_type(int channelid, unsigned char type)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d type: %d", __FUNCTION__, __LINE__, channelid, type);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEDtmf* dtmf = VoEDtmf::GetInterface(m_voe);
    if (dtmf) {
        int ret = dtmf->SetRecvTelephoneEventPayloadType(channelid, type);
        dtmf->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set recv telephone event payload type", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_send_dtmf(int channelid, const char dtmfch)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d, dtmf: %c", __FUNCTION__, __LINE__, channelid, dtmfch);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    int playtone;
    
    if ( isdigit(dtmfch) || dtmfch == '#' || dtmfch == '*' )
    {
        if ( isdigit(dtmfch) )
            playtone = ((int)dtmfch)-48;
        else if ( dtmfch == '*')
            playtone = 10;
        else if ( dtmfch == '#')
            playtone = 11;
        else
            playtone = 12;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d invalid dtmf char %c", __FUNCTION__, __LINE__, dtmfch);
        return -100;
    }
    
    VoEDtmf* dtmf  = VoEDtmf::GetInterface(m_voe);
    
    //dtmf->SendTelephoneEvent(call->m_AudioChannelID, playtone,false);
    if(dtmf){
        int ret = dtmf->SendTelephoneEvent(channelid, playtone, true);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to send telephone event", __FUNCTION__, __LINE__);
        }
        ret = dtmf->PlayDtmfTone(playtone);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to play dtmf tone", __FUNCTION__, __LINE__);
        }
        dtmf->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}


int ECMedia_set_dtmf_cb(int channelid, onEcMediaReceivingDtmf dtmf_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d ", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetDtmfCb(channelid, dtmf_cb);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set dtmf cb", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_set_media_packet_timeout_cb(int channelid, onEcMediaPacketTimeout media_timeout_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetMediaTimeoutCb(channelid, media_timeout_cb);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set media timeout cb", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_stun_cb(int channelid, onEcMediaStunPacket stun_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetStunCb(channelid, stun_cb);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set stun cb", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_audio_data_cb(int channelid, onEcMediaAudioData audio_data_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetAudioDataCb(channelid, audio_data_cb);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set audio data cb", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_pcm_audio_data_cb(int channelid, ECMedia_PCMDataCallBack callback) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetPCMAudioDataCallBack(channelid, callback);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set pcm audio data cb", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_setECMedia_ConferenceParticipantCallback(int channelid, ECMedia_ConferenceParticipantCallback* callback) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->setConferenceParticipantCallback(channelid, callback);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set conference participant", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_setECMedia_ConferenceParticipantCallbackTimeInterVal(int channelid, int timeInterVal) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d, timeInterVal: %d", __FUNCTION__, __LINE__, channelid, timeInterVal);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->setConferenceParticipantCallbackTimeInterVal(channelid, timeInterVal);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set conference participant timeInterVal", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_set_video_data_cb(int channelid, onEcMediaVideoDataV video_data_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
#ifdef VIDEO_ENABLED
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->setVideoDataCb(channelid, video_data_cb);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video data cb", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        return -99;
    }
#endif
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d don't support video", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_set_voe_cb(int channelid, onVoeCallbackOnError voe_callback_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        if(!g_VoeObserver) {
            g_VoeObserver = new VoeObserver();
        }
        g_VoeObserver->SetCallback(channelid, voe_callback_cb);
        int ret = base->RegisterVoiceEngineObserver(*g_VoeObserver);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to register voice engine observer", __FUNCTION__, __LINE__);
        }
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
    
}

int ECMedia_sendRaw(int channelid, int8_t *data, uint32_t length, bool isRTCP, uint16_t port, const char* ip)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... ,channelid %d, data:%0x len:%d isRTCP:%d port:%d ip:%s",
                 __FUNCTION__, __LINE__, channelid, data, length, isRTCP, port, ip);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret=base->SendRaw(channelid, data, length, isRTCP, port, ip);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to send raw", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_start_receive(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    g_statsCollector->AddAudioRecvStatsProxy(channelid);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartReceive(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio start receive", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_stop_receive(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (g_statsCollector) {
        g_statsCollector->DeleteAudioRecvStatsProxy(channelid);
    }
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopReceive(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio stop receive", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_start_send(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    g_statsCollector->AddAudioSendStatsProxy(channelid);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartSend(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio start send", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_audio_stop_send(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (g_statsCollector) {
        g_statsCollector->DeleteAudioSendStatsProxy(channelid);
    }
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopSend(channelid);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio stop send", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_Register_voice_engine_observer(int channelid)
{
    //TODO
    return 0;
}

int ECMedia_DeRegister_voice_engine_observer()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->DeRegisterVoiceEngineObserver();
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to deregister voice engine observer", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

/*
 * AUDIO PROCESSING
 */
int ECMedia_set_AgcStatus(bool agc_enabled, yuntongxunwebrtc::AgcModes agc_mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... agc_enabled=%d agc_mode=%d", __FUNCTION__, __LINE__, agc_enabled, agc_mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetAgcStatus(agc_enabled, agc_mode);
        audio->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set agc status", __FUNCTION__, __LINE__);
        }
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEAudioProcessing", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_EcStatus(bool ec_enabled, yuntongxunwebrtc::EcModes ec_mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... ec_enabled=%d ec_mode=%d", __FUNCTION__, __LINE__, ec_enabled, ec_mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetEcStatus(ec_enabled, ec_mode);
        audio->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set ec status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEAudioProcessing", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_NsStatus(bool ns_enabled, yuntongxunwebrtc::NsModes ns_mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... ns_enabled=%s ns_mode=%d", __FUNCTION__, __LINE__, ns_enabled?"true":"false", ns_mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetNsStatus(ns_enabled, yuntongxunwebrtc::kNsVeryHighSuppression);
        audio->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set ns status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEAudioProcessing", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_SetAecmMode(yuntongxunwebrtc::AecmModes aecm_mode, bool cng_enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... aecm_mode=%d cng_enabled=%s", __FUNCTION__, __LINE__, aecm_mode, cng_enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetAecmMode(aecm_mode,  cng_enabled);
        audio->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set aecm mode", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%dfailed to get VoEAudioProcessing", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_EnableHowlingControl(bool enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... enabled:%s", __FUNCTION__, __LINE__, enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->EnableHowlingControl(enabled);
        audio->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable howling control", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEAudioProcessing", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_IsHowlingControlEnabled(bool &enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        enabled = audio->IsHowlingControlEnabled();
        audio->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEAudioProcessing", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

/*
 * NETWORK
 */
//VoENetwork
int ECMedia_set_packet_timeout_noti(int channel, int timeout)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d, timeout: %d", __FUNCTION__, __LINE__, channel, timeout);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        int ret = 0;
        ret = network->SetPacketTimeoutNotification(channel, true, timeout);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set packet timeout notification", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_packet_timeout_noti(int channel, bool& enabled, int& timeout)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        int ret = 0;
        //        ret = network->GetPacketTimeoutNotification(channel, enabled, timeout);
        network->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_network_statistic(int channelid_audio, int channelid_video, long long *duration, long long *sendTotalSim,
                                  long long *recvTotalSim, long long *sendTotalWifi, long long *recvTotalWifi)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid_audio:%d, channelid_video:%d", __FUNCTION__, __LINE__, channelid_audio, channelid_video);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    time_t voe_time = 0;
    time_t vie_time = 0;
    long long voe_send_total_sim =0;
    long long voe_recv_total_sim = 0;
    long long vie_send_total_sim = 0;
    long long vie_recv_total_sim = 0;
    
    long long voe_send_total_wifi =0;
    long long voe_recv_total_wifi = 0;
    long long vie_send_total_wifi = 0;
    long long vie_recv_total_wifi = 0;
    
    long long voe_duration = 0;
    long long vie_duration = 0;
    
    if(channelid_audio >= 0) {
        VoENetwork *network = VoENetwork::GetInterface(m_voe);
        if(network){
            network->getNetworkStatistic(channelid_audio, voe_time, voe_send_total_sim, voe_recv_total_sim, voe_send_total_wifi, voe_recv_total_wifi);
            network->Release();
        }
    }
    
#ifdef VIDEO_ENABLED
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if(channelid_video >= 0) {
        ViENetwork *network = ViENetwork::GetInterface(m_vie);
        if(network){
            network->getNetworkStatistic(channelid_video, vie_time, vie_send_total_sim, vie_recv_total_sim, vie_send_total_wifi, vie_recv_total_wifi);
            network->Release();
        }
    }
#endif
    if(voe_time) {
        voe_duration = time(NULL) - voe_time;
    }
    if(vie_time) {
        vie_duration = time(NULL) - vie_time;
    }
    *duration = voe_duration>vie_duration ? voe_duration:vie_duration;
    *sendTotalSim = voe_send_total_sim  + vie_send_total_sim ;
    *sendTotalWifi = voe_send_total_wifi + vie_send_total_wifi;
    *recvTotalSim = voe_recv_total_sim + vie_recv_total_sim;
    *recvTotalWifi = voe_recv_total_wifi + vie_recv_total_wifi;
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...",__FUNCTION__, __LINE__);
    return 0;
}

//ViENetwork
#ifdef VIDEO_ENABLED
int ECMedia_video_set_local_receiver(int channelid, int rtp_port, int rtcp_port, bool ipv6)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid: %d, rtp_port: %d rtcp_port: %d ipv6: %s", __FUNCTION__, __LINE__, channelid, rtp_port, rtcp_port, ipv6?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetLocalReceiver(channelid, rtp_port, rtcp_port, ipv6);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video set local receiver", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_set_socks5_send_data(int channel_id, unsigned char *data, int length, bool isRTCP)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid: %d ", __FUNCTION__, __LINE__, channel_id);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetSocks5SendData(channel_id, data, length, isRTCP);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set socks5 send data", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_set_send_destination(int channelid, const char *rtp_addr, int rtp_port, const char *rtcp_addr, int rtcp_port)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid: %d, rtp_addr: %s rtp_port: %d rtcp_port: %d rtcp_addr: %s", __FUNCTION__, __LINE__, channelid, rtp_addr, rtp_port, rtcp_port, rtcp_addr);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetSendDestination(channelid, rtp_addr, rtp_port, rtcp_addr , rtcp_port);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video set send destination", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_MTU(int channelid, int mtu)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid: %d, mtu: %d", __FUNCTION__, __LINE__, channelid, mtu);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetMTU(channelid, mtu);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set mtu", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

/*
 * RTP_RTCP
 */
int ECMedia_set_video_rtp_keepalive(int channelid, bool enable, int interval, int payloadType)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d, enable: %s interval: %d payloadType: %d", __FUNCTION__, __LINE__,
                 channelid, enable?"true":"false", interval, payloadType);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret=0;
        ret = rtp_rtcp->SetRTPKeepAliveStatus(channelid, enable, payloadType, interval);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video rtp keepalive", __FUNCTION__, __LINE__);
        }
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d already set or failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_set_local_ssrc(int channelid, unsigned int ssrc)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid: %d, ssrc: %u", __FUNCTION__, __LINE__, channelid, ssrc);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->SetLocalSSRC(channelid, ssrc, kViEStreamTypeNormal, 0);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video set local ssrc", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_request_remote_ssrc(int channelid, unsigned int ssrc)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid: %d, ssrc: %u", __FUNCTION__, __LINE__, channelid, ssrc);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->RequestRemoteSSRC(channelid, ssrc);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video request remote ssrc", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_cancel_remote_ssrc(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d ", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->CancelRemoteSSRC(channelid);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video cancel remote ssrc", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
#endif

int ECMedia_set_audio_rtp_keepalive(int channelid, bool enable, int interval, int payloadType)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d enable: %s interval: %d payloadType: %d", __FUNCTION__, __LINE__,
                 channelid, enable?"true":"false", interval, payloadType);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->SetRTPKeepAliveStatus(channelid, enable, payloadType, interval);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set audio rtp keepalive", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_NACK_status(int channelid, bool enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d, enabled: %s",__FUNCTION__, __LINE__, channelid, enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetNACKStatus(channelid, enabled);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set nack status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_RTCP_status(int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetRTCPStatus(channelid, true);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set rtcp status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_media_statistics(int channelid, bool is_video, MediaStatisticsInfo& call_stats)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    if(channelid == -1) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d wrong channenl id", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    if(!is_video)
    {
        AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
        if(rtp_rtcp){
            yuntongxunwebrtc::CallStatistics stats;
            rtp_rtcp->GetRTCPStatistics(channelid,stats);
            call_stats.bytesReceived = stats.bytesReceived;
            call_stats.bytesSent =stats.bytesSent;
            call_stats.cumulativeLost = stats.cumulativeLost;
            call_stats.extendedMax = stats.extendedMax;
            call_stats.fractionLost = stats.fractionLost;
            call_stats.jitterSamples = stats.jitterSamples;
            call_stats.packetsReceived = stats.packetsReceived;
            call_stats.packetsSent = stats.packetsSent;
            call_stats.rttMs = stats.rttMs;
            rtp_rtcp->Release();
        }
    }
#ifdef VIDEO_ENABLED
    else
    {
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
        if(rtp_rtcp){
            yuntongxunwebrtc::CallStatistics stats;
            //rtp_rtcp->GetSentRTCPStatistics(channelid, stats.fractionLost, stats.cumulativeLost, stats.extendedMax, stats.jitterSamples, stats.rttMs);
            rtp_rtcp->GetReceivedRTCPStatistics(channelid, stats.fractionLost, stats.cumulativeLost, stats.extendedMax, stats.jitterSamples, stats.rttMs);
            rtp_rtcp->GetRTPStatistics(channelid, stats.bytesSent, stats.packetsSent, stats.bytesReceived, stats.packetsReceived);
            
            call_stats.bytesReceived = stats.bytesReceived;
            call_stats.bytesSent =stats.bytesSent;
            call_stats.cumulativeLost = stats.cumulativeLost;
            call_stats.extendedMax = stats.extendedMax;
            call_stats.fractionLost = stats.fractionLost;
            call_stats.jitterSamples = stats.jitterSamples;
            call_stats.packetsReceived = stats.packetsReceived;
            call_stats.packetsSent = stats.packetsSent;
            call_stats.rttMs = stats.rttMs;
            rtp_rtcp->Release();
        }
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_start_rtp_dump(int channelid, bool is_video, const char *file, RTPDirections dir)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d is_video: %s file: %s dir: %d", __FUNCTION__, __LINE__,
                 channelid, is_video?"true":"false", file, dir);
    if(channelid == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d wrong channelid id", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    
    int ret = -1;
    if(!is_video)
    {
        AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
        if (rtp_rtcp)
        {
            ret = rtp_rtcp->StartRTPDump(channelid, file, dir);
            rtp_rtcp->Release();
        }
        
    }
#ifdef VIDEO_ENABLED
    else
    {
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
        ret = rtp_rtcp->StartRTPDump(channelid, file, dir);
        rtp_rtcp->Release();
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
    return ret;
}

int ECMedia_stop_rtp_dump(int channelid, bool is_video, RTPDirections dir)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d is_video: %s dir: %d", __FUNCTION__, __LINE__,
                 channelid, is_video ? "true" : "false", dir);
    if(channelid == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d wrong channelid id", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    
    int ret = -1;
    if(!is_video)
    {
        AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
        if (rtp_rtcp)
        {
            ret = rtp_rtcp->StopRTPDump(channelid, dir);
            rtp_rtcp->Release();
        }
        
    }
#ifdef VIDEO_ENABLED
    else
    {
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
        ret = rtp_rtcp->StopRTPDump(channelid, dir);
        rtp_rtcp->Release();
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
    return ret;
}

/*
 * HARDWARE
 */
int ECMedia_get_playout_device_num(int& speaker_count)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if( hardware)
    {
        int ret = 0;
        ret = hardware->GetNumOfPlayoutDevices(speaker_count);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get playout device num", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//Mem to store device name should be allocated and at least 128 bytes. So does guid.
int ECMedia_get_specified_playout_device_info(int index, char *name, char *guid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... index: %d", __FUNCTION__, __LINE__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfPlayoutDevices(count);
        if (ret != 0) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to check playout device count", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d index range exception", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -100;
        }
        ret = hardware->GetPlayoutDeviceName(index, name, guid);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get playout device name", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_select_playout_device(int index)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... index: %d", __FUNCTION__, __LINE__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfPlayoutDevices(count);
        if (ret != 0) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to check playout device count", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d index range exception", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -100;
        }
        ret = hardware->SetPlayoutDevice(index);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set playout device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_record_device_num(int& microphone_count)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if( hardware)
    {
        int ret = 0;
        ret = hardware->GetNumOfRecordingDevices(microphone_count);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get recording device num", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//Mem to store device name should be allocated and at least 128 bytes. So does guid.
int ECMedia_get_specified_record_device_info(int index, char *name, char *guid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... index: %d", __FUNCTION__, __LINE__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfRecordingDevices(count);
        if (ret != 0) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to check record device count", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d index range exception", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -100;
        }
        ret = hardware->GetRecordingDeviceName(index, name, guid);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get recording device name", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_select_record_device(int index)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... index: %d", __FUNCTION__, __LINE__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfRecordingDevices(count);
        if (ret != 0) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to check playout device count", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d index range exception", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -100;
        }
        ret = hardware->SetRecordingDevice(index);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set recording device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_loudspeaker_status(bool enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... enabled=%s", __FUNCTION__, __LINE__, enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if( hardware)
    {
        int ret = 0;
        ret = hardware->SetLoudspeakerStatus(enabled);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set loudspeaker status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_loudpeaker_status(bool& enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if (hardware) {
        int ret = hardware->GetLoudspeakerStatus(enabled);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get loudspeaker status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... enabled=%d", __FUNCTION__, __LINE__, enabled);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_reset_audio_device()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...",__FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if (hardware) {
        int ret = hardware->ResetAudioDevice();
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to reset audio device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_global_audio_in_device(bool enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins. enabled=%d",__FUNCTION__, __LINE__, enabled?"true":"false");
#ifdef WIN32
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        bool bAudioRecording = base->GetRecordingIsRecording();
        base->Release();
        if (bAudioRecording == true)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d audio recording",__FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ",__FUNCTION__, __LINE__, -99);
        }
        else
        {
            g_bGlobalAudioInDevice = enabled;
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ",__FUNCTION__, __LINE__, 0);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get voe base",__FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ",__FUNCTION__, __LINE__, -99);
        return -99;
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ",__FUNCTION__, __LINE__, 0);
    return 0;
}
/*
 * ENCRYPTION
 */

//int ECMedia_init_srtp(int channelid)
//{
//    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and Channel ID: %d", __FUNCTION__, __LINE__, channelid);
//    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
//    VoEEncryption *encrypt = VoEEncryption::GetInterface(m_voe);
//    if (encrypt) {
//        int ret = encrypt->CcpSrtpInit(channelid);
//        encrypt->Release();
//        return ret;
//    }
//    else
//    {
//        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get VoEEncryption",__FUNCTION__, __LINE__);
//        return -99;
//    }
//}
//
//int ECMedia_enable_srtp_receive(int channelid, const char *key)
//{
//    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and Channel ID: %d", __FUNCTION__, __LINE__, channelid);
//    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
//    char master_key[65];
//    Base64decode(master_key, key);
//    VoEEncryption *encrypt = VoEEncryption::GetInterface(m_voe);
//    if (encrypt) {
//        int ret = encrypt->EnableSRTPReceive(channelid, kCipherAes256CounterMode, 64, kAuthHmacSha1, 0, kAuthTagLength80, kEncryptionAndAuthentication, reinterpret_cast<const unsigned char *>(master_key));
//        encrypt->Release();
//        return ret;
//    }
//    else
//    {
//        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get VoEEncryption",__FUNCTION__, __LINE__);
//        return -99;
//    }
//}
//
//int ECMedia_enable_srtp_send(int channelid, const char *key)
//{
//    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and Channel ID: %d", __FUNCTION__, __LINE__, channelid);
//    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
//    char master_key[65];
//    Base64decode(master_key, key);
//    VoEEncryption *encrypt = VoEEncryption::GetInterface(m_voe);
//    if (encrypt) {
//        int ret = encrypt->EnableSRTPSend(channelid, kCipherAes256CounterMode, 64, kAuthHmacSha1, 0, kAuthTagLength80, kEncryptionAndAuthentication, reinterpret_cast<const unsigned char *>(master_key), 0);
//        encrypt->Release();
//        return ret;
//    }
//    else
//    {
//        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get VoEEncryption",__FUNCTION__, __LINE__);
//        return -99;
//    }
//}
//
//int ECMedia_shutdown_srtp(int channel)
//{
//    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and Channel ID: %d", __FUNCTION__, __LINE__, channelid);
//    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
//    VoEEncryption *encrypt = VoEEncryption::GetInterface(m_voe);
//    if (encrypt) {
//        int ret = encrypt->CcpSrtpShutdown(channel);
//        encrypt->Release();
//        return ret;
//    }
//    else
//    {
//        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get VoEEncryption",__FUNCTION__, __LINE__);
//        return -99;
//    }
//}

/*
 * VOLUME
 */
int ECMedia_set_speaker_volume(int volumep)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... volume: %d", __FUNCTION__, __LINE__, volumep);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->SetSpeakerVolume(volumep);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set speaker volume", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_speaker_volume(unsigned int& volumep)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->GetSpeakerVolume(volumep);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get speaker volume", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d  volume:%d", __FUNCTION__, __LINE__, ret, volumep);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

/*
 *
 */
int ECMedia_set_mic_volume(int volumep)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... volume: %d", __FUNCTION__, __LINE__, volumep);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->SetMicVolume(volumep);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set mic volume", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
/*
 *
 */
int ECMedia_get_mic_volume(unsigned int& volumep)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->GetMicVolume(volumep);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get mic volume", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d volume:%d", __FUNCTION__, __LINE__, ret, volumep);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_mute_status(bool mute)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... mute: %s", __FUNCTION__, __LINE__, mute?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->SetSystemInputMute(mute);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set mute status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_mute_status(bool& mute)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->GetSystemInputMute(mute);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get system input mute status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_speaker_mute_status(bool mute)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... mute: %s", __FUNCTION__, __LINE__, mute?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->SetSystemOutputMute(mute);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d api not support", __FUNCTION__, __LINE__);
        }
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_speaker_mute_status(bool& mute)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->GetSystemOutputMute(mute);
        volume->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get system output mute status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEVolumeControl", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

/*
 * Capture
 */
#ifdef VIDEO_ENABLED
int ECMdeia_num_of_capture_devices()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int num = capture->NumberOfCaptureDevices();
        capture->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, num);
        return num;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//buffer for name should allocated and length should be at least 256. So does id.
int ECMedia_get_capture_device(int index, char *name, int name_len, char *id, int id_len)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... index: %d", __FUNCTION__, __LINE__, index);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->GetCaptureDevice(index, name, name_len, id, id_len);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get capture device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}


int ECMedia_num_of_capabilities(const char *id, int id_len)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *caputure = ViECapture::GetInterface(m_vie);
    if (caputure) {
        int num = caputure->NumberOfCapabilities(id, id_len);
        caputure->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, num);
        return num;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//Mem for capability should be allocated first.
int ECMedia_get_capture_capability(const char *id, int id_len, int index, CameraCapability& capabilityp)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *caputure = ViECapture::GetInterface(m_vie);
    if (caputure) {
        CaptureCapability capability;
        int ret = caputure->GetCaptureCapability(id, id_len, index, capability);
        capabilityp.height = capability.height;
        capabilityp.width = capability.width;
        capabilityp.maxfps = capability.maxFPS;
        caputure->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get capture capability", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_allocate_capture_device(const char *id, size_t len, int& deviceid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->AllocateCaptureDevice(id, (unsigned int)len, deviceid);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to allocate capture device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//add by dingxf
int ECMedia_get_file_capture_capability(int capture_id, CameraCapability& capabilityp)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
	VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
	ViECapture *caputure = ViECapture::GetInterface(m_vie);
	if (caputure) {
		CaptureCapability capability;
		int ret = caputure->GetCaptureCapability(capture_id, capability);
		capabilityp.height = capability.height;
		capabilityp.width = capability.width;
		capabilityp.maxfps = capability.maxFPS;
		caputure->Release();
		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ", __FUNCTION__, __LINE__, ret);
		return ret;
	}
	else
	{
		WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
		return -99;
	}
}

//add by dingxf
int ECMedia_allocate_capture_file(int& deviceid, const char *fileUTF8, const char *filesSplit)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
	VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
	ViECapture *capture = ViECapture::GetInterface(m_vie);
	if (capture) {
		int ret = capture->AllocateCaptureFile(deviceid, fileUTF8, filesSplit);
		capture->Release();
		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ", __FUNCTION__, __LINE__, ret);
		return ret;
	}
	else
	{
		WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
		return -99;
	}
}

int ECMedia_connect_capture_device(int deviceid, int channelid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d deviceid: %d", __FUNCTION__, __LINE__, channelid, deviceid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->ConnectCaptureDevice(deviceid, channelid);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to connect capture device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//The id is got by func "ECMedia_get_capture_device"
int ECMedia_getOrientation(const char *id, ECMediaRotateCapturedFrame &tr)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        RotateCapturedFrame tmp_tr;
        int ret = capture->GetOrientation(id, tmp_tr);
        tr = (ECMediaRotateCapturedFrame)tmp_tr;
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get orientation", __FUNCTION__, __LINE__);
        }
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_no_camera_capture_cb(int deviceid, onEcMediaNoCameraCaptureCb no_camera_capture_cb)
{
    //g_NoCameraCaptureCb = no_camera_capture_cb;
	ECViECaptureObserverManager::getInst()->addCaptureObserver(deviceid, no_camera_capture_cb);
    return 0;
}

int ECMedia_start_capture(int deviceid, CameraCapability cam)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... deviceid: %d width: %d height: %d maxfps: %d", __FUNCTION__, __LINE__, deviceid, cam.width, cam.height, cam.maxfps);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        
		ECViECaptureObserver* obsv = ECViECaptureObserverManager::getInst()->requestCaptureObserver(deviceid);
		if (obsv)
		{
			capture->RegisterObserver(deviceid, *obsv);
		}
        //if (!g_ECViECaptureObserver) {
        //    if (g_NoCameraCaptureCb) {
        //        g_ECViECaptureObserver = new ECViECaptureObserver(g_NoCameraCaptureCb);
        //    }
        //}
        //if (g_ECViECaptureObserver) {
        //    capture->RegisterObserver(deviceid, *g_ECViECaptureObserver);
        //}
        
        CaptureCapability cap;
        cap.height = cam.height;
        cap.width = cam.width;
        cap.maxFPS = cam.maxfps;
        //capture->EnableBrightnessAlarm(deviceid, true); //ylr for test
        int ret = capture->StartCapture(deviceid, cap);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start capture", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_stop_capture(int captureid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... captureid: %d", __FUNCTION__, __LINE__, captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
		ECViECaptureObserverManager::getInst()->removeCaptureObserver(captureid);
        //if (g_ECViECaptureObserver) {
        //    if (capture->DeregisterObserver(captureid) == 0) {
        //        delete g_ECViECaptureObserver;
        //        g_ECViECaptureObserver = NULL;
        //    }
        //}
        
        int ret = capture->StopCapture(captureid);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to stop capture", __FUNCTION__, __LINE__);
        }
        ret = capture->ReleaseCaptureDevice(captureid);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to release capture device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//The deviceid is got by func "ECMedia_allocate_capture_device"
int ECMedia_set_rotate_captured_frames(int deviceid, ECMediaRotateCapturedFrame tr)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... deviceid: %d rotate: %d", __FUNCTION__, __LINE__, deviceid, tr);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->SetRotateCapturedFrames(deviceid, (RotateCapturedFrame)tr);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set rotate captured frames", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_local_video_window(int deviceid, void *video_window)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... deviceid:%d video_window:%p ", __FUNCTION__, __LINE__, deviceid, video_window);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = 0;
#ifdef WIN32
        ViERender* render =  ViERender::GetInterface(m_vie);
		if (render)
		{
			ret = render->AddRenderer(deviceid, video_window, 1, 0, 0, 1, 1, NULL);
			if (ret) {
				render->Release();
				WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to add renderer", __FUNCTION__, __LINE__);
				WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
				return ret;
			}
			ret = render->MirrorRenderStream(deviceid, true, false, true);
			if (ret != 0) {
				WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to mirror render stream", __FUNCTION__, __LINE__);
			}
			ret = render->StartRender(deviceid);
			render->Release();
			if (ret != 0) {
				WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start render", __FUNCTION__, __LINE__);
			}
		}
		else
		{
			return -1;
			WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d render is null. ", __FUNCTION__, __LINE__);
		}
#else
        ret = capture->SetLocalVideoWindow(deviceid, video_window);
        capture->Release();
#endif
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}



/*
 * Render
 */

int ECMedia_add_render(int channelid, void *video_window, ReturnVideoWidthHeightM videoResolutionCallback)
{
    
    //WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid:%d video_window:%0x", __FUNCTION__, __LINE__, channelid, video_window);
    
    //static void *f_video_window = NULL;
    //static int f_channelid = -1;
    //static ReturnVideoWidthHeightM f_videoResolutionCallback = NULL;
    
    //static void *s_video_window = NULL;
    //static int s_channelid = -1;
    //static ReturnVideoWidthHeightM s_videoResolutionCallback = NULL;
    
    //static void *t_video_window = NULL;
    //static int t_channelid = -1;
    //static ReturnVideoWidthHeightM t_videoResolutionCallback = NULL;
    
    //if (channelid == 0) {
    //    f_video_window = video_window;
    //    f_channelid = channelid;
    //    f_videoResolutionCallback = videoResolutionCallback;
    //}
    //else if (channelid == 1) {
    //    s_video_window = video_window;
    //    s_channelid = channelid;
    //    s_videoResolutionCallback = videoResolutionCallback;
    
    //    ViERender *render = ViERender::GetInterface(m_vie);
    //    if (render) {
    //        int ret = render->AddRenderer(s_channelid, f_video_window, 2, 0, 0, 1, 1, f_videoResolutionCallback);
    //        render->StartRender(s_channelid);
    
    //        ret = render->AddRenderer(f_channelid, s_video_window, 2, 0, 0, 1, 1, s_videoResolutionCallback);
    //        render->StartRender(f_channelid);
    //        render->Release();
    //    }
    //}
    //else if (channelid == 2) {
    //    t_video_window = video_window;
    //    t_channelid = channelid;
    //    t_videoResolutionCallback = videoResolutionCallback;
    
    //    ViERender *render = ViERender::GetInterface(m_vie);
    //    if (render) {
    //        //int ret = render->AddRenderer(f_channelid, t_video_window, 2, 0, 0, 1, 1, t_videoResolutionCallback);
    //        //render->StartRender(f_channelid);
    
    //        int ret = render->AddRenderer(t_channelid, f_video_window, 2, 0, 0, 1, 1, f_videoResolutionCallback);
    //        render->StartRender(t_channelid);
    
    //        render->Release();
    //    }
    //}
    //return 0;
    
    
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid:%d video_window:%p", __FUNCTION__, __LINE__, channelid, video_window);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERender *render = ViERender::GetInterface(m_vie);
    if (render) {
        int ret = render->AddRenderer(channelid, video_window, 2, 0, 0, 1, 1, videoResolutionCallback);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to add renderer", __FUNCTION__, __LINE__);
        }
        ret = render->StartRender(channelid);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start render", __FUNCTION__, __LINE__);
        }
        render->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERender", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//todo: 这个接口需要重构，device id stop render 总是失败的
int ECMedia_stop_render(int channelid, int deviceid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid=%d,deviceid=%d",__FUNCTION__, __LINE__, channelid, deviceid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERender *render = ViERender::GetInterface(m_vie);
    if (render) {
        int ret = render->StopRender(channelid);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to stop render for channelid %d", __FUNCTION__, __LINE__, channelid);
        }
        ret = render->RemoveRenderer(channelid);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to remove renderer for channelid %d", __FUNCTION__, __LINE__, channelid);
        }
        ret = render->StopRender(deviceid);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to stop render for deviceid %d", __FUNCTION__, __LINE__, deviceid);
        }
        ret = render->RemoveRenderer(deviceid);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to remove renderer for deviceid %d", __FUNCTION__, __LINE__, deviceid);
        }
        render->Release();
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...",__FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_reset_remote_view(int channelid, void *video_window) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid:%d video_window:%p", __FUNCTION__, __LINE__, channelid, video_window);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERender *render = ViERender::GetInterface(m_vie);
    if (render) {
        int ret = render->ChangeWindow(channelid, video_window);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to add renderer", __FUNCTION__, __LINE__);
        }
        render->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERender", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    
    return 0;
}
#endif

#ifdef VIDEO_ENABLED
int ECMedia_set_i420_framecallback(int channelid, yuntongxunwebrtc::ECMedia_I420FrameCallBack callback) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid:%d ", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->AddI420FrameCallback(channelid, callback);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to add i420 frame callback", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
#endif

/*
 * VoECodec ViECodec
 */
int ECMedia_num_of_supported_codecs_audio()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int num = codec->NumOfCodecs();
        codec->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, num);
        return num;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//Mem to store should be allocated.
int ECMedia_get_supported_codecs_audio(CodecInst codecs[])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int total_num = codec->NumOfCodecs();
        for (int cursor = 0; cursor < total_num; cursor++) {
            codec->GetCodec(cursor, codecs[cursor]);
        }
        codec->Release();
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}
#ifdef VIDEO_ENABLED
int ECMedia_num_of_supported_codecs_video()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int num = codec->NumberOfCodecs();
        codec->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, num);
        return num;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//Mem to store should be allocated.
int ECMedia_get_supported_codecs_video(VideoCodec codecs[])
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int total_num = codec->NumberOfCodecs();
        for (int cursor = 0; cursor < total_num; cursor++) {
            codec->GetCodec(cursor, codecs[cursor]);
        }
        codec->Release();
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

// enable h264 hard encode, only support ios
// call it after m_vie have create
int ECMedia_iOS_h264_hard_codec_switch(bool encoder, bool decoder)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        codec->iOSH264HardCodecSwitch(encoder, decoder);
        codec->Release();
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable ios h264 hard encode.", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_set_key_frame_request_cb(int channelid, bool isVideoConf,onEcMediaRequestKeyFrameCallback cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->SetKeyFrameRequestCb(channelid,isVideoConf,cb);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set key frame request cb", __FUNCTION__, __LINE__);
        }
        codec->Release();
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

#endif
int ECMedia_get_send_codec_audio(int channelid, CodecInst& audioCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int ret = codec->GetSendCodec(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get audio send codec", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_send_codec_audio(int channelid, CodecInst& audioCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0,
                 "%s:%d begins... and channelid: %d audioCodec(pltype: %d plname: %s plfreq: %d pacsize: %d channels: %d rate: %d)",
                 __FUNCTION__,
                 __LINE__,
                 channelid,
                 audioCodec.pltype,
                 audioCodec.plname,
                 audioCodec.plfreq,
                 audioCodec.pacsize,
                 audioCodec.channels,
                 audioCodec.rate);
    
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int ret = codec->SetSendCodec(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set audio send codec", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoECodec", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_receive_playloadType_audio(int channelid, CodecInst& audioCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d audioCodec(pltype: %d plname: %s plfreq: %d pacsize: %d channels: %d rate: %d)", __FUNCTION__, __LINE__, channelid,
                 audioCodec.pltype, audioCodec.plname, audioCodec.plfreq, audioCodec.pacsize, audioCodec.channels, audioCodec.rate);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d plType:%d plname:%s", __FUNCTION__, __LINE__, audioCodec.pltype,
                     audioCodec.plname);
        int ret = codec->SetRecPayloadType(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set audio receive playload type", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_receive_playloadType_audio(int channelid, CodecInst& audioCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int ret = codec->GetRecPayloadType(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get audio receive playload type", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

#ifdef VIDEO_ENABLED
static void ECMedia_reset_send_codecinfo(VideoCodec& videoCodec)
{
	unsigned short scale = 0;
	if (videoCodec.mode == kScreensharing) {//Updated by zhangn 20190723
		videoCodec.manualMode = true;
	}

  if (!videoCodec.manualMode){
#ifdef WIN32 //Updated by zhangn 20190326
    //only support (160*n,90*n) [0<n<=12]
    scale = videoCodec.width / 160;
    if (0 != videoCodec.width % 160 || 0 != videoCodec.height % 90) {
      videoCodec.width = scale * 160;
      videoCodec.height = scale * 90;
    }
#else
    if (videoCodec.height > videoCodec.width) {
      scale = videoCodec.height / 160;
      if (0 != videoCodec.height % 160 || 0 != videoCodec.width % 90) {
        videoCodec.width = scale * 90;
        videoCodec.height = scale * 160;
      }
    }
    else {
      scale = videoCodec.width / 160;
      if (0 != videoCodec.width % 160 || 0 != videoCodec.height % 90) {
        videoCodec.width = scale * 160;
        videoCodec.height = scale * 90;
      }
    }
#endif
  }
  
	if (videoCodec.width % 8)
		videoCodec.width = (videoCodec.width / 8 + 1) * 8;

	if (videoCodec.height % 8)
		videoCodec.height = (videoCodec.height / 8 + 1) * 8;
	
	if (videoCodec.codecType == kVideoCodecH264) {
		videoCodec.codecType = kVideoCodecH264HIGH;
	}
  
  if (!videoCodec.manualMode){
    if (kVideoCodecH264HIGH == videoCodec.codecType) {
      switch (scale)
      {
        case 4://360p
          videoCodec.maxBitrate = 500;
          videoCodec.minBitrate = 30;
          videoCodec.startBitrate = 350;
          break;
        case 8://720p
          videoCodec.maxBitrate = 1100;
          videoCodec.minBitrate = 100;
          videoCodec.startBitrate = 900;
          break;
        default:
          videoCodec.maxBitrate = 500;
          videoCodec.minBitrate = 30;
          videoCodec.startBitrate = 350;
        break;
      }
    }
  }
}
int ECMedia_set_send_codec_video(int channelid, VideoCodec& videoCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid:%d videoCodec(width:%d height:%d pltype:%d plname:%s, startBitrate:%d, maxBitrate:%d, minBitrate:%d)",
                 __FUNCTION__, __LINE__, channelid, videoCodec.width,videoCodec.height, videoCodec.plType,videoCodec.plName, videoCodec.startBitrate,videoCodec.maxBitrate, videoCodec.minBitrate);
    if (videoCodec.width == 0 || videoCodec.height == 0) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d invalid param width or height", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return ERR_INVALID_PARAM;
    }
	VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
  
  ECMedia_reset_send_codecinfo(videoCodec);

  WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid:%d videoCodec(width:%d height:%d pltype:%d plname:%s, startBitrate:%d, maxBitrate:%d, minBitrate:%d)",
               __FUNCTION__, __LINE__, channelid, videoCodec.width,videoCodec.height, videoCodec.plType,videoCodec.plName, videoCodec.startBitrate,videoCodec.maxBitrate, videoCodec.minBitrate);

    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d plType:%d plname:%s", __FUNCTION__, __LINE__, videoCodec.plType,
                     videoCodec.plName);
        int ret = codec->SetSendCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video send codec", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_send_codec_video(int channelid, VideoCodec& videoCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->GetSendCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get video send codec", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_video_qm_mode(int channelid,  yuntongxunwebrtc::VCMQmResolutionMode mode) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid:%d , VCMQmResolutionMode: %d",
                 __FUNCTION__, __LINE__, channelid, mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        
        int ret = codec->SetVideoSendQmMode(channelid, mode);
        codec->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_frame_scale_type(int channelid, FrameScaleType type) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d frameScaleType: %d", __FUNCTION__, __LINE__, channelid, type);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d", __FUNCTION__, __LINE__);
        int ret = codec->SetFrameScaleType(channelid, type);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set frame scale type", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_video_conf_cb(int channelid, onEcMediaVideoConference video_conf_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        network->setVideoConfCb(channelid, video_conf_cb);
        network->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_stun_cb_video(int channelid, onEcMediaStunPacket stun_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        network->setStunCb(channelid, stun_cb);
        network->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_receive_codec_video(int channelid, VideoCodec& videoCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->SetReceiveCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video receive codec", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_get_receive_codec_video(int channelid, VideoCodec& videoCodec)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->GetReceiveCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get video receive codec", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_sendUDPPacket(const int channelid,
                          const void* data,
                          const unsigned int length,
                          int& transmitted_bytes,
                          bool use_rtcp_socket,
                          uint16_t port,
                          const char* ip)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid:%d len:%d use_rtcp_socket:%s port:%d ip:%s",
                 __FUNCTION__, __LINE__, channelid, length, use_rtcp_socket?"true":"false", port, ip);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret=network->SendUDPPacket(channelid, data, length, transmitted_bytes, use_rtcp_socket, port, ip);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to send udp packet", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_set_NACK_status_video(int channelid, bool enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetNACKStatus(channelid, enabled);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video nack status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_FEC_status_video(const int channelid,
                                 const bool enable,
                                 const unsigned char payload_typeRED,
                                 const unsigned char payload_typeFEC)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetFECStatus(channelid, enable, payload_typeRED, payload_typeFEC);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video fec status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    
}

int ECMedia_set_HybridNACKFEC_status_video(const int channelid,
                                           const bool enable,
                                           const unsigned char payload_typeRED,
                                           const unsigned char payload_typeFEC)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetHybridNACKFECStatus(channelid, enable, payload_typeRED, payload_typeFEC);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video hybrid nack fec status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    
}

int ECMedia_set_RTCP_status_video(int channelid, int mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetRTCPStatus(channelid, (ViERTCPMode)mode);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video rtcp status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_setVideoConferenceFlag(int channel,const char *selfSipNo ,const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        network->setVideoConferenceFlag(channel, selfSipNo, sipNo, conferenceNo, confPasswd, port, ip);
        network->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_send_key_frame(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid=%d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *tempCodec = ViECodec::GetInterface(m_vie);
    if(tempCodec)
    {
        int ret = tempCodec->SendKeyFrame(channel);
        tempCodec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to send key frame", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_EnableIPV6(int channel, bool flag)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->EnableIPv6(channel);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video enable ipv6", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_IsIPv6Enabled(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        bool ret = network->IsIPv6Enabled(channel);
        network->Release();
        if (ret) {
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return 1; //support IPV6
        }
        else
        {
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return 0; // not support IPV6
        }
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_start_record_screen(int audioChannel, const char* filename, int bitrates, int fps, int screen_index)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... audioChannel: %d filename: %s bitrates: %d fps: %d screen_index: %d", __FUNCTION__, __LINE__,
                 audioChannel, filename?filename:"NULL", bitrates, fps, screen_index);
    if(!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d create recorder failed", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    
    if(g_recordVoip->isStartRecordScree()) {
        g_recordVoip->StopRecordScreen(0);
    }
    
    if(!g_recordVoip->isRecording() && m_voe && audioChannel >= 0) {
        //WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "RegisterExternalMediaProcessin in ECMedia_start_record_screen\n");
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if(exmedia) {
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kPlaybackPerChannel, *g_recordVoip);
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kRecordingPerChannel, *g_recordVoip);
            exmedia->Release();
        }
    }
    
    int ret = g_recordVoip->StartRecordScreen(filename, bitrates, fps, screen_index);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
    return ret;
}

int ECMedia_start_record_screen_ex(int audioChannel, const char* filename, int bitrates, int fps, int screen_index, int left, int top, int width, int height)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... audioChannel: %d filename: %s bitrates: %d fps: %d screen_index: %d left: %d top: %d width: %d height: %d", __FUNCTION__, __LINE__,
                 audioChannel, filename?filename:"NULL", bitrates, fps, screen_index, left, top, width, height);
    if(!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d create recorder failed", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    
    if(g_recordVoip->isStartRecordScree()) {
        g_recordVoip->StopRecordScreen(0);
    }
    
    if(!g_recordVoip->isRecording() && m_voe) {
        //WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "RegisterExternalMediaProcessin in ECMedia_start_record_screen_ex\n");
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if(exmedia) {
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kPlaybackPerChannel, *g_recordVoip);
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kRecordingPerChannel, *g_recordVoip);
            exmedia->Release();
        }
    }
    
    int ret = g_recordVoip->StartRecordScreenEx(filename, bitrates, fps, screen_index, left, top, width, height);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
    return ret;
}
int ECMedia_stop_record_screen(int audioChannel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... audioChannel: %d", __FUNCTION__, __LINE__, audioChannel);
    
    if(!g_recordVoip)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d recorder is null", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    
    g_recordVoip->StopRecordScreen(0);
    if(!g_recordVoip->isRecording() && m_voe) {
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if(exmedia) {
            exmedia->DeRegisterExternalMediaProcessing(audioChannel,  kPlaybackPerChannel);
            exmedia->DeRegisterExternalMediaProcessing(audioChannel,  kRecordingPerChannel);
            exmedia->Release();
        }
    }
    if (!g_recordVoip->isRecording()) {
        delete g_recordVoip;
        g_recordVoip = NULL;
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

ECMEDIA_API int ECMedia_start_record_remote_video(int audioChannel, int videoChannel, const char* filename)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... audioChannel: %d videoChannel: %d filename: %s", __FUNCTION__, __LINE__,
                 audioChannel, videoChannel, filename?filename:"NULL");
    if (!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d create recorder failed", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    
    if (g_recordVoip->isStartRecordRVideo()) {
        g_recordVoip->StopRecordRemoteVideo(0);
    }
    
    
    if (!g_recordVoip->isRecording() && m_voe) {
        //WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "RegisterExternalMediaProcessin in ECMedia_start_record_screen\n");
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if (exmedia) {
            exmedia->RegisterExternalMediaProcessing(audioChannel, kPlaybackPerChannel, *g_recordVoip);
            exmedia->RegisterExternalMediaProcessing(audioChannel, kRecordingPerChannel, *g_recordVoip);
            exmedia->Release();
        }
    }
    
    int ret = g_recordVoip->StartRecordRemoteVideo(filename);
    
    if (m_vie) {
        ViEFile *file = ViEFile::GetInterface(m_vie);
        if (file) {
            file->RegisterVideoFrameStorageCallBack(videoChannel, (yuntongxunwebrtc::VCMFrameStorageCallback *)g_recordVoip);
            file->Release();
        }
        ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
        if (rtp_rtcp) {
            rtp_rtcp->RequestKeyFrame(videoChannel);
            rtp_rtcp->Release();
        }
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
    return ret;
}

ECMEDIA_API int ECMedia_stop_record_remote_video(int audioChannel, int videoChannel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... audioChannel: %d videoChannel: %d", __FUNCTION__, __LINE__, audioChannel, videoChannel);
    
    if (!g_recordVoip)
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d recorder is null", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    
    g_recordVoip->StopRecordRemoteVideo(0);
    
    if (!g_recordVoip->isRecording() && m_voe) {
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if (exmedia) {
            exmedia->DeRegisterExternalMediaProcessing(audioChannel, kPlaybackPerChannel);
            exmedia->DeRegisterExternalMediaProcessing(audioChannel, kRecordingPerChannel);
            exmedia->Release();
        }
    }
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    if (file) {
        file->RegisterVideoFrameStorageCallBack(videoChannel, NULL);
        file->Release();
    }
    
    if (!g_recordVoip->isRecording()) {
        delete g_recordVoip;
        g_recordVoip = NULL;
    }
    
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

ECMEDIA_API int ECMedia_start_record_local_video(int audioChannel, int videoChannel, const char* filename)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... audioChannel: %d videoChannel: %d filename: %s", __FUNCTION__, __LINE__,
                 audioChannel, videoChannel, filename?filename:"NULL");
    if (!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d create recorder failed", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    
    if (g_recordVoip->isStartRecordLVideo()) {
        g_recordVoip->StopRecordLocalVideo(0);
    }
    
    if (!g_recordVoip->isRecording() && m_voe) {
        //WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "RegisterExternalMediaProcessin in %s\n", __FUNCTION__, __LINE__);
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if (exmedia) {
            exmedia->RegisterExternalMediaProcessing(audioChannel, kPlaybackPerChannel, *g_recordVoip);
            exmedia->RegisterExternalMediaProcessing(audioChannel, kRecordingPerChannel, *g_recordVoip);
            exmedia->Release();
        }
    }
    
    //should start record before SendKeyFrame.
    int ret = g_recordVoip->StartRecordLocalVideo(filename);
    
    if (m_vie) {
        ViENetwork *vietwork = ViENetwork::GetInterface(m_vie);
        if (vietwork) {
            vietwork->RegisterEncoderDataObserver(videoChannel, (yuntongxunwebrtc::VCMPacketizationCallback *)g_recordVoip);
            vietwork->Release();
        }
        ViECodec *codec = ViECodec::GetInterface(m_vie);
        if (codec) {
            codec->SendKeyFrame(videoChannel);
            codec->Release();
        }
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
    return ret;
}
ECMEDIA_API int ECMedia_stop_record_local_video(int audioChannel, int videoChannel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... audioChannel: %d videoChannel: %d", __FUNCTION__, __LINE__, audioChannel, videoChannel);
    
    if (!g_recordVoip) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d recorder is null", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    
    g_recordVoip->StopRecordLocalVideo(0);
    
    if (!g_recordVoip->isRecording() && m_voe) {
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if (exmedia) {
            exmedia->DeRegisterExternalMediaProcessing(audioChannel, kPlaybackPerChannel);
            exmedia->DeRegisterExternalMediaProcessing(audioChannel, kRecordingPerChannel);
            exmedia->Release();
        }
    }
    
    ViENetwork *vietwork = ViENetwork::GetInterface(m_vie);
    if (vietwork) {
        vietwork->DeRegisterEncoderDataObserver(videoChannel);
        vietwork->Release();
    }
    
    if (!g_recordVoip->isRecording()) {
        delete g_recordVoip;
        g_recordVoip = NULL;
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}


int ECMedia_get_local_video_snapshot(int deviceid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., deviceid: %d", __FUNCTION__, __LINE__, deviceid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    ViEPicture capPicture;
    if( file->GetCaptureDeviceSnapshot(deviceid, capPicture, kVideoMJPEG) < 0) {
        file->Release();
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d GetCaptureDeviceSnapshot failed", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    
    if(g_snapshotBuf) {
        free(g_snapshotBuf);
        g_snapshotBuf = NULL;
    }
    g_snapshotBuf = (uint8_t*)malloc(capPicture.size);
    memcpy(g_snapshotBuf, capPicture.data, capPicture.size);
    *size = capPicture.size;
    *width = capPicture.width;
    *height = capPicture.height;
    *buf = g_snapshotBuf;
    
    file->FreePicture(capPicture);
    file->Release();
    
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_save_local_video_snapshot(int deviceid, const char* filePath)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., deviceid: %d, filePath: %s", __FUNCTION__, __LINE__, deviceid, filePath);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    if(file) {
        ViEPicture capPicture;
        if( file->GetCaptureDeviceSnapshot(deviceid, filePath) < 0) {
            file->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d  GetCaptureDeviceSnapshot failed.", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -1;
        }
        file->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d  get ViEFile failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_get_remote_video_snapshot(int channelid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    ViEPicture capPicture;
    if( file->GetRenderSnapshot(channelid, capPicture, kVideoMJPEG) < 0) {
        file->Release();
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d  GetCaptureDeviceSnapshot failed", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    
    if(g_snapshotBuf) {
        free(g_snapshotBuf);
        g_snapshotBuf = NULL;
    }
    g_snapshotBuf = (uint8_t*)malloc(capPicture.size);
    memcpy(g_snapshotBuf, capPicture.data, capPicture.size);
    *size = capPicture.size;
    *width = capPicture.width;
    *height = capPicture.height;
    *buf = g_snapshotBuf;
    
    file->FreePicture(capPicture);
    file->Release();
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_save_remote_video_snapshot(int channelid, const char* filePath)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d, filePath: %s", __FUNCTION__, __LINE__, channelid, filePath);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    if(file) {
        ViEPicture capPicture;
        if( file->GetRenderSnapshot(channelid, filePath) < 0) {
            file->Release();
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d  GetRenderSnapshot failed", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -1;
        }
        file->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get ViEFile failed.", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECmedia_enable_deflickering(int captureid, bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., captureid: %d, enabled: %s", __FUNCTION__, __LINE__, captureid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEImageProcess *imageProcess = ViEImageProcess::GetInterface(m_vie);
    if (imageProcess) {
        int ret = imageProcess->EnableDeflickering(captureid, enable);
        imageProcess->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable deflickering", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEImageProcess", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECmedia_enable_EnableColorEnhancement(int channelid, bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d, enable: %s", __FUNCTION__, __LINE__, channelid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEImageProcess *imageProcess = ViEImageProcess::GetInterface(m_vie);
    if (imageProcess) {
        int ret = imageProcess->EnableColorEnhancement(channelid, enable);
        imageProcess->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable color enhancement", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEImageProcess", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECmedia_enable_EnableDenoising(int captureid, bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., captureid: %d, enable: %s", __FUNCTION__, __LINE__, captureid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEImageProcess *imageProcess = ViEImageProcess::GetInterface(m_vie);
    if (imageProcess) {
        int ret = imageProcess->EnableDenoising(captureid, enable);
        imageProcess->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable denoising", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEImageProcess", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECmedia_enable_EnableBrightnessAlarm(int captureid, bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., captureid: %d, enable: %s", __FUNCTION__, __LINE__, captureid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->EnableBrightnessAlarm(captureid, enable);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable brightness alarm", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECmedia_enable_EnableBeautyFilter(int captureid, bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., captureid: %d, enable: %s", __FUNCTION__, __LINE__, captureid, enable ? "true" : "false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->EnableBeautyFilter(captureid, enable);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable beauty filter", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_init_srtp_video(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->CcpSrtpInit(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video init srtp", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get ViEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}
int ECMedia_shutdown_srtp_video(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->CcpSrtpShutdown(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to video shutdown srtp", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get ViEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_enable_srtp_send_video(int channel, yuntongxunwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d type: %d key: %s", __FUNCTION__, __LINE__, channel, crypt_type, key);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->EnableSRTPSend(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable video srtp send", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get ViEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}
int ECMedia_disable_srtp_send_video(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->DisableSRTPSend(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to disable video srtp send", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get ViEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_enable_srtp_recv_video(int channel, yuntongxunwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d type: %d key: %s", __FUNCTION__, __LINE__, channel, crypt_type, key);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->EnableSRTPReceive(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable video srtp recv", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get ViEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}
int ECMedia_disable_srtp_recv_video(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->DisableSRTPReceive(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to disable video srtp recv", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get ViEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_video_set_remb(int channelid, bool enableRemb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid:%d, enableRemb:%s", __FUNCTION__, __LINE__,
                 channelid, enableRemb ? "true" : "false");
    
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->SetRembStatus(channelid, enableRemb, enableRemb);
        //ret = rtp_rtcp->SetTMMBRStatus(channelid, tmmbrEnabled);
        if (enableRemb) {
            rtp_rtcp->SetSendAbsoluteSendTimeStatus(channelid, true, kRtpExtensionAbsoluteSendTime);
            rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(channelid, true, kRtpExtensionAbsoluteSendTime);
        }
        rtp_rtcp->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get ViERTP_RTCP", __FUNCTION__, __LINE__);
        return -99;
    }
}

#endif

int ECMedia_set_VAD_status(int channelid, VadModes mode, bool dtx_enabled)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid: %d, mode: %d dtx_enabled: %s", __FUNCTION__, __LINE__, channelid, mode, dtx_enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int ret = codec->SetVADStatus(channelid, false, mode, !dtx_enabled);
        codec->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set vad status", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoECodec", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}


//CAUTION: noNetwork/wifi/other
int ECMedia_set_network_type(int audio_channelid, int video_channelid, const char *type)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., audio_channelid: %d, video_channelid: %d ", __FUNCTION__, __LINE__, audio_channelid, video_channelid);
    if (!type || strcmp(type, "noNetwork")==0) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d invalid network type", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = yuntongxunwebrtc::VoEBase::GetInterface(m_voe);
    if (base) {
        base->SetNetworkType(audio_channelid, strcmp(type, "wifi")==0?true:false);
        base->Release();
    }
#ifdef VIDEO_ENABLED
    if (video_channelid >= 0) {
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViENetwork *network = yuntongxunwebrtc::ViENetwork::GetInterface(m_vie);
        if (network) {
            network->setNetworkType(video_channelid, strcmp(type, "wifi")==0?true:false);
            network->Release();
        }
    }
    
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}


int ECMedia_EnableIPV6(int channel, bool flag)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d, flag: %s", __FUNCTION__, __LINE__, channel, flag?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        int ret = network->EnableIPv6(channel);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable ipv6", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_IsIPv6Enabled(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... and channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        bool ret = network->IPv6IsEnabled(channel);
        network->Release();
        if (ret) {
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return 1; //support IPV6
        }
        else
        {
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return 0; // not support IPV6
        }
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECMedia_AmrNBCreateEnc()
{
    return AmrNBCreateEnc();
}
int ECMedia_AmrNBCreateDec()
{
    return AmrNBCreateDec();
}
int ECMedia_AmrNBFreeEnc()
{
    return AmrNBFreeEnc();
}
int ECMedia_AmrNBFreeDec()
{
    return AmrNBFreeDec();
}
int ECMedia_AmrNBEncode(short* input, short len, short*output, short mode)
{
    return AmrNBEncode(input, len, output, mode);
}
int ECMedia_AmrNBEncoderInit(short dtxMode)
{
    return AmrNBEncoderInit(dtxMode);
}
int ECMedia_AmrNBDecode(short* encoded, int len, short* decoded)
{
    return AmrNBDecode(encoded, len, decoded);
}

int ECMedia_AmrNBVersion(char *versionStr, short len)
{
    return AmrNBVersion(versionStr, len);
}

int ECMedia_init_srtp_audio(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->CcpSrtpInit(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio init srtp", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}
int ECMedia_shutdown_srtp_audio(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->CcpSrtpShutdown(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio shutdown srtp", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_enable_srtp_send_audio(int channel, yuntongxunwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d crypt_type: %d key: %s", __FUNCTION__, __LINE__, channel, crypt_type, key);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->EnableSRTPSend(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable audio srtp send", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}
int ECMedia_disable_srtp_send_audio(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->DisableSRTPSend(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to disable audio srtp send", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_enable_srtp_recv_audio(int channel, yuntongxunwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d crypt_type: %d key: %s", __FUNCTION__, __LINE__, channel, crypt_type, key);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->EnableSRTPReceive(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable audio srtp recv", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}
int ECMedia_disable_srtp_recv_audio(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->DisableSRTPReceive(channel);
        encryt->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to disable audio srtp recv", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEEncryption failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_start_record_playout(int channel, char *filename)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StartRecordingPlayout(channel, filename);
        file->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start record playout", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEFile failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_stop_record_playout(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., channelid: %d", __FUNCTION__, __LINE__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StopRecordingPlayout(channel);
        file->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d playout not recording or failed to stop record playout", __FUNCTION__, __LINE__);
        }
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEFile failed", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_start_record_microphone(char *filename)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StartRecordingMicrophone(filename);
        file->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start record microphone", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEFile failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_stop_record_microphone()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StopRecordingMicrophone();
        file->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to stop record microphone", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEFile failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_start_record_send_voice(char *filename)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StartRecordingCall(filename);
        file->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start record send voice", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEFile failed", __FUNCTION__, __LINE__);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_stop_record_send_voice()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StopRecordingCall();
        file->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d not recording or failed to stop record send voice", __FUNCTION__, __LINE__);
        }
        return ret;
    }
    WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d get VoEFile failed", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_set_CaptureDeviceID(int videoCapDevId)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., videoCapDevId: %d", __FUNCTION__, __LINE__, videoCapDevId);
#ifdef VIDEO_ENABLED
    g_CaptureDeviceId = videoCapDevId;
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...",__FUNCTION__, __LINE__);
    return 0;
}

int ECMedia_Check_Record_Permission(bool &enabled) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...",__FUNCTION__, __LINE__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if (hardware) {
        int ret = hardware->CheckRecordPermission(enabled);
        hardware->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to check record permission", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEHardware",__FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
#ifdef VIDEO_ENABLED
ECMEDIA_API int ECMedia_setBeautyFace(int deviceid, bool enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... ", __FUNCTION__, __LINE__);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->setBeautyFace(deviceid, enable);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set beauty face", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        return -99;
    }
}

ECMEDIA_API int ECMedia_iOS_SetVideoFilter(int deviceid, ECImageFilterType filterType)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins, deviceid:%d, image filter type:%d ", __FUNCTION__, __LINE__, deviceid, filterType);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->setVideoFilter(deviceid, filterType);
        capture->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video filter", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        return -99;
    }
}



int ECMedia_allocate_desktopShare_capture(int& desktop_captureid, int capture_type)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., desktop_captureid: %d, capture_type: %d", __FUNCTION__, __LINE__, desktop_captureid, capture_type);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        int ret = vie_desktopshare->AllocateDesktopShareCapturer(desktop_captureid, (DesktopShareType)capture_type);
        if (ret != 0)
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d AllocateDesktopShareCapturer failed", __FUNCTION__, __LINE__);
        else
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d AllocateDesktopShareCapturer desktop_captureid:%d", __FUNCTION__, __LINE__, desktop_captureid);
        vie_desktopshare->Release();
        g_statsCollector->SetVideoCaptureDeviceId(desktop_captureid);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_release_desktop_capture(int desktop_captureid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... desktop_captureid: %d", __FUNCTION__, __LINE__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        vie_desktopShare->StopDesktopShareCapture(desktop_captureid);
        vie_desktopShare->ReleaseDesktopShareCapturer(desktop_captureid);
        vie_desktopShare->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_connect_desktop_captureDevice(int desktop_captureid, int video_channelId)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... desktop_captureid: %d video_channelId: %d", __FUNCTION__, __LINE__, desktop_captureid, video_channelId);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        int ret = vie_desktopShare->ConnectDesktopCaptureDevice(desktop_captureid, video_channelId);
        vie_desktopShare->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to connect desktop capture device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_disconnect_desktop_captureDevice(int video_channelId)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... video_channelId: %d", __FUNCTION__, __LINE__, video_channelId);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        int ret = vie_desktopShare->DisConnectDesktopCaptureDevice(video_channelId);
        vie_desktopShare->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to disconnect desktop capture device", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}


int ECMedia_get_screen_list(int desktop_captureid, ScreenID **screenList)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., desktop_captureid: %d", __FUNCTION__, __LINE__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (!screenList) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d screenList is NULL.", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        if (m_pScreenlist != NULL)
            delete[] m_pScreenlist;
        m_screenlist.clear();
        bool ret = vie_desktopshare->GetScreenList(desktop_captureid, m_screenlist);
        vie_desktopshare->Release();
        int num = m_screenlist.size();
        m_pScreenlist = new ScreenID[num];
        ScreenID *temp = m_pScreenlist;
        for (ScreenList::iterator it = m_screenlist.begin(); it!=m_screenlist.end(); it++)
        {
            *temp = *it;
            temp++;
        }
        *screenList = m_pScreenlist;
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get screen list", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, num);
        return num;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
}


int ECMedia_get_window_list(int desktop_captureid, WindowShare **windowList)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., desktop_captureid: %d", __FUNCTION__, __LINE__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (!windowList) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d windowList is NULL.", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        if (m_pWindowlist != NULL)
            delete[] m_pWindowlist;
        m_windowlist.clear();
        bool ret = vie_desktopshare->GetWindowList(desktop_captureid, m_windowlist);
        vie_desktopshare->Release();
        int num = m_windowlist.size();
        //m_pWindowlist = (WindowShare*)malloc(num * sizeof(WindowShare));
        m_pWindowlist = new WindowShare[num];
        WindowShare *temp = m_pWindowlist;
        for (WindowList::iterator it = m_windowlist.begin(); it != m_windowlist.end(); it++)
        {
            (*temp).id = it->id;
            memcpy((*temp).title, it->title.c_str(), kTitleLength);
            temp++;
        }
        *windowList = m_pWindowlist;
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get window list", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, num);
        return num;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -1;
    }
}



bool ECMedia_select_screen(int desktop_captureid, ScreenID screeninfo)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... captureid: %d", __FUNCTION__, __LINE__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        bool ret = vie_desktopshare->SelectScreen(desktop_captureid, screeninfo);
        vie_desktopshare->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to select screen", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return false;
    }
}

bool ECMedia_select_window(int desktop_captureid, WindowID windowinfo)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... desktop_captureid: %d", __FUNCTION__, __LINE__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        bool ret = vie_desktopshare->SelectWindow(desktop_captureid, windowinfo);
        vie_desktopshare->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to select window", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return false;
    }
}


int ECMedia_start_desktop_capture(int captureId, int fps)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... captureId: %d fps: %d", __FUNCTION__, __LINE__, captureId, fps);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        int ret = vie_desktopshare->StartDesktopShareCapture(captureId, fps);
        vie_desktopshare->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start desktop capture", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_stop_desktop_capture(int desktop_captureid)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... desktop_captureid: %d", __FUNCTION__, __LINE__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        int ret = vie_desktopShare->StopDesktopShareCapture(desktop_captureid);
        vie_desktopShare->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to stop desktop capture", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_desktop_share_err_code_cb(int desktop_captureid, int channelid, onEcMediaDesktopCaptureErrCode capture_err_code_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... desktop_captureid: %d channelid: %d", __FUNCTION__, __LINE__, desktop_captureid, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktop_capture = ViEDesktopShare::GetInterface(m_vie);
    if (desktop_capture) {
        desktop_capture->setCaptureErrCb(desktop_captureid, channelid, capture_err_code_cb);
        desktop_capture->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_desktop_share_window_change_cb(int desktop_captureid, int channelid, onEcMediaShareWindowSizeChange share_window_change_cb)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... desktop_captureid: %d channelid: %d", __FUNCTION__, __LINE__, desktop_captureid, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktop_capture = ViEDesktopShare::GetInterface(m_vie);
    if (desktop_capture) {
        desktop_capture->setShareWindowChangeCb(desktop_captureid, channelid, share_window_change_cb);
        desktop_capture->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
int ECmedia_set_shield_mosaic(int video_channel, bool flag)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., video_channel: %d", __FUNCTION__, __LINE__, video_channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->setShieldMosaic(video_channel, flag);
        network->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set shield mosaic", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return 0;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}


int ECMedia_get_desktop_capture_size(int desktop_captureid, int &width, int &height)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., desktop_captureid: %d", __FUNCTION__, __LINE__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (desktopshare) {
        bool ret = desktopshare->GetDesktopShareCaptureRect(desktop_captureid, width, height);
        desktopshare->Release();
        if (ret == false) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get desktop capture size", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret ? 0 : -99);
        return ret ? 0 : -99;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_set_screen_share_activity(int desktop_captureid, void* activity)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... desktop_captureid: %d activity: %0x", __FUNCTION__, __LINE__, desktop_captureid, activity);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (desktopshare) {
        int ret = desktopshare->SetScreenShareActivity(desktop_captureid, activity);
        desktopshare->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set screen share activity", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEDesktopShare", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

//add by chwd
/*
 @deviceid : Device Id
 @water : see define in common_types.h
 @width height : the size of capture data,not watermark size
 */
int ECMedia_set_watermark(int deviceid, WaterMark watermark,int width,int height)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,args fontfile: %s,fontcolor: %s,\
                 fontsize: %d,text: %s, x: %d,y: %d,imagepath: %s,startposition :%s,flag: %d,width: %d,height: %d",
                 __FUNCTION__, __LINE__,watermark.fontfile,watermark.fontcolor,watermark.fontsize,watermark.text,watermark.x,watermark.y,
                 watermark.imagepath,watermark.startposition,watermark.flag,width,height);
#ifdef VIDEO_ENABLED
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->AllocateWaterMark(deviceid, watermark,width,height);
        capture->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
        return -99;
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -99;
}

/********************* ec live stream api begin ****************/
// create live stream object.
void *ECMedia_createLiveStream()
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    ECLiveEngine* engine = ECLiveEngine::getInstance();
    return engine;
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return NULL;
}

// not support yet.
void ECMedia_SetLiveVideoSource(void *handle, int video_source)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    //    ECLiveEngine *engine = (ECLiveEngine*)handle;
    //    engine->setVideoPreview(renderView);
    
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
#endif
}

// set video preview view.
int ECMedia_setVideoPreviewViewer(void *handle, void *viewer) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    int ret = -1;
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        ret = engine->setVideoPreview(viewer);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#endif
    return -1;
}

int ECMedia_ConfigLiveVideoStream(void *handle, LiveVideoStreamConfig config)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        int ret = engine->configLiveVideoStream(config);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set video profile", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_setLiveVideoFrameDegree(void *handle, ECLiveFrameDegree degree) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        int ret = -1;
        ret = engine->setCaptureFrameDegree(degree);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set live video frame degree.", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    return -1;
#endif
}

int ECMedia_SwitchLiveCamera(void *handle, int camera_index) {
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        int ret = -1;
        ret = engine->switchCamera(camera_index);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to switch live video stream.", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

// push stream
int ECMedia_pushLiveStream(void *handle, const char *url, ECLiveStreamNetworkStatusCallBack callback)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        int ret = 0;
        
        ret = engine->startPublish(url, callback);
        
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to push stream", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

int ECMedia_playLiveStream(void *handle, const char * url, ECLiveStreamNetworkStatusCallBack callback)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    int ret = -1;
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        ret = engine->startPlay(url, callback);
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to play stream", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -1;
}

void ECMedia_stopLiveStream(void *handle)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        engine->stopPlay();
        engine->stopPublish();
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
#endif
}

void ECMedia_releaseLiveStream(void *handle)
{

    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
            ECLiveEngine::destroy();
    }
#endif
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
}

void ECMedia_enableLiveStreamBeauty(void *handle)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        engine->setBeautyFace(true);
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
#endif
}

void ECMedia_disableLiveStreamBeauty(void *handle)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
#ifdef VIDEO_ENABLED
    if(handle) {
        ECLiveEngine *engine = (ECLiveEngine*)handle;
        engine->setBeautyFace(false);
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
#endif
}

// not support yet
ECMEDIA_API int ECMedia_GetShareWindows(void *handle, WindowShare ** windows)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    //    RTMPLiveSession *p = (RTMPLiveSession*)handle;
    //    std::vector<ShareWindowInfo> list;
    //    p->GetShareWindowList(list);
    //
    //    if (m_pWindowlist != NULL)
    //        delete[] m_pWindowlist;
    //
    //    if (list.size() == 0)
    //        return 0;
    //    m_pWindowlist = new WindowShare[list.size()];
    //    WindowShare *temp = m_pWindowlist;
    //
    //    for (int i = 0 ; i< list.size() ; i++ )
    //    {
    //        (*temp).id = list[i].id;
    //        (*temp).type = list[i].type;
    //        memcpy((*temp).title, list[i].name.c_str(), kTitleLength);
    //        temp++;
    //    }
    //    *windows = m_pWindowlist;
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0; //list.size();
}

// not support yet
ECMEDIA_API int ECMedia_SelectShareWindow(void *handle, int type, int id)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    //    RTMPLiveSession *p = (RTMPLiveSession*)handle;
    //    p->SelectShareWindow(type,id);
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return 0;
}
/********************* ec live stream api end ****************/

ECMEDIA_API int  ECMedia_startRecordLocalMedia(const char *fileName, void *localview)
{
    
#ifdef VIDEO_ENABLED
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... ", __FUNCTION__, __LINE__);
    if (!g_recordLocal) {
        g_recordLocal = new RecordLocal();
        if (!g_recordLocal) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d create recorder failed", __FUNCTION__, __LINE__);
            WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
            return -1;
        }
    }
    
    int ret = g_recordLocal->Start(fileName, localview);
    if (ret != 0) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start record local media", __FUNCTION__, __LINE__);
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d", __FUNCTION__, __LINE__, ret);
    return ret;
#endif
    return -1;
}

ECMEDIA_API void ECMedia_stopRecordLocalMedia()
{
#ifdef VIDEO_ENABLED
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    if (!g_recordLocal) {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d not start recorder", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return;
    }
    
    g_recordLocal->Stop();
    
    delete g_recordLocal;
    g_recordLocal = NULL;
    
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
#endif
}
#endif

int ECMedia_getStatsReports(int type, char* callid, void** pMediaStatisticsDataInnerArray, int *pArraySize)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins..., type: %d, callid: %s", __FUNCTION__, __LINE__, type, callid);
    if (g_statsCollector)
    {
        g_statsCollector->GetStats((StatsContentType)type, callid, pMediaStatisticsDataInnerArray, pArraySize);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return 0;
    }
    
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
    return -99;
}

void ECMedia_deletePbData(void* pbDataArray)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
    if (g_statsCollector)
    {
        g_statsCollector->DeletePbData(pbDataArray);
    }
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
}


ECMEDIA_API int ECMedia_setAudioRed(int channelid, bool enable, int payloadType)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d enable: %s payloadType: %d", __FUNCTION__, __LINE__,
                 channelid, enable?"true":"false", payloadType);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->SetREDStatus(channelid, enable, payloadType);
        rtp_rtcp->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to set audio red", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoERTP_RTCP", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
    
}

ECMEDIA_API int ECMedia_audio_enable_magic_sound(int channelid, bool is_enable)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d",
                 __FUNCTION__, __LINE__, channelid);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->enableSoundTouch(channelid, is_enable);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to enable audio magic sound", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else{
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

ECMEDIA_API int ECMedia_audio_set_magic_sound(int channelid, int pitch, int tempo, int rate)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d pitch: %d tempo: %d rate: %d",
                 __FUNCTION__, __LINE__, channelid, pitch, tempo, rate);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->setSoundTouch(channelid, pitch, tempo, rate);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio set magic sound", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else{
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
ECMEDIA_API int ECMedia_select_magic_sound_mode(int channelid, ECMagicSoundMode mode)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d mode:%d",
                 __FUNCTION__, __LINE__, channelid, (int)mode);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->selectSoundTouchMode(channelid, mode);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio set magic sound", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else{
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}
ECMEDIA_API int ECMedia_audio_set_playout_gain(int channelid, float gain)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d gain:%f",
                 __FUNCTION__, __LINE__, channelid, gain);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        //        For the time being, channelid is useless
        int ret = base->setEnlargeAudioFlagIncoming(true, gain);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio set playout gain", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else{
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

ECMEDIA_API int ECMedia_audio_set_microphone_gain(int channelid, float gain)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d gain:%f",
                 __FUNCTION__, __LINE__, channelid, gain);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        //        For the time being, channelid is useless
        int ret = base->setEnlargeAudioFlagOutgoing(true, gain);
        base->Release();
        if (ret != 0) {
            WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to audio set microphone gain", __FUNCTION__, __LINE__);
        }
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    else{
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get VoEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
}

int ECMedia_video_set_mix_mediastream(int channel, bool enable, char *mixture, unsigned char version)
{
#ifdef VIDEO_ENABLED
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...,channelid:%d ", __FUNCTION__, __LINE__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetMixMediaStream(channel, enable, mixture, version);
        network->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d end with code: %d ",__FUNCTION__, __LINE__, ret);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceMediaApi, 0, "%s:%d failed to get ViENetwork", __FUNCTION__, __LINE__);
        return -99;
    }
#endif
    return 0;
}

//add by dingxf
int ECMedia_set_remote_i420_framecallback(int channelid, yuntongxunwebrtc::ECMedia_I420FrameCallBack callback) {
#ifdef VIDEO_ENABLED
	WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... channelid: %d", __FUNCTION__, __LINE__, channelid);
	VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
	ViEBase *base = ViEBase::GetInterface(m_vie);
	if (base) {
		int ret = base->AddRemoteI420FrameCallback(channelid, callback);
		base->Release();
		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends with video channelid %d just created...", __FUNCTION__, __LINE__, channelid);
		return ret;
	}
	else
	{
		WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
		return -99;
	}
	WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
#endif
    return -1;
}

int ECMedia_releaseAll(){
#ifdef VIDEO_ENABLED
    WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... ", __FUNCTION__, __LINE__);
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->ReleaseAllUdp();
        base->Release();
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends ...", __FUNCTION__, __LINE__);
        return ret;
    }
    else
    {
        WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
        WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
        return -99;
    }
#endif
    return -1;
}

bool ECMedia_StartDesktopShareConnect(DesktopShareConnectData* pConnectData)
{
#ifdef VIDEO_ENABLED
	if (!pConnectData)
	{
		return false;
	}

	bool bRet = true;
	int nCodecPayloadType = pConnectData->nCodecPayloadType;
	bool bRtcpMultiplexing = pConnectData->bRtcpMultiplexing;
	unsigned int nVideoSsrc = pConnectData->nVideoSsrc;
	string strCodecName = pConnectData->codecName;

	if (pConnectData->nCodecPayloadType >= 0)
	{
		nCodecPayloadType = pConnectData->nCodecPayloadType;
	}
	else
	{
		nCodecPayloadType = 96;
	}

	if (strlen(pConnectData->codecName) > 0)
	{
		strCodecName = pConnectData->codecName;
	}
	else
	{
		strCodecName = "H264";
	}

	int nShareWidth = 0, nShareHeight = 0;
	int nMaxShareFps = pConnectData->nMaxFPS;
	if (nMaxShareFps <= 0)
	{
		nMaxShareFps = 5;
	}

	bool bFoundCodec = false;
	yuntongxunwebrtc::VideoCodec codec_params;

	do 
	{
		bRet |= ECMedia_audio_create_channel(pConnectData->nDesktopShareChannelId, true) == 0;
		if (!bRet) break;
		bRet |= ECMedia_allocate_desktopShare_capture(pConnectData->nDesktopShareCaptureId, 0) == 0;
		if (!bRet) break;
		bRet |= ECMedia_connect_desktop_captureDevice(pConnectData->nDesktopShareCaptureId, pConnectData->nDesktopShareChannelId) == 0;
		if (!bRet) break;
		bRet |= ECMedia_get_desktop_capture_size(pConnectData->nDesktopShareCaptureId, nShareWidth, nShareHeight) == 0;
		if (!bRet) break;
		bRet |= ECMedia_start_desktop_capture(pConnectData->nDesktopShareCaptureId, nMaxShareFps) == 0;
		if (!bRet) break;
		bRet |= ECMedia_select_screen(pConnectData->nDesktopShareCaptureId, 0);
		if (!bRet) break;
		bRet |= ECMedia_get_send_codec_video(pConnectData->nDesktopShareChannelId, codec_params) == 0;
		if (!bRet) break;

		int num_codec = ECMedia_num_of_supported_codecs_video();
		yuntongxunwebrtc::VideoCodec *codecArray = new yuntongxunwebrtc::VideoCodec[num_codec];
		bRet |= ECMedia_get_supported_codecs_video(codecArray) == 0;
		if (bRet)
		{
			if (!strCodecName.empty())
			{
				for (int i = 0; i < num_codec; i++) {
					codec_params = codecArray[i];
					if (strcmp(codec_params.plName, strCodecName.c_str()) == 0)
					{
						bFoundCodec = true;
						break;
					}
				}
			}
			if (!bFoundCodec && nCodecPayloadType >= 0)
			{
				for (int i = 0; i < num_codec; i++) {
					codec_params = codecArray[i];
					if (codec_params.plType == nCodecPayloadType)
					{
						bFoundCodec = true;
						break;
					}
				}
			}
			delete[]codecArray;
		}
		if (strcmp(codec_params.plName, "VP8") == 0)
		{
			codec_params.numberOfSimulcastStreams = 2;
		}
		else
		{
			codec_params.numberOfSimulcastStreams = 0;
		}
		codec_params.startBitrate = nShareWidth * nShareHeight * nMaxShareFps * 3 * 0.07 / 1000;
		codec_params.maxBitrate = codec_params.startBitrate;
		codec_params.minBitrate = codec_params.startBitrate >> 2;
		codec_params.width = nShareWidth;
		codec_params.height = nShareHeight;
		codec_params.maxFramerate = nMaxShareFps;
		codec_params.mode = kScreensharing;

		int bOk = 0;
		for (int count = 10, bOk = 1; bOk && count >= 0; count--)
		{
			bOk = ECMedia_video_set_local_receiver(pConnectData->nDesktopShareChannelId, pConnectData->nLocalVideoPort, pConnectData->nLocalVideoPort + (bRtcpMultiplexing ? 0 : 1));
			pConnectData->nLocalVideoPort += bOk ? (bRtcpMultiplexing ? 1 : 2) : 0;
		}
		bRet |= bOk;
		if (!bRet) break;
		for (int count = 10, bOk = 1; bOk && count >= 0; count--)
		{
			bOk = ECMedia_video_set_send_destination(pConnectData->nDesktopShareChannelId, pConnectData->ipRemote, pConnectData->nRemoteVideoPort, pConnectData->ipRemote, pConnectData->nRemoteVideoPort + (bRtcpMultiplexing ? 0 : 1));
			pConnectData->nRemoteVideoPort += bOk ? (bRtcpMultiplexing ? 1 : 2) : 0;
		}
		bRet |= bOk;
		if (!bRet) break;

		//setSsrcMediaType(nVideoSsrc, 1);
		//setSsrcMediaAttribute(nVideoSsrc, codec_params.width, codec_params.height, codec_params.maxFramerate);

		bRet |= ECMedia_video_set_local_ssrc(pConnectData->nDesktopShareChannelId, pConnectData->nVideoSsrc) == 0;
		if (!bRet) break;

		pConnectData->nVideoSsrc = nVideoSsrc;
		pConnectData->nMaxFPS = nMaxShareFps;

		bRet |= ECMedia_set_send_codec_video(pConnectData->nDesktopShareChannelId, codec_params) == 0;
		if (!bRet) break;
		bRet |= ECMedia_video_start_send(pConnectData->nDesktopShareChannelId) == 0;
		if (!bRet) break;
		bRet |= ECMedia_video_start_receive(pConnectData->nDesktopShareChannelId) == 0;

	} while (false);

	if (!bRet)
	{
		ECMedia_StopDesktopShareConnect(pConnectData);
		pConnectData->nDesktopShareChannelId = -1;
		pConnectData->nDesktopShareCaptureId = -1;
	}

	return bRet;

#endif
	return -1;
}

bool ECMedia_StopDesktopShareConnect(DesktopShareConnectData* pConnectData)
{
#ifdef VIDEO_ENABLED
	if (!pConnectData || (pConnectData->nDesktopShareChannelId < 0 && pConnectData->nDesktopShareCaptureId < 0))
	{
		return false;
	}

	bool bRet = true;
	if (pConnectData->nDesktopShareCaptureId > 0)
	{
		bRet |= ECMedia_stop_desktop_capture(pConnectData->nDesktopShareCaptureId) == 0;
		bRet |= ECMedia_stop_render(pConnectData->nDesktopShareChannelId, pConnectData->nDesktopShareCaptureId) == 0;
		bRet |= ECMedia_release_desktop_capture(pConnectData->nDesktopShareCaptureId) == 0;
	}
	bRet |= ECMedia_video_stop_receive(pConnectData->nDesktopShareChannelId) == 0;
	bRet |= ECMedia_video_stop_send(pConnectData->nDesktopShareChannelId) == 0;
	bRet |= ECMedia_delete_channel(pConnectData->nDesktopShareChannelId, true) == 0;

	return bRet;
#endif
	return -1;
}

int ECMedia_set_local_offline_video_window(int deviceid, void *video_window)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins... deviceid:%d video_window:%p ", __FUNCTION__, __LINE__, deviceid, video_window);
#ifdef VIDEO_ENABLED
	VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
	ViECapture *capture = ViECapture::GetInterface(m_vie);
	if (capture) {
		int ret = 0;
#ifdef WIN32
		ViERender* render = ViERender::GetInterface(m_vie);
		if (render)
		{
			ret = render->AddRenderer(deviceid, video_window, 1, 0, 0, 1, 1, NULL);
			if (ret) {
				render->Release();
				WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to add renderer", __FUNCTION__, __LINE__);
				WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
				return ret;
			}
			ret = render->StartRender(deviceid);
			render->Release();
			if (ret != 0) {
				WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to start render", __FUNCTION__, __LINE__);
			}
		}
		else
		{
			return -1;
			WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d render is null. ", __FUNCTION__, __LINE__);
		}
#else
		ret = capture->SetLocalVideoWindow(deviceid, video_window);
		capture->Release();
#endif
		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends... with code: %d ", __FUNCTION__, __LINE__, ret);
		return ret;
	}
	else
	{
		WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViECapture", __FUNCTION__, __LINE__);
		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
		return -99;
	}
#endif
	return -1;
}

bool ECMedia_GetVideoCodec(int nCodecPayloadType, yuntongxunwebrtc::VideoCodec *pCodecParams = nullptr, CameraCapability *pCapability = nullptr)
{
#ifdef VIDEO_ENABLED
	string strCodecName;
	CameraCapability capability;
	if (!pCapability)
	{
		capability.width = 640;
		capability.height = 480;
		capability.maxfps = 15;
	}
	else
	{
		memcpy(&capability, pCapability, sizeof(capability));
	}

	switch (nCodecPayloadType)
	{
	case 120:
		strCodecName = "VP8";
		break;
	case 96:
		strCodecName = "H264";
		break;
	default:
		nCodecPayloadType = 96;
		strCodecName = "H264";
		break;
	}

	bool codec_found = false;
	yuntongxunwebrtc::VideoCodec codec_params;

	int num_codec = ECMedia_num_of_supported_codecs_video();
	if (num_codec <= 0)
	{
		return false;
	}
	yuntongxunwebrtc::VideoCodec *codecArray = new yuntongxunwebrtc::VideoCodec[num_codec];
	if (ECMedia_get_supported_codecs_video(codecArray) != 0)
	{
		return false;
	}
	for (int i = 0; i < num_codec; i++) {
		codec_params = codecArray[i];
		if (strcmp(codec_params.plName, strCodecName.c_str()) == 0)
		{
			codec_found = true;
			codec_params.plType = nCodecPayloadType;
			break;
		}
	}
	delete[]codecArray;

	codec_params.startBitrate = capability.width * capability.height * capability.maxfps * 3 * 0.07 / 1000;
	codec_params.maxBitrate = codec_params.startBitrate > 0 ? codec_params.startBitrate : 2500;
	codec_params.minBitrate = codec_params.minBitrate > 0 ? codec_params.minBitrate : 50;

	codec_params.width = capability.width;
	codec_params.height = capability.height;
	codec_params.maxFramerate = capability.maxfps;
	if (pCodecParams)
	{
		memcpy(pCodecParams, &codec_params, sizeof(codec_params));
	}
	return true;
#endif
	return false;
}

bool ECMedia_StopOfflineVideoConnect(int nChannelId, int nDeviceId)
{
#ifdef VIDEO_ENABLED
	if (nChannelId < 0 || (nChannelId < 0 && nDeviceId < 0))
	{
		return false;
	}

	bool bRet = false;
	if (nDeviceId > 0)
	{
		bRet |= ECMedia_stop_render(nChannelId, nDeviceId) == 0;
		bRet |= ECMedia_stop_capture(nDeviceId) == 0;
	}
	bRet |= ECMedia_video_stop_receive(nChannelId) == 0;
	bRet |= ECMedia_video_stop_send(nChannelId) == 0;
	bRet |= ECMedia_delete_channel(nChannelId, true) == 0;

	return bRet;
#endif
	return false;
}

bool ECMedia_StartOfflineVideoConnect(int *pChannelId, int *pCaptureId, void* pLocalWindow, int nCodecPayloadType, yuntongxunwebrtc::VideoCodec *pCodecParams)
{
#ifdef VIDEO_ENABLED
	if (!pChannelId || !pCaptureId)
	{
		return false;
	}

	int nCount = 0;
	int nLocalVideoPort = 6778;
	int nRemoteVideoPort = 8778;
	bool bRet = true, bOk = false;
	do
	{
		bRet |= ECMedia_audio_create_channel(*pChannelId, true) == 0;
		if (!bRet) break;
		if ((nCount = ECMdeia_num_of_capture_devices()) > 0)
		{
			int nIndex = 0;
			while (nIndex < nCount)
			{
				char name[256] = { 0 }, id[256] = { 0 };
				if (ECMedia_get_capture_device(nIndex, name, sizeof(name), id, sizeof(id)) >= 0)
				{
					bOk |= ECMedia_allocate_capture_device(id, strlen(id), *pCaptureId) == 0;
					bOk |= ECmedia_enable_deflickering(*pCaptureId, true) == 0;
					bOk |= ECmedia_enable_EnableBrightnessAlarm(*pCaptureId, true) == 0;
					bOk |= ECMedia_set_CaptureDeviceID(*pCaptureId) == 0;
					bOk |= ECMedia_connect_capture_device(*pCaptureId, *pChannelId) == 0;
				}
				bRet |= bOk;
				if (bRet) break;
				nIndex++;
			}
			if (nIndex == nCount && !bRet)
			{
				break;
			}
		}
		else
		{
			WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d no found capture devices.", __FUNCTION__, __LINE__);
			bRet != false;
			break;
		}

		if (pLocalWindow)
		{
			bRet |= ECMedia_set_local_offline_video_window(*pCaptureId, pLocalWindow) == 0;
		}
		if (!bRet) break;

		CameraCapability capability;
		capability.width = 640;
		capability.height = 480;
		capability.maxfps = 15;

		yuntongxunwebrtc::VideoCodec codec_params;
		bRet |= ECMedia_GetVideoCodec(nCodecPayloadType, &codec_params, &capability);
		if (!bRet) break;
		bRet |= ECMedia_set_send_codec_video(*pChannelId, codec_params) == 0;
		if (!bRet) break;
		bRet |= ECMedia_set_receive_codec_video(*pChannelId, codec_params) == 0;
		if (!bRet) break;
		bRet |= ECMedia_start_capture(*pCaptureId, capability) == 0;
		if (!bRet) break;
		for (int count = 10, bOk = 1; !bOk && count >= 0; count--)
		{
			bOk = ECMedia_video_set_local_receiver(*pChannelId, nLocalVideoPort, nLocalVideoPort) == 0;
			nLocalVideoPort += bOk ? 0 : 1;
		}
		bRet |= bOk;
		if (!bRet) break;
		for (int count = 10, bOk = 1; !bOk && count >= 0; count--)
		{
			bOk = ECMedia_video_set_send_destination(*pChannelId, "127.0.0.1", nRemoteVideoPort, "127.0.0.1", nRemoteVideoPort) == 0;
			nRemoteVideoPort += bOk ? 0 : 1;
		}
		bRet |= bOk;
		if (!bRet) break;
		bRet |= ECMedia_video_set_local_ssrc(*pChannelId, nLocalVideoPort) == 0;
		if (!bRet) break;
		bRet |= ECMedia_video_start_send(*pChannelId) == 0;
		if (!bRet) break;
		bRet |= ECMedia_video_start_receive(*pChannelId) == 0;
		if (!bRet) break;
		if (pCodecParams)
		{
			memcpy(pCodecParams, &codec_params, sizeof(codec_params));
		}
	} while (false);

	if (!bRet)
	{
		ECMedia_StopOfflineVideoConnect(*pChannelId, *pCaptureId);
	}

	return bRet;
#endif
	return false;
}

bool ECMedia_StopOfflineAudioConnect(int nChannelId)
{
	if (nChannelId < 0)
	{
		return -1;
	}

	bool bRet = false;
	bRet |= ECMedia_DeRegister_voice_engine_observer() == 0;
	bRet |= ECMedia_audio_stop_record() == 0;
	bRet |= ECMedia_audio_stop_playout(nChannelId) == 0;
	bRet |= ECMedia_audio_stop_receive(nChannelId) == 0;
	bRet |= ECMedia_audio_stop_send(nChannelId) == 0;
	bRet |= ECMedia_delete_channel(nChannelId, false) == 0;

	return bRet;
}

bool ECMedia_StartOfflineAudioConnect(int *pChannelId)
{
	if (!pChannelId)
	{
		return false;
	}

	bool bRet = false;
	int nLocalAudioPort = 5678;
	int nRemoteAudioPort = 6789;

	do
	{
		bRet |= ECMedia_audio_create_channel(*pChannelId, false) == 0;
		if (!bRet) break;
		bRet |= ECMedia_set_local_receiver(*pChannelId, nLocalAudioPort, nLocalAudioPort) == 0;
		bRet |= ECMedia_audio_start_playout(*pChannelId) == 0;
		ECMedia_audio_set_send_destination(*pChannelId, nRemoteAudioPort, "127.0.0.1", 0, nRemoteAudioPort, "127.0.0.1");
		bRet |= ECMedia_audio_set_ssrc(*pChannelId, nLocalAudioPort, nRemoteAudioPort) == 0;
		bRet |= ECMedia_audio_start_send(*pChannelId) == 0;
		bRet |= ECMedia_audio_start_receive(*pChannelId) == 0;
		bRet |= ECMedia_audio_start_record() == 0;
	} while (false);

	if (!bRet && *pChannelId >= 0)
	{
		ECMedia_StopOfflineAudioConnect(*pChannelId);
	}

	return bRet;
}

bool ECMedia_Start_record_offline_video(int nVideoChannelId, int nCodecPayloadType, const char* pFilename, yuntongxunwebrtc::VideoCodec *pCodecParams = nullptr)
{
#ifdef VIDEO_ENABLED
	CodecInst audio_codec;
	int num_codec = ECMedia_num_of_supported_codecs_audio();
	if (num_codec > 0)
	{
		yuntongxunwebrtc::CodecInst *audioCodecArray = new yuntongxunwebrtc::CodecInst[num_codec];
		if (m_vie && ECMedia_get_supported_codecs_audio(audioCodecArray) == 0)
		{
			for (int i = 0; i < num_codec; i++)
			{
				audio_codec = audioCodecArray[i];
				if (strcmp(audio_codec.plname, "PCMU") == 0)
					//if (audio_codec.pltype == 110)
				{
					break;
				}
			}
			delete[]audioCodecArray;

			yuntongxunwebrtc::VideoCodec codec_params;
			if (pCodecParams)
			{
				memcpy(&codec_params, pCodecParams, sizeof(codec_params));
			}
			else
			{
				if (!ECMedia_GetVideoCodec(nCodecPayloadType, &codec_params))
				{
					return false;
				}
			}

			ViEFile *file_record = ViEFile::GetInterface(m_vie);
			if (file_record)
			{
				memcpy(codec_params.plName, "I420", kPayloadNameSize);
				if (file_record->StartRecordOutgoingVideo(nVideoChannelId, pFilename, AudioSource::MICROPHONE, audio_codec, codec_params) == 0)
				{
					file_record->Release();
					return true;
				}
			}
		}
	}
#endif
	return false;
}

bool ECMedia_Stop_record_offline_video(int nVideoChannelId)
{
#ifdef VIDEO_ENABLED
	if (m_vie)
	{
		ViEFile *file_record = ViEFile::GetInterface(m_vie);
		if (file_record)
		{
			if (file_record->StopRecordOutgoingVideo(nVideoChannelId) == 0)
			{
				file_record->Release();
				return true;
			}
		}
	}
#endif
	return false;
}

bool ECMedia_StartStoreLocalOfflineMediaToFile(int *pAudioChannelId, int *pVideoChannelId, int *pDeviceCaptureId, void *pLocalWindow, int nCodecPayloadType, const char* pFilename)
{
#ifdef VIDEO_ENABLED
	yuntongxunwebrtc::VideoCodec videoCodec;
	if (ECMedia_StartOfflineAudioConnect(pAudioChannelId) && ECMedia_StartOfflineVideoConnect(pVideoChannelId, pDeviceCaptureId, pLocalWindow, nCodecPayloadType, &videoCodec))
	{
		if (ECMedia_start_record_local_video(*pAudioChannelId, *pVideoChannelId, pFilename) == 0)
		{
			return true;
		}
		else
		{
			ECMedia_StopOfflineAudioConnect(*pAudioChannelId);
			ECMedia_StopOfflineVideoConnect(*pVideoChannelId, *pDeviceCaptureId);
		}
	}
#endif
	return false;
}

bool ECMedia_StopStoreLocalOfflineMediaToFile(int nAudioChannelId, int nVideoChannelId, int nDeviceId)
{
#ifdef VIDEO_ENABLED
	bool bOk = true;
	bOk |= ECMedia_stop_record_local_video(nAudioChannelId, nVideoChannelId) == 0;
	bOk |= ECMedia_StopOfflineAudioConnect(nAudioChannelId);
	bOk |= ECMedia_StopOfflineVideoConnect(nVideoChannelId, nDeviceId);
	return bOk;
#else
	return false;
#endif
}

int ECMedia_SetLocalRenderWatermark(int nVideoDeviceId, bool bRenderWatermark)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d begins...", __FUNCTION__, __LINE__);
//#ifdef VIDEO_ENABLED
//	VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
//
//	ViECapture *caputure = ViECapture::GetInterface(m_vie);
//	if (caputure) {
//		int ret = caputure->SetLocalRenderWatermark(nVideoDeviceId, bRenderWatermark);
//		caputure->Release();
//		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends with video channelid %d, jbRenderWatermark: %d...", __FUNCTION__, __LINE__, nVideoDeviceId, bRenderWatermark);
//		return ret;
//	}
//	else {
//		WEBRTC_TRACE(kTraceError, kTraceMediaApi, 0, "%s:%d failed to get ViEBase", __FUNCTION__, __LINE__);
//		WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
//		return -99;
//	}
//
//	return 0;
//#endif
	WEBRTC_TRACE(kTraceApiCall, kTraceMediaApi, 0, "%s:%d ends...", __FUNCTION__, __LINE__);
	return -99;
}
