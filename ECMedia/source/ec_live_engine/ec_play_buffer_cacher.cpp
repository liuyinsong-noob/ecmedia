//
//  ec_av_cacher.cpp
//  ECMedia
//
//  Created by gezhaoyou on 17/7/29.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#include "ec_play_buffer_cacher.h"
#include "ec_live_utility.h"
#include "event_wrapper.h"
#include "faaccodec.h"

namespace cloopenwebrtc{
#define PLY_MIN_TIME	500		// 0.5s
#define PLY_MAX_TIME	600000	// 10minute
#define PLY_RED_TIME	250		// redundancy time
#define PLY_MAX_DELAY	1000	// 1 second
#define PLY_MAX_CACHE   16      // 16s
    
    EC_AVCacher::EC_AVCacher():
    cache_time_(1000)	// default 1000ms(1s)
    , cache_delta_(1)
    , buf_cache_time_(0)
    , ply_status_(PS_Fast)
    , sys_fast_video_time_(0)
    , rtmp_fast_video_time_(0)
    , rtmp_cache_time_(0)
    , play_cur_time_(0) {
        playnetworkThread_ = ThreadWrapper::CreateThread(EC_AVCacher::decodingThreadRun,
                                                         this,
                                                         kHighPriority,
                                                         "decodingThreadRun");
        
        audioHandleThread_ = ThreadWrapper::CreateThread(EC_AVCacher::decodingAudioThreadRun,
                                                      this,
                                                      kHighPriority,
                                                      "decodingAudioThreadRun");
        got_audio_ = false;
        
        _cs_list_audio = CriticalSectionWrapper::CreateCriticalSection();
        _cs_list_video = CriticalSectionWrapper::CreateCriticalSection();
        running_ = false;
        callback_ = NULL;
        cacher_update_event_ = EventWrapper::Create();
        is_playing_ = false;
        faac_decode_handle_ = nullptr;
        a_cache_len_ = 0;
        aac_frame_per10ms_size_ = 0;
    }
    
    EC_AVCacher::~EC_AVCacher() {
        if(playnetworkThread_) {
            delete playnetworkThread_;
        }
        if(audioHandleThread_) {
            delete audioHandleThread_;
        }
    }

    void EC_AVCacher::run() {
        running_ = true;
        clearCacher();
        cacher_update_event_->StartTimer(true, 10);

        unsigned int pthread_id;
        playnetworkThread_->Start(pthread_id);
        
        unsigned int audio_pthread_id;
        audioHandleThread_->Start(audio_pthread_id);
    }

    void EC_AVCacher::shutdown() {
        running_ = false;
        cacher_update_event_->Set();
        playnetworkThread_->Stop();
        audioHandleThread_->Stop();
    
        sys_fast_video_time_ = 0;
        rtmp_fast_video_time_ = 0;
        play_cur_time_ = 0;
    }

    void EC_AVCacher::setReceiverCallback(EC_ReceiverCallback *cb) {
        callback_ = cb;
    }

    void EC_AVCacher::CacheH264Data(const uint8_t*pdata, int len, uint32_t ts)
    {
        PlyPacket* pkt = new PlyPacket(true);
        pkt->SetData(pdata, len, ts);
        pkt->_b_video = true;
        if (sys_fast_video_time_ == 0)
        {
            sys_fast_video_time_ = EC_Live_Utility::time();
            rtmp_fast_video_time_ = ts;
        }
        CriticalSectionScoped cs(_cs_list_video);
        lst_video_buffer_.push_back(pkt);
    }

    void EC_AVCacher::CachePcmData(const uint8_t*pdata, int len, uint32_t ts)
    {
        if(len == 0 || pdata == nullptr) {
            return;
        }
        
        PlyPacket* pkt = new PlyPacket(false);
        pkt->SetData(pdata, len, ts);
        pkt->_b_video = false;
        CriticalSectionScoped cs(_cs_list_audio);
        lst_audio_buffer_.push_back(pkt);
        if (sys_fast_video_time_ == 0) {
            PlyPacket* pkt_front = lst_audio_buffer_.front();
            PlyPacket* pkt_back = lst_audio_buffer_.back();
            sys_fast_video_time_ = EC_Live_Utility::time();
            rtmp_fast_video_time_ = ts;
        }
    }
    
