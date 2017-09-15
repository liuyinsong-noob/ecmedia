//
// Created by gezhaoyou on 17/7/23.
// Copyright (c) 2017 Sean Lee. All rights reserved.
//

#include "ec_live_engine.h"
#include "ec_media_core.h"
#include "ec_rtmp_publisher.h"
#include "ec_rtmp_puller.h"
#include "ec_hls_puller.h"

namespace cloopenwebrtc {
    static ECLiveEngine *ec_live_engine_ = NULL;
    ECLiveEngine::ECLiveEngine()
    {
        publiser_running_           = false;
        puller_runnig_              = false;
        network_adaptive_enable_    = false;

        meida_puller_               = nullptr;
        rtmp_publisher_             = nullptr;
        bitrate_controller_         = nullptr;

        ec_media_core_ =  new ECMediaMachine();
    }

    ECLiveEngine::~ECLiveEngine() {
        if(rtmp_publisher_) {
            delete rtmp_publisher_;
            rtmp_publisher_ = NULL;
        }

        if(meida_puller_) {
            delete meida_puller_;
            meida_puller_ = NULL;
        }

        if(ec_media_core_) {
            ec_media_core_->UnInit();
            delete ec_media_core_;
            ec_media_core_ = NULL;
        }

        if(bitrate_controller_) {
            delete bitrate_controller_;
        }
    }

    int ECLiveEngine::init() {
        return ec_media_core_->Init();
    }

    // singleton
    ECLiveEngine *ECLiveEngine::getInstance() {
        PrintConsole("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        if(!ec_live_engine_) {
            PrintConsole("[ECLiveEngine INFO] %s: create new live engine instance.", __FUNCTION__);
            ec_live_engine_ = new ECLiveEngine();
            if(ec_live_engine_->init() != 0) {
                PrintConsole("[ECLiveEngine Error] %s: ec live engine init failed.", __FUNCTION__);
                return nullptr;
            }
        }
        PrintConsole("[ECLiveEngine INFO] %s: end", __FUNCTION__);
        return ec_live_engine_;
    }
    
    void ECLiveEngine::destroy() {
        if(!ec_live_engine_) {
            return;
        }
        
        delete ec_live_engine_;
        ec_live_engine_ = nullptr;
    }

    // publish rtmp stream
    int ECLiveEngine::startPublish(const char *url, EC_RtmpPublishCallback *callback)
    {
        PrintConsole("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        if(!publiser_running_) {
            publiser_running_ = true;
            if(!bitrate_controller_) {
                bitrate_controller_ = new EC_RTMP_BitrateController();
                bitrate_controller_->setBitrateControllerCallback(this);
            }

            if(!rtmp_publisher_) {
                rtmp_publisher_ = new ECRtmpPublisher(callback, bitrate_controller_);
                ec_media_core_->setCapturerCallback(rtmp_publisher_);
            }

            rtmp_publisher_->start(url);
            int ret = ec_media_core_->startCapture();
            return ret;
        }
        PrintConsole("[ECLiveEngine INFO] %s: end", __FUNCTION__);
        return 0;
    }

    int ECLiveEngine::stopPublish() {
        PrintConsole("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        if(publiser_running_) {
            int ret = -1;
            ret = ec_media_core_->stopCapture();
            rtmp_publisher_->stop();

            publiser_running_ = false;
            return ret;
        }
        PrintConsole("[ECLiveEngine INFO] %s: stop", __FUNCTION__);
        return 0;
    }

