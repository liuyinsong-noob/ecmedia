//
//  ECRTMPPlayer.cpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#include "ec_rtmp_puller.h"
#include "srs_librtmp.h"
#include "ec_play_buffer_cacher.h"

#if defined(_WIN32)
//#include <winsock2.h>
//#include <ws2tcpip.h>
#include <cstdint>
//#else
//#include <arpa/inet.h>  // ntohl()
#endif

namespace cloopenwebrtc {
    #ifndef _WIN32
    #define ERROR_SUCCESS   0
    #endif
    #define MAX_RETRY_TIME  3
 
    EC_RtmpPuller::EC_RtmpPuller(ECLiveStreamNetworkStatusCallBack callback) {
        retry_ct_ = 0;
        running_ = false;
        callback_ = callback;
        rtmp_status_ =RS_PLY_Init;
        rtmpPullingThread_ = ThreadWrapper::CreateThread(EC_RtmpPuller::pullingThreadRun,
                this,
                kHighPriority,
                "rtmpPullingThread_");
        av_packet_cacher = NULL;
    }
    
    EC_RtmpPuller::~EC_RtmpPuller() {
        if(running_) {
            stop();
            running_ = false;
        }
        if(av_packet_cacher) {
            delete av_packet_cacher;
        }

        if(rtmpPullingThread_) {
            delete rtmpPullingThread_;
        }
    }

    void EC_RtmpPuller::start(const char *url) {
        if(!running_) {
            running_ = true;
            
            rtmp_ = srs_rtmp_create(url);
            
            // start pull rtmp packer.
            unsigned int pthread_id;
            rtmpPullingThread_->Start(pthread_id);
            av_packet_cacher->run();
        }
    }
    
    void EC_RtmpPuller::stop() {
        if(running_) {
            running_ = false;
            rtmpPullingThread_->Stop();
            srs_rtmp_destroy(rtmp_);
            rtmp_ = nullptr;
            av_packet_cacher->shutdown();
            rtmp_status_ = RS_PLY_Init;
        }
    }

    void EC_RtmpPuller::setReceiverCallback(EC_ReceiverCallback *cb) {
        if(!av_packet_cacher) {
            av_packet_cacher = new EC_AVCacher();
        }
        av_packet_cacher->setReceiverCallback(cb);
    }

    bool EC_RtmpPuller::pullingThreadRun(void* pThis) {
        return static_cast<EC_RtmpPuller*>(pThis)->run();
    }

    bool EC_RtmpPuller::run() {
        while(running_) {
            if (rtmp_ != NULL)
            {
                switch (rtmp_status_) {
                    case RS_PLY_Init:
                    {
                        if (srs_rtmp_handshake(rtmp_) == 0) {
                            PrintConsole("SRS: simple handshake ok.");
                            rtmp_status_ = RS_PLY_Handshaked;
                        }
                        else {
                            CallDisconnect();
                        }
                    }
                        break;
                    case RS_PLY_Handshaked:
                    {
                        if (srs_rtmp_connect_app(rtmp_) == 0) {
                            PrintConsole("SRS: connect vhost/app ok.");
                            rtmp_status_ = RS_PLY_Connected;
                        }
                        else {
                            CallDisconnect();
                        }
                    }
                        break;
                    case RS_PLY_Connected:
                    {
                        if (srs_rtmp_play_stream(rtmp_) == 0) {
                            PrintConsole("SRS: play stream ok.");
                            rtmp_status_ = RS_PLY_Played;
                            CallConnect();
                        }
                        else {
                            CallDisconnect();
                        }
                    }
                        break;
                    case RS_PLY_Played:
                    {
                        doReadRtmpData();
                    }
                    break;
                }
            }
        }
        return false;
    }

    void EC_RtmpPuller::CallConnect() {
        retry_ct_ = 0;
        connected_ = true;
        if(callback_) {
            callback_(EC_LIVE_PLAY_SUCCESS);
        }
    }

    void EC_RtmpPuller::CallDisconnect() {
        if (rtmp_) {
            srs_rtmp_destroy(rtmp_);
            rtmp_ = NULL;
        }
        
        if(rtmp_status_ != RS_PLY_Closed) {
            rtmp_status_ = RS_PLY_Init;
            retry_ct_ ++;
            if(retry_ct_ <= MAX_RETRY_TIME)
            {
                rtmp_ = srs_rtmp_create(str_url_.c_str());
            } else {
                if(!callback_) {
                    running_ = false;
                    return;
                }
                
                if(connected_) {
                    running_ = false;
                    callback_(EC_LIVE_DISCONNECTED);
                }
                else {
                    running_ = false;
                    callback_(EC_LIVE_PLAY_FAILED);
                }
            }
        }
    }
    

