//
//  ec_hls_puller.hpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/8/21.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_hls_puller_hpp
#define ec_hls_puller_hpp

#include "ec_live_common.h"
#include "ec_media_puller_base.h"
#include "htl_app_hls_load.hpp"
#include "ec_ts_parser.h"

#include <list>

namespace cloopenwebrtc {
    
    typedef struct TS_Video_Slices
    {
        TS_Video_Slices() :_data(NULL), _data_len(0)
        {}
        
        virtual ~TS_Video_Slices(void){
            if (_data)
                delete[] _data;
        }
        
        void SetData(const uint8_t*pdata, int len) {
          
            if (len > 0 && pdata != NULL) {
                if (_data) {
                    delete[] _data;
                }
                
                _data = new uint8_t[len];
                memcpy(_data, pdata, len);
                _data_len = len;
            }
        }
        uint8_t*_data;
        int _data_len;
    } TS_Video_Slices;
    
    class EC_AVCacher;
    class EC_HLS_Puller: public EC_MediaPullerBase,
                         public StHlsTaskCallback ,
                         public EC_TS_ParserCallback {
        
    public:
        EC_HLS_Puller(EC_MediaPullCallback* callback);
        ~EC_HLS_Puller();
        void start(const char* url);
        void stop();
        void setReceiverCallback(EC_ReceiverCallback* callback);
    protected:
        // StHlsTaskCallback
        void onTSDownloaded(const char* ts_packet, int length);
        // EC_TS_ParserCallback: avc data callback
        void onGotAvcframe(const char* avc_data, int length, int64_t dts, int64_t pts);
        // EC_TS_ParserCallback: aac data callback
        void onGotAacframe(const char* aac_data, int length, int64_t dts, int64_t pts);
    private:
        static bool pullingThreadRun(void* pThis);
        static bool depacketThreadRun(void* pThis);
                             
        bool run();
        bool doDepacket();
        void clearTSlist();
    private:
        bool running_;
        StHlsTask* hls_task_;
        std::string str_url_;
        ThreadWrapper* hls_pulling_thread_;
        ThreadWrapper* hls_depacket_thread_;
        EC_TS_Parser *ts_parser_;
        EC_AVCacher* av_packet_cacher;

        std::list<TS_Video_Slices*>	list_ts_slices_;
        EC_MediaPullCallback *callback_;
    };
}

#endif /* ec_hls_puller_hpp */