    // play live(rtmp/hls/http-flv) stream
    int ECLiveEngine::startPlay(const char* url, EC_MediaPullCallback* callback) {
        PrintConsole("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        int ret = -1;
        if(!puller_runnig_) {
            puller_runnig_ = true;

            if(!meida_puller_) {
                meida_puller_ = createMediaPuller(url, callback);
                if(!meida_puller_) {
                    PrintConsole("[ECLiveEngine INFO] %s: create media puller faild.", __FUNCTION__);
                    return -1;
                }
                meida_puller_->setReceiverCallback(ec_media_core_);
            }

            meida_puller_->start(url);
            ret = ec_media_core_->startPlayout();
        }
        PrintConsole("[ECLiveEngine INFO] %s: end with code: %d", __FUNCTION__, ret);
        return 0;
    }

    int ECLiveEngine::stopPlay() {
        PrintConsole("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        int ret = -1;
        if(puller_runnig_) {
            if(meida_puller_) {
                meida_puller_->stop();
            }
            ret = ec_media_core_->stopPlayout();

            puller_runnig_ = false;
        }
        PrintConsole("[ECLiveEngine INFO] %s: end with code: %d", __FUNCTION__, ret);
        return ret;
    }

    // preview viewer setting.
    int ECLiveEngine::setVideoPreview(void * view) {
        return ec_media_core_->setVideoPreview(view);
    }

    // enable or disable rtmp auto bitrate.
    void ECLiveEngine::setAutoBitrate(bool isEnable) {
        network_adaptive_enable_ = isEnable;
    }
    
    int ECLiveEngine::configLiveVideoStream(LiveVideoStreamConfig config) {
        network_adaptive_enable_ = config._auto_bitrate;
        
        int width = 0, height = 0, bitrate = 0;
        getVideoStreamInfo(config._resolution, width, height, bitrate);
        return ec_media_core_->setVideoCaptureInfo(config._camera_index, config._fps, bitrate, width, height);
    }
    
    int ECLiveEngine::switchCamera(int index) {
        if(publiser_running_) {
            return ec_media_core_->switchCamera(index);
        }
        return 0;
    }

    // simple live stream puller factory
    EC_MediaPullerBase* ECLiveEngine::createMediaPuller(const char* url, EC_MediaPullCallback* callback) {
        if(strncmp(url, "rtmp", 4) == 0) {
            return new EC_RtmpPuller(callback);
        } else if(strncmp(url, "http", 4) == 0) {
            return new EC_HLS_Puller(callback);
        } else {
            
            // todo: http-flv player
            return nullptr;
        }
        return 0;
    }

    void ECLiveEngine::onOutputBitrateChanged(int bitrate) {
        EC_LiveVideoResolution resolution;
        int kbps = 8*bitrate/1000;
        if(kbps > 1280) { // 720p
            resolution = EC_VIDEO_RESOLUTION_720P;
        } else if(kbps > 1024) { // HD
            resolution = EC_VIDEO_RESOLUTION_HD;
        } else if(kbps > 768) { //  QHD
            resolution = EC_VIDEO_RESOLUTION_QHD;
        } else if(kbps > 512) { //  SD
            resolution = EC_VIDEO_RESOLUTION_SD;
        } else if(kbps > 384) { //  Fluency
            resolution = EC_VIDEO_RESOLUTION_LOW;
        } else {
            resolution = EC_VIDEO_RESOLUTION_LOW;
        }
        int w = 0, h = 0, br = 0;
        getVideoStreamInfo(resolution, w, h, br);
        if(network_adaptive_enable_) {
            ec_media_core_->setVideoFrameProperty(br, w, h);
        }
    }

    void ECLiveEngine::onNeedClearBuffer() {
        rtmp_publisher_->needClearCacher();
    }

    void ECLiveEngine::getVideoStreamInfo(EC_LiveVideoResolution resolution, int &width, int &height, int &bitrate) {
        width = 640;
        height = 360;
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
        width = 360; 
        height = 640;
#endif
        switch (resolution) {
            case EC_VIDEO_RESOLUTION_720P:
                width = 1280;
                height = 720;
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
                width = 720;
                height = 1280;
#endif
                bitrate = 1280;
                
                break;
            case EC_VIDEO_RESOLUTION_HD:
                width = 960;
                height = 540;
#if defined(WEBRTC_ANDROID) || defined(WEBRTC_IOS)
                width = 540;
                height = 960;
                bitrate = 1024;
#endif
                break;
            case EC_VIDEO_RESOLUTION_QHD:
                bitrate = 768;
                break;
            case EC_VIDEO_RESOLUTION_SD:
                bitrate = 512;
                break;
            case EC_VIDEO_RESOLUTION_LOW:
                bitrate = 384;
                break;
            default:
                break;
        }
    }
}
