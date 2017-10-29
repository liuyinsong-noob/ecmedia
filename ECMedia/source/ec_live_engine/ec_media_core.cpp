//
//  ec_media_ machine.cpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#include "ec_media_core.h"
#include "ECMedia.h"
#include "ec_live_engine.h"
#include "librtmp/rtmp.h"
//#include "rtmp_sys.h"
#include "librtmp/log.h"
#include "voe_base.h"
#include "trace.h"
#include "voe_file.h"
#include "voe_encryption.h"
#include "voe_network.h"
#include "voe_audio_processing.h"
#include "voe_codec.h"
#include "voe_rtp_rtcp.h"
#include "voe_hardware.h"
#include "sleep.h"
#include "voe_volume_control.h"
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
//#include "webrtc_libyuv.h"
#include "vie_file_impl.h"
#include "vie_desktop_share_impl.h"
#include "vie_image_process_impl.h"
#endif


// #include "ec_aac_codec.h"

#include "clock.h"
#include "faaccodec.h"
#include "push_resampler.h"
#include "librtmp/rtmp.h"
#include "ec_live_common.h"
#include "ec_live_utility.h"

#include <iostream>

namespace cloopenwebrtc {
    
#define VIDEO_CODEC "H264"
#define VIDEO_SAMPLE_RATE 90000
#define AUDIO_CODEC  "L16"
#define AUDIO_SAMPLE_RATE  16000
#define AAC_CODEC_SAMPLE_RATE 44100
#define AUDIO_CHANNEL_COUNT 2
    
    static ECMediaMachine *g_rtmpLiveSession = NULL;

    ECMediaMachine *ECMediaMachine::getInstance()
    {
        if (!g_rtmpLiveSession) {
            g_rtmpLiveSession = new ECMediaMachine();
        }
        return g_rtmpLiveSession;
    }
    
    
    ECMediaMachine::ECMediaMachine()
    : audio_channel_(-1)
    , video_channel_(-1)
    , voe_(nullptr), vie_(nullptr)
    , audio_data_cb_(nullptr)
    , video_data_cb_(nullptr)
    , audio_rtp_seq_(0)
    , video_rtp_seq_(0)
    , faac_encode_handle_(nullptr)
    , capture_id_(-1)
    , desktop_capture_id_(-1)
    , share_window_id_(-1)
    , video_source_(VIDEO_SOURCE_CAMERA)
    , desktop_share_type_(ShareScreen)
    , local_view_(NULL)
    , info_video_bitrates_(768)
    , info_video_width_(960)
    , info_video_height_(540)
    , info_video_fps_(15)
    , info_camera_index_(0)
    , capturer_data_callback_(nullptr)
    {
#ifndef WIN32
        info_video_height_ = 640;
        info_video_width_  = 360;
        info_camera_index_ = 0;
#endif
        g_rtmpLiveSession = this;
    }
    
    ECMediaMachine::~ECMediaMachine()
    {
        if (faac_encode_handle_) {
            faac_encoder_close(faac_encode_handle_);
            faac_encode_handle_ = nullptr;
        }
    }
    
    bool ECMediaMachine::Init()
    {
        int ret = -1;
        ret = initAudioEngine();
        ret = initVideoEngine();
        
        ret = initAudioNetwork();
//        ret = initVideoNetwork();

        return ret;
    }
    
    int ECMediaMachine::initAudioEngine() {
        voe_ = VoiceEngine::Create();
        if (NULL == voe_)
        {
            PrintConsole("media_init Create audio engine fail\n");
            return -1;
        }
        VoEBase* base = VoEBase::GetInterface(voe_);
        if( base->Init() != 0) {
            VoiceEngine::Delete(voe_);
            voe_ = NULL;
            
            PrintConsole("Init Voice Engine Error, error code is %d\n",base->LastError());
            return base->LastError(); //base init failed
        }
        
        VoEVolumeControl* volume = VoEVolumeControl::GetInterface(voe_);
        if(volume){
            volume->SetMicVolume(255);
            volume->Release();
        }
        
        base->Release();
 
        if (vie_) {
            ViEBase *viebase = (ViEBase*)ViEBase::GetInterface(vie_);
            viebase->SetVoiceEngine(voe_);
            viebase->Release();
        }
        return 0;
    }
   
    int ECMediaMachine::uninitAudioEngine(){
        if(!voe_)
        {
            PrintConsole("[ECMEDIA WARNNING] %s failed with error code: %d.", __FUNCTION__ , -99);
            return -1;
        }
        
        VoiceEngine::Delete(voe_);
        voe_ = NULL;
        return 0;
    }
    
    int ECMediaMachine::uninitVideoEngine(){
        if(!vie_)
        {
            PrintConsole("[ECMEDIA WARNNING] %s failed with error code: %d.", __FUNCTION__ , -99);
            return -1;
        }
 
        VideoEngine::Delete(vie_);
        vie_ = NULL;
        return 0;
    }
    
