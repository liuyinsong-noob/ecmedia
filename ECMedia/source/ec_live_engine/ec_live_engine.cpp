//
// Created by gezhaoyou on 17/7/23.
// Copyright (c) 2017 Sean Lee. All rights reserved.
//

#include "ec_live_engine.h"
#include "ec_media_core.h"
#include "ec_rtmp_publisher.h"
#include "ec_rtmp_puller.h"
#include "ec_play_buffer_cacher.h"

#ifdef WIN32
#else
#include "ec_hls_puller.h"
#endif

namespace yuntongxunwebrtc {
    static ECLiveEngine *ec_live_engine_ = NULL;
    ECLiveEngine::ECLiveEngine()
    {
        publiser_running_           = false;
        puller_runnig_              = false;
        network_adaptive_enable_    = false;

        ec_media_core_              = nullptr;
        media_puller_               = nullptr;
        rtmp_publisher_             = nullptr;
        bitrate_controller_         = nullptr;
    }

    ECLiveEngine::~ECLiveEngine() {
        if(rtmp_publisher_) {
            delete rtmp_publisher_;
            rtmp_publisher_ = NULL;
        }

        if(media_puller_) {
            delete media_puller_;
            media_puller_ = NULL;
        }
        
        if(bitrate_controller_) {
            delete bitrate_controller_;
        }
    }

    // singleton
    ECLiveEngine *ECLiveEngine::getInstance() {
        WriteLogToFile("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        if(!ec_live_engine_) {
            WriteLogToFile("[ECLiveEngine INFO] %s: create new live engine instance.", __FUNCTION__);
            ec_live_engine_ = new ECLiveEngine();
        }
        WriteLogToFile("[ECLiveEngine INFO] %s: end", __FUNCTION__);
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
    int ECLiveEngine::startPublish(const char *url, ECLiveStreamNetworkStatusCallBack callback)
    {
        WriteLogToFile("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        if(!publiser_running_ && !puller_runnig_) {
            publiser_running_ = true;
            if(ec_media_core_ == nullptr) {
                ec_media_core_ =  new ECMediaMachine();
            }
            ec_media_core_->Init();
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
        WriteLogToFile("[ECLiveEngine INFO] %s: end", __FUNCTION__);
        return 0;
    }

    int ECLiveEngine::stopPublish() {
        WriteLogToFile("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        if(publiser_running_ && !puller_runnig_) {
            int ret = -1;
            ret = ec_media_core_->stopCapture();
            ec_media_core_->UnInit();
            delete ec_media_core_;
            ec_media_core_ = nullptr;
            
            rtmp_publisher_->stop();
			delete rtmp_publisher_;
			rtmp_publisher_ = nullptr;
            publiser_running_ = false;
            WriteLogToFile("[ECLiveEngine INFO] %s: stop with code: %d", __FUNCTION__, ret);
            return ret;
        }
        WriteLogToFile("[ECLiveEngine INFO] %s: stop", __FUNCTION__);
        return 0;
    }

    // play live(rtmp/hls/http-flv) stream
    int ECLiveEngine::startPlay(const char* url, ECLiveStreamNetworkStatusCallBack callback) {
        WriteLogToFile("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        int ret = -1;
        if(!puller_runnig_ && !publiser_running_) {
            puller_runnig_ = true;
            if(ec_media_core_ == nullptr) {
                ec_media_core_ =  new ECMediaMachine();
            }
            if(!media_puller_) {
                media_puller_ = createMediaPuller(url, callback);
                if(!media_puller_) {
                    WriteLogToFile("[ECLiveEngine INFO] %s: create media puller faild.", __FUNCTION__);
                    return -1;
                }
                media_puller_->setReceiverCallback(ec_media_core_);
                ec_media_core_->Init(reinterpret_cast<AudioTransport *>(media_puller_->getMediaPacketBuffer()));
            }

            media_puller_->start(url);
            ret = ec_media_core_->startPlayout();
        }
        WriteLogToFile("[ECLiveEngine INFO] %s: end with code: %d", __FUNCTION__, ret);
        return 0;
    }

    int ECLiveEngine::stopPlay() {
        WriteLogToFile("[ECLiveEngine INFO] %s: start", __FUNCTION__);
        int ret = -1;
        if(puller_runnig_ && !publiser_running_) {
            if(media_puller_) {
                media_puller_->stop();
            }
            
            ret = ec_media_core_->stopPlayout();
            ec_media_core_->UnInit();
            
            delete ec_media_core_;
            ec_media_core_ = nullptr;
            
            // media puller  must delete after ecmedia core, otherwise ecmedia core will crash.
            delete media_puller_;
            media_puller_ = nullptr;
            
        }
		puller_runnig_ = false;
        WriteLogToFile("[ECLiveEngine INFO] %s: end with code: %d", __FUNCTION__, ret);
        return ret;
    }

    // preview viewer setting.
    int ECLiveEngine::setVideoPreview(void * view) {
        if(ec_media_core_ == nullptr) {
            ec_media_core_ =  new ECMediaMachine();
        }
        
        return ec_media_core_->setVideoPreview(view);
    }

    // enable or disable rtmp auto bitrate.
    void ECLiveEngine::setAutoBitrate(bool isEnable) {
        network_adaptive_enable_ = isEnable;
    }
    
    int ECLiveEngine::configLiveVideoStream(LiveVideoStreamConfig config) {
        if(ec_media_core_ == nullptr) {
            ec_media_core_ =  new ECMediaMachine();
        }
        network_adaptive_enable_ = config._auto_bitrate;
        
        int width = 0, height = 0, bitrate = 0;
        getVideoStreamInfo(config._resolution, width, height, bitrate);
        return ec_media_core_->setVideoCaptureInfo(config._camera_index, RotateCapturedFrame(config._frmae_degree) ,config._fps, bitrate, width, height);
    }
    int ECLiveEngine::setCaptureFrameDegree(ECLiveFrameDegree degree) {
        if(publiser_running_) {
            return ec_media_core_->setCaptureFrameDegree(RotateCapturedFrame(degree));
        }
        return -1;
    }
    
    int ECLiveEngine::switchCamera(int index) {
        if(!ec_media_core_) {
            return -1;
        }
        if(publiser_running_ && !puller_runnig_) {
            return ec_media_core_->switchCamera(index);
        }
        return 0;
    }

    int ECLiveEngine::setBeautyFace(bool enable) {
        if(!ec_media_core_) {
            return -1;
        }
        return ec_media_core_->setBeautyFace(enable);
    }
    
    // simple live stream puller factory
    EC_MediaPullerBase* ECLiveEngine::createMediaPuller(const char* url, ECLiveStreamNetworkStatusCallBack callback) {
        if(strncmp(url, "rtmp", 4) == 0) {
            return new EC_RtmpPuller(callback);
        } else if(strncmp(url, "http", 4) == 0) {
#ifdef WIN32
			return nullptr; // not support win32
#else
            return nullptr; //new EC_HLS_Puller(callback);
#endif
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
        height = 480;
        switch (resolution) {
            case EC_VIDEO_RESOLUTION_720P:
                width = 1280;
                height = 720;
                bitrate = 1280;
                break;
            case EC_VIDEO_RESOLUTION_HD:
                bitrate = 1024;
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
