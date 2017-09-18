//
//  ec_av_cacher.hpp
//  ECMedia
//
//  Created by gezhaoyou on 17/7/29.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_av_cacher_hpp
#define ec_av_cacher_hpp
#include "ec_live_common.h"
#include <list>

namespace cloopenwebrtc{
    enum PlyStuts {
        PS_Fast = 0,	//	Fast video decode
        PS_Normal,
        PS_Cache,
    };
    
    typedef struct PlyPacket
    {
        PlyPacket(bool isvideo) :_data(NULL), _data_len(0),
        _b_video(isvideo), _dts(0) {}
        
        virtual ~PlyPacket(void){
            if (_data)
                delete[] _data;
        }
        void SetData(const uint8_t*pdata, int len, uint32_t ts) {
            _dts = ts;
            if (len > 0 && pdata != NULL) {
                if (_data)
                    delete[] _data;
                if (_b_video)
                    _data = new uint8_t[len + 8];
                else
                    _data = new uint8_t[len];
                memcpy(_data, pdata, len);
                _data_len = len;
            }
        }
        uint8_t*_data;
        int _data_len;
        bool _b_video;
        uint32_t _dts;
    } PlyPacket;
    
    class EventWrapper;
    class EC_AVCacher{
    public:
        EC_AVCacher();
        ~EC_AVCacher();
        void CacheH264Data(const uint8_t*pdata, int len, uint32_t ts);
        void CachePcmData(const uint8_t*pdata, int len, uint32_t ts);
        void CacheAacData(const uint8_t*pdata, int len, uint32_t ts);
        void setReceiverCallback(EC_ReceiverCallback *cb);
        void run();
        void shutdown();

        static bool decodingThreadRun(void *pThis);
        static bool decodingAudioThreadRun(void *pThis);

     protected:
        bool DoDecode();
    private:
        bool handleAudio();
        void clearCacher();
    private:
        ThreadWrapper* playnetworkThread_;
        ThreadWrapper* audioHandleThread_;
 
        int						cache_time_;
        int						cache_delta_;
        int                     buf_cache_time_;
        PlyStuts				ply_status_;
        
        uint32_t				sys_fast_video_time_;	// √Îø™ ±º‰÷·
        uint32_t				rtmp_fast_video_time_;
        uint32_t				rtmp_cache_time_;
        uint32_t				play_cur_time_;
        
        std::list<PlyPacket*>	lst_audio_buffer_;
        std::list<PlyPacket*>	lst_video_buffer_;
        
        bool got_audio_;
        
        EC_ReceiverCallback * callback_;
        CriticalSectionWrapper* _cs_list_audio;
        CriticalSectionWrapper* _cs_list_video;
        EventWrapper* cacher_update_event_;
        bool running_;
        bool is_playing_;
        
        void *faac_decode_handle_;
        uint8_t			audio_cache_[8192];
        int				a_cache_len_;
        unsigned int aac_frame_per10ms_size_;

    };
}
#endif /* ec_av_cacher_hpp */