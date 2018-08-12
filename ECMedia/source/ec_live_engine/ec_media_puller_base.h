//
// Created by 葛昭友 on 2017/8/1.
// Copyright (c) 2017 Sean Lee. All rights reserved.
//

#ifndef ECMEDIA_MEDIA_PULLER_H
#define ECMEDIA_MEDIA_PULLER_H

namespace yuntongxunwebrtc {
    class EC_AVCacher;
    class EC_MediaPullerBase {
    public:
        EC_MediaPullerBase() {};
        virtual ~EC_MediaPullerBase() {};
        virtual void start(const char* url) = 0;
        virtual void stop() = 0;
        virtual void setReceiverCallback(EC_ReceiverCallback* callback) = 0;
        virtual EC_AVCacher* getMediaPacketBuffer() = 0;
    };
}

#endif //ECMEDIA_MEDIA_PULLER_H
