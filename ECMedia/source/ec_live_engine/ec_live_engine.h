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
    class AudioTransport;

    class ECLiveEngine: public EC_RTMP_BitrateControllerCallback
    {
    public:
        static ECLiveEngine *getInstance();
        static void destroy();

        int startPublish(const char *url, ECLiveStreamNetworkStatusCallBack callback);
        int startPlay(const char* url, ECLiveStreamNetworkStatusCallBack callback);
       
        int stopPublish();
        int stopPlay();
        
        int setVideoPreview(void * view);
        void setAutoBitrate(bool isEnable);
        int configLiveVideoStream(LiveVideoStreamConfig config);
        int setCaptureFrameDegree(ECLiveFrameDegree degree);
        //
        int switchCamera(int index);
        int setBeautyFace(bool enable);
        int prepare();
    protected:
        void onOutputBitrateChanged(int bitrate);
        void onNeedClearBuffer();

    private:
        ECLiveEngine();
        ~ECLiveEngine();
        // simple live stream puller factory, only supporting rtmp now.
        EC_MediaPullerBase* createMediaPuller(const char* url, ECLiveStreamNetworkStatusCallBack callback);
        void getVideoStreamInfo(EC_LiveVideoResolution resolution, int &width, int &height, int &bitrate);

    private:
        ECMediaMachine*             ec_media_core_;
        ECRtmpPublisher*            rtmp_publisher_;
        EC_MediaPullerBase*         media_puller_;
        EC_RTMP_BitrateController*  bitrate_controller_;

        bool publiser_running_;
        bool puller_runnig_;
        bool network_adaptive_enable_;
    };
}

#endif //ECMEDIA_EC_LIVE_ENGINE_H
