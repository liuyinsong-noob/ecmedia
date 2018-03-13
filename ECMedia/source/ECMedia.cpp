//
//  ECMedia.c
//  servicecoreVideo
//
//  Created by Sean Lee on 15/6/8.
//
//

#include <string>
#include <ctype.h>

#ifdef WIN32
#include "curl_post.h"
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
#include "codingHelper.h"
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

#include "base64.h"
#include "MediaStatisticsData.pb.h"


// char *filename_path_yuv;

#ifdef ENABLE_LIB_CURL
#ifdef WIN32
CurlPost *g_curlpost = nullptr;
#endif
#endif

#define WEBRTC_TRACE_FILTER \
cloopenwebrtc::kTraceStateInfo | cloopenwebrtc::kTraceWarning | cloopenwebrtc::kTraceError | cloopenwebrtc::kTraceCritical | \
cloopenwebrtc::kTraceApiCall

enum {
    ERR_SDK_ALREADY_INIT =-1000,
    ERR_NO_MEMORY,
    ERR_ENGINE_UN_INIT,
    ERR_INVALID_PARAM,
    ERR_NOT_SUPPORT
};

#define AUDIO_ENGINE_UN_INITIAL_ERROR(ret) \
if(!m_voe) { \
PrintConsole("[ECMEDIA ERROR] %s m_voe is NULL.",__FUNCTION__); \
PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret); \
return ret;}

#ifdef VIDEO_ENABLED
#define VIDEO_ENGINE_UN_INITIAL_ERROR(ret) \
if(!m_vie) { \
PrintConsole("[ECMEDIA ERROR] %s m_vie is NULL.",__FUNCTION__);\
PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret); \
return ret;}
#endif
#ifdef VIDEO_ENABLED
class ECViECaptureObserver :public ViECaptureObserver
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
#endif
#ifdef VIDEO_ENABLED
ECViECaptureObserver::ECViECaptureObserver(onEcMediaNoCameraCaptureCb fp)
:m_firstStart(true),
m_callback(fp)
{
    
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
cloopenwebrtc::VoiceEngine* m_voe = NULL;
static StatsCollector *g_statsCollector = NULL;

static VoeObserver* g_VoeObserver = NULL;
#ifdef WIN32
bool g_bGlobalAudioInDevice = false;
HWAVEIN g_hWaveIn = NULL;
#endif
static onEcMediaNoCameraCaptureCb g_NoCameraCaptureCb = NULL;
#ifdef VIDEO_ENABLED
static ECViECaptureObserver* g_ECViECaptureObserver = NULL;
#endif


#ifdef VIDEO_ENABLED
cloopenwebrtc::VideoEngine* m_vie = NULL;
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

static CameraInfo *m_cameraInfo = NULL;

static char gVersionString[256]={'\0'};

static int m_cameraCount = 0;
using namespace cloopenwebrtc;
using namespace std;

#define ECMEDIA_VERSION "ecmedia_version: 2.3.1.0-beta1"

//extern bool g_media_TraceFlag;
//void PrintConsole(const char * fmt,...){};

///////////////////////////log////////////////////////////////////////////////////
//#define   _CRTDBG_MAP_ALLOC
//#include "serphoneinterface.h"
//#include "servicecore.h"
//#include "serphonecall.h"
//#include "friends.h"
//#include "lpconfig.h"
//#include "enum.h"
#include "../system_wrappers/include/critical_section_wrapper.h"

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

bool g_media_TraceFlag = false;//
const char * g_log_media_filename = "./mediaConsole.log";
FILE *g_media_interface_fp = NULL;
#define MAX_LOG_LINE   3000
//int g_log_line =0;
#define MAX_LOG_SIZE    104857600//100M bytes

long long g_max_log_size = MAX_LOG_SIZE;

typedef void(*PrintConsoleHook_media)(int loglevel, const char *);
PrintConsoleHook_media gPrintConsoleHook_media = NULL;
cloopenwebrtc::CriticalSectionWrapper  *g_printConsole_lock;
static void media_init_print_log()
{
    if (!g_media_TraceFlag) {
        return;
    }
    g_printConsole_lock = cloopenwebrtc::CriticalSectionWrapper::CreateCriticalSection();
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
        cloopenwebrtc::CriticalSectionScoped lock(g_printConsole_lock);
        if (g_media_interface_fp)
            fclose(g_media_interface_fp);
        g_media_interface_fp = NULL;
    }
    
    if (g_printConsole_lock)
        delete g_printConsole_lock;
    g_printConsole_lock = NULL;
}

