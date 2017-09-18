//
// Created by gezhaoyou on 17/7/23.
// Copyright (c) 2017 Sean Lee. All rights reserved.
//

#ifndef ECMEDIA_EC_LIVE_ENGINE_H
#define ECMEDIA_EC_LIVE_ENGINE_H

#include "ec_live_common.h"
#include "ec_media_puller_base.h"
#include "ec_rtmp_bitrate_controller.h"

namespace cloopenwebrtc {
    class ECRtmpPublisher;
    class ECMediaMachine;
    class EC_RtmpPuller;
    class EC_RtmpPublishCallback;
    class EC_MediaPullCallback;

    class ECLiveEngine: public EC_RTMP_BitrateControllerCallback
    {
    public:
        static ECLiveEngine *getInstance();
        static void destroy();

        int startPublish(const char *url, EC_RtmpPublishCallback *callback);
        int startPlay(const char* url, EC_MediaPullCallback* callback);
       
        int stopPublish();
        int stopPlay();
        
        int setVideoPreview(void * view);
        void setAutoBitrate(bool isEnable);
        int  configLiveVideoStream(LiveVideoStreamConfig config);
        //
        int switchCamera(int index);

    protected:
        void onOutputBitrateChanged(int bitrate);
        void onNeedClearBuffer();
        int init();

    private:
        ECLiveEngine();
        ~ECLiveEngine();
        // simple live stream puller factory, only supporting rtmp now.
        EC_MediaPullerBase* createMediaPuller(const char* url, EC_MediaPullCallback* callback);
        void getVideoStreamInfo(EC_LiveVideoResolution resolution, int &width, int &height, int &bitrate);

    private:
        ECMediaMachine*             ec_media_core_;
        ECRtmpPublisher*            rtmp_publisher_;
        EC_MediaPullerBase*         meida_puller_;
        EC_RTMP_BitrateController*  bitrate_controller_;

        bool publiser_running_;
        bool puller_runnig_;
        bool network_adaptive_enable_;
    };
}

#endif //ECMEDIA_EC_LIVE_ENGINE_H