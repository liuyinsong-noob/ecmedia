//
//  ec_av_cacher.cpp
//  ECMedia
//
//  Created by gezhaoyou on 17/7/29.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#include "ec_play_buffer_cacher.h"
#include "ec_live_utility.h"

#if defined(_WIN32)
#include <cstdint>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "ec_live_utility.h"

namespace yuntongxunwebrtc{
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
    , play_cur_time_(0)
    , base_time_video_ (0)
    , aac_decoder_(nullptr)
    {
        playnetworkThread_ = ThreadWrapper::CreateThread(EC_AVCacher::decodingThreadRun,
                                                         this,
                                                         kNormalPriority,
                                                         "playnetworkThread_");

        
        _cs_list_audio = CriticalSectionWrapper::CreateCriticalSection();
        _cs_list_video = CriticalSectionWrapper::CreateCriticalSection();
        _cs_list_aac = CriticalSectionWrapper::CreateCriticalSection();
        callback_ = nullptr;
        
        running_ = false;
        got_audio_ = false;
        is_playing_ = false;

        a_cache_len_ = 0;
        audio_sampleRate_ = 0;
        audio_channels_ = 0;
        sys_fast_video_time_ = 0;
        rtmp_fast_video_time_ = 0;
    }
    
    EC_AVCacher::~EC_AVCacher() {
        if(playnetworkThread_) {
            delete playnetworkThread_;
        }
        
        if (aac_decoder_) {
            aac_decoder_close(aac_decoder_);
            aac_decoder_ = NULL;
        }
        
    }
    
    bool EC_AVCacher::decodingThreadRun(void *pThis) {
        return static_cast<EC_AVCacher *>(pThis)->handleVideo();
    }
    