    void EC_AVCacher::CacheAacData(const uint8_t*pdata, int len, uint32_t ts) {
        unsigned int audio_sampleRate_ = 0;
        unsigned int audio_channels_ = 0 ;

        if (faac_decode_handle_ == NULL) {
            faad_decoder_getinfo((char*)pdata, audio_sampleRate_, audio_channels_);
            faac_decode_handle_ = faad_decoder_create(audio_sampleRate_, audio_channels_, 48000);
            faad_decoder_init(faac_decode_handle_, (unsigned char *)pdata, len, audio_sampleRate_, audio_channels_);
            aac_frame_per10ms_size_ = (audio_sampleRate_ / 100) * sizeof(int16_t) * audio_channels_;
        }
        else {
            unsigned int outlen = 0;
            if (faad_decode_frame(faac_decode_handle_, (unsigned char*)pdata, len, audio_cache_ + a_cache_len_, &outlen) == 0) {
                //printf("");
                a_cache_len_ += outlen;
                int ct = 0;
                int fsize = aac_frame_per10ms_size_;
                while (a_cache_len_ > fsize) {
                    CachePcmData(audio_cache_ + ct * fsize, fsize, ts);
                    a_cache_len_ -= fsize;
                    ct++;
                }
                memmove(audio_cache_, audio_cache_ + ct * fsize, a_cache_len_);
            }
        }
    }

    bool EC_AVCacher::decodingThreadRun(void *pThis) {
        return static_cast<EC_AVCacher*>(pThis)->DoDecode();
    }

    bool EC_AVCacher::DoDecode()
    {
        while(running_) {
            PlyPacket* pkt_video = nullptr;
            {//* Get video
                CriticalSectionScoped cs(_cs_list_video);
                if (lst_video_buffer_.size() > 0) {
                    pkt_video = lst_video_buffer_.front();
                    if (pkt_video->_dts <= play_cur_time_) {
                        lst_video_buffer_.pop_front();
                    }
                    else {
                        pkt_video = nullptr;
                    }
                }
            }
           
            if (pkt_video) {
                callback_->onAvcDataComing((uint8_t*)pkt_video->_data, pkt_video->_data_len, pkt_video->_dts);
                delete pkt_video;
            } else {
                 usleep(10*1000);
            }
        }

        return false;
    }

    
    bool EC_AVCacher::decodingAudioThreadRun(void *pThis) {
        return static_cast<EC_AVCacher*>(pThis)->handleAudio();
    }
    
    bool EC_AVCacher::handleAudio() {
        cacher_update_event_->Wait(10); //10ms
 
        PlyPacket* pkt_audio = nullptr;

        CriticalSectionScoped cs(_cs_list_audio);
        if (lst_audio_buffer_.size() > 0) {
            pkt_audio = lst_audio_buffer_.front();
            play_cur_time_ = pkt_audio->_dts;
            lst_audio_buffer_.pop_front();
        }
       
        if (pkt_audio) {
            callback_->onAacDataComing((uint8_t*)pkt_audio->_data, pkt_audio->_data_len, pkt_audio->_dts);
            delete  pkt_audio;
        }

        return true;
    }
    
    void EC_AVCacher::clearCacher() {
        // clear audio cache buffer.
        std::list<PlyPacket*>::iterator iter = lst_video_buffer_.begin();
        while (iter != lst_video_buffer_.end()) {
            PlyPacket* pkt = *iter;
            lst_video_buffer_.erase(iter++);
            delete pkt;
        }
        
        // clear video cache buffer
        iter = lst_audio_buffer_.begin();
        while (iter != lst_audio_buffer_.end()) {
            PlyPacket* pkt = *iter;
            lst_audio_buffer_.erase(iter++);
            delete pkt;
        }
        
        // clear aac cache
        memset(audio_cache_, 0, 8192);
        a_cache_len_ = 0;
    }
    
}
