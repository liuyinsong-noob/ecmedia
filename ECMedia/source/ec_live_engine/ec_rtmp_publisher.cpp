//
//  ec_rtmp_publisher.cpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.

#include "ec_rtmp_publisher.h"
#include "srs_librtmp.h"
#include "event_wrapper.h"

namespace cloopenwebrtc {
#define MAX_RETRY_TIME 3 // max retry connect to rtmp server times.
    
    ECRtmpPublisher::ECRtmpPublisher(ECLiveStreamNetworkStatusCallBack callback, EC_RTMP_BitrateController *bc) :
            running_(false)
            ,rtmp_status_(RS_STM_Init)
            ,need_keyframe_(false)
            ,rtmp_lock_(CriticalSectionWrapper::CreateCriticalSection())
    {
        retrys_ = 0;
        need_clear_av_cacher_ = false;
        rtmp_ = nullptr;
        callback_ = callback;
        rtmp_bitrate_ontroller_ = bc;
        rtmpPublishThread_ = ThreadWrapper::CreateThread(ECRtmpPublisher::publishThreadRun,
                this,
                kNormalPriority,
                "rtmpPublishThread_");
        cacher_update_event_ = EventTimerWrapper::Create();
    }
    
    ECRtmpPublisher::~ECRtmpPublisher() {
        if(running_) {
            stop();
        }

        if(cacher_update_event_) {
            cacher_update_event_->Set();
            delete cacher_update_event_;
        }
        
        if(rtmpPublishThread_) {
            rtmpPublishThread_->Stop();
            delete rtmpPublishThread_;
        }

        rtmp_status_ = RS_STM_Closed;
    }
    
    void ECRtmpPublisher::start(const char *url) {
        PrintConsole("[ECRtmpPublisher INFO] %s: begin", __FUNCTION__);
        if(!running_) {
            running_ = true;
            // save rtmp url;
            rtmp_url = url;
            cacher_update_event_->StartTimer(true, 10);
            rtmp_ = srs_rtmp_create(url);
            rtmp_bitrate_ontroller_->start();
            
            unsigned int pthread_id;
            rtmpPublishThread_->Start(pthread_id);
        }
        PrintConsole("[ECRtmpPublisher INFO] %s: end.", __FUNCTION__);
    }
    
    void ECRtmpPublisher::stop() {
        PrintConsole("[ECRtmpPublisher INFO] %s: begin", __FUNCTION__);
        if(running_) {
            running_ = false;
            rtmp_bitrate_ontroller_->shutdown();
            srs_rtmp_disconnect_server(rtmp_);
            rtmpPublishThread_->Stop();
            if (rtmp_) {
                srs_rtmp_destroy(rtmp_);
                rtmp_ = nullptr;
            }
            cacher_update_event_->StopTimer();
            retrys_ = 0;
            rtmp_status_ = RS_STM_Init;
        }
        PrintConsole("[ECRtmpPublisher INFO] %s: end.", __FUNCTION__);
    }
    
    void ECRtmpPublisher::needClearCacher() {
        need_clear_av_cacher_ = true;
    }
    
    void ECRtmpPublisher::EnableOnlyAudioMode()
    {
        // only_audio_mode_ = true;
    }