    void EC_AVCacher::run() {
        WriteLogToFile("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        running_ = true;
        clearCacher();

        unsigned int pthread_id;
        playnetworkThread_->Start(pthread_id);
        
    }
    
    void EC_AVCacher::shutdown() {
        WriteLogToFile("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        running_ = false;
       
        playnetworkThread_->Stop();
        play_cur_time_ = 0;
    }
    
    void EC_AVCacher::setReceiverCallback(EC_ReceiverCallback *cb) {
        WriteLogToFile("[EC_AVCacher INFO] %s begin\n", __FUNCTION__);
        callback_ = cb;
    }
    
    // cache avc data
    void EC_AVCacher::onAvcDataComing(const uint8_t *pdata, int len, uint32_t ts)
    {
        if(!running_) {
            return;
        }
        PlyPacket* pkt = new PlyPacket(true);
        pkt->SetData(pdata, len, ts);
        pkt->_b_video = true;
        if (sys_fast_video_time_ == 0)
        {
            sys_fast_video_time_ = EC_Live_Utility::Time();
            rtmp_fast_video_time_ = ts;
        }
        _cs_list_video->Enter();
        lst_video_buffer_.push_back(pkt);
        _cs_list_video->Leave();
    }
    
    void EC_AVCacher::cache10MsecPcmData(const uint8_t*pdata, int len, uint32_t ts)
    {
        if(len == 0 || pdata == nullptr) {
            return;
        }
        PlyPacket* pkt = new PlyPacket(false);
        pkt->SetData(pdata, len, ts);
        pkt->_b_video = false;
        _cs_list_audio->Enter();
        lst_audio_buffer_.push_back(pkt);

        if (sys_fast_video_time_ == 0) {
            PlyPacket* pkt_front = lst_audio_buffer_.front();
            PlyPacket* pkt_back = lst_audio_buffer_.back();
            int diff = (pkt_back->_dts - pkt_front->_dts);
            if (diff >= PLY_MAX_DELAY) {
                sys_fast_video_time_ = EC_Live_Utility::Time();
                rtmp_fast_video_time_ = ts;
            }
        }
        _cs_list_audio->Leave();
    }
    
    bool EC_AVCacher::handleVideo()
    {
#if defined(_WIN32)
        Sleep(10);
#else
		usleep(10*1000);
#endif // 

        uint32_t curTime = EC_Live_Utility::Time();
        if (sys_fast_video_time_ == 0)
            return true;

        if (ply_status_ == PS_Fast) {
            PlyPacket* pkt = NULL;
            uint32_t videoSysGap = curTime - sys_fast_video_time_;
            uint32_t videoPlyTime = rtmp_fast_video_time_ + videoSysGap;
            
            if (videoSysGap >= PLY_RED_TIME) {
                //* Start play a/v
                _cs_list_audio->Enter();
                if (lst_audio_buffer_.size() > 0) {
                    PlyPacket* pkt_front = lst_audio_buffer_.front();
                    PlyPacket* pkt_back = lst_audio_buffer_.back();
                    if ((pkt_back->_dts - pkt_front->_dts) > PLY_RED_TIME) {
                        ply_status_ = PS_Normal;
                        play_cur_time_ = pkt_front->_dts;
                        is_playing_ = true;
                    }
                } else {
                    if (videoSysGap >= PLY_RED_TIME * 4)
                    {
                        _cs_list_video->Enter();
                        if (lst_video_buffer_.size() > 0) {
                            PlyPacket* pkt_front = lst_video_buffer_.front();
                            ply_status_ = PS_Normal;
                            play_cur_time_ = pkt_front->_dts;
                            is_playing_ = true;
                        }
                        _cs_list_video->Leave();
                    }
                }
                _cs_list_audio->Leave();
            }
        } else if (ply_status_ == PS_Normal) {
            PlyPacket* pkt_video = NULL;
            uint32_t media_buf_time = 0;
            uint32_t play_video_time = play_cur_time_;
            {//* Get audio
                _cs_list_audio->Enter();
                if (lst_audio_buffer_.size() > 0) {
                    media_buf_time = lst_audio_buffer_.back()->_dts - lst_audio_buffer_.front()->_dts;
                }
                _cs_list_audio->Leave();
            }
            
            if (media_buf_time == 0 && !got_audio_) {
                _cs_list_video->Enter();
                if (lst_video_buffer_.size() > 0) {
                    media_buf_time = lst_video_buffer_.back()->_dts - lst_video_buffer_.front()->_dts;
                    uint32_t videoSysGap = curTime - sys_fast_video_time_;
                    play_video_time = rtmp_fast_video_time_ + videoSysGap;
                }
                _cs_list_video->Leave();
            }
            
            {//* Get video
                _cs_list_video->Enter();
                if (lst_video_buffer_.size() > 0) {
                    pkt_video = lst_video_buffer_.front();
                    if (pkt_video->_dts <= play_video_time) {
                        lst_video_buffer_.pop_front();
                    }
                    else {
                        pkt_video = NULL;
                    }
                }
                _cs_list_video->Leave();
            }
            if (pkt_video) {
                if(callback_) {
                    callback_->onAvcDataComing((uint8_t*)pkt_video->_data, pkt_video->_data_len, pkt_video->_dts);
                }
                delete pkt_video;
            }
            
            if (media_buf_time <= PLY_RED_TIME) {
                // Play buffer is so small, then we need buffer it?
                is_playing_ = false;
                ply_status_ = PS_Cache;
                cache_time_ = cache_delta_ * 1000;
                if(cache_delta_ < PLY_MAX_CACHE)
                    cache_delta_ *= 2;

                rtmp_cache_time_ = EC_Live_Utility::Time() + cache_time_;
            }
            buf_cache_time_ = media_buf_time;
        } else if (ply_status_ == PS_Cache) {
            if (rtmp_cache_time_ <= EC_Live_Utility::Time()) {
                uint32_t media_buf_time = 0;
                {
                    _cs_list_audio->Enter();
                    if (lst_audio_buffer_.size() > 0) {
                        media_buf_time = lst_audio_buffer_.back()->_dts - lst_audio_buffer_.front()->_dts;

                    }
                    _cs_list_audio->Leave();
                }
                if (media_buf_time == 0 && !got_audio_) {
                    _cs_list_video->Enter();
                    if (lst_video_buffer_.size() > 0) {
                        media_buf_time = lst_video_buffer_.back()->_dts - lst_video_buffer_.front()->_dts;
                    }
                    _cs_list_video->Leave();
                }
                
                if (media_buf_time >= cache_time_ - PLY_RED_TIME) {
                    ply_status_ = PS_Normal;

                    if (cache_delta_ == PLY_MAX_CACHE)
                        cache_delta_ /= 2;
                    is_playing_ = true;
                }
                else {
                    rtmp_cache_time_ = EC_Live_Utility::Time() + cache_time_;
                }
                
                if (!got_audio_) {
                    sys_fast_video_time_ += cache_time_;
                }
                buf_cache_time_ = media_buf_time;
            }
        }
        
        return true;

    }

    // decode aac data and cache pcm data
    void EC_AVCacher::onAacDataComing(const uint8_t *pdata, int len, uint32_t ts) {
        if(!running_) {
            return;
        }
        got_audio_ = true;
        unsigned int outlen = 0;
        if (aac_decoder_ == NULL) {
            WriteLogToFile("[EC_AVCacher INFO] %s create new faac decode handler\n", __FUNCTION__);
            aac_decoder_ = aac_decoder_open((unsigned char*)pdata, len, &audio_channels_, &audio_sampleRate_);
            if (audio_channels_ == 0)
                audio_channels_ = 1;
            aac_frame_per10ms_size_ = (audio_sampleRate_ / 100) * sizeof(int16_t) * audio_channels_;
        } else {
            if (aac_decoder_decode_frame(aac_decoder_, (unsigned char*)pdata, len, audio_cache_ + a_cache_len_, &outlen) > 0) {
                a_cache_len_ += outlen;
                int ct = 0;
                int fsize = aac_frame_per10ms_size_;
                while (a_cache_len_ > fsize) {
                    cache10MsecPcmData(audio_cache_ + ct * fsize, fsize, ts);
                    a_cache_len_ -= fsize;
                    ct++;
                }
                memmove(audio_cache_, audio_cache_ + ct * fsize, a_cache_len_);
            } else{
                WriteLogToFile("[EC_AVCacher ERROR] %s faac decode failed, data: %p, length:%d, a_cache_len_:%d\n", __FUNCTION__, pdata, len, a_cache_len_);
            }
        }
    }
    
    int32_t EC_AVCacher::RecordedDataIsAvailable(const void* audioSamples,
                                                    const uint32_t nSamples,
                                                    const uint8_t nBytesPerSample,
                                                    const uint8_t nChannels,
                                                    const uint32_t samplesPerSec,
                                                    const uint32_t totalDelayMS,
                                                    const int32_t clockDrift,
                                                    const uint32_t currentMicLevel,
                                                    const bool keyPressed,
                                                    uint32_t& newMicLevel) {
        return 0;
    }
    
    int32_t EC_AVCacher::NeedMorePlayData(const uint32_t nSamples,
                                             const uint8_t nBytesPerSample,
                                             const uint8_t nChannels,
                                             const uint32_t samplesPerSec,
                                             void* audioSamples,
                                             uint32_t& nSamplesOut,
                                             int64_t* elapsed_time_ms,
                                             int64_t* ntp_time_ms) {
        if(!is_playing_) {
            int samples_per_channel_int = samplesPerSec / 100;
            if (samples_per_channel_int > 0) {
                memset(audioSamples, 0, samples_per_channel_int * sizeof(int16_t) * nChannels);
                nSamplesOut = samples_per_channel_int;
            }
            return 0;
        }
        
        PlyPacket* pkt_audio = nullptr;
        _cs_list_audio->Enter();
        if (lst_audio_buffer_.size() > 0) {
            WriteLogToFile("EC_AVCacher INFO] %s , lst_audio_buffer_ size:%d\n", __FUNCTION__, lst_audio_buffer_.size());
            pkt_audio = lst_audio_buffer_.front();
            play_cur_time_ = pkt_audio->_dts;
            
            
            PlyPacket* pkt_front = lst_audio_buffer_.front();
            PlyPacket* pkt_back = lst_audio_buffer_.back();
            int diff = (pkt_back->_dts - pkt_front->_dts);
            lst_audio_buffer_.pop_front();
        }
        _cs_list_audio->Leave();
        
        if (pkt_audio) {
            *elapsed_time_ms = 0;
            *ntp_time_ms = 0;
            
            const size_t kMaxDataSizeSamples = 3840;
            int16_t temp_output[kMaxDataSizeSamples];
            
            int samples_out = resampler_record_.Resample10Msec((int16_t*)pkt_audio->_data, audio_sampleRate_*audio_channels_, samplesPerSec*nChannels, 1,  kMaxDataSizeSamples, (int16_t*)temp_output);
            

            memcpy(audioSamples, (uint8_t*)temp_output, samples_out*sizeof(uint16_t));
            nSamplesOut = samples_out;
            delete  pkt_audio;
        } else {
            int samples_per_channel_int = samplesPerSec / 100;
            if (samples_per_channel_int > 0) {
                memset(audioSamples, 0, samples_per_channel_int * sizeof(int16_t) * nChannels);
                nSamplesOut = samples_per_channel_int;
            }
        }
        
        return 0;
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