    int ECMediaMachine::initVideoEngine() {
            vie_ = VideoEngine::Create();
            if ( NULL == vie_)
            {
                PrintConsole("media_init Create Video engine fail\n");
                return -1;;
            }
        
            ViEBase* videobase = ViEBase::GetInterface(vie_);
            if(videobase->Init()!= 0) {
                int lastError = videobase->LastError(); // base init failed
                PrintConsole("Init Video Engine error, error code is %d\n", lastError);
                videobase->Release();
                VideoEngine::Delete(vie_);
                vie_ = NULL;
                return lastError;
            }
        
            if(voe_){
                videobase->SetVoiceEngine(voe_);
            }
            videobase->Release();
            return 0;
    }

    int ECMediaMachine::initAudioNetwork() {
        // init audio network
        VoEBase *base = VoEBase::GetInterface(voe_);
        audio_channel_ = base->CreateChannel();
        base->Release();

        VoENetwork *network = VoENetwork::GetInterface(voe_);
        if(!network) {
            return -1;
        }
        network->RegisterExternalTransport(audio_channel_, *this);   // receive audio data callback
        network->RegisterExternalPacketization(audio_channel_,this); // senddata() callback
        network->Release();

        // 解码的采样率设置为32k, 使用16k发现播放声音速率不正常
        // rtmp 收到的 aac 44.1k 经过重采样成32k
        int ret  = -1;
        ret = initAudioTransportCodec(AUDIO_CODEC, 32000, AUDIO_CHANNEL_COUNT);
        return ret;
    }
    
    int ECMediaMachine::initVideoNetwork(){
        //init video network
        ViEBase *vbase = ViEBase::GetInterface(vie_);
        int ret = vbase->CreateChannel(video_channel_);
        vbase->Release();
        
        ViENetwork *vnetwork = ViENetwork::GetInterface(vie_);
        ret = vnetwork->RegisterSendTransport(video_channel_, *this); // data callback
        ret = vnetwork->RegisterExternalPacketization(video_channel_, this); // SetRtpData callback
        vnetwork->Release();
        if(ret != 0) {
            return ret;
        }

        ret = initVideoTransportCodec(VIDEO_CODEC, VIDEO_SAMPLE_RATE);
        return ret;
    }

    int ECMediaMachine::doVideoDataSend() {

        ViEBase *vbase = ViEBase::GetInterface(vie_);

        // start send video rtp packet
        int ret = vbase->StartSend(video_channel_);
        ret = vbase->Release();
        return ret;
    }

    int ECMediaMachine::shutdownVideoDataSend() {
        ViEBase *vbase = ViEBase::GetInterface(vie_);
        int ret = -1;
        ret = vbase->StopSend(video_channel_);
        vbase->Release();
        return ret;
    }


    int ECMediaMachine::shutdownVideoDataReceive() {
        ViEBase *vbase = ViEBase::GetInterface(vie_);
        int ret = -1;
        ret = vbase->StopReceive(video_channel_);
        ret = vbase->StartReceive(video_channel_);
        vbase->Release();
        return ret;
    }

    int ECMediaMachine::doVideoDataReceive() {
        ViEBase *vbase = ViEBase::GetInterface(vie_);
 
        int ret = -1;
        ret = vbase->StartReceive(video_channel_);
        vbase->Release();
        return ret;
    }
    
    void ECMediaMachine::UnInit()
    {
        uninitAudioNetwork();
//        uninitVideoNetwork();
        
        uninitAudioEngine();
        uninitVideoEngine();
    }
    
    int ECMediaMachine::uninitAudioNetwork() {

        // stop audio network
        VoENetwork *network = VoENetwork::GetInterface(voe_);
        network->DeRegisterExternalTransport(audio_channel_);
        network->DeRegisterExternalPacketization(audio_channel_);
        network->Release();

        VoEBase *base = VoEBase::GetInterface(voe_);
        base->DeleteChannel(audio_channel_);
        base->Release();

        audio_channel_ = -1;
        return 0;
    }
    
    
    int ECMediaMachine::uninitVideoNetwork() {
        // stop video send
        ViEBase *vbase = ViEBase::GetInterface(vie_);

        //stop video network
        ViENetwork *vnetwork = ViENetwork::GetInterface(vie_);
        int ret =  vnetwork->DeregisterSendTransport(video_channel_);
        ret = vnetwork->Release();

        ret = vbase->DeleteChannel(video_channel_);
        vbase->Release();
        
        video_channel_ = -1;
        return ret;
 
    }
    
