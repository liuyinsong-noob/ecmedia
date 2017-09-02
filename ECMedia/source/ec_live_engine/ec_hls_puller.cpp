//
//  ec_hls_puller.cpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/8/21.
//  Copyright © 2017年 ronglianyun.com. All rights reserved.
//

#include "ec_hls_puller.h"
using namespace std;

// project lib
#include "htl_core_log.hpp"
#include "htl_core_error.hpp"
#include "htl_main_utility.hpp"
#include "ec_play_buffer_cacher.h"

#define DefaultDelaySeconds -1
#define DefaultHttpUrl "http://127.0.0.1:3080/hls/hls.m3u8"
#define DefaultVod false

namespace cloopenwebrtc {

    EC_HLS_Puller::EC_HLS_Puller() {
        running_ = false;
        av_packet_cacher = nullptr;
        ts_parser_ = new EC_TS_Parser();
        ts_parser_->setCallback(this);
        hls_pulling_thread_ = ThreadWrapper::CreateThread(EC_HLS_Puller::pullingThreadRun,
                                                          this,
                                                          kNormalPriority,
                                                          "hls_pulling_thread_");
        
        hls_depacket_thread_ = ThreadWrapper::CreateThread(EC_HLS_Puller::depacketThreadRun,
                                                           this,
                                                           kNormalPriority,
                                                           "hls_depacket_thread_");

    }
    
    EC_HLS_Puller::~EC_HLS_Puller() {
        running_ = false;
        delete ts_parser_;
        if(av_packet_cacher) {
            delete av_packet_cacher;
        }
    }
    
    
    void EC_HLS_Puller::start(const char* url) {
        if(!running_) {
            running_ = true;
            str_url_ = url;
            clearTSlist();
            
            av_packet_cacher->run();
            unsigned int pull_thread_id = 0;
            hls_pulling_thread_->Start(pull_thread_id);
  
            unsigned int depacket_thread_id = 0;
            hls_depacket_thread_->Start(depacket_thread_id);
        }
    }
    
    void EC_HLS_Puller::stop() {
        if(running_) {
            running_ = false;
            hls_task_->StopProcess();
            
            av_packet_cacher->shutdown();
            hls_pulling_thread_->Stop();
        }
    }
    
    void EC_HLS_Puller::setReceiverCallback(EC_ReceiverCallback* callback) {
        if(!av_packet_cacher) {
            av_packet_cacher = new EC_AVCacher();
        }
        av_packet_cacher->setReceiverCallback(callback);
    }
    
    bool EC_HLS_Puller::pullingThreadRun(void* pThis) {
        return static_cast<EC_HLS_Puller*>(pThis)->run();
    }

    bool EC_HLS_Puller::run() {
        hls_task_ = new StHlsTask();
        int ret = -1;
        string url; bool vod = false;
        double start = 0, delay = -1, error = 1.0;
        int count = 0;
        
        if((ret = hls_task_->Initialize(str_url_.c_str(), vod, start, delay, error, count, this)) != ERROR_SUCCESS) {
            Error("initialize task failed, url=%s, ret=%d", str_url_.c_str(), ret);
            return ret;
        }
        
        ret = hls_task_->Process();
        
        if(ret != ERROR_SUCCESS) {
            Warn("st task terminate with ret=%d", ret);
        }
        else {
            Trace("st task terminate with ret=%d", ret);
        }
        delete hls_task_;
        hls_task_ = nullptr;
        return false;
    }
    
    bool EC_HLS_Puller::depacketThreadRun(void* pThis) {
        return static_cast<EC_HLS_Puller*>(pThis)->doDepacket();
    }
    
    bool EC_HLS_Puller::doDepacket() {
        while(running_) {
            if(list_ts_slices_.size() > 0) {
                TS_Video_Slices* slices = list_ts_slices_.front();
                ts_parser_->parse((u_int8_t*)slices->_data, slices->_data_len);
                list_ts_slices_.pop_front();
                delete slices;
            } else {
                usleep(20*1000);
            }
        }
        return false;
    }
    
    void EC_HLS_Puller::onTSDownloaded(const char* ts_packets, int length) {
        
        TS_Video_Slices *ts_slices = new TS_Video_Slices();
        ts_slices->SetData((const uint8_t*)ts_packets, length);
        list_ts_slices_.push_back(ts_slices);
    }
    
    // EC_TS_ParserCallback: avc data callback
    void EC_HLS_Puller::onGotAvcframe(const char* avc_data, int length, int64_t dts, int64_t pts) {
        if(av_packet_cacher) {
            av_packet_cacher->CacheH264Data((uint8_t*)avc_data, length, pts);
        }
    }
    
    // EC_TS_ParserCallback: aac data callback
    void EC_HLS_Puller::onGotAacframe(const char* aac_data, int length, int64_t dts, int64_t pts) {
        if(av_packet_cacher) {
            av_packet_cacher->CacheAacData((uint8_t*)aac_data, length, pts);
        }
    }   
    
    void EC_HLS_Puller::clearTSlist() {
        // clear audio cache buffer.
        std::list<TS_Video_Slices*>::iterator iter = list_ts_slices_.begin();
        while (iter != list_ts_slices_.end()) {
            TS_Video_Slices* ts_slice = *iter;
            list_ts_slices_.erase(iter++);
            delete ts_slice;
        }
        
        
    }
}
