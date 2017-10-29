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
#if defined(_WIN32)
#include <cstdint>
#include <windows.h>
#else
#include <unistd.h>
#endif


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
        
        aacDecodeHandleThread_ = ThreadWrapper::CreateThread(EC_AVCacher::aacDecodingThreadRun,
                                                             this,
                                                             kHighPriority,
                                                             "aacDecodingThreadRun");

        
        _cs_list_audio = CriticalSectionWrapper::CreateCriticalSection();
        _cs_list_video = CriticalSectionWrapper::CreateCriticalSection();
        _cs_list_aac = CriticalSectionWrapper::CreateCriticalSection();
        cacher_update_event_ = EventTimerWrapper::Create();
        
        faac_decode_handle_ = nullptr;
        callback_ = nullptr;
        
        running_ = false;
        got_audio_ = false;
        is_playing_ = false;

        a_cache_len_ = 0;
        aac_frame_per10ms_size_ = 0;
        last_viedo_ts_delay_ = 0;
        audio_sampleRate_ = 0;
        audio_channels_ = 0;
    }
    
    EC_AVCacher::~EC_AVCacher() {
        if(playnetworkThread_) {
            delete playnetworkThread_;
        }
        if(audioHandleThread_) {
            delete audioHandleThread_;
        }
        if(aacDecodeHandleThread_) {
            delete aacDecodeHandleThread_;
        }
        if (faac_decode_handle_) {
            faad_decoder_close(faac_decode_handle_);
            faac_decode_handle_ = nullptr;
        }
        
    }
    
    bool EC_AVCacher::decodingThreadRun(void *pThis) {
        return static_cast<EC_AVCacher *>(pThis)->handleVideo();
    }
    
    bool EC_AVCacher::decodingAudioThreadRun(void *pThis) {
        return static_cast<EC_AVCacher*>(pThis)->handleAudio();
    }
    
    bool EC_AVCacher::aacDecodingThreadRun(void *pThis) {
        return static_cast<EC_AVCacher*>(pThis)->aacDecode();
    }
    
    void EC_AVCacher::run() {
        PrintConsole("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        running_ = true;
        clearCacher();
        cacher_update_event_->StartTimer(true, 10);
        
        unsigned int pthread_id;
        playnetworkThread_->Start(pthread_id);
        
        unsigned int audio_pthread_id;
        audioHandleThread_->Start(audio_pthread_id);
        
        unsigned int aac_decoding_pthread_id;
        aacDecodeHandleThread_->Start(aac_decoding_pthread_id);
    }
    
    void EC_AVCacher::shutdown() {
        PrintConsole("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        running_ = false;
        cacher_update_event_->Set();
        playnetworkThread_->Stop();
        audioHandleThread_->Stop();
        aacDecodeHandleThread_->Stop();
        
        play_cur_time_ = 0;
    }
    
    void EC_AVCacher::setReceiverCallback(EC_ReceiverCallback *cb) {
        PrintConsole("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        callback_ = cb;
    }
    
    // cache avc data
    void EC_AVCacher::onAvcDataComing(const uint8_t *pdata, int len, uint32_t ts)
    {
        if(!running_) {
            return;
        }
        PrintConsole("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        PlyPacket* pkt = new PlyPacket(true);
        pkt->SetData(pdata, len, ts);
        pkt->_b_video = true;
        _cs_list_video->Enter();
        lst_video_buffer_.push_back(pkt);
        _cs_list_video->Leave();
        PrintConsole("EC_AVCacher INFO] %s end\n", __FUNCTION__);
    }
    
    void EC_AVCacher::CachePcmData(const uint8_t*pdata, int len, uint32_t ts)
    {
        
        PrintConsole("[EC_AVCacher INFO] %s start\n", __FUNCTION__);
        int audio_record_sample_hz_ = 32000;
        const size_t kMaxDataSizeSamples = 3840;
        
        int16_t temp_output[kMaxDataSizeSamples];
        
        int length = resampler_record_.Resample10Msec((int16_t*)pdata, audio_sampleRate_, audio_record_sample_hz_, audio_channels_,  kMaxDataSizeSamples, (int16_t*)temp_output);
        
        if (len < 0) {
            PrintConsole("[ECMEDIA CORE ERROR] %s resample error\n", __FUNCTION__);
            return;
        }
#ifdef __ANDROID__
        EC_Live_Utility::pcm_s16le_to_s16be((short*)temp_output, audio_record_sample_hz_/100*2);
#endif
        
        
        PrintConsole("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        if(len == 0 || pdata == nullptr) {
            return;
        }
        
        PlyPacket* pkt = new PlyPacket(false);
        pkt->SetData((uint8_t*)temp_output, audio_record_sample_hz_/100*audio_channels_*2, ts);
        pkt->_b_video = false;
        _cs_list_audio->Enter();
        lst_audio_buffer_.push_back(pkt);
        _cs_list_audio->Leave();
    }
    
    
    
    bool EC_AVCacher::handleVideo()
    {
        while(running_) {
            PlyPacket* pkt_video = nullptr;
            {//* Get video
                _cs_list_video->Enter();
                if (lst_video_buffer_.size() > 0) {
                    PrintConsole("[ECMEDIA CORE INFO] %s lst_video_buffer_ size :%d\n", __FUNCTION__, lst_video_buffer_.size());
                    pkt_video = lst_video_buffer_.front();
                    int diff = pkt_video->_dts - play_cur_time_;
                    if (diff <= 0) {
                        lst_video_buffer_.pop_front();
                    } else {
                        pkt_video = nullptr;
                    }
                }
                _cs_list_video->Leave();
            }
            
            if (pkt_video) {
                callback_->onAvcDataComing((uint8_t*)pkt_video->_data, pkt_video->_data_len, pkt_video->_dts);
                delete pkt_video;
            } else {
#if defined(_WIN32)
                Sleep(5);
#else
                usleep(10 * 1000);
#endif
            }
        }
        
        return false;
    }
    
    // decode aac data and cache pcm data
    void EC_AVCacher::onAacDataComing(const uint8_t *pdata, int len, uint32_t ts) {
        if(!running_) {
            return;
        }
        
        PrintConsole("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        PlyPacket* pkt = new PlyPacket(true);
        pkt->SetData(pdata, len, ts);
        pkt->_b_video = true;
        _cs_list_aac->Enter();
        lst_aac_buffer_.push_back(pkt);
        _cs_list_aac->Leave();
        PrintConsole("EC_AVCacher INFO] %s end\n", __FUNCTION__);
    }
    
    bool EC_AVCacher::aacDecode() {
        PlyPacket* pkt_aac = nullptr;
        _cs_list_aac->Enter();
        if (lst_aac_buffer_.size() > 0) {
            pkt_aac = lst_aac_buffer_.front();
            lst_aac_buffer_.pop_front();
        }
        _cs_list_aac->Leave();
        
        if (pkt_aac)	 {
            uint8_t *pdata = pkt_aac->_data;
            int len = pkt_aac->_data_len;
            uint32_t ts = pkt_aac->_dts;
            PrintConsole("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
            if (faac_decode_handle_ == NULL) {
                PrintConsole("[EC_AVCacher INFO] %s create new faac decode handler\n", __FUNCTION__);
                faad_decoder_getinfo((char*)pdata, audio_sampleRate_, audio_channels_);
                faac_decode_handle_ = faad_decoder_create(audio_sampleRate_, audio_channels_, 48000);
                faad_decoder_init(faac_decode_handle_, (unsigned char *)pdata, len, audio_sampleRate_, audio_channels_);
                aac_frame_per10ms_size_ = (audio_sampleRate_ / 100) * sizeof(int16_t) * audio_channels_;
            }
            
            // 1B aac packet type, 4B AAC Specific, zhaoyou
            // @see http://billhoo.blog.51cto.com/2337751/1557646
            if(len < 5) {
                return true;
            }
            unsigned int outlen = 0;
            if (faad_decode_frame(faac_decode_handle_, (unsigned char*)pdata, len, audio_cache_ + a_cache_len_, &outlen) == 0) {
                PrintConsole("[EC_AVCacher INFO] %s faac decode success, data: %p, length:%d, a_cache_len_:%d\n", __FUNCTION__, pdata, len, a_cache_len_);
                a_cache_len_ += outlen;
                int ct = 0;
                int fsize = aac_frame_per10ms_size_;
                while (a_cache_len_ > fsize) {
                    CachePcmData(audio_cache_ + ct * fsize, fsize, ts);
                    a_cache_len_ -= fsize;
                    ct++;
                }
                
                memmove(audio_cache_, audio_cache_ + ct * fsize, a_cache_len_);
            } else{
                PrintConsole("[EC_AVCacher ERROR] %s faac decode failed, data: %p, length:%d, a_cache_len_:%d\n", __FUNCTION__, pdata, len, a_cache_len_);
            }
            
            delete  pkt_aac;
        } else {
            usleep(5*1000);
        }

        return true;
    }
    
    bool EC_AVCacher::handleAudio() {
        cacher_update_event_->Wait(10); //10ms
        
        PlyPacket* pkt_audio = nullptr;
        _cs_list_audio->Enter();
        if (lst_audio_buffer_.size() > 0) {
            PrintConsole("EC_AVCacher INFO] %s , lst_audio_buffer_ size:%d\n", __FUNCTION__, lst_audio_buffer_.size());
            pkt_audio = lst_audio_buffer_.front();
            play_cur_time_ = pkt_audio->_dts;
            lst_audio_buffer_.pop_front();
        }
        _cs_list_audio->Leave();
        
        if (pkt_audio) {
            callback_->onAacDataComing((uint8_t*)pkt_audio->_data, pkt_audio->_data_len, pkt_audio->_dts, audio_sampleRate_, audio_channels_);
            delete  pkt_audio;
        } else {
            usleep(10*1000);
        }
        
        return true;
    }
    
    void EC_AVCacher::clearCacher() {
        // clear avc cache buffer.
        std::list<PlyPacket *>::iterator iter = lst_video_buffer_.begin();
        while (iter != lst_video_buffer_.end()) {
            PlyPacket *pkt = *iter;
            lst_video_buffer_.erase(iter++);
            delete pkt;
        }
        
        // clear pcm cache buffer
        iter = lst_audio_buffer_.begin();
        while (iter != lst_audio_buffer_.end()) {
            PlyPacket *pkt = *iter;
            lst_audio_buffer_.erase(iter++);
            delete pkt;
        }
        
        // clear aac cache buffer
        iter = lst_aac_buffer_.begin();
        while (iter != lst_aac_buffer_.end()) {
            PlyPacket *pkt = *iter;
            lst_aac_buffer_.erase(iter++);
            delete pkt;
        }

        // clear aac cache
        memset(audio_cache_, 0, 8192);
        a_cache_len_ = 0;
    }
}
