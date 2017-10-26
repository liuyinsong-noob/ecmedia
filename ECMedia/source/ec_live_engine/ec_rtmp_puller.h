//
//  ECRTMPPlayer.hpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ECRTMPPlayer_hpp
#define ECRTMPPlayer_hpp

#include "ec_live_common.h"
#include "srs-librtmp/srs_kernel_codec.h"
#include "ec_media_puller_base.h"

namespace cloopenwebrtc {
    typedef struct DemuxData
    {
        DemuxData(int size) : _data(NULL), _data_len(0), _data_size(size){
            _data = new char[_data_size];
        }
        virtual ~DemuxData(void){ delete[] _data; }
        void reset() {
            _data_len = 0;
        }
        int append(const char* pData, int len){
            if (_data_len + len > _data_size)
                return 0;
            memcpy(_data + _data_len, pData, len);
            _data_len += len;
            return len;
        }

        char*_data;
        int _data_len;
        int _data_size;
    } DemuxData;

    enum RTMPLAYER_STATUS
    {
        RS_PLY_Init,
        RS_PLY_Handshaked,
        RS_PLY_Connected,
        RS_PLY_Played,
        RS_PLY_Closed
    };

    class EC_AVCacher;
    
    class EC_RtmpPuller: public EC_MediaPullerBase {
    public:
        EC_RtmpPuller(ECLiveStreamNetworkStatusCallBack callback);
        ~EC_RtmpPuller();

        void start(const char *url);
        void stop();

        void setReceiverCallback(EC_ReceiverCallback* callback);
        static bool pullingThreadRun(void* pThis);

    private:
        bool run();
        void CallConnect();
        void CallDisconnect();
        void doReadRtmpData();

        int GotVideoSample(u_int32_t timestamp, SrsCodecSample *sample);
        int GotAudioSample(u_int32_t timestamp, SrsCodecSample *sample);
        void RescanVideoframe(const char*pdata, int len, uint32_t timestamp);

        
        bool  UnPackNAL(const char *data, int data_size, std::vector<uint8_t> & nal);
        
        bool  UnpackSpsPps(char *data , std::vector<uint8_t> &sps, std::vector<uint8_t> &pps);
        void handleVideoPacket(char* data, int len, u_int32_t timestamp);
        void HandleAuidoPacket(char * data, int length, u_int32_t timestamp);
    private:
        std::string str_url_;
        ThreadWrapper* rtmpPullingThread_;
        bool running_;
        void* rtmp_;

        RTMPLAYER_STATUS rtmp_status_;
        EC_AVCacher* av_packet_cacher;
        ECLiveStreamNetworkStatusCallBack callback_;
        
        bool connected_;
        int  retry_ct_;

//        SrsAvcAacCodec*		srs_codec_;
//        DemuxData*			audio_payload_;
//        DemuxData*			video_payload_;
    };
}

#endif