    bool EC_RtmpPuller::UnPackNAL(const char *data, int data_size, std::vector<uint8_t> & nal)
    {
        int index =0 ;
        int count = 0;
        do {
            int nalu_size = ntohl( *(int*)(data+index));
            index += 4;
            if( index > 4 ) {
                nal.push_back(0);
                nal.push_back(0);
                nal.push_back(0);
                nal.push_back(1);
            }
            
            nal.insert(nal.end(), data+index, data+index+nalu_size);
            index += nalu_size;
        } while( index < data_size);
        
        if( index != data_size)
            return false;
        
        return true;
    }
    
    bool EC_RtmpPuller::UnpackSpsPps(const char *data , std::vector<uint8_t> & sps_pps)
    {
        if(data[0]!= 1) {
            // PrintConsole("[RTMP ERROR] %s SPS PPS version not correct\n", __FUNCTION__);
            return false;
        }
        int index = 5;
        int sps_num = (unsigned char) ( data[index++] & 0x1F );
        for (int i = 0 ; i < sps_num ; i++ ){
            int sps_len = ntohs(  *(unsigned short*)( data+index) );
            index += 2;
            sps_pps.insert(sps_pps.end(), data+index,  data+index+sps_len);
            index += sps_len;
        }
        
        sps_pps.push_back(0);
        sps_pps.push_back(0);
        sps_pps.push_back(0);
        sps_pps.push_back(1);
        int pps_num = (unsigned char) ( data[index++] );
        for (int i = 0 ; i < pps_num ; i++ ){
            int pps_len = ntohs(  *(unsigned short*)( data+index) );
            index += 2;
            sps_pps.insert(sps_pps.end(), data+index,  data+index+pps_len);
            index += pps_len;
        }
        return true;
    }
    
    void EC_RtmpPuller::doReadRtmpData() {
        int size;
        char type;
        char *data;
        u_int32_t timestamp;

 
        if (srs_rtmp_read_packet(rtmp_, &type, &timestamp, &data, &size) != 0) {
            CallDisconnect();
            return;
        }

        if (type == SRS_RTMP_TYPE_VIDEO) {
             handleVideoPacket(data, size, timestamp);
        } else if (type == SRS_RTMP_TYPE_AUDIO) {
            HandleAuidoPacket(data, size, timestamp);
        } else if (type == SRS_RTMP_TYPE_SCRIPT) {
            if (!srs_rtmp_is_onMetaData(type, data, size)) {
                // LOG(LS_ERROR) << "No flv";
                PrintConsole("drop message type=%#x, size=%dB", type, size);
            }
        }
        free(data);
    }

    void EC_RtmpPuller::handleVideoPacket(char* data, int len, u_int32_t timestamp) {
        PrintConsole("[EC_RtmpPuller INFO] %s begin\n", __FUNCTION__);
        unsigned frameType = ((unsigned char)data[0]) >> 4;
        unsigned codecId = data[0] &0xF;
        RTPHeader rtpHeader;
        unsigned char *payloadData;
        unsigned int payloadLen = 0;
        
        if(codecId == 7 ) {
            std::vector<uint8_t> sps_pps;
            std::vector<uint8_t> nal;
            switch( data[1] ) {
                case 0:
                    UnpackSpsPps(data+5, sps_pps);
                    payloadData = &sps_pps[0];
                    payloadLen = sps_pps.size();
                    break;
                case 1:
                    if (!UnPackNAL(data + 5, len - 5, nal)) {
                        PrintConsole("[RTMP ERROR] %s unpack nalu error\n", __FUNCTION__);
                        return;
                    }
                    payloadData = &nal[0];
                    payloadLen = nal.size();
                    break;
                default:
                    PrintConsole("[RTMP ERROR] %s codec %d not supported\n", __FUNCTION__, data[1]);
                    return;
            }
            
            if(av_packet_cacher){
                av_packet_cacher->onAvcDataComing((uint8_t *) payloadData, payloadLen, timestamp);
            }
        }
    }
    
    void EC_RtmpPuller::HandleAuidoPacket(char* data, int length, u_int32_t timestamp)
    {
        PrintConsole("[EC_RtmpPuller] %s begin, data: %p, length:%d, timestamp:%d\n", __FUNCTION__, data, length, timestamp);
        unsigned voiceCodec = ((unsigned char)data[0]) >> 4;
        switch (voiceCodec) {
            case 0:
                PrintConsole("[RTMP INFO] %s Linear PCM\n", __FUNCTION__);
                break;
            case 10: //AAC
                if(av_packet_cacher) {
                    av_packet_cacher->onAacDataComing((uint8_t *) data + 2, length - 2, timestamp);
                }
                break;
            default:
                PrintConsole("[RTMP ERROR] %s codec id %d not support\n", __FUNCTION__,voiceCodec);
                break;
        }
    }
}