    int ECMediaMachine::startCapture()
    {
        initVideoNetwork();
        PrintConsole("[ECMEDIA CORE INFO] %s begin.\n", __FUNCTION__);
        int ret = -1;
        ret = doAudioDataSend();
        if(ret < 0) {
            PrintConsole("[ECMEDIA CORE ERROR] %s do audio data send error.\n", __FUNCTION__);
            return ret;
        }
        ret = doVideoDataSend();
        if(ret < 0) {
            PrintConsole("[ECMEDIA CORE ERROR] %s do video data send error.\n", __FUNCTION__);
            return ret;
        }

        if(video_source_ == VIDEO_SOURCE_CAMERA) {
            ret = doCameraCapture();
            if(ret < 0) {
                PrintConsole("[ECMEDIA CORE ERROR] %s do camera capture error.\n", __FUNCTION__);
                return ret;
            }
            ret = doCameraPreviewRender(capture_id_);
            if(ret < 0) {
                PrintConsole("[ECMEDIA CORE ERROR] %s do preview render error.\n", __FUNCTION__);
                return ret;
            }
        } else {
            ret = doDesktopCapture();
            if(ret < 0) {
                PrintConsole("[ECMEDIA CORE ERROR] %s do desktop capturer error.\n", __FUNCTION__);
                return ret;
            }
        }
        PrintConsole("[ECMEDIA CORE INFO] %s end with code:%d.\n", __FUNCTION__, ret);
        return ret;
    }
    
    int ECMediaMachine::stopCapture()
    {
        int ret = -1;
        if(video_source_ == VIDEO_SOURCE_CAMERA) {
            ret = shutdownCameraCapture();
            ret = shutdownCameraPreviewRender(capture_id_);
        } else {
            ret = shutdownDesktopCapture();
        }

        // shutdown network
        ret = shutdownVideoDataSend();
        ret = shutdownAudioDataSend();
        
        uninitVideoNetwork();
        return ret;
    }

    int ECMediaMachine::startPlayout() {
        PrintConsole("[ECMEDIA CORE INFO] %s start\n", __FUNCTION__);
        int ret = -1;
        ret = initVideoNetwork();
        if(ret != 0) {
            return ret;
        }
        // start Playout
        ret = doAudioPlayout();
        if(ret != 0) {
            PrintConsole("[ECMEDIA CORE INFO] %s doAudioPlayout error: %d\n", __FUNCTION__, ret);
        }
        ret = doAudioDataReceive();
        if(ret != 0) {
            return ret;
        }
        ret = doPlayingPreviewRender(video_channel_);
        if(ret != 0) {
            return ret;
        }
        ret = doVideoDataReceive();
        if(ret != 0) {
            return ret;
        }
        PrintConsole("[ECMEDIA CORE INFO] %s end with code: %d\n", __FUNCTION__, ret);
        return ret;
    }

    int ECMediaMachine::stopPlayout() {
        PrintConsole("[RTMP ERROR] %s start\n", __FUNCTION__);
        int ret = -1;
        ret = shutdownVideoDataReceive();
        if(ret != 0) {
            return ret;
        }
        ret = doPlayingPreviewRender(video_channel_);
        if(ret != 0) {
            return ret;
        }
        ret = shutdownAudioPlayout();
        if(ret != 0) {
            return ret;
        }
        ret = shutdownAudioDataReceive();
        if(ret != 0) {
            return ret;
        }
        ret = uninitVideoNetwork();
        if(ret != 0) {
            return ret;
        }
        PrintConsole("[RTMP ERROR] %s end with code:%d\n", __FUNCTION__, ret);
        return ret;
    }
    
    int ECMediaMachine::setVideoPreview(void *view) {
        if(view == NULL) {
            PrintConsole("[RTMP ERROR] %s video view in null\n", __FUNCTION__);
            return -1;
        }
#ifdef __ANDROID__
        unsigned int len = strlen((char *)view);
        memcpy(render_viewer_id, view, len);
        render_viewer_id[len] = 0;
        local_view_ = render_viewer_id; //str;
#else
        local_view_ = view;
#endif
        return 0;
    }
    
    
    int ECMediaMachine::shutdownCameraCapture() {
        PrintConsole("[RTMP INFO] %s begin.\n", __FUNCTION__);
        uninitCameraDevice();
        // stop camera capture
        ViECapture *capture = ViECapture::GetInterface(vie_);
        int ret = -1;
        ret = capture->DisconnectCaptureDevice(video_channel_);
        ret = capture->StopCapture(capture_id_);
        ret = capture->ReleaseCaptureDevice(capture_id_);
        ret = capture->Release();
        capture_id_ = -1;
        return ret;
    }
    