    void ECRtmpPublisher::OnCapturerAvcDataReady(uint8_t* pData, int nLen, uint32_t ts) {
        uint8_t *p = pData;
        int nal_type = p[4] & 0x1f;
        if(nal_type == 7)
            need_keyframe_ = false;
        if(need_keyframe_)
            return;
        if(nal_type == 7)
        {// keyframe
            int find7 = 0;
            uint8_t* ptr7 = NULL;
            int size7 = 0;
            int find8 = 0;
            uint8_t* ptr8 = NULL;
            int size8 = 0;
            uint8_t* ptr5 = NULL;
            int size5 = 0;
            int head01 = 4;
            // what for, zhaoyou?
            for (int i = 4; i < nLen - 4; i++)
            {
                if ((p[i] == 0x0 && p[i + 1] == 0x0 && p[i + 2] == 0x0 && p[i + 3] == 0x1) || (p[i] == 0x0 && p[i + 1] == 0x0 && p[i + 2] == 0x1))
                {
                    if (p[i + 2] == 0x01)
                        head01 = 3;
                    else
                        head01 = 4;
                    if (find7 == 0)
                    {
                        find7 = i;
                        ptr7 = p;
                        size7 = find7;
                        i++;
                    }
                    else if (find8 == 0)
                    {
                        find8 = i;
                        ptr8 = p + find7;
                        size8 = find8 - find7;
                        unsigned char* ptr = p + i;
                        if ((ptr[head01] & 0x1f) == 5)
                        {
                            ptr5 = p + find8;
                            size5 = nLen - find8;
                            break;
                        }
                    }
                    else
                    {
                        ptr5 = p + i;
                        size5 = nLen - i;
                        break;
                    }
                }
            }
            
            GotH264Nal(ptr7, size7, false, ts);
            GotH264Nal(ptr8, size8, false, ts);
            GotH264Nal(ptr5, size5, true, ts);
        }
        else 
        {
            GotH264Nal(pData, nLen, false, ts);
        }
    }
    
    void ECRtmpPublisher::GotH264Nal(uint8_t* pData, int nLen, bool isKeyFrame, uint32_t ts) {
        static int a = 0;
        if( a >= 3) {
            return;
        }
        
        EncData* pdata = new EncData();
        pdata->_data = new uint8_t[nLen];
        memcpy(pdata->_data, pData, nLen);
        pdata->_dataLen = nLen;
        // pdata->_isVideo = true;
        pdata->_type = VIDEO_DATA;
        pdata->_dts = ts;
 
        rtmp_lock_->Enter();
        if(need_clear_av_cacher_) {
            need_clear_av_cacher_ = false;
            clearMediaCacher();
        }
        rtmp_bitrate_ontroller_->inputDataCount(nLen);
        
        lst_enc_data_.push_back(pdata);
        rtmp_lock_->Leave();
    }
    
    void ECRtmpPublisher::OnCapturerAacDataReady(uint8_t *pData, int nLen, uint32_t ts) {
        if(need_keyframe_) {
            return;
        }

        EncData* pdata = new EncData();
        pdata->_data = new uint8_t[nLen];
        memcpy(pdata->_data, pData, nLen);
        pdata->_dataLen = nLen;
        pdata->_isVideo = false;
        pdata->_type = AUDIO_DATA;
        pdata->_dts = ts;
        
        rtmp_bitrate_ontroller_->inputDataCount(nLen);
        rtmp_lock_->Enter();
        lst_enc_data_.push_back(pdata);
        rtmp_lock_->Leave();
    }
    
    bool ECRtmpPublisher::publishThreadRun(void *pThis) {
        return static_cast<ECRtmpPublisher*>(pThis)->run();
    }

    bool ECRtmpPublisher::run() {
        PrintConsole("[ECRtmpPublisher INFO] %s: publishing...", __FUNCTION__);
        while(running_) {
            if(rtmp_ != NULL)
            {
                switch (rtmp_status_) {
                    case RS_STM_Init:
                    {
                        if (srs_rtmp_handshake(rtmp_) == 0) {
                            PrintConsole("SRS: simple handshake ok.");
                            rtmp_status_ = RS_STM_Handshaked;
                        }
                        else {
                            callOnDisconnect();
                        }
                    }
                        break;
                    case RS_STM_Handshaked:
                    {
                        if (srs_rtmp_connect_app(rtmp_) == 0) {
                            PrintConsole("SRS: connect vhost/app ok.");
                            rtmp_status_ = RS_STM_Connected;
                        }
                        else {
                            callOnDisconnect();
                        }
                    }
                        break;
                    case RS_STM_Connected:
                    {
                        if (srs_rtmp_publish_stream(rtmp_) == 0) {
                            PrintConsole("SRS: publish stream ok.");
                            rtmp_status_ = RS_STM_Published;
                            clearMediaCacher();
                            if(callback_) {
                                callback_(EC_LIVE_PUSH_SUCCESS);
                            }
                        }
                        else {
                            callOnDisconnect();
                        }
                    }
                        break;
                    case RS_STM_Published:
                    {
                         doPushRtmpPacket();
                    }
                        break;
                }
            }
        }
        PrintConsole("[ECRtmpPublisher INFO] %s: ending...", __FUNCTION__);
        return false;
    }