void PrintConsole(const char * fmt, ...)
{
    if (!g_media_TraceFlag) {
        return;
    }
    
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
    char log_buffer[2048] = { 0 };
    va_list ap;
    va_start(ap, fmt);
#ifndef WIN32
    int count = sprintf(log_buffer, "%02d%02d %02d:%02d:%02d ",
                        pt->tm_mon + 1, pt->tm_mday,
                        pt->tm_hour, pt->tm_min, pt->tm_sec);
    if (count > 0)
        vsnprintf(log_buffer + count, 2047 - count, fmt, ap);
#else
    int count = sprintf(log_buffer, "%02d%02d %02d:%02d:%02d ",
                        pt->tm_mon + 1, pt->tm_mday,
                        pt->tm_hour, pt->tm_min, pt->tm_sec);
    if (count > 0)
        _vsnprintf(log_buffer + count, 2047 - count, fmt, ap);
#endif
    
    va_end(ap);
    
#ifdef WEBRTC_ANDROID
    __android_log_print(ANDROID_LOG_DEBUG, "console", "%s", log_buffer);
#else
    printf("%s\n", log_buffer);
#endif
    
    if (gPrintConsoleHook_media)
        gPrintConsoleHook_media(0, log_buffer);
    
    if (NULL == g_printConsole_lock) {
        return;
    }
    cloopenwebrtc::CriticalSectionScoped lock(g_printConsole_lock);
    if (g_media_interface_fp) {
        fprintf(g_media_interface_fp, "%s\n", log_buffer);
        fflush(g_media_interface_fp);
        //g_log_line ++;
        
        if (ftell(g_media_interface_fp) >= g_max_log_size)//100M,100*1024*1024
        {
            fclose(g_media_interface_fp);
            g_media_interface_fp = NULL;
            
            char new_file[1024];
            strcpy(new_file, g_log_media_filename);
            char file_postfix[40] = { 0 };
            sprintf(file_postfix, "_%04d%02d%02d%02d%02d%02d.bak",
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
        const char *platform ="unknow",*arch = "arm",*voice="voice=false",*video="video=false";
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
        sprintf(gVersionString,"%s#%s#%s#%s#%s#%s %s",ECMEDIA_VERSION,platform,arch,voice, video,__DATE__,__TIME__);
    }
    
    return gVersionString;
}



//////////////////////////////////////////////////////////////////////////////////////////////



namespace cloopenwebrtc {
    class ECMediaTraceCallBack : public TraceCallback {
    public:
        virtual void Print(const TraceLevel level,
                           const char *traceString,
                           const int length)
        {
            PrintConsole("%s\n",traceString);
        }
    };
}

cloopenwebrtc::ECMediaTraceCallBack g_mediaTraceCallBack;


int ECMedia_set_trace(const char *logFileName,void *printhoolk,int level, int lenMb)
{
    uint32_t nLevel=WEBRTC_TRACE_FILTER;
    g_media_TraceFlag = true;//
    if(NULL!=printhoolk)
    {
        gPrintConsoleHook_media=(PrintConsoleHook_media)printhoolk;
    }
    if(NULL!=logFileName)
    {
        g_log_media_filename=logFileName;
    }
    if (lenMb > 0)
        g_max_log_size = lenMb * 1024 * 1024;
    
    media_init_print_log();
    PrintConsole("[ECMEDIA INFO] %s begins...",__FUNCTION__);
    PrintConsole("[ECMEDIA INFO] ECMedia version:%s", ECMedia_get_Version());
    Trace::CreateTrace();
    Trace::SetTraceCallback(&g_mediaTraceCallBack);
    
    switch(level)
    {
        case 20:
        {
            nLevel=kTraceError;
            break;
        }
        case 21:
        {
            nLevel=kTraceError|kTraceCritical;
            break;
        }
        case 22:
        {
            nLevel=kTraceError|kTraceCritical|kTraceWarning;
            break;
        }
        case 23:
        {
            nLevel=kTraceDefault;
            break;
        }
        case 24:
        {
            nLevel=kTraceDefault|kTraceInfo|kTraceDebug;
            break;
        }
        default:
        {
            if(nLevel>24)
            {
                nLevel=kTraceAll;
            }
            break;
        }
    }
    Trace::set_level_filter(nLevel);
    PrintConsole("[ECMEDIA INFO] ECmedia version: %s", ECMedia_get_Version());
    PrintConsole("[ECMEDIA INFO] %s ends...",__FUNCTION__);
    return 0;
}
int ECMedia_un_trace()
{
    if (!g_media_TraceFlag) {
        return -1;
    }
    g_media_TraceFlag = false;//
    media_uninit_print_log();
    Trace::ReturnTrace();
    return 0;
}

void ECMedia_set_android_objects(void* javaVM, void* env, void* context)
{
#if !defined(NO_VOIP_FUNCTION)
    cloopenwebrtc::VoiceEngine::SetAndroidObjects(javaVM,env,context);
#ifdef VIDEO_ENABLED
    cloopenwebrtc::VideoEngine::SetAndroidObjects(javaVM,env,context);
#endif
    
#endif
}

int ECMedia_init_video()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    m_pScreenlist = NULL;
    m_pWindowlist = NULL;
    
    if (m_vie)
    {
        PrintConsole("[ECMEDIA WARNNING] %s Video engine already create", __FUNCTION__);
        return 1;
    }
    
    //VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    m_vie = VideoEngine::Create();
    if ( NULL == m_vie)
    {
        PrintConsole("[ECMEDIA ERROR] %s Create Video engine fail", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;;
    }
    ViEBase* videobase = ViEBase::GetInterface(m_vie);
    PrintConsole("[ECMEDIA INFO] %s Init Video Engine...", __FUNCTION__);
    if(videobase->Init()!= 0) {
        int lastError = videobase->LastError(); //base init failed
        PrintConsole("[ECMEDIA ERROR] %s Init Video Engine error, error code is %d", __FUNCTION__, lastError);
        videobase->Release();
        VideoEngine::Delete(m_vie);
        m_vie = NULL;
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return lastError;
    }
    else {
        PrintConsole("[ECMEDIA INFO] %s Init Video Engine...OK", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

int ECMedia_uninit_video()
{
    PrintConsole("[ECMEDIA INFO] %s begins...",__FUNCTION__);
#ifdef VIDEO_ENABLED
    if (m_pScreenlist != NULL)
        delete[] m_pScreenlist;
    if (m_pWindowlist != NULL)
        delete[] m_pWindowlist;
    if(!m_vie)
    {
        PrintConsole("[ECMEDIA ERROR] %s Video Engine is null",__FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
    
}

int ECMedia_ring_stop(int& channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and Channel ID: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if ( channelid >=0 )
    {
        PrintConsole("[ECMEDIA INFO] %s Stop play ring, channelID=%d", __FUNCTION__, channelid);
        VoEBase* base = VoEBase::GetInterface(m_voe);
        VoEFile* file  = VoEFile::GetInterface(m_voe);
        if ( file->IsPlayingFileAsMicrophone(channelid) >=0)
        {
            PrintConsole("[ECMEDIA INFO] %s Stop play ring file locally, channelID=%d", __FUNCTION__, channelid);
            file->StopPlayingFileLocally( channelid );
        }
        //add by gyf to stop play ring file
        base->StopPlayout(channelid);
        base->DeleteChannel( channelid );
        file->Release();
        base->Release();
        channelid = -1;
    }
    PrintConsole("[ECMEDIA INFO] %s ends... ", __FUNCTION__);
    return 0;
}


//In this func, channel is created automatically. You should store this channel to use in func ring_stop.
int ECMedia_ring_start(int& channelid, const char *filename, bool loop)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d filename: %s loop: %s", __FUNCTION__, channelid, filename, loop?"true":"false");
    FILE *fp  = fopen(filename,"r") ;
    if( fp == NULL ) {
        PrintConsole("[ECMEDIA ERROR] %s open file failed", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
        PrintConsole("[ECMEDIA INFO] %s playfile is processing,channelID=%d,name:%s,ret:%d\n", __FUNCTION__, channelid, filename, ret);
        
        if (ret >=0)
        {
            base->StartPlayout(channelid);
        }
        base->Release();
        file->Release();
    }
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

/*
 VoEBase functions
 */

int ECMedia_init_audio()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    if(m_voe)
    {
        PrintConsole("[ECMEDIA WARNNING] %s Voice engine already create", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 1;
    }
    m_voe = VoiceEngine::Create();
    
    if ( NULL == m_voe)
    {
        PrintConsole("[ECMEDIA ERROR] %s Create Voice engine fail", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    VoEBase* base = VoEBase::GetInterface(m_voe);
    PrintConsole("[ECMEDIA INFO] %s Init Voice Engine...", __FUNCTION__);
    if( base->Init() != 0) {
        VoiceEngine::Delete(m_voe);
        m_voe = NULL;
        
        PrintConsole("[ECMEDIA ERROR] %s Init Voice Engine Error, error code is %d", __FUNCTION__, base->LastError());
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return base->LastError(); //base init failed
    }
    else {
        PrintConsole("[ECMEDIA INFO] %s Init Voice Engine...OK", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...",__FUNCTION__);
    return 0;
}

int ECMedia_uninit_audio()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef ENABLE_LIB_CURL
#ifdef WIN32
    if (g_curlpost)
    {
        delete g_curlpost;
        g_curlpost = nullptr;
    }
#endif
#endif
    if( !m_voe)
    {
        PrintConsole("[ECMEDIA ERROR] %s audio engine is null", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

int ECMedia_audio_create_channel(int& channelid, bool is_video)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., create %s channel, channelid is: %d", __FUNCTION__, is_video?"video":"audio", channelid);
    if (!is_video) {
        AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        VoEBase *base = VoEBase::GetInterface(m_voe);
        if (base) {
            channelid = base->CreateChannel();
            base->Release();
            PrintConsole("[ECMEDIA INFO] %s ends with channelid %d just created...", __FUNCTION__, channelid);
            return 0;
        }
        else
        {
            PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
            PrintConsole("[ECMEDIA INFO] %s ends with video channelid %d just created...", __FUNCTION__, channelid);
            return 0;
        }
        else
        {
            PrintConsole("[ECMEDIA ERROR] %s failed to get ViEBase", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            channelid = -1;
            return -99;
        }
#endif
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -1;
    }
}

bool ECMedia_get_recording_status()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        bool ret = base->GetRecordingIsInitialized();
        base->Release();
        PrintConsole("[ECMEDIA INFO] %s ends.. with code:%d", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_delete_channel(int& channelid, bool is_video)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., delete %s channel, channelid:%d",__FUNCTION__, is_video?"video":"audio", channelid);
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
                PrintConsole("[ECMEDIA ERROR] %s failed to delete channel:%d, ret:%d",__FUNCTION__, channelid, ret);
            }
            base->Release();
            PrintConsole("[ECMEDIA INFO] %s ends deleting audio channelid %d... with code: %d", __FUNCTION__, channelid, ret);
            return ret;
        }
        else
        {
            PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -99;
        }
    }
    else
    {
#ifdef VIDEO_ENABLED
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViEBase *base = ViEBase::GetInterface(m_vie);
        if (base) {
            int ret = base->DeleteChannel(channelid);
            if (ret == 0) {
                channelid = -1;
            }
            else
            {
                PrintConsole("[ECMEDIA ERROR] failed to delete channel:%d, ret:%d", __FUNCTION__, channelid, ret);
            }
            base->Release();
            PrintConsole("[ECMEDIA INFO] %s ends deleting video channelid %d... with code: %d", __FUNCTION__, channelid, ret);
            return ret;
        }
        else
        {
            PrintConsole("[ECMEDIA ERROR] %s failed to get ViEBase", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -99;
        }
#endif
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -1;
    }
}

int ECMedia_audio_set_ssrc(int channelid, unsigned int localssrc, unsigned int remotessrc)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid:%d, localssrc: %u, remotessrc %u", __FUNCTION__,channelid, localssrc, remotessrc);
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
        PrintConsole("[ECMEDIA INFO] %s end with localssrc set: %d, remotessrc set %d", __FUNCTION__, localret, remoteret);
        return (localret + remoteret);
    }
    else
    {
        PrintConsole("[ECMEDIA WARNNING] failed to set video ssrc, %s", __FUNCTION__);
        return -99;
    }
}
int ECMedia_set_local_receiver(int channelid, int rtp_port, int rtcp_port, bool ipv6)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid:%d rtp_port:%d rtcp_port:%d, ipv6 %s",__FUNCTION__, channelid, rtp_port, rtcp_port, ipv6?"YES":"NO");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetLocalReceiver(channelid, rtp_port, rtcp_port, ipv6);
        base->Release();
        PrintConsole("[ECMEDIA INFO] %s end with code: %d ",__FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA WARNNING] %s failed to get VoEBase",__FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_set_send_destination(int channelid, int rtp_port, const char *rtp_addr, int source_port, int rtcp_port, const char *rtcp_ipaddr)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid:%d rtp_port:%d rtp_addr:%s source_port:%d rtcp_port:%d rtcp_ipaddr:%s",
                 __FUNCTION__, channelid, rtp_port, rtp_addr?"NULL":rtp_addr, source_port, rtcp_port, rtcp_ipaddr);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetSendDestination(channelid, rtp_port, rtp_addr, source_port, rtcp_port, rtcp_ipaddr);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set send destination", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else{
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_set_socks5_send_data(int channel_id, unsigned char *data, int length, bool isRTCP)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel_id);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetSocks5SendData(channel_id, data, length, isRTCP);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set socks5 send data", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else{
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

#ifdef VIDEO_ENABLED
int ECMedia_video_start_receive(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    g_statsCollector->AddVideoRecvStatsProxy(channelid);
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StartReceive(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video start receive", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_stop_receive(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (g_statsCollector) {
        g_statsCollector->DeleteVideoRecvStatsProxy(channelid);
    }
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StopReceive(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video stop receive", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_start_send(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StartSend(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video start send", __FUNCTION__);
        }
		else
			g_statsCollector->AddVideoSendStatsProxy(channelid);
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_stop_send(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
   
    ViEBase *base = ViEBase::GetInterface(m_vie);
    if (base) {
        int ret = base->StopSend(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video stop send", __FUNCTION__);
        }
		else
		{
			if (g_statsCollector) {
				g_statsCollector->DeleteVideoSendStatsProxy(channelid);
			}
		}
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
#endif
int ECMedia_audio_start_playout(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartPlayout(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio start playout", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_stop_playout(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopPlayout(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio stop playout", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
int ECMedia_audio_start_record()
{
    PrintConsole("[ECMEDIA INFO] %s begins...",__FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartRecord();
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio start record", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase",__FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_stop_record()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopRecord();
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio stop record", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_soundcard_on_cb(onSoundCardOn soundcard_on_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->RegisterSoundCardOnCb(soundcard_on_cb);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set soundcard on cb", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_send_telephone_event_payload_type(int channelid, unsigned char type)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d type: %d", __FUNCTION__, channelid, type);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEDtmf* dtmf = VoEDtmf::GetInterface(m_voe);
    if (dtmf) {
        int ret = dtmf->SetSendTelephoneEventPayloadType(channelid, type);
        dtmf->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set send telephone event payload type", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_recv_telephone_event_payload_type(int channelid, unsigned char type)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d type: %d", __FUNCTION__, channelid, type);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEDtmf* dtmf = VoEDtmf::GetInterface(m_voe);
    if (dtmf) {
        int ret = dtmf->SetRecvTelephoneEventPayloadType(channelid, type);
        dtmf->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set recv telephone event payload type", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_send_dtmf(int channelid, const char dtmfch)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d, dtmf: %c", __FUNCTION__, channelid, dtmfch);
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
        PrintConsole("[ECMEDIA ERROR] %s invalid dtmf char %c", __FUNCTION__, dtmfch);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -100;
    }
    
    VoEDtmf* dtmf  = VoEDtmf::GetInterface(m_voe);
    
    //dtmf->SendTelephoneEvent(call->m_AudioChannelID, playtone,false);
    if(dtmf){
        int ret = dtmf->SendTelephoneEvent(channelid, playtone, true);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to send telephone event", __FUNCTION__);
        }
        ret = dtmf->PlayDtmfTone(playtone);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to play dtmf tone", __FUNCTION__);
        }
        dtmf->Release();
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}


int ECMedia_set_dtmf_cb(int channelid, onEcMediaReceivingDtmf dtmf_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d ", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetDtmfCb(channelid, dtmf_cb);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set dtmf cb", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
int ECMedia_set_media_packet_timeout_cb(int channelid, onEcMediaPacketTimeout media_timeout_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetMediaTimeoutCb(channelid, media_timeout_cb);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set media timeout cb", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_stun_cb(int channelid, onEcMediaStunPacket stun_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetStunCb(channelid, stun_cb);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set stun cb", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_audio_data_cb(int channelid, onEcMediaAudioData audio_data_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetAudioDataCb(channelid, audio_data_cb);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set audio data cb", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_pcm_audio_data_cb(int channelid, ECMedia_PCMDataCallBack callback) {
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->SetPCMAudioDataCallBack(channelid, callback);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set pcm audio data cb", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_setECMedia_ConferenceParticipantCallback(int channelid, ECMedia_ConferenceParticipantCallback* callback) {
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->setConferenceParticipantCallback(channelid, callback);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set conference participant", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_video_data_cb(int channelid, onEcMediaVideoDataV video_data_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
#ifdef VIDEO_ENABLED
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->setVideoDataCb(channelid, video_data_cb);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video data cb", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
#endif
    PrintConsole("[ECMEDIA ERROR] %s don't support video", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_set_voe_cb(int channelid, onVoeCallbackOnError voe_callback_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
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
            PrintConsole("[ECMEDIA ERROR] %s failed to register voice engine observer", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    
}

int ECMedia_sendRaw(int channelid, int8_t *data, uint32_t length, bool isRTCP, uint16_t port, const char* ip)
{
    PrintConsole("[ECMEDIA INFO] %s begins... ,channelid %d, data:%0x len:%d isRTCP:%d port:%d ip:%s",
                 __FUNCTION__, channelid, data, length, isRTCP, port, ip);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret=base->SendRaw(channelid, data, length, isRTCP, port, ip);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to send raw", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_start_receive(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    g_statsCollector->AddAudioRecvStatsProxy(channelid);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartReceive(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio start receive", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_stop_receive(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (g_statsCollector) {
        g_statsCollector->DeleteAudioRecvStatsProxy(channelid);
    }
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopReceive(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio stop receive", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_start_send(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    g_statsCollector->AddAudioSendStatsProxy(channelid);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StartSend(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio start send", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_audio_stop_send(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (g_statsCollector) {
        g_statsCollector->DeleteAudioSendStatsProxy(channelid);
    }
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->StopSend(channelid);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio stop send", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->DeRegisterVoiceEngineObserver();
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to deregister voice engine observer", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

/*
 * AUDIO PROCESSING
 */
int ECMedia_set_AgcStatus(bool agc_enabled, cloopenwebrtc::AgcModes agc_mode)
{
    PrintConsole("[ECMEDIA INFO] %s begins... agc_enabled=%d agc_mode=%d", __FUNCTION__, agc_enabled, agc_mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetAgcStatus(agc_enabled, agc_mode);
        audio->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set agc status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEAudioProcessing", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_EcStatus(bool ec_enabled, cloopenwebrtc::EcModes ec_mode)
{
    PrintConsole("[ECMEDIA INFO] %s begins... ec_enabled=%d ec_mode=%d", __FUNCTION__, ec_enabled, ec_mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetEcStatus(ec_enabled, ec_mode);
        audio->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set ec status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEAudioProcessing", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_NsStatus(bool ns_enabled, cloopenwebrtc::NsModes ns_mode)
{
    PrintConsole("[ECMEDIA INFO] %s begins... ns_enabled=%s ns_mode=%d", __FUNCTION__, ns_enabled?"true":"false", ns_mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetNsStatus(ns_enabled, cloopenwebrtc::kNsVeryHighSuppression);
        audio->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set ns status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEAudioProcessing", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_SetAecmMode(cloopenwebrtc::AecmModes aecm_mode, bool cng_enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins... aecm_mode=%d cng_enabled=%s", __FUNCTION__, aecm_mode, cng_enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->SetAecmMode(aecm_mode,  cng_enabled);
        audio->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set aecm mode", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] failed to get VoEAudioProcessing, %s", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_EnableHowlingControl(bool enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins... enabled:%s", __FUNCTION__, enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        int ret = audio->EnableHowlingControl(enabled);
        audio->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable howling control", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEAudioProcessing", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_IsHowlingControlEnabled(bool &enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEAudioProcessing *audio = VoEAudioProcessing::GetInterface(m_voe);
    if (audio) {
        enabled = audio->IsHowlingControlEnabled();
        audio->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEAudioProcessing", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

/*
 * NETWORK
 */
//VoENetwork
int ECMedia_set_packet_timeout_noti(int channel, int timeout)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d, timeout: %d", __FUNCTION__, channel, timeout);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        int ret = 0;
        ret = network->SetPacketTimeoutNotification(channel, true, timeout);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set packet timeout notification", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_packet_timeout_noti(int channel, bool& enabled, int& timeout)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        int ret = 0;
        //        ret = network->GetPacketTimeoutNotification(channel, enabled, timeout);
        network->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_network_statistic(int channelid_audio, int channelid_video, long long *duration, long long *sendTotalSim,
                                  long long *recvTotalSim, long long *sendTotalWifi, long long *recvTotalWifi)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid_audio:%d, channelid_video:%d", __FUNCTION__, channelid_audio, channelid_video);
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
    PrintConsole("[ECMEDIA INFO] %s ends...",__FUNCTION__);
    return 0;
}

//ViENetwork
#ifdef VIDEO_ENABLED
int ECMedia_video_set_local_receiver(int channelid, int rtp_port, int rtcp_port, bool ipv6)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid: %d, rtp_port: %d rtcp_port: %d ipv6: %s", __FUNCTION__, channelid, rtp_port, rtcp_port, ipv6?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetLocalReceiver(channelid, rtp_port, rtcp_port, ipv6);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video set local receiver", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_set_socks5_send_data(int channel_id, unsigned char *data, int length, bool isRTCP)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid: %d ", __FUNCTION__, channel_id);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetSocks5SendData(channel_id, data, length, isRTCP);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set socks5 send data", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_set_send_destination(int channelid, const char *rtp_addr, int rtp_port, const char *rtcp_addr, int rtcp_port)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid: %d, rtp_addr: %s rtp_port: %d rtcp_port: %d rtcp_addr: %s", __FUNCTION__, channelid, rtp_addr, rtp_port, rtcp_port, rtcp_addr);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetSendDestination(channelid, rtp_addr, rtp_port, rtcp_addr , rtcp_port);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video set send destination", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_MTU(int channelid, int mtu)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid: %d, mtu: %d", __FUNCTION__, channelid, mtu);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->SetMTU(channelid, mtu);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set mtu", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

/*
 * RTP_RTCP
 */
int ECMedia_set_video_rtp_keepalive(int channelid, bool enable, int interval, int payloadType)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d, enable: %s interval: %d payloadType: %d", __FUNCTION__,
                 channelid, enable?"true":"false", interval, payloadType);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret=0;
        ret = rtp_rtcp->SetRTPKeepAliveStatus(channelid, enable, payloadType, interval);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video rtp keepalive", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_set_local_ssrc(int channelid, unsigned int ssrc)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid: %d, ssrc: %u", __FUNCTION__, channelid, ssrc);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->SetLocalSSRC(channelid, ssrc, kViEStreamTypeNormal, 0);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video set local ssrc", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_request_remote_ssrc(int channelid, unsigned int ssrc)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid: %d, ssrc: %u", __FUNCTION__, channelid, ssrc);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->RequestRemoteSSRC(channelid, ssrc);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video request remote ssrc", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_cancel_remote_ssrc(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d ", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->CancelRemoteSSRC(channelid);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video cancel remote ssrc", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
#endif

int ECMedia_set_audio_rtp_keepalive(int channelid, bool enable, int interval, int payloadType)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d enable: %s interval: %d payloadType: %d", __FUNCTION__,
                 channelid, enable?"true":"false", interval, payloadType);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->SetRTPKeepAliveStatus(channelid, enable, payloadType, interval);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set audio rtp keepalive", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_NACK_status(int channelid, bool enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d, enabled: %s",__FUNCTION__, channelid, enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetNACKStatus(channelid, enabled);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set nack status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_RTCP_status(int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetRTCPStatus(channelid, true);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set rtcp status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_media_statistics(int channelid, bool is_video, MediaStatisticsInfo& call_stats)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    if(channelid == -1) {
        PrintConsole("[ECMEDIA ERROR] %s wrong channenl id", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -1;
    }
    if(!is_video)
    {
        AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
        if(rtp_rtcp){
            cloopenwebrtc::CallStatistics stats;
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
            cloopenwebrtc::CallStatistics stats;
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

int ECMedia_start_rtp_dump(int channelid, bool is_video, const char *file, RTPDirections dir)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d is_video: %s file: %s dir: %d", __FUNCTION__,
                 channelid, is_video?"true":"false", file, dir);
    if(channelid == -1)
    {
        PrintConsole("[ECMEDIA ERROR] %s wrong channelid id", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
    return ret;
}

int ECMedia_stop_rtp_dump(int channelid, bool is_video, RTPDirections dir)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d is_video: %s dir: %d", __FUNCTION__,
                 channelid, is_video ? "true" : "false", dir);
    if(channelid == -1)
    {
        PrintConsole("[ECMEDIA ERROR] %s wrong channelid id", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
    return ret;
}

/*
 * HARDWARE
 */
int ECMedia_get_playout_device_num(int& speaker_count)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if( hardware)
    {
        int ret = 0;
        ret = hardware->GetNumOfPlayoutDevices(speaker_count);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get playout device num", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//Mem to store device name should be allocated and at least 128 bytes. So does guid.
int ECMedia_get_specified_playout_device_info(int index, char *name, char *guid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... index: %d", __FUNCTION__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfPlayoutDevices(count);
        if (ret != 0) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s failed to check playout device count", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s index range exception", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -100;
        }
        ret = hardware->GetPlayoutDeviceName(index, name, guid);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get playout device name", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_select_playout_device(int index)
{
    PrintConsole("[ECMEDIA INFO] %s begins... index: %d", __FUNCTION__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfPlayoutDevices(count);
        if (ret != 0) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s failed to check playout device count", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s index range exception", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -100;
        }
        ret = hardware->SetPlayoutDevice(index);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set playout device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_record_device_num(int& microphone_count)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if( hardware)
    {
        int ret = 0;
        ret = hardware->GetNumOfRecordingDevices(microphone_count);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get recording device num", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//Mem to store device name should be allocated and at least 128 bytes. So does guid.
int ECMedia_get_specified_record_device_info(int index, char *name, char *guid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... index: %d", __FUNCTION__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfRecordingDevices(count);
        if (ret != 0) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s failed to check record device count", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s index range exception", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -100;
        }
        ret = hardware->GetRecordingDeviceName(index, name, guid);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get recording device name", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_select_record_device(int index)
{
    PrintConsole("[ECMEDIA INFO] %s begins... index: %d", __FUNCTION__, index);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if(hardware)
    {
        int count = 0;
        int ret = hardware->GetNumOfRecordingDevices(count);
        if (ret != 0) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s failed to check playout device count", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return ret;
        }
        if (index >= count) {
            hardware->Release();
            PrintConsole("[ECMEDIA ERROR] %s index range exception", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -100;
        }
        ret = hardware->SetRecordingDevice(index);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set recording device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_loudspeaker_status(bool enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins... enabled=%s", __FUNCTION__, enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if( hardware)
    {
        int ret = 0;
        ret = hardware->SetLoudspeakerStatus(enabled);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set loudspeaker status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_loudpeaker_status(bool& enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if (hardware) {
        int ret = hardware->GetLoudspeakerStatus(enabled);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get loudspeaker status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... enabled=%d", __FUNCTION__, enabled);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_reset_audio_device()
{
    PrintConsole("[ECMEDIA INFO] %s begins...",__FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if (hardware) {
        int ret = hardware->ResetAudioDevice();
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to reset audio device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_global_audio_in_device(bool enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins. enabled=%d",__FUNCTION__, enabled?"true":"false");
#ifdef WIN32
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        bool bAudioRecording = base->GetRecordingIsRecording();
        base->Release();
        if (bAudioRecording == true)
        {
            PrintConsole("[ECMEDIA WARNNING] audio recording, %s",__FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s end with code: %d ",__FUNCTION__, -99);
        }
        else
        {
            g_bGlobalAudioInDevice = enabled;
        }
        PrintConsole("[ECMEDIA INFO] %s end with code: %d ",__FUNCTION__, 0);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA WARNNING] failed to get voe base, %s",__FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s end with code: %d ",__FUNCTION__, -99);
        return -99;
    }
#endif
    PrintConsole("[ECMEDIA INFO] %s end with code: %d ",__FUNCTION__, 0);
    return 0;
}
/*
 * ENCRYPTION
 */

//int ECMedia_init_srtp(int channelid)
//{
//    PrintConsole("[ECMEDIA INFO] %s begins... and Channel ID: %d", __FUNCTION__, channelid);
//    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
//    VoEEncryption *encrypt = VoEEncryption::GetInterface(m_voe);
//    if (encrypt) {
//        int ret = encrypt->CcpSrtpInit(channelid);
//        encrypt->Release();
//        return ret;
//    }
//    else
//    {
//        PrintConsole("[ECMEDIA WARNNING] failed to get VoEEncryption, %s",__FUNCTION__);
//        return -99;
//    }
//}
//
//int ECMedia_enable_srtp_receive(int channelid, const char *key)
//{
//    PrintConsole("[ECMEDIA INFO] %s begins... and Channel ID: %d", __FUNCTION__, channelid);
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
//        PrintConsole("[ECMEDIA WARNNING] failed to get VoEEncryption, %s",__FUNCTION__);
//        return -99;
//    }
//}
//
//int ECMedia_enable_srtp_send(int channelid, const char *key)
//{
//    PrintConsole("[ECMEDIA INFO] %s begins... and Channel ID: %d", __FUNCTION__, channelid);
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
//        PrintConsole("[ECMEDIA WARNNING] failed to get VoEEncryption, %s",__FUNCTION__);
//        return -99;
//    }
//}
//
//int ECMedia_shutdown_srtp(int channel)
//{
//    PrintConsole("[ECMEDIA INFO] %s begins... and Channel ID: %d", __FUNCTION__, channelid);
//    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
//    VoEEncryption *encrypt = VoEEncryption::GetInterface(m_voe);
//    if (encrypt) {
//        int ret = encrypt->CcpSrtpShutdown(channel);
//        encrypt->Release();
//        return ret;
//    }
//    else
//    {
//        PrintConsole("[ECMEDIA WARNNING] failed to get VoEEncryption, %s",__FUNCTION__);
//        return -99;
//    }
//}

/*
 * VOLUME
 */
int ECMedia_set_speaker_volume(int volumep)
{
    PrintConsole("[ECMEDIA INFO] %s begins... volume: %d", __FUNCTION__, volumep);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->SetSpeakerVolume(volumep);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set speaker volume", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_speaker_volume(unsigned int& volumep)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->GetSpeakerVolume(volumep);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get speaker volume", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d  volume:%d", __FUNCTION__, ret, volumep);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

/*
 *
 */
int ECMedia_set_mic_volume(int volumep)
{
    PrintConsole("[ECMEDIA INFO] %s begins... volume: %d", __FUNCTION__, volumep);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->SetMicVolume(volumep);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set mic volume", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
/*
 *
 */
int ECMedia_get_mic_volume(unsigned int& volumep)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = 0;
        ret = volume->GetMicVolume(volumep);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get mic volume", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d volume:%d", __FUNCTION__, ret, volumep);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_mute_status(bool mute)
{
    PrintConsole("[ECMEDIA INFO] %s begins... mute: %s", __FUNCTION__, mute?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->SetSystemInputMute(mute);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set mute status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_mute_status(bool& mute)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->GetSystemInputMute(mute);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get system input mute status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_speaker_mute_status(bool mute)
{
    PrintConsole("[ECMEDIA INFO] %s begins... mute: %s", __FUNCTION__, mute?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->SetSystemOutputMute(mute);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set system output mute status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_speaker_mute_status(bool& mute)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEVolumeControl *volume = VoEVolumeControl::GetInterface(m_voe);
    if (volume) {
        int ret = volume->GetSystemOutputMute(mute);
        volume->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get system output mute status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEVolumeControl", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

/*
 * Capture
 */
#ifdef VIDEO_ENABLED
int ECMdeia_num_of_capture_devices()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int num = capture->NumberOfCaptureDevices();
        capture->Release();
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, num);
        return num;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//buffer for name should allocated and length should be at least 256. So does id.
int ECMedia_get_capture_device(int index, char *name, int name_len, char *id, int id_len)
{
    PrintConsole("[ECMEDIA INFO] %s begins... index: %d", __FUNCTION__, index);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->GetCaptureDevice(index, name, name_len, id, id_len);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get capture device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}


int ECMedia_num_of_capabilities(const char *id, int id_len)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *caputure = ViECapture::GetInterface(m_vie);
    if (caputure) {
        int num = caputure->NumberOfCapabilities(id, id_len);
        caputure->Release();
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, num);
        return num;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//Mem for capability should be allocated first.
int ECMedia_get_capture_capability(const char *id, int id_len, int index, CameraCapability& capabilityp)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
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
            PrintConsole("[ECMEDIA ERROR] %s failed to get capture capability", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_allocate_capture_device(const char *id, size_t len, int& deviceid)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->AllocateCaptureDevice(id, (unsigned int)len, deviceid);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to allocate capture device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_connect_capture_device(int deviceid, int channelid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d deviceid: %d", __FUNCTION__, channelid, deviceid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->ConnectCaptureDevice(deviceid, channelid);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to connect capture device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//The id is got by func "ECMedia_get_capture_device"
int ECMedia_getOrientation(const char *id, ECMediaRotateCapturedFrame &tr)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        RotateCapturedFrame tmp_tr;
        int ret = capture->GetOrientation(id, tmp_tr);
        tr = (ECMediaRotateCapturedFrame)tmp_tr;
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get orientation", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_no_camera_capture_cb(int deviceid, onEcMediaNoCameraCaptureCb no_camera_capture_cb)
{
    g_NoCameraCaptureCb = no_camera_capture_cb;
    return 0;
}

int ECMedia_start_capture(int deviceid, CameraCapability cam)
{
    PrintConsole("[ECMEDIA INFO] %s begins... deviceid: %d width: %d height: %d maxfps: %d", __FUNCTION__, deviceid, cam.width, cam.height, cam.maxfps);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        
        if (!g_ECViECaptureObserver) {
            if (g_NoCameraCaptureCb) {
                g_ECViECaptureObserver = new ECViECaptureObserver(g_NoCameraCaptureCb);
            }
        }
        if (g_ECViECaptureObserver) {
            capture->RegisterObserver(deviceid, *g_ECViECaptureObserver);
        }
        
        CaptureCapability cap;
        cap.height = cam.height;
        cap.width = cam.width;
        cap.maxFPS = cam.maxfps;
        //capture->EnableBrightnessAlarm(deviceid, true); //ylr for test
        int ret = capture->StartCapture(deviceid, cap);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to start capture", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_stop_capture(int captureid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... captureid: %d", __FUNCTION__, captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        if (g_ECViECaptureObserver) {
            if (capture->DeregisterObserver(captureid) == 0) {
                delete g_ECViECaptureObserver;
                g_ECViECaptureObserver = NULL;
            }
        }
        
        int ret = capture->StopCapture(captureid);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to stop capture", __FUNCTION__);
        }
        ret = capture->ReleaseCaptureDevice(captureid);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to release capture device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//The deviceid is got by func "ECMedia_allocate_capture_device"
int ECMedia_set_rotate_captured_frames(int deviceid, ECMediaRotateCapturedFrame tr)
{
    PrintConsole("[ECMEDIA INFO] %s begins... deviceid: %d rotate: %d", __FUNCTION__, deviceid, tr);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->SetRotateCapturedFrames(deviceid, (RotateCapturedFrame)tr);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set rotate captured frames", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_local_video_window(int deviceid, void *video_window)
{
    PrintConsole("[ECMEDIA INFO] %s begins... deviceid:%d video_window:%p ", __FUNCTION__, deviceid, video_window);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = 0;
#ifdef WIN32
        ViERender* render =  ViERender::GetInterface(m_vie);
        ret = render->AddRenderer(deviceid,video_window,1,0,0,1,1,NULL);
        if (ret) {
            render->Release();
            PrintConsole("[ECMEDIA ERROR] %s failed to add renderer", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
            return ret;
        }
        ret = render->StartRender(deviceid);
        render->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to start render", __FUNCTION__);
        }
#else
        ret = capture->SetLocalVideoWindow(deviceid, video_window);
        capture->Release();
#endif
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}



/*
 * Render
 */

int ECMedia_add_render(int channelid, void *video_window, ReturnVideoWidthHeightM videoResolutionCallback)
{
    
    //PrintConsole("[ECMEDIA INFO] %s begins... channelid:%d video_window:%0x", __FUNCTION__, channelid, video_window);
    
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
    
    
    PrintConsole("[ECMEDIA INFO] %s begins... channelid:%d video_window:%p", __FUNCTION__, channelid, video_window);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERender *render = ViERender::GetInterface(m_vie);
    if (render) {
        int ret = render->AddRenderer(channelid, video_window, 2, 0, 0, 1, 1, videoResolutionCallback);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to add renderer", __FUNCTION__);
        }
        ret = render->StartRender(channelid);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to start render", __FUNCTION__);
        }
        render->Release();
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERender", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_stop_render(int channelid, int deviceid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid=%d,deviceid=%d",__FUNCTION__,channelid,deviceid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERender *render = ViERender::GetInterface(m_vie);
    if (render) {
        int ret = render->StopRender(channelid);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to stop render for channelid %d", __FUNCTION__, channelid);
        }
        ret = render->RemoveRenderer(channelid);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to remove renderer for channelid %d", __FUNCTION__, channelid);
        }
        ret = render->StopRender(deviceid);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to stop render for deviceid %d", __FUNCTION__, deviceid);
        }
        ret = render->RemoveRenderer(deviceid);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to remove renderer for deviceid %d", __FUNCTION__, deviceid);
        }
        render->Release();
    }
    PrintConsole("[ECMEDIA INFO] %s ends...",__FUNCTION__);
    return 0;
}
#endif

#ifdef VIDEO_ENABLED
int ECMedia_set_i420_framecallback(int channelid, cloopenwebrtc::ECMedia_I420FrameCallBack callback) {
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid:%d ", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->AddI420FrameCallback(channelid, callback);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to add i420 frame callback", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
#endif

/*
 * VoECodec ViECodec
 */
int ECMedia_num_of_supported_codecs_audio()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int num = codec->NumOfCodecs();
        codec->Release();
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, num);
        return num;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//Mem to store should be allocated.
int ECMedia_get_supported_codecs_audio(CodecInst codecs[])
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
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
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}
#ifdef VIDEO_ENABLED
int ECMedia_num_of_supported_codecs_video()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int num = codec->NumberOfCodecs();
        codec->Release();
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, num);
        return num;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

//Mem to store should be allocated.
int ECMedia_get_supported_codecs_video(VideoCodec codecs[])
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
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
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

// enable h264 hard encode, only support ios
// call it after m_vie have create
int ECMedia_iOS_h264_hard_codec_switch(bool encoder, bool decoder)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        codec->iOSH264HardCodecSwitch(encoder, decoder);
        codec->Release();
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to enable ios h264 hard encode.", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

int ECMedia_set_key_frame_request_cb(int channelid, bool isVideoConf,onEcMediaRequestKeyFrameCallback cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->SetKeyFrameRequestCb(channelid,isVideoConf,cb);
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set key frame request cb", __FUNCTION__);
        }
        codec->Release();
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

#endif
int ECMedia_get_send_codec_audio(int channelid, CodecInst& audioCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int ret = codec->GetSendCodec(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get audio send codec", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_send_codec_audio(int channelid, CodecInst& audioCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d audioCodec(pltype: %d plname: %s plfreq: %d pacsize: %d channels: %d rate: %d)", __FUNCTION__, channelid,
                 audioCodec.pltype, audioCodec.plname, audioCodec.plfreq, audioCodec.pacsize, audioCodec.channels, audioCodec.rate);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        PrintConsole("[ECMEDIA INFO] %s plType:%d plname:%s", __FUNCTION__, audioCodec.pltype,
                     audioCodec.plname);
        int ret = codec->SetSendCodec(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set audio send codec", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_receive_playloadType_audio(int channelid, CodecInst& audioCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d audioCodec(pltype: %d plname: %s plfreq: %d pacsize: %d channels: %d rate: %d)", __FUNCTION__, channelid,
                 audioCodec.pltype, audioCodec.plname, audioCodec.plfreq, audioCodec.pacsize, audioCodec.channels, audioCodec.rate);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        PrintConsole("[ECMEDIA INFO] %s plType:%d plname:%s", __FUNCTION__, audioCodec.pltype,
                     audioCodec.plname);
        int ret = codec->SetRecPayloadType(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set audio receive playload type", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_receive_playloadType_audio(int channelid, CodecInst& audioCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int ret = codec->GetRecPayloadType(channelid, audioCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get audio receive playload type", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

#ifdef VIDEO_ENABLED
int ECMedia_set_send_codec_video(int channelid, VideoCodec& videoCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid:%d videoCodec(width:%d height:%d pltype:%d plname:%s, startBitrate:%d, maxBitrate:%d, minBitrate:%d)",
                 __FUNCTION__, channelid, videoCodec.width,videoCodec.height, videoCodec.plType,videoCodec.plName, videoCodec.startBitrate,videoCodec.maxBitrate, videoCodec.minBitrate);
    if (videoCodec.width == 0 || videoCodec.height == 0) {
        PrintConsole("[ECMEDIA ERROR] %s invalid param width or height", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return ERR_INVALID_PARAM;
    }
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        PrintConsole("[ECMEDIA INFO] %s plType:%d plname:%s", __FUNCTION__, videoCodec.plType,
                     videoCodec.plName);
        int ret = codec->SetSendCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video send codec", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_send_codec_video(int channelid, VideoCodec& videoCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->GetSendCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get video send codec", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_video_qm_mode(int channelid,  cloopenwebrtc::VCMQmResolutionMode mode) {
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid:%d , VCMQmResolutionMode: %d",
                 __FUNCTION__, channelid, mode);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        
        int ret = codec->SetVideoSendQmMode(channelid, mode);
        codec->Release();
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_frame_scale_type(int channelid, FrameScaleType type) {
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d frameScaleType: %d", __FUNCTION__, channelid, type);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        PrintConsole("[ECMEDIA INFO] %s", __FUNCTION__);
        int ret = codec->SetFrameScaleType(channelid, type);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set frame scale type", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_video_conf_cb(int channelid, onEcMediaVideoConference video_conf_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        network->setVideoConfCb(channelid, video_conf_cb);
        network->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_stun_cb_video(int channelid, onEcMediaStunPacket stun_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        network->setStunCb(channelid, stun_cb);
        network->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_receive_codec_video(int channelid, VideoCodec& videoCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->SetReceiveCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video receive codec", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_get_receive_codec_video(int channelid, VideoCodec& videoCodec)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *codec = ViECodec::GetInterface(m_vie);
    if (codec) {
        int ret = codec->GetReceiveCodec(channelid, videoCodec);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get video receive codec", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s begins... channelid:%d len:%d use_rtcp_socket:%s port:%d ip:%s",
                 __FUNCTION__, channelid, length, use_rtcp_socket?"true":"false", port, ip);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret=network->SendUDPPacket(channelid, data, length, transmitted_bytes, use_rtcp_socket, port, ip);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to send udp packet", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
int ECMedia_set_NACK_status_video(int channelid, bool enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetNACKStatus(channelid, enabled);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video nack status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_FEC_status_video(const int channelid,
                                 const bool enable,
                                 const unsigned char payload_typeRED,
                                 const unsigned char payload_typeFEC)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetFECStatus(channelid, enable, payload_typeRED, payload_typeFEC);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video fec status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    
}

int ECMedia_set_HybridNACKFEC_status_video(const int channelid,
                                           const bool enable,
                                           const unsigned char payload_typeRED,
                                           const unsigned char payload_typeFEC)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetHybridNACKFECStatus(channelid, enable, payload_typeRED, payload_typeFEC);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video hybrid nack fec status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    
}

int ECMedia_set_RTCP_status_video(int channelid, int mode)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
    if (rtp_rtcp) {
        int ret = rtp_rtcp->SetRTCPStatus(channelid, (ViERTCPMode)mode);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video rtcp status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
int ECMedia_setVideoConferenceFlag(int channel,const char *selfSipNo ,const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        network->setVideoConferenceFlag(channel, selfSipNo, sipNo, conferenceNo, confPasswd, port, ip);
        network->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
int ECMedia_send_key_frame(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid=%d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECodec *tempCodec = ViECodec::GetInterface(m_vie);
    if(tempCodec)
    {
        int ret = tempCodec->SendKeyFrame(channel);
        tempCodec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to send key frame", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_EnableIPV6(int channel, bool flag)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->EnableIPv6(channel);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video enable ipv6", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_video_IsIPv6Enabled(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        bool ret = network->IsIPv6Enabled(channel);
        network->Release();
        if (ret) {
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return 1; //support IPV6
        }
        else
        {
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return 0; // not support IPV6
        }
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_start_record_screen(int audioChannel, const char* filename, int bitrates, int fps, int screen_index)
{
    PrintConsole("[ECMEDIA INFO] %s begins... audioChannel: %d filename: %s bitrates: %d fps: %d screen_index: %d", __FUNCTION__,
                 audioChannel, filename?filename:"NULL", bitrates, fps, screen_index);
    if(!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            PrintConsole("[ECMEDIA ERROR] %s create recorder failed", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -1;
        }
    }
    
    if(g_recordVoip->isStartRecordScree()) {
        g_recordVoip->StopRecordScreen(0);
    }
    
    if(!g_recordVoip->isRecording() && m_voe && audioChannel >= 0) {
        //PrintConsole("RegisterExternalMediaProcessin in ECMedia_start_record_screen\n");
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if(exmedia) {
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kPlaybackPerChannel, *g_recordVoip);
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kRecordingPerChannel, *g_recordVoip);
            exmedia->Release();
        }
    }
    
    int ret = g_recordVoip->StartRecordScreen(filename, bitrates, fps, screen_index);
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
    return ret;
}

int ECMedia_start_record_screen_ex(int audioChannel, const char* filename, int bitrates, int fps, int screen_index, int left, int top, int width, int height)
{
    PrintConsole("[ECMEDIA INFO] %s begins... audioChannel: %d filename: %s bitrates: %d fps: %d screen_index: %d left: %d top: %d width: %d height: %d", __FUNCTION__,
                 audioChannel, filename?filename:"NULL", bitrates, fps, screen_index, left, top, width, height);
    if(!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            PrintConsole("[ECMEDIA ERROR] %s create recorder failed", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -1;
        }
    }
    
    if(g_recordVoip->isStartRecordScree()) {
        g_recordVoip->StopRecordScreen(0);
    }
    
    if(!g_recordVoip->isRecording() && m_voe) {
        //PrintConsole("RegisterExternalMediaProcessin in ECMedia_start_record_screen_ex\n");
        VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
        if(exmedia) {
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kPlaybackPerChannel, *g_recordVoip);
            exmedia->RegisterExternalMediaProcessing(audioChannel,  kRecordingPerChannel, *g_recordVoip);
            exmedia->Release();
        }
    }
    
    int ret = g_recordVoip->StartRecordScreenEx(filename, bitrates, fps, screen_index, left, top, width, height);
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
    return ret;
}
int ECMedia_stop_record_screen(int audioChannel)
{
    PrintConsole("[ECMEDIA INFO] %s begins... audioChannel: %d", __FUNCTION__, audioChannel);
    
    if(!g_recordVoip)
    {
        PrintConsole("[ECMEDIA ERROR] %s recorder is null", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

ECMEDIA_API int ECMedia_start_record_remote_video(int audioChannel, int videoChannel, const char* filename)
{
    PrintConsole("[ECMEDIA INFO] %s begins... audioChannel: %d videoChannel: %d filename: %s", __FUNCTION__,
                 audioChannel, videoChannel, filename?filename:"NULL");
    if (!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            PrintConsole("[ECMEDIA ERROR] %s create recorder failed", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -1;
        }
    }
    
    if (g_recordVoip->isStartRecordRVideo()) {
        g_recordVoip->StopRecordRemoteVideo(0);
    }
    
    
    if (!g_recordVoip->isRecording() && m_voe) {
        //PrintConsole("RegisterExternalMediaProcessin in ECMedia_start_record_screen\n");
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
            file->RegisterVideoFrameStorageCallBack(videoChannel, (cloopenwebrtc::VCMFrameStorageCallback *)g_recordVoip);
            file->Release();
        }
        ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
        if (rtp_rtcp) {
            rtp_rtcp->RequestKeyFrame(videoChannel);
            rtp_rtcp->Release();
        }
    }
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
    return ret;
}

ECMEDIA_API int ECMedia_stop_record_remote_video(int audioChannel, int videoChannel)
{
    PrintConsole("[ECMEDIA INFO] %s begins... audioChannel: %d videoChannel: %d", __FUNCTION__, audioChannel, videoChannel);
    
    if (!g_recordVoip)
    {
        PrintConsole("[ECMEDIA ERROR] %s recorder is null", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

ECMEDIA_API int ECMedia_start_record_local_video(int audioChannel, int videoChannel, const char* filename)
{
    PrintConsole("[ECMEDIA INFO] %s begins... audioChannel: %d videoChannel: %d filename: %s", __FUNCTION__,
                 audioChannel, videoChannel, filename?filename:"NULL");
    if (!g_recordVoip) {
        g_recordVoip = new RecordVoip();
        if (!g_recordVoip) {
            PrintConsole("[ECMEDIA ERROR] %s create recorder failed", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -1;
        }
    }
    
    if (g_recordVoip->isStartRecordLVideo()) {
        g_recordVoip->StopRecordLocalVideo(0);
    }
    
    if (!g_recordVoip->isRecording() && m_voe) {
        //PrintConsole("RegisterExternalMediaProcessin in %s\n", __FUNCTION__);
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
            vietwork->RegisterEncoderDataObserver(videoChannel, (cloopenwebrtc::VCMPacketizationCallback *)g_recordVoip);
            vietwork->Release();
        }
        ViECodec *codec = ViECodec::GetInterface(m_vie);
        if (codec) {
            codec->SendKeyFrame(videoChannel);
            codec->Release();
        }
    }
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
    return ret;
}
ECMEDIA_API int ECMedia_stop_record_local_video(int audioChannel, int videoChannel)
{
    PrintConsole("[ECMEDIA INFO] %s begins... audioChannel: %d videoChannel: %d", __FUNCTION__, audioChannel, videoChannel);
    
    if (!g_recordVoip) {
        PrintConsole("[ECMEDIA ERROR] %s recorder is null", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}


int ECMedia_get_local_video_snapshot(int deviceid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., deviceid: %d", __FUNCTION__, deviceid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    ViEPicture capPicture;
    if( file->GetCaptureDeviceSnapshot(deviceid, capPicture, kVideoMJPEG) < 0) {
        file->Release();
        PrintConsole("[ECMEDIA ERROR] %s GetCaptureDeviceSnapshot failed", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

int ECMedia_save_local_video_snapshot(int deviceid, const char* filePath)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., deviceid: %d, filePath: %s", __FUNCTION__, deviceid, filePath);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    if(file) {
        ViEPicture capPicture;
        if( file->GetCaptureDeviceSnapshot(deviceid, filePath) < 0) {
            file->Release();
            PrintConsole("[ECMEDIA ERROR] %s  GetCaptureDeviceSnapshot failed.", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -1;
        }
        file->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    PrintConsole("[ECMEDIA ERROR] %s  get ViEFile failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_get_remote_video_snapshot(int channelid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    ViEPicture capPicture;
    if( file->GetRenderSnapshot(channelid, capPicture, kVideoMJPEG) < 0) {
        file->Release();
        PrintConsole("[ECMEDIA ERROR] %s  GetCaptureDeviceSnapshot failed", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}

int ECMedia_save_remote_video_snapshot(int channelid, const char* filePath)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d, filePath: %s", __FUNCTION__, channelid, filePath);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    ViEFile *file = ViEFile::GetInterface(m_vie);
    if(file) {
        ViEPicture capPicture;
        if( file->GetRenderSnapshot(channelid, filePath) < 0) {
            file->Release();
            PrintConsole("[ECMEDIA ERROR] %s  GetRenderSnapshot failed", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -1;
        }
        file->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    PrintConsole("[ECMEDIA ERROR] %s get ViEFile failed.", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECmedia_enable_deflickering(int captureid, bool enable)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., captureid: %d, enabled: %s", __FUNCTION__, captureid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEImageProcess *imageProcess = ViEImageProcess::GetInterface(m_vie);
    if (imageProcess) {
        int ret = imageProcess->EnableDeflickering(captureid, enable);
        imageProcess->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable deflickering", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEImageProcess", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECmedia_enable_EnableColorEnhancement(int channelid, bool enable)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d, enable: %s", __FUNCTION__, channelid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEImageProcess *imageProcess = ViEImageProcess::GetInterface(m_vie);
    if (imageProcess) {
        int ret = imageProcess->EnableColorEnhancement(channelid, enable);
        imageProcess->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable color enhancement", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEImageProcess", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECmedia_enable_EnableDenoising(int captureid, bool enable)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., captureid: %d, enable: %s", __FUNCTION__, captureid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEImageProcess *imageProcess = ViEImageProcess::GetInterface(m_vie);
    if (imageProcess) {
        int ret = imageProcess->EnableDenoising(captureid, enable);
        imageProcess->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable denoising", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEImageProcess", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECmedia_enable_EnableBrightnessAlarm(int captureid, bool enable)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., captureid: %d, enable: %s", __FUNCTION__, captureid, enable?"true":"false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->EnableBrightnessAlarm(captureid, enable);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable brightness alarm", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECmedia_enable_EnableBeautyFilter(int captureid, bool enable)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., captureid: %d, enable: %s", __FUNCTION__, captureid, enable ? "true" : "false");
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->EnableBeautyFilter(captureid, enable);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable beauty filter", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_init_srtp_video(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->CcpSrtpInit(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video init srtp", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get ViEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}
int ECMedia_shutdown_srtp_video(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->CcpSrtpShutdown(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to video shutdown srtp", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get ViEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_enable_srtp_send_video(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d type: %d key: %s", __FUNCTION__, channel, crypt_type, key);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->EnableSRTPSend(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable video srtp send", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get ViEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}
int ECMedia_disable_srtp_send_video(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->DisableSRTPSend(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to disable video srtp send", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get ViEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_enable_srtp_recv_video(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d type: %d key: %s", __FUNCTION__, channel, crypt_type, key);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->EnableSRTPReceive(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable video srtp recv", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get ViEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}
int ECMedia_disable_srtp_recv_video(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEEncryption *encryt = ViEEncryption::GetInterface(m_vie);
    if (encryt) {
        int ret = encryt->DisableSRTPReceive(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to disable video srtp recv", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get ViEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_video_set_remb(int channelid, bool enableRemb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid:%d, enableRemb:%s", __FUNCTION__,
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
        PrintConsole("[ECMEDIA INFO] %s end with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA WARNNING] failed to get ViERTP_RTCP, %s", __FUNCTION__);
        return -99;
    }
}

#endif

int ECMedia_set_VAD_status(int channelid, VadModes mode, bool dtx_enabled)
{
    PrintConsole("[ECMEDIA INFO] %s begins...,channelid: %d, mode: %d dtx_enabled: %s", __FUNCTION__, channelid, mode, dtx_enabled?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoECodec *codec = VoECodec::GetInterface(m_voe);
    if (codec) {
        int ret = codec->SetVADStatus(channelid, false, mode, !dtx_enabled);
        codec->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set vad status", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoECodec", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}


//CAUTION: noNetwork/wifi/other
int ECMedia_set_network_type(int audio_channelid, int video_channelid, const char *type)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., audio_channelid: %d, video_channelid: %d ", __FUNCTION__, audio_channelid, video_channelid);
    if (!type || strcmp(type, "noNetwork")==0) {
        PrintConsole("[ECMEDIA ERROR] %s invalid network type", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEBase *base = cloopenwebrtc::VoEBase::GetInterface(m_voe);
    if (base) {
        base->SetNetworkType(audio_channelid, strcmp(type, "wifi")==0?true:false);
        base->Release();
    }
#ifdef VIDEO_ENABLED
    if (video_channelid >= 0) {
        VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
        ViENetwork *network = cloopenwebrtc::ViENetwork::GetInterface(m_vie);
        if (network) {
            network->setNetworkType(video_channelid, strcmp(type, "wifi")==0?true:false);
            network->Release();
        }
    }
    
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}


int ECMedia_EnableIPV6(int channel, bool flag)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d, flag: %s", __FUNCTION__, channel, flag?"true":"false");
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        int ret = network->EnableIPv6(channel);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable ipv6", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_IsIPv6Enabled(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins... and channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoENetwork *network = VoENetwork::GetInterface(m_voe);
    if (network) {
        bool ret = network->IPv6IsEnabled(channel);
        network->Release();
        if (ret) {
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return 1; //support IPV6
        }
        else
        {
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return 0; // not support IPV6
        }
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->CcpSrtpInit(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio init srtp", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}
int ECMedia_shutdown_srtp_audio(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->CcpSrtpShutdown(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio shutdown srtp", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_enable_srtp_send_audio(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d crypt_type: %d key: %s", __FUNCTION__, channel, crypt_type, key);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->EnableSRTPSend(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable audio srtp send", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}
int ECMedia_disable_srtp_send_audio(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->DisableSRTPSend(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to disable audio srtp send", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_enable_srtp_recv_audio(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d crypt_type: %d key: %s", __FUNCTION__, channel, crypt_type, key);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->EnableSRTPReceive(channel, crypt_type, key);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable audio srtp recv", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}
int ECMedia_disable_srtp_recv_audio(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEEncryption *encryt = VoEEncryption::GetInterface(m_voe);
    if (encryt) {
        int ret = encryt->DisableSRTPReceive(channel);
        encryt->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to disable audio srtp recv", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEEncryption failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_start_record_playout(int channel, char *filename)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StartRecordingPlayout(channel, filename);
        file->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to start record playout", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEFile failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_stop_record_playout(int channel)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., channelid: %d", __FUNCTION__, channel);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StopRecordingPlayout(channel);
        file->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to stop record playout", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEFile failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_start_record_microphone(char *filename)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StartRecordingMicrophone(filename);
        file->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to start record microphone", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEFile failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_stop_record_microphone()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StopRecordingMicrophone();
        file->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to stop record microphone", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEFile failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_start_record_send_voice(char *filename)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StartRecordingCall(filename);
        file->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to start record send voice", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEFile failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_stop_record_send_voice()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEFile *file = VoEFile::GetInterface(m_voe);
    if (file) {
        int ret = file->StopRecordingCall();
        file->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to stop record send voice", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    PrintConsole("[ECMEDIA ERROR] %s get VoEFile failed", __FUNCTION__);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_set_CaptureDeviceID(int videoCapDevId)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., videoCapDevId: %d", __FUNCTION__, videoCapDevId);
#ifdef VIDEO_ENABLED
    g_CaptureDeviceId = videoCapDevId;
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...",__FUNCTION__);
    return 0;
}

int ECMedia_Check_Record_Permission(bool &enabled) {
    PrintConsole("[ECMEDIA INFO] %s begins...",__FUNCTION__);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoEHardware *hardware = VoEHardware::GetInterface(m_voe);
    if (hardware) {
        int ret = hardware->CheckRecordPermission(enabled);
        hardware->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to check record permission", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ",__FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEHardware",__FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
#ifdef VIDEO_ENABLED
ECMEDIA_API int ECMedia_setBeautyFace(int deviceid, bool enable)
{
    PrintConsole("[ECMEDIA INFO] %s begins... ", __FUNCTION__);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->setBeautyFace(deviceid, enable);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set beauty face", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        return -99;
    }
}

ECMEDIA_API int ECMedia_iOS_SetVideoFilter(int deviceid, ECImageFilterType filterType)
{
    PrintConsole("[ECMEDIA INFO] %s begins, deviceid:%d, image filter type:%d ", __FUNCTION__, deviceid, filterType);
    ViECapture *capture = ViECapture::GetInterface(m_vie);
    if (capture) {
        int ret = capture->setVideoFilter(deviceid, filterType);
        capture->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set video filter", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViECapture", __FUNCTION__);
        return -99;
    }
}



int ECMedia_allocate_desktopShare_capture(int& desktop_captureid, int capture_type)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., desktop_captureid: %d, capture_type: %d", __FUNCTION__, desktop_captureid, capture_type);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        int ret = vie_desktopshare->AllocateDesktopShareCapturer(desktop_captureid, (DesktopShareType)capture_type);
        if (ret != 0)
            PrintConsole("[ECMEDIA ERROR] %s AllocateDesktopShareCapturer failed", __FUNCTION__);
        else
            PrintConsole("[ECMEDIA ERROR] %s AllocateDesktopShareCapturer desktop_captureid:%d", __FUNCTION__, desktop_captureid);
        vie_desktopshare->Release();
        g_statsCollector->SetVideoCaptureDeviceId(desktop_captureid);
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_release_desktop_capture(int desktop_captureid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... desktop_captureid: %d", __FUNCTION__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        vie_desktopShare->StopDesktopShareCapture(desktop_captureid);
        vie_desktopShare->ReleaseDesktopShareCapturer(desktop_captureid);
        vie_desktopShare->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_connect_desktop_captureDevice(int desktop_captureid, int video_channelId)
{
    PrintConsole("[ECMEDIA INFO] %s begins... desktop_captureid: %d video_channelId: %d", __FUNCTION__, desktop_captureid, video_channelId);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        int ret = vie_desktopShare->ConnectDesktopCaptureDevice(desktop_captureid, video_channelId);
        vie_desktopShare->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to connect desktop capture device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_disconnect_desktop_captureDevice(int video_channelId)
{
    PrintConsole("[ECMEDIA INFO] %s begins... video_channelId: %d", __FUNCTION__, video_channelId);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        int ret = vie_desktopShare->DisConnectDesktopCaptureDevice(video_channelId);
        vie_desktopShare->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to disconnect desktop capture device", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}


int ECMedia_get_screen_list(int desktop_captureid, ScreenID **screenList)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., desktop_captureid: %d", __FUNCTION__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (!screenList) {
        PrintConsole("[ECMEDIA ERROR] %s screenList is NULL.", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
            PrintConsole("[ECMEDIA ERROR] %s failed to get screen list", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, num);
        return num;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -1;
    }
}


int ECMedia_get_window_list(int desktop_captureid, WindowShare **windowList)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., desktop_captureid: %d", __FUNCTION__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    if (!windowList) {
        PrintConsole("[ECMEDIA ERROR] %s windowList is NULL.", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
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
            PrintConsole("[ECMEDIA ERROR] %s failed to get window list", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, num);
        return num;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -1;
    }
}



bool ECMedia_select_screen(int desktop_captureid, ScreenID screeninfo)
{
    PrintConsole("[ECMEDIA INFO] %s begins... captureid: %d", __FUNCTION__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        bool ret = vie_desktopshare->SelectScreen(desktop_captureid, screeninfo);
        vie_desktopshare->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to select screen", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return false;
    }
}

bool ECMedia_select_window(int desktop_captureid, WindowID windowinfo)
{
    PrintConsole("[ECMEDIA INFO] %s begins... desktop_captureid: %d", __FUNCTION__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        bool ret = vie_desktopshare->SelectWindow(desktop_captureid, windowinfo);
        vie_desktopshare->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to select window", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return false;
    }
}


int ECMedia_start_desktop_capture(int captureId, int fps)
{
    PrintConsole("[ECMEDIA INFO] %s begins... captureId: %d fps: %d", __FUNCTION__, captureId, fps);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopshare) {
        int ret = vie_desktopshare->StartDesktopShareCapture(captureId, fps);
        vie_desktopshare->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to start desktop capture", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_stop_desktop_capture(int desktop_captureid)
{
    PrintConsole("[ECMEDIA INFO] %s begins... desktop_captureid: %d", __FUNCTION__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *vie_desktopShare = ViEDesktopShare::GetInterface(m_vie);
    if (vie_desktopShare)
    {
        int ret = vie_desktopShare->StopDesktopShareCapture(desktop_captureid);
        vie_desktopShare->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to stop desktop capture", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d\n", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_desktop_share_err_code_cb(int desktop_captureid, int channelid, onEcMediaDesktopCaptureErrCode capture_err_code_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... desktop_captureid: %d channelid: %d", __FUNCTION__, desktop_captureid, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktop_capture = ViEDesktopShare::GetInterface(m_vie);
    if (desktop_capture) {
        desktop_capture->setCaptureErrCb(desktop_captureid, channelid, capture_err_code_cb);
        desktop_capture->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_desktop_share_window_change_cb(int desktop_captureid, int channelid, onEcMediaShareWindowSizeChange share_window_change_cb)
{
    PrintConsole("[ECMEDIA INFO] %s begins... desktop_captureid: %d channelid: %d", __FUNCTION__, desktop_captureid, channelid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktop_capture = ViEDesktopShare::GetInterface(m_vie);
    if (desktop_capture) {
        desktop_capture->setShareWindowChangeCb(desktop_captureid, channelid, share_window_change_cb);
        desktop_capture->Release();
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
int ECmedia_set_shield_mosaic(int video_channel, bool flag)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., video_channel: %d", __FUNCTION__, video_channel);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViENetwork *network = ViENetwork::GetInterface(m_vie);
    if (network) {
        int ret = network->setShieldMosaic(video_channel, flag);
        network->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set shield mosaic", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return 0;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViENetwork", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}


int ECMedia_get_desktop_capture_size(int desktop_captureid, int &width, int &height)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., desktop_captureid: %d", __FUNCTION__, desktop_captureid);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (desktopshare) {
        bool ret = desktopshare->GetDesktopShareCaptureRect(desktop_captureid, width, height);
        desktopshare->Release();
        if (ret == false) {
            PrintConsole("[ECMEDIA ERROR] %s failed to get desktop capture size", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d", __FUNCTION__, ret ? 0 : -99);
        return ret ? 0 : -99;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

int ECMedia_set_screen_share_activity(int desktop_captureid, void* activity)
{
    PrintConsole("[ECMEDIA INFO] %s begins... desktop_captureid: %d activity: %0x", __FUNCTION__, desktop_captureid, activity);
    VIDEO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    ViEDesktopShare *desktopshare = ViEDesktopShare::GetInterface(m_vie);
    if (desktopshare) {
        int ret = desktopshare->SetScreenShareActivity(desktop_captureid, activity);
        desktopshare->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set screen share activity", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get ViEDesktopShare", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

/********************* ec live stream api begin ****************/
// create live stream object.
void *ECMedia_createLiveStream()
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    ECLiveEngine* engine = ECLiveEngine::getInstance();
    return engine;
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return NULL;
}

// not support yet.
void ECMedia_SetLiveVideoSource(void *handle, int video_source)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    //    ECLiveEngine *engine = (ECLiveEngine*)handle;
    //    engine->setVideoPreview(renderView);
    
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
#endif
}

// set video preview view.
int ECMedia_setVideoPreviewViewer(void *handle, void *viewer) {
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    int ret = -1;
#ifdef VIDEO_ENABLED
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    ret = engine->setVideoPreview(viewer);
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d", __FUNCTION__, ret);
    return ret;
#endif
    return -1;
}

int ECMedia_ConfigLiveVideoStream(void *handle, LiveVideoStreamConfig config)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    int ret = engine->configLiveVideoStream(config);
    if (ret != 0) {
        PrintConsole("[ECMEDIA ERROR] %s failed to set video profile", __FUNCTION__);
    }
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d", __FUNCTION__, ret);
    return ret;
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_SwitchLiveCamera(void *handle, int camera_index) {
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    int ret = -1;
    ret = engine->switchCamera(camera_index);
    if (ret != 0) {
        PrintConsole("[ECMEDIA ERROR] %s failed to switch live video stream.", __FUNCTION__);
    }
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d", __FUNCTION__, ret);
    return ret;
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

// push stream
int ECMedia_pushLiveStream(void *handle, const char *url, ECLiveStreamNetworkStatusCallBack callback)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    int ret = 0;
    
    ret = engine->startPublish(url, callback);
    
    if (ret != 0) {
        PrintConsole("[ECMEDIA ERROR] %s failed to push stream", __FUNCTION__);
    }
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d", __FUNCTION__, ret);
    return ret;
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

int ECMedia_playLiveStream(void *handle, const char * url, ECLiveStreamNetworkStatusCallBack callback)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    int ret = -1;
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    ret = engine->startPlay(url, callback);
    if (ret != 0) {
        PrintConsole("[ECMEDIA ERROR] %s failed to play stream", __FUNCTION__);
    }
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d", __FUNCTION__, ret);
    return ret;
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -1;
}

void ECMedia_stopLiveStream(void *handle)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    engine->stopPlay();
    engine->stopPublish();
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
#endif
}

void ECMedia_releaseLiveStream(void *handle)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    ECLiveEngine::destroy();
#endif
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
}

void ECMedia_enableLiveStreamBeauty(void *handle)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    engine->setBeautyFace(true);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
#endif
}

void ECMedia_disableLiveStreamBeauty(void *handle)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
#ifdef VIDEO_ENABLED
    ECLiveEngine *engine = (ECLiveEngine*)handle;
    engine->setBeautyFace(false);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
#endif
}

// not support yet
ECMEDIA_API int ECMedia_GetShareWindows(void *handle, WindowShare ** windows)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
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
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0; //list.size();
}

// not support yet
ECMEDIA_API int ECMedia_SelectShareWindow(void *handle, int type, int id)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    //    RTMPLiveSession *p = (RTMPLiveSession*)handle;
    //    p->SelectShareWindow(type,id);
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return 0;
}
/********************* ec live stream api end ****************/

ECMEDIA_API int  ECMedia_startRecordLocalMedia(const char *fileName, void *localview)
{
    
#ifdef VIDEO_ENABLED
    PrintConsole("[ECMEDIA INFO] %s begins... ", __FUNCTION__);
    if (!g_recordLocal) {
        g_recordLocal = new RecordLocal();
        if (!g_recordLocal) {
            PrintConsole("[ECMEDIA ERROR] %s create recorder failed", __FUNCTION__);
            PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
            return -1;
        }
    }
    
    int ret = g_recordLocal->Start(fileName, localview);
    if (ret != 0) {
        PrintConsole("[ECMEDIA ERROR] %s failed to start record local media", __FUNCTION__);
    }
    PrintConsole("[ECMEDIA INFO] %s ends... with code: %d", __FUNCTION__, ret);
    return ret;
#endif
    return -1;
}

ECMEDIA_API void ECMedia_stopRecordLocalMedia()
{
#ifdef VIDEO_ENABLED
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    if (!g_recordLocal) {
        PrintConsole("[ECMEDIA ERROR] %s not start recorder", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return;
    }
    
    g_recordLocal->Stop();
    
    delete g_recordLocal;
    g_recordLocal = NULL;
    
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
#endif
}
#endif

int ECMedia_getStatsReports(int type, char* callid, void** pMediaStatisticsDataInnerArray, int *pArraySize)
{
    PrintConsole("[ECMEDIA INFO] %s begins..., type: %d, callid: %s", __FUNCTION__, type, callid);
    if (g_statsCollector)
    {
        g_statsCollector->GetStats((StatsContentType)type, callid, pMediaStatisticsDataInnerArray, pArraySize);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return 0;
    }
    
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
    return -99;
}

void ECMedia_deletePbData(void* pbDataArray)
{
    PrintConsole("[ECMEDIA INFO] %s begins...", __FUNCTION__);
    if (g_statsCollector)
    {
        g_statsCollector->DeletePbData(pbDataArray);
    }
    PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
}


ECMEDIA_API int ECMedia_setAudioRed(int channelid, bool enable, int payloadType)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d enable: %s payloadType: %d", __FUNCTION__,
                 channelid, enable?"true":"false", payloadType);
    AUDIO_ENGINE_UN_INITIAL_ERROR(ERR_ENGINE_UN_INIT);
    VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
    if (rtp_rtcp)
    {
        int ret = 0;
        ret = rtp_rtcp->SetREDStatus(channelid, enable, payloadType);
        rtp_rtcp->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to set audio red", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else
    {
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoERTP_RTCP", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
    
}

ECMEDIA_API int ECMedia_audio_enable_magic_sound(int channelid, bool is_enable)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d",
                 __FUNCTION__, channelid);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->enableSoundTouch(channelid, is_enable);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to enable audio magic sound", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else{
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

ECMEDIA_API int ECMedia_audio_set_magic_sound(int channelid, int pitch, int tempo, int rate)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d pitch: %d tempo: %d rate: %d",
                 __FUNCTION__, channelid, pitch, tempo, rate);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->setSoundTouch(channelid, pitch, tempo, rate);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio set magic sound", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else{
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
ECMEDIA_API int ECMedia_select_magic_sound_mode(int channelid, ECMagicSoundMode mode)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d mode:%d",
                 __FUNCTION__, channelid, (int)mode);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        int ret = base->selectSoundTouchMode(channelid, mode);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio set magic sound", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else{
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
ECMEDIA_API int ECMedia_audio_set_playout_gain(int channelid, float gain)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d gain:%f",
                 __FUNCTION__, channelid, gain);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        //        For the time being, channelid is useless
        int ret = base->setEnlargeAudioFlagIncoming(true, gain);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio set playout gain", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else{
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}

ECMEDIA_API int ECMedia_audio_set_microphone_gain(int channelid, float gain)
{
    PrintConsole("[ECMEDIA INFO] %s begins... channelid: %d gain:%f",
                 __FUNCTION__, channelid, gain);
    VoEBase *base = VoEBase::GetInterface(m_voe);
    if (base) {
        //        For the time being, channelid is useless
        int ret = base->setEnlargeAudioFlagOutgoing(true, gain);
        base->Release();
        if (ret != 0) {
            PrintConsole("[ECMEDIA ERROR] %s failed to audio set microphone gain", __FUNCTION__);
        }
        PrintConsole("[ECMEDIA INFO] %s ends... with code: %d ", __FUNCTION__, ret);
        return ret;
    }
    else{
        PrintConsole("[ECMEDIA ERROR] %s failed to get VoEBase", __FUNCTION__);
        PrintConsole("[ECMEDIA INFO] %s ends...", __FUNCTION__);
        return -99;
    }
}
