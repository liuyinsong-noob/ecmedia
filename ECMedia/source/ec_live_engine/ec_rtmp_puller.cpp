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

namespace yuntongxunwebrtc {
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
        av_packet_cacher    = NULL;
        hasStreaming_       = false;
        running_            = false;
        srs_codec_          = new SrsAvcAacCodec();
        audio_payload_      = new DemuxData(1024);
        av_packet_cacher    = new EC_AVCacher();
    }
    
    EC_RtmpPuller::~EC_RtmpPuller() {
        if(running_) {
            stop();
            running_ = false;
        }
        if(av_packet_cacher) {
            delete av_packet_cacher;
        }
        
        if (srs_codec_) {
            delete srs_codec_;
            srs_codec_ = NULL;
        }
        
        if (audio_payload_) {
            delete audio_payload_;
            audio_payload_ = NULL;
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
            av_packet_cacher->shutdown();
            rtmpPullingThread_->Stop();
            srs_rtmp_destroy(rtmp_);
            rtmp_ = nullptr;
            hasStreaming_ = false;
        }
    }
    
    

    void EC_RtmpPuller::setReceiverCallback(EC_ReceiverCallback *cb) {
        if(!av_packet_cacher) {
            av_packet_cacher = new EC_AVCacher();
        }
        av_packet_cacher->setReceiverCallback(cb);
    }
    
    EC_AVCacher * EC_RtmpPuller::getMediaPacketBuffer() {
        return av_packet_cacher;
    }

    bool EC_RtmpPuller::pullingThreadRun(void* pThis) {
        return static_cast<EC_RtmpPuller*>(pThis)->run();
    }

    bool EC_RtmpPuller::run() {
        while(running_) {
            usleep(10*1000);
            if (rtmp_ != NULL && running_)
            {
                switch (rtmp_status_) {
                    case RS_PLY_Init:
                    {
                        if(callback_){
                            callback_(EC_LIVE_CONNECTING);
                        }
                        if (srs_rtmp_handshake(rtmp_) == 0) {
                            WriteLogToFile("SRS: simple handshake ok.");
                            rtmp_status_ = RS_PLY_Handshaked;
                        }
                        else {
							rtmp_status_ = RS_PLY_Connect_Faild;
							if (callback_) {
								callback_(EC_LIVE_CONNECT_FAILED);
							}
                            return false;
                        }
                    }
                        break;
                    case RS_PLY_Handshaked:
                    {
                        if (srs_rtmp_connect_app(rtmp_) == 0) {
                            WriteLogToFile("SRS: connect vhost/app ok.");
                            rtmp_status_ = RS_PLY_Connected;
                        }
                        else {
							rtmp_status_ = RS_PLY_Connect_Faild;
							if (callback_) {
								callback_(EC_LIVE_CONNECT_FAILED);
							}
							return false;
                        }
                    }
                        break;
                    case RS_PLY_Connected:
                    {
                        if (srs_rtmp_play_stream(rtmp_) == 0) {
                            WriteLogToFile("SRS: play stream ok.");
                            rtmp_status_ = RS_PLY_Played;
                            if(callback_) {
                                callback_(EC_LIVE_CONNECT_SUCCESS);
                            }
                        }
                        else {
							rtmp_status_ = RS_PLY_Connect_Faild; 
							if (callback_) {
								callback_(EC_LIVE_CONNECT_FAILED);
							}
                            return false;
                        }
                    }
                        break;
                    case RS_PLY_Played:
                    {
						int ret = doReadRtmpData();
                        // todo: 根据 ret 返回值，判断错误类型，决定是否停止读取流
                        // bugfix: #128169
                        // @see http://redmine.yuntongxun.com/redmine/issues/128169?issue_count=82&issue_position=1&next_issue_id=131511
                        if(ret == 0) {
                            if(!hasStreaming_) {
                                hasStreaming_ = true;
                                if(callback_) {
                                    callback_(EC_LIVE_PLAY_SUCCESS);
                                }
                            }
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
                WriteLogToFile("drop message type=%#x, size=%dB", type, size);
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
                        WriteLogToFile("[RTMP ERROR] %s unpack nalu error\n", __FUNCTION__);
                        return;
                    }
                    payloadData = &nal[0];
                    payloadLen = nal.size();
                    if(av_packet_cacher){
                        av_packet_cacher->onAvcDataComing((uint8_t *) payloadData, payloadLen, timestamp);
                    }
                    break;
                default:
                    WriteLogToFile("[RTMP ERROR] %s codec %d not supported\n", __FUNCTION__, data[1]);
                    return;
            }
            
       
        }
    }
    
    void EC_RtmpPuller::handleAuidoPacket(char* data, int size, u_int32_t timestamp)
    {
        SrsCodecSample sample;
        if (srs_codec_->audio_aac_demux(data, size, &sample) != ERROR_SUCCESS) {
            if (sample.acodec == SrsCodecAudioMP3 && srs_codec_->audio_mp3_demux(data, size, &sample) != ERROR_SUCCESS) {
                return;
            }
            return;    // Just support AAC.
        }
        SrsCodecAudio acodec = (SrsCodecAudio)srs_codec_->audio_codec_id;
        
        // ts support audio codec: aac/mp3
        if (acodec != SrsCodecAudioAAC && acodec != SrsCodecAudioMP3) {
            return;
        }
        // for aac: ignore sequence header
        if ((acodec == SrsCodecAudioAAC && sample.aac_packet_type == SrsCodecAudioTypeSequenceHeader)
            || srs_codec_->aac_object == SrsAacObjectTypeReserved) {
            return;
        }
        GotAudioSample(timestamp, &sample);
    }
    
    int EC_RtmpPuller::GotAudioSample(u_int32_t timestamp, SrsCodecSample *sample)
    {
        int ret = ERROR_SUCCESS;
        for (int i = 0; i < sample->nb_sample_units; i++) {
            SrsCodecSampleUnit* sample_unit = &sample->sample_units[i];
            int32_t size = sample_unit->size;
            
            if (!sample_unit->bytes || size <= 0 || size > 0x1fff) {
                ret = -1;
                return ret;
            }
            
            // the frame length is the AAC raw data plus the adts header size.
            int32_t frame_length = size + 7;
            
            // AAC-ADTS
            // 6.2 Audio Data Transport Stream, ADTS
            // in aac-iso-13818-7.pdf, page 26.
            // fixed 7bytes header
            u_int8_t adts_header[7] = { 0xff, 0xf9, 0x00, 0x00, 0x00, 0x0f, 0xfc };
            /*
             // adts_fixed_header
             // 2B, 16bits
             int16_t syncword; //12bits, '1111 1111 1111'
             int8_t ID; //1bit, '1'
             int8_t layer; //2bits, '00'
             int8_t protection_absent; //1bit, can be '1'
             // 12bits
             int8_t profile; //2bit, 7.1 Profiles, page 40
             TSAacSampleFrequency sampling_frequency_index; //4bits, Table 35, page 46
             int8_t private_bit; //1bit, can be '0'
             int8_t channel_configuration; //3bits, Table 8
             int8_t original_or_copy; //1bit, can be '0'
             int8_t home; //1bit, can be '0'
             
             // adts_variable_header
             // 28bits
             int8_t copyright_identification_bit; //1bit, can be '0'
             int8_t copyright_identification_start; //1bit, can be '0'
             int16_t frame_length; //13bits
             int16_t adts_buffer_fullness; //11bits, 7FF signals that the bitstream is a variable rate bitstream.
             int8_t number_of_raw_data_blocks_in_frame; //2bits, 0 indicating 1 raw_data_block()
             */
            // profile, 2bits
            SrsAacProfile aac_profile = srs_codec_aac_rtmp2ts(srs_codec_->aac_object);
            adts_header[2] = (aac_profile << 6) & 0xc0;
            // sampling_frequency_index 4bits
            adts_header[2] |= (srs_codec_->aac_sample_rate << 2) & 0x3c;
            // channel_configuration 3bits
            adts_header[2] |= (srs_codec_->aac_channels >> 2) & 0x01;
            adts_header[3] = (srs_codec_->aac_channels << 6) & 0xc0;
            // frame_length 13bits
            adts_header[3] |= (frame_length >> 11) & 0x03;
            adts_header[4] = (frame_length >> 3) & 0xff;
            adts_header[5] = ((frame_length << 5) & 0xe0);
            // adts_buffer_fullness; //11bits
            adts_header[5] |= 0x1f;
            
            // copy to audio buffer
            audio_payload_->append((const char*)adts_header, sizeof(adts_header));
            audio_payload_->append(sample_unit->bytes, sample_unit->size);
            if(av_packet_cacher) {
                av_packet_cacher->onAacDataComing((uint8_t*)audio_payload_->_data, audio_payload_->_data_len, timestamp);
            }
            audio_payload_->reset();
        }
        
        return ret;
    }
}