    void ECRtmpPublisher::doPushRtmpPacket() {
        if(lst_enc_data_.size() <= 0) {
            cacher_update_event_->Wait(10);
            return;
        }
        
        EncData* dataPtr = nullptr;
        {
            rtmp_lock_->Enter();
            dataPtr = lst_enc_data_.front();
            lst_enc_data_.pop_front();
            rtmp_lock_->Leave();
        }

        if (dataPtr->_type == VIDEO_DATA) {
            char *ptr = (char*)dataPtr->_data;
            int len = dataPtr->_dataLen;
            int ret = 0;
            ret = srs_h264_write_raw_frames(rtmp_, ptr, len, dataPtr->_dts, dataPtr->_dts);

            if (ret != 0) {
                if (srs_h264_is_dvbsp_error(ret)) {
                    PrintConsole("ignore drop video error, code=%d", ret);
                }
                else if (srs_h264_is_duplicated_sps_error(ret)) {
                    //srs_human_trace("ignore duplicated sps, code=%d", ret);
                }
                else if (srs_h264_is_duplicated_pps_error(ret)) {
                    //srs_human_trace("ignore duplicated pps, code=%d", ret);
                }
                else {
                    PrintConsole("send h264 raw data failed. ret=%d", ret);
                    callOnDisconnect();
                    return;
                }
            }

        } else if(dataPtr->_type == AUDIO_DATA){
            int ret = 0;
            if ((ret = srs_audio_write_raw_frame(rtmp_,
                    10, 3, 1, 1,
                    (char*)dataPtr->_data, dataPtr->_dataLen, dataPtr->_dts)) != 0) {
                PrintConsole("send audio raw data failed. ret=%d", ret);
                callOnDisconnect();
                return;
            }
        } else if(dataPtr->_type == META_DATA) {
            int ret = srs_rtmp_write_packet(rtmp_, SRS_RTMP_TYPE_SCRIPT, dataPtr->_dts, (char*)dataPtr->_data, dataPtr->_dataLen);
            if (ret != 0) {
                PrintConsole("send metadata failed. ret=%d", ret);
            }
            return;
        }
        rtmp_bitrate_ontroller_->outputDataCount(dataPtr->_dataLen);
        // net_band_ += dataPtr->_dataLen;
        delete[] dataPtr->_data;
        delete dataPtr;
    }

    void ECRtmpPublisher::clearMediaCacher() {
        need_keyframe_ = true;
        retrys_ = 0;

        // clear lst_enc_data_ buffer.
        rtmp_lock_->Enter();
        std::list<EncodedData*>::iterator iter = lst_enc_data_.begin();
        while (iter != lst_enc_data_.end()) {
            EncodedData* ptr = *iter;
            lst_enc_data_.erase(iter++);
            delete[] ptr->_data;
            delete ptr;
        }
        rtmp_lock_->Leave();
    }

    void ECRtmpPublisher::callOnDisconnect()
    {
        need_keyframe_ = true;
        {
            if (rtmp_) {
                srs_rtmp_destroy(rtmp_);
                rtmp_ = NULL;
            }
            if(rtmp_status_ != RS_STM_Closed) {
                rtmp_status_ = RS_STM_Init;
                retrys_ ++;
                if(retrys_ <= MAX_RETRY_TIME)
                {
                    rtmp_ = srs_rtmp_create(rtmp_url.c_str());
                    if(callback_) {
                        callback_(EC_LIVE_CONNECTING);
                    }
                } else {
                    stop();
                    if(callback_) {
                        callback_(EC_LIVE_DISCONNECTED);
                    }
                }
            }
        }
    }
}





