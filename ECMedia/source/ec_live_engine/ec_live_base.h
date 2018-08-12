//
//  ec_rtmp_base.hpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_rtmp_base_h
#define ec_rtmp_base_h


namespace yuntongxunwebrtc {
    class ECLiveBase {
    public:
        // create live stream instance
        virtual int CreateLiveStream(int type) = 0;
        
        // choose publish camera or
        virtual int SetVideoSource(int video_source) = 0;
        
        // start and stop palystream
        virtual int PlayStream(char *url, void *renderView, void *callback) = 0;
        virtual int StopPlay() = 0;
        
        // start and stop publish stream
        virtual int PushStream(char *url, void *localView) = 0;
        virtual int StopPush() = 0;
        
        // set video profile
        virtual int setVideoProfile(void  ) = 0;
        
        // set rtmp network callback
        virtual int setNetworkStatusCallBack( ) = 0;
        
        // beauty face
        virtual int EnableBeautyFace() = 0;
        virtual int DisableBeautyFace() = 0;
    
    protected:
        ECLiveBase() {}
        virtual ~ECLiveBase() {}
        
    };

    class AVCImageCallback {
        virtual void OnEncodeDataCallback(bool audio, uint8_t *p, uint32_t length, uint32_t ts) = 0;
    };
}

#endif /* ec_rtmp_base_h */
