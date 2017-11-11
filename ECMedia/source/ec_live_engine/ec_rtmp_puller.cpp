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
#include <cstdint>
#else
#include <arpa/inet.h>  // ntohl()
#include <stdlib.h>
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
        rtmp_status_ = RS_PLY_Init;
        rtmpPullingThread_ = ThreadWrapper::CreateThread(EC_RtmpPuller::pullingThreadRun,
                this,
                kNormalPriority,
                "rtmpPullingThread_");
        av_packet_cacher = NULL;
        hasStreaming_ = false;
        running_ = false;
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
            rtmpPullingThread_->SetNotAlive();
            delete rtmpPullingThread_;
        }
    }

    void EC_RtmpPuller::start(const char *url) {
        if(!running_) {
            running_ = true;
            rtmp_ = srs_rtmp_create(url);
            srs_rtmp_set_timeout(rtmp_, 3000, 3000);
            // start pull rtmp packer.
            unsigned int pthread_id;
            rtmpPullingThread_->Start(pthread_id);
            av_packet_cacher->run();
        }
    }
    
    void EC_RtmpPuller::stop() {
        if(running_) {
            running_ = false;
            srs_rtmp_disconnect_server(rtmp_);
            rtmpPullingThread_->Stop();
            srs_rtmp_destroy(rtmp_);
            rtmp_ = nullptr;
            av_packet_cacher->shutdown();
            hasStreaming_ = false;
            callbackStateIfNeed();
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
                        if(callback_){
                            callback_(EC_LIVE_CONNECTING);
                        }
                        if (srs_rtmp_handshake(rtmp_) == 0) {
                            PrintConsole("SRS: simple handshake ok.");
                            rtmp_status_ = RS_PLY_Handshaked;
                        }
                        else {
							rtmp_status_ = RS_PLY_Connect_Faild;
                            return false;
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
							rtmp_status_ = RS_PLY_Connect_Faild;
							return false;
                        }
                    }
                        break;
                    case RS_PLY_Connected:
                    {
                        if (srs_rtmp_play_stream(rtmp_) == 0) {
                            PrintConsole("SRS: play stream ok.");
                            rtmp_status_ = RS_PLY_Played;
                            if(callback_) {
                                callback_(EC_LIVE_CONNECT_SUCCESS);
                            }
                        }
                        else {
							rtmp_status_ = RS_PLY_Connect_Faild; 
                            return false;
                        }
                    }
                        break;
                    case RS_PLY_Played:
                    {
						int ret = doReadRtmpData();
                        if(ret == 0) {
                            if(!hasStreaming_) {
                                hasStreaming_ = true;
                                if(callback_) {
                                    callback_(EC_LIVE_PLAY_SUCCESS);
                                }
                            }
                        } else {
                            if(running_) {
								rtmp_status_ = RS_PLY_Read_Faild;
                            }
                            return false;
                        }
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
            callback_(EC_LIVE_CONNECT_SUCCESS);
        }
    }

    void EC_RtmpPuller::callbackStateIfNeed() {
		/*错误状态在子线程中callback,若客户在callback中操作UI，会照成程序崩溃
		将错误状态直接在stop函数中callback回去，可以避免此问题
		*/
		if (rtmp_status_ == RS_PLY_Connect_Faild) {
			if (callback_) {
				callback_(EC_LIVE_CONNECT_FAILED);
			}
		}

		if (rtmp_status_ == RS_PLY_Read_Faild) {
			if (callback_) {
				callback_(EC_LIVE_CONNECT_FAILED);
			}
		}

		if (callback_) {
			callback_(EC_LIVE_FINISHED);
		}
		rtmp_status_ = RS_PLY_Init;
    }
    

    bool EC_RtmpPuller::unPackNAL(const char *data, int data_size, std::vector<uint8_t> & nal)
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
    
    bool EC_RtmpPuller::unpackSpsPps(char *data , std::vector<uint8_t> &sps, std::vector<uint8_t> &pps)
    {
        //sps
        if(data[0]!= 1) {
            // PrintConsole("[RTMP ERROR] %s SPS PPS version not correct\n", __FUNCTION__);
            return false;
        }
        int index = 5;
        int sps_num = (unsigned char) ( data[index++] & 0x1F );
        for (int i = 0 ; i < sps_num ; i++ ){
            int sps_len = ntohs(  *(unsigned short*)( data+index) );
            index += 2;
            sps.insert(sps.end(), data+index,  data+index+sps_len);
            index += sps_len;
        }
        
        //pps
        int pps_num = (unsigned char) ( data[index++] );
        for (int i = 0 ; i < pps_num ; i++ ){
            int pps_len = ntohs(  *(unsigned short*)( data+index) );
            index += 2;
            pps.insert(pps.end(), data+index,  data+index+pps_len);
            index += pps_len;
        }
        return true;
    }
    
    int EC_RtmpPuller::doReadRtmpData() {
        int size;
        char type;
        char *data;
        u_int32_t timestamp;

 
        if (srs_rtmp_read_packet(rtmp_, &type, &timestamp, &data, &size) != 0) {
            return -1;
        }
        
        if (type == SRS_RTMP_TYPE_VIDEO) {
             handleVideoPacket(data, size, timestamp);
        } else if (type == SRS_RTMP_TYPE_AUDIO) {
            handleAuidoPacket(data, size, timestamp);
        } else if (type == SRS_RTMP_TYPE_SCRIPT) {
            if (!srs_rtmp_is_onMetaData(type, data, size)) {
                PrintConsole("drop message type=%#x, size=%dB", type, size);
            }
        }
        free(data);
     
        return 0;
    }

    void EC_RtmpPuller::handleVideoPacket(char* data, int len, u_int32_t timestamp) {
        unsigned frameType = ((unsigned char)data[0]) >> 4;
        unsigned codecId = data[0] &0xF;
        RTPHeader rtpHeader;
        unsigned char *payloadData;
        unsigned int payloadLen = 0;
        
        if(codecId == 7 ) {
            std::vector<uint8_t> sps;
            std::vector<uint8_t> pps;
            std::vector<uint8_t> nal;
            switch( data[1] ) {
                case 0:
                    unpackSpsPps(data+5, sps, pps);
                    payloadData = &sps[0];
                    payloadLen = sps.size();
                    if(av_packet_cacher){
                        av_packet_cacher->onAvcDataComing((uint8_t *) payloadData, payloadLen, timestamp);
                    }
                    payloadData = &pps[0];
                    payloadLen = pps.size();
                    if(av_packet_cacher){
                        av_packet_cacher->onAvcDataComing((uint8_t *) payloadData, payloadLen, timestamp);
                    }
                    break;
                case 1:
                    if (!unPackNAL(data + 5, len - 5, nal)) {
                        PrintConsole("[RTMP ERROR] %s unpack nalu error\n", __FUNCTION__);
                        return;
                    }
                    payloadData = &nal[0];
                    payloadLen = nal.size();
                    if(av_packet_cacher){
                        av_packet_cacher->onAvcDataComing((uint8_t *) payloadData, payloadLen, timestamp);
                    }
                    break;
                default:
                    PrintConsole("[RTMP ERROR] %s codec %d not supported\n", __FUNCTION__, data[1]);
                    return;
            }
            
       
        }
    }
    
    void EC_RtmpPuller::handleAuidoPacket(char* data, int length, u_int32_t timestamp)
    {
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
                PrintConsole("[RTMP ERROR] %s codec id %d not support\n", __FUNCTION__, voiceCodec);
                break;
        }
    }
}