    int ECMediaMachine::doCameraCapture() {
        initCameraDevice();
        CameraInfo *camera = cameras_[info_camera_index_];
        if(!camera) {
            PrintConsole("[RTMP ERROR] %s find camera device failed\n", __FUNCTION__);
            return -1;
        }
        ViECapture *capture = ViECapture::GetInterface(vie_);
        int ret = capture->AllocateCaptureDevice(camera->id, sizeof(camera->id), capture_id_);
        
        // Rotate camere frame
        RotateCapturedFrame tr = RotateCapturedFrame_0;
        ret = capture->GetOrientation(camera->id, tr);
        capture->SetRotateCapturedFrames(capture_id_, tr);
        
        // camera capture infomation
        CaptureCapability cap;
        {
            cap.height      = info_video_height_;
            cap.width        = info_video_width_;
            cap.maxFPS      = info_video_fps_;
        }
        ret = capture->StartCapture(capture_id_, cap);
        ret = capture->ConnectCaptureDevice(capture_id_, video_channel_);
        PrintConsole("[RTMP INFO] %s end with ret:%d\n", __FUNCTION__, ret);
        capture->Release();
        return ret;
    }

    int ECMediaMachine::doCameraPreviewRender(int render_id) {
        PrintConsole("[RTMP INFO] %s : start video preview.\n", __FUNCTION__);
        if(!local_view_) {
            PrintConsole("[RTMP INFO] %s : not set video preview viewer.\n", __FUNCTION__);
            return 0;
        }
        int ret = -1;
#ifndef __APPLE__
        ViERender* render = ViERender::GetInterface(vie_);
        if(!render) {
            PrintConsole("[RTMP ERROR] %s get vierender failed\n", __FUNCTION__);
        }
        
        ret = render->AddRenderer(render_id, local_view_, 2, 0, 0, 1, 1, NULL);
        if (ret != 0) {
            PrintConsole("[RTMP INFO] %s add renderer error, end with ret:%d\n", __FUNCTION__, ret);
            render->Release();
            return ret;
        }
        ret = render->StartRender(render_id);
        render->Release();
        PrintConsole("[RTMP INFO] %s end with ret:%d\n", __FUNCTION__, ret);
#else
        ViECapture *capture = ViECapture::GetInterface(vie_);
        if (capture) {
            ret = capture->SetLocalVideoWindow(render_id, local_view_);
            capture->Release();
        }
#endif
        return ret;
    }
    
    int ECMediaMachine::shutdownCameraPreviewRender(int render_id) {\
        int ret = -1;
#ifndef __APPLE__
        // stop preview render.
        ViERender *render = ViERender::GetInterface(vie_);
        ret = render->StopRender(render_id);
        ret = render->RemoveRenderer(render_id);
        render->Release();
#else
        ret = 0;
#endif
        return ret;
    }
    
    int ECMediaMachine::doPlayingPreviewRender(int render_id) {
        int ret = -1;
        ViERender* render = ViERender::GetInterface(vie_);
        if(!render) {
            PrintConsole("[RTMP ERROR] %s get vierender failed\n", __FUNCTION__);
        }
        
        ret = render->AddRenderer(render_id, local_view_, 2, 0, 0, 1, 1, NULL);
        if (ret != 0) {
            PrintConsole("[RTMP INFO] %s add renderer error, end with ret:%d\n", __FUNCTION__, ret);
            render->Release();
            return ret;
        }
        ret = render->StartRender(render_id);
        render->Release();
        PrintConsole("[RTMP INFO] %s end with ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    
    int ECMediaMachine::shutdownPlayingPreviewRender(int render_id) {
        // stop preview render.
        ViERender *render = ViERender::GetInterface(vie_);
        render->StopRender(render_id);
        render->RemoveRenderer(render_id);
        render->Release();
        return 0;
    }
    

    
    int ECMediaMachine::initVideoTransportCodec(const char *plname, int plfreq)
    {
        ViECodec *vcodec = ViECodec::GetInterface(vie_);
        VideoCodec videoCodec;
        {
            memset(&videoCodec, 0, sizeof(videoCodec));
            strcpy(videoCodec.plName, plname);
            
            videoCodec.plType           = 98;
            videoCodec.codecType        = kVideoCodecH264;
            videoCodec.width            = info_video_width_;
            videoCodec.height           = info_video_height_;
            videoCodec.startBitrate     = info_video_bitrates_;
            videoCodec.maxBitrate       = info_video_bitrates_*2;
            videoCodec.minBitrate       = info_video_bitrates_/2;
            videoCodec.targetBitrate    = info_video_bitrates_;
            videoCodec.maxFramerate     = info_video_fps_;
            videoCodec.mode             = kRealtimeVideo;
        }
        int ret = -1;
        // receive codec setting
        ret = vcodec->SetReceiveCodec(video_channel_, videoCodec);
        // send codec setting
        ret = vcodec->SetSendCodec(video_channel_, videoCodec);
        ret =vcodec->Release();
        return ret;
    }
    
    int ECMediaMachine::initAudioTransportCodec(const char *plname, int plfreq, int channels) {
        VoECodec *codec = VoECodec::GetInterface(voe_);
        CodecInst audioCodec;
        {
            strcpy(audioCodec.plname, plname);
            audioCodec.pltype       = 113;
            audioCodec.plfreq       = plfreq;
            audioCodec.channels     = channels;
            audioCodec.fecEnabled   = false;
            audioCodec.pacsize      = plfreq /100;
            audioCodec.rate         = plfreq *16;
        }
        
        // receive codec
        int ret = codec->SetRecPayloadType(audio_channel_, audioCodec);
        //  send codec
        ret = codec->SetSendCodec(audio_channel_, audioCodec);
        codec->Release();
        return ret;
    }

    int ECMediaMachine::doAudioDataSend() {
        //initAudioTransportCodec(AUDIO_CODEC, 16000, AUDIO_CHANNEL_COUNT);

        VoEBase *base = VoEBase::GetInterface(voe_);
        int ret = -1;
        ret = base->StartRecord();
        ret = base->StartSend(audio_channel_);
        base->Release();
        return ret;
    }

    int ECMediaMachine::doAudioDataReceive() {
        VoEBase *base = VoEBase::GetInterface(voe_);
        int ret = -1;
        ret = base->StartReceive(audio_channel_);
        base->Release();
        return ret;
    }

    int ECMediaMachine::shutdownAudioDataReceive() {
        VoEBase *base = VoEBase::GetInterface(voe_);
        int ret = -1;
        ret = base->StopReceive(audio_channel_);
        base->Release();
        return ret;
    }

    int ECMediaMachine::doAudioPlayout() {
        int ret = -1;
        VoEBase *base = VoEBase::GetInterface(voe_);
        ret = base->StartPlayout(audio_channel_);
        base->Release();
        
        // enable audio speaker.
        VoEHardware *hardware = VoEHardware::GetInterface(voe_);
        ret = hardware->SetLoudspeakerStatus(true);
        hardware->Release();
        if(ret != 0) {
            PrintConsole("[ECMEDIA CORE ERROR] %s  set loudspeaker status failed\n", __FUNCTION__);
        }
        return ret;
    }

    int ECMediaMachine::shutdownAudioPlayout() {
        // enable audio speaker.
        VoEBase *base = VoEBase::GetInterface(voe_);
        int ret = -1;
        ret = base->StopPlayout(audio_channel_);
        base->Release();
        return ret;
    }
    
    int ECMediaMachine::shutdownAudioDataSend() {
        VoEBase *base = VoEBase::GetInterface(voe_);
        int ret = -1;
        ret = base->StopRecord();
        ret = base->StopSend(audio_channel_);
        base->Release();
        return ret;
    }

    int ECMediaMachine::setVideoCaptureInfo(int camera_index, int fps, int bitrate, int width, int height)
    {
        PrintConsole("[ECMEDIA CORE INFO] %s start, camera_index:%d, fps:%d, bitrate:%d, width:%d, height:%d\n", __FUNCTION__, camera_index, fps, bitrate, width, height);
      
        info_camera_index_      = camera_index;
        info_video_fps_         = fps;

        info_video_bitrates_    = bitrate;
        info_video_width_       = width;
        info_video_height_      = height;
        
        PrintConsole("[ECMEDIA CORE INFO] %s end with code:%d\n", __FUNCTION__, 0);
        return 0; //initVideoTransportCodec("H264", 90000);;
    }

    int ECMediaMachine::setVideoFrameProperty(int bitrate, int width, int height) {
        PrintConsole("[ECMEDIA CORE INFO] %s start, bitrate:%d, width:%d, height:%d\n", __FUNCTION__, bitrate, width, height);
        // no change
        if(info_video_bitrates_ == bitrate && info_video_width_ == width && info_video_height_ == height) {
            return 0;
        } else if(info_video_bitrates_ != bitrate && info_video_width_ == width && info_video_height_ == height) {
            // only bitrate change
            info_video_bitrates_ = bitrate;
            return initVideoTransportCodec("H264", 90000);
        } else {
            // width or height change.
            info_video_width_       = width;
            info_video_height_      = height;
            info_video_bitrates_    = bitrate;

            int ret = -1;
            // stop render and capture video.
            ret = shutdownCameraPreviewRender(capture_id_);
            ret = shutdownCameraCapture();

            // reset video codec info.
            ret = initVideoTransportCodec("H264", 90000);
            
            // restart capturer and render.
            ret = doCameraCapture();
            ret = doCameraPreviewRender(capture_id_);
            PrintConsole("[ECMEDIA CORE INFO] %s end with code:%d\n", __FUNCTION__, ret);
            return ret;
        }
    }

    int ECMediaMachine::setBitrate(int bitrate)  {
        if(info_video_bitrates_ == bitrate) {
            return 0;
        }
        info_video_bitrates_ = bitrate;
        return initVideoTransportCodec("H264", 90000);
    }

    int ECMediaMachine::switchCamera(int index) {
        PrintConsole("[ECMEDIA CORE INFO] %s start\n", __FUNCTION__);
        if(cameras_.size() <= index) {
            // log:无效的 index
            return -1;
        }
        
        if(info_camera_index_ == index) {
            return 0;
        }
        
        info_camera_index_ = index;

        int ret = -1;
        // stop render and capture video.
        ret = shutdownCameraPreviewRender(capture_id_);
        ret = shutdownCameraCapture();

        // restart capturer and render.
        ret = doCameraCapture();
        ret = doCameraPreviewRender(capture_id_);
        PrintConsole("[ECMEDIA CORE INFO] %s end\n", __FUNCTION__);
        return ret;
    }
    
    void ECMediaMachine::setCapturerCallback(EC_CapturerCallback *callback) {
        capturer_data_callback_ = callback;
    }
    
    int ECMediaMachine::initCameraDevice()
    {
        PrintConsole("[ECMEDIA CORE INFO] %s start\n", __FUNCTION__);
        if(cameras_.size() != 0) {
            return 0;
        }

        CameraInfo *camera;
        ViECapture *capture = ViECapture::GetInterface(vie_);
        for (int i = 0; i < capture->NumberOfCaptureDevices(); i++) {
            camera = new CameraInfo;
            camera->index = i;
            capture->GetCaptureDevice(i, camera->name, sizeof(camera->name), camera->id, sizeof(camera->id));
            cameras_.push_back(camera);
        }
        capture->Release();
        
        if(cameras_.size() == 0) {
            PrintConsole("[RTMP INFO] %s get device camera failed.\n", __FUNCTION__);
            return -1;
        }
        PrintConsole("[ECMEDIA CORE INFO] %s end\n", __FUNCTION__);
        return 0;
    }
    
    void ECMediaMachine::uninitCameraDevice() {
        CameraInfo *camera;
        if (cameras_.size() != 0)
        {
            for (int i = 0; i < cameras_.size(); i++)
            {
                camera = cameras_[i];
                delete camera;
                camera = NULL;
            }
            cameras_.clear();
        }
    }

    // if pcm not is 44.1khz then resample pcm data.
    int32_t ECMediaMachine::resamplePCMData(const void *audio_data, const size_t nSamples, const uint32_t samplesRate, const size_t nChannels)
    {
        // rtc::CritScope cs(&cs_audio_record_);
        int audio_record_sample_hz_ = AAC_CODEC_SAMPLE_RATE;
        int audio_record_channels_ = 2;
        const size_t kMaxDataSizeSamples = 3840;
            if (audio_record_sample_hz_ != samplesRate || nChannels != audio_record_channels_) {
                int16_t temp_output[kMaxDataSizeSamples];
                int samples_per_channel_int = resampler_record_.Resample10Msec((int16_t*)audio_data, samplesRate * nChannels,
                                                                               audio_record_sample_hz_ * audio_record_channels_, 1, kMaxDataSizeSamples, temp_output);

                if(!faac_encode_handle_) {
                    faac_encode_input_samples_ = 0;
                    faac_encode_handle_ = faac_encoder_crate(AAC_CODEC_SAMPLE_RATE, 2, &faac_encode_input_samples_);
                }
                
                uint8_t *pcmdata , *aac_data;
                int aac_data_len = 0;
                
                recordbuffer_.PushData((unsigned char *)temp_output, 2*2*audio_record_sample_hz_/100);
                while (pcmdata = recordbuffer_.ConsumeData(faac_encode_input_samples_*2))
                {
                    aac_data_len = 0;
                    faac_encode_frame(faac_encode_handle_, pcmdata, &aac_data, &aac_data_len);
                    if (aac_data_len > 0) {
                        if(capturer_data_callback_) {
                            capturer_data_callback_->OnCapturerAacDataReady(aac_data, aac_data_len, EC_Live_Utility::getTimestamp());
                        }
                    }
                }
            }
        return 0;
    }

    #pragma mark - callback of audio data
    /**
     * ECmedia 发送音频的回调，从回调中取出音频数据，进行RTMP封装发送
     * @param frame_type
     * @param payload_type
     * @param timestamp
     * @param payload_data
     * @param payload_len_bytes
     * @param fragmentation
     * @return
     */
    int32_t ECMediaMachine::SendData(FrameType frame_type,
                                      uint8_t payload_type,
                                      uint32_t timestamp,
                                      const uint8_t* payload_data,
                                      size_t payload_len_bytes,
                                      const RTPFragmentationHeader* fragmentation)
    {
#ifdef __ANDROID__
         EC_Live_Utility::pcm_s16le_to_s16be((short*)payload_data, payload_len_bytes/2);
#endif
        resamplePCMData(payload_data, payload_len_bytes / 4, 32000, 2);
        return 0;
    }

    #pragma mark - callback of video data
    /**
     * ECmedia 视频采集完，发送视频的回调，从回调中取出视频数据，进行RTMP封装发送
     * @param payloadType
     * @param encoded_image
     * @param fragmentationHeader
     * @param rtpVideoHdr
     * @return
     */
    int32_t ECMediaMachine::SendData(uint8_t payloadType,
                                      const EncodedImage& encoded_image,
                                      const RTPFragmentationHeader& fragmentationHeader,
                                      const RTPVideoHeader* rtpVideoHdr)
    {
        PrintConsole("[ECMediaMachine INFO] %s: start\n", __FUNCTION__);
        uint8_t *data = encoded_image._buffer;
        std::vector<uint8_t> nalus;
        for (int i = 0; i < fragmentationHeader.fragmentationVectorSize; i++) {
            uint32_t nalu_size = fragmentationHeader.fragmentationLength[i];
            uint8_t * nalu = data + fragmentationHeader.fragmentationOffset[i];
            // nalu
            nalus.push_back(0x00);
            nalus.push_back(0x00);
            nalus.push_back(0x00);
            nalus.push_back(0x01);
            nalus.insert(nalus.end(), nalu, nalu + nalu_size);
        }

        if(capturer_data_callback_) {
            capturer_data_callback_->OnCapturerAvcDataReady(&nalus[0], nalus.size(), EC_Live_Utility::getTimestamp());
        }
        PrintConsole("[ECMediaMachine INFO] %s: end\n", __FUNCTION__);
        return 0;
    }

    /**
     * 视频源选择，相机或桌面
     * @param video_source
     */
    void ECMediaMachine::SetVideoCaptureSource(VIDEO_SOURCE video_source) {
       video_source_ = video_source;
    }

    void ECMediaMachine::SelectShareWindow(int type, int id)
    {
        share_window_id_ = id;
        if (desktop_capture_id_ < 0)
            return;
        ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
        if (DesktopShareType::ShareScreen == type) {
            desktopShare->SelectScreen(desktop_capture_id_, id);
        }
        else if (DesktopShareType::ShareWindow == type) {
            desktopShare->SelectWindow(desktop_capture_id_, id);
        }
        desktopShare->Release();
    }

    void ECMediaMachine::GetShareWindowList(std::vector<ShareWindowInfo> & list)
    {
        int capture_id;
        ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
        if (desktop_capture_id_ >= 0) {
            capture_id = desktop_capture_id_;
        }
        else {
            desktopShare->AllocateDesktopShareCapturer(capture_id, desktop_share_type_);
        }

        ShareWindowInfo win_info;
        ScreenList screens;
        desktopShare->GetScreenList(capture_id, screens);
        for (int i = 0; i < screens.size(); i++) {
            win_info.id = screens[i];
            win_info.name = "desktop";
            win_info.type = ShareScreen;
            list.push_back(win_info);
        }
        WindowList windows;
        desktopShare->GetWindowList(capture_id, windows);
        for (int i = 0; i < windows.size(); i++) {
            win_info.id = windows[i].id;
            win_info.name = windows[i].title;
            win_info.type = ShareWindow;
            list.push_back(win_info);
        }
        if (desktop_capture_id_ < 0) {
            desktopShare->ReleaseDesktopShareCapturer(capture_id);
        }
        desktopShare->Release();
    }
    
    /***************************   receive  data ******************************/
    void ECMediaMachine::SetRtpData(int channel,void *rtpdata,int type)
    {
        PrintConsole("[ECMEDIA CORE INFO] %s start\n", __FUNCTION__);
        if( 0 == type ) {
            audio_data_cb_ = (RtpData*)rtpdata;
        }
        else
        {
            video_data_cb_ = (RtpData*)rtpdata;
        }
    }

    void ECMediaMachine::onAvcDataComing(void* nalu_data, int len, uint32_t timestamp) {
        if(!nalu_data) {
            return;
        }
        PrintConsole("[ECMEDIA CORE INFO] %s start\n", __FUNCTION__);
        if( video_data_cb_) {
            RTPHeader rtpHeader;
            rtpHeader.sequenceNumber = video_rtp_seq_++;
            rtpHeader.ssrc = 1;
            rtpHeader.payloadType = 98;
            rtpHeader.timestamp = timestamp;
  
            uint8_t frame_type = *(uint8_t*)nalu_data & 0x1f;
            static bool isSpsPpsHasComing = false;
      
            //some sei may come befor sps and pps, picture appare mosaic, so we jump it.
            uint8_t type_sei = 6;
            if(!isSpsPpsHasComing && frame_type == type_sei) {
                return;
            }
            
            if (frame_type == 7 || frame_type == 8) { //sps or pps
                isSpsPpsHasComing = true;
                rtpHeader.markerBit = false;
            } else {
                isSpsPpsHasComing = false;
                rtpHeader.markerBit = true;
            }
            video_data_cb_->ReceivePacket((const uint8_t*) nalu_data, len, rtpHeader, true);
        }
        PrintConsole("[ECMEDIA CORE INFO] %s end\n", __FUNCTION__);
    }

    void ECMediaMachine::onAacDataComing(uint8_t* pData, int nLen, uint32_t ts, uint32_t sample_rate, int audio_channels) {
        if(audio_data_cb_) {
                WebRtcRTPHeader rtpHeader;
                rtpHeader.header.sequenceNumber = audio_rtp_seq_++;
                rtpHeader.header.ssrc = 1;
                rtpHeader.header.payloadType = 113;
                rtpHeader.header.timestamp = 320 * rtpHeader.header.sequenceNumber;
                audio_data_cb_->OnReceivedPayloadData((const uint8_t*)pData, 32000/100*audio_channels*2, &rtpHeader);
            }
            PrintConsole("[ECMEDIA CORE INFO] %s end\n", __FUNCTION__);
    }

    int ECMediaMachine::doDesktopCapture()
    {
        ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
        if (desktop_capture_id_ >= 0) {
            desktopShare->StopDesktopShareCapture(desktop_capture_id_);
        }
        int ret = desktopShare->AllocateDesktopShareCapturer(desktop_capture_id_, desktop_share_type_);
        
        if (desktop_share_type_ == ShareWindow) {
            if (share_window_id_ == -1) {
                WindowList windows;
                desktopShare->GetWindowList(desktop_capture_id_, windows);
                for (int i = 0; i < windows.size();i++) {
                    if (windows[i].title.find("cdr") != std::string::npos) {
                        share_window_id_ = windows[i].id;
                    }
                }
            }
            desktopShare->SelectWindow(desktop_capture_id_, share_window_id_);
        }
        else if (desktop_share_type_ == ShareScreen) {
            if (share_window_id_ == -1) {
                ScreenList screens;
                desktopShare->GetScreenList(desktop_capture_id_, screens);
                if (!screens.empty()) {
                    share_window_id_ = screens[0];
                } else {
                    /*iOS 上没有实现获取窗口list的代码，获取为空时，给share_window_id_随机赋值一个正值，防止访问screens[0] 崩溃*/
                    share_window_id_ = 1;
                }
            }
            desktopShare->SelectScreen(desktop_capture_id_, share_window_id_);
        }
        int width =0 , heigth = 0;
        desktopShare->GetDesktopShareCaptureRect(desktop_capture_id_, info_video_width_, info_video_height_);
        //info_video_width_ = 640;// info_video_width_ / 4 * 4;
        //info_video_height_ = 480;//;info_video_height_ / 4 * 4;
        info_video_bitrates_ = info_video_width_ * info_video_height_ * 0.07 *15 /1000;
        PrintConsole("desktop share width is %d heigth is %d \n", info_video_width_, info_video_height_);
        initVideoTransportCodec("H264", 90000);
        
        ret = desktopShare->StartDesktopShareCapture(desktop_capture_id_, 15);
        ret = desktopShare->ConnectDesktopCaptureDevice(desktop_capture_id_, video_channel_);
        
#ifdef WIN32
        ViERender* render = ViERender::GetInterface(vie_);
        ret = render->AddRenderer(desktop_capture_id_, local_view_, 1, 0, 0, 1, 1, NULL);
        if (ret) {
            render->Release();
            return ret;
        }
        ret = render->StartRender(desktop_capture_id_);
        render->Release();
        
#else
        //ret = desktopShare->SetLocalVideoWindow(desktop_capture_id_, local_view_);
#endif
        desktopShare->Release();
        return 0;
    }
    
    int ECMediaMachine::shutdownDesktopCapture() {
        ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
#ifdef WIN32
        ViERender* render = ViERender::GetInterface(vie_);
        render = ViERender::GetInterface(vie_);
        render->StopRender(desktop_capture_id_);
        render->RemoveRenderer(desktop_capture_id_);
#endif
        desktopShare->DisConnectDesktopCaptureDevice(video_channel_);
        desktopShare->StopDesktopShareCapture(desktop_capture_id_);
        desktopShare->ReleaseDesktopShareCapturer(desktop_capture_id_);
        desktopShare->Release();
        desktop_capture_id_ = -1;
        share_window_id_ = -1;
        return 0;
    }
} // end of namespace cloopenwebrtc
