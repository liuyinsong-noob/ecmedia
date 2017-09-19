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
#include <winsock2.h>
#include <ws2tcpip.h>
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

    static u_int8_t fresh_nalu_header[] = { 0x00, 0x00, 0x00, 0x01 };
    static u_int8_t cont_nalu_header[] =  { 0x00, 0x00, 0x01};

    EC_RtmpPuller::EC_RtmpPuller(EC_MediaPullCallback* callback) {
        retry_ct_ = 0;
        running_ = false;
        callback_ = callback;
        rtmp_status_ =RS_PLY_Init;
        rtmpPullingThread_ = ThreadWrapper::CreateThread(EC_RtmpPuller::pullingThreadRun,
                this,
                kNormalPriority,
                "rtmpPullingThread_");
        av_packet_cacher = NULL;
//        srs_codec_ = new SrsAvcAacCodec();
//        audio_payload_ = new DemuxData(1024);
//        video_payload_ = new DemuxData(384 * 1024);
        
      
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
                            srs_human_trace("SRS: simple handshake ok.");
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
                            srs_human_trace("SRS: connect vhost/app ok.");
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
                            srs_human_trace("SRS: play stream ok.");
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
                        DoReadData();
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
            callback_->OnLivePullerConnected();
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
                    return;
                }
                
                if(connected_) {
                    callback_->OnLivePullerDisconnect();
                }
                else {
                    callback_->OnLivePullerFailed();
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
    
    void EC_RtmpPuller::DoReadData() {
        int size;
        char type;
        char *data;
        u_int32_t timestamp;

        if (srs_rtmp_read_packet(rtmp_, &type, &timestamp, &data, &size) != 0) {
            //todo log error
        }

        if (type == SRS_RTMP_TYPE_VIDEO) {
             handleVideoPacket(data, size, timestamp);
        } else if (type == SRS_RTMP_TYPE_AUDIO) {
            HandleAuidoPacket(data, size, timestamp);
        } else if (type == SRS_RTMP_TYPE_SCRIPT) {
            if (!srs_rtmp_is_onMetaData(type, data, size)) {
                // LOG(LS_ERROR) << "No flv";
                srs_human_trace("drop message type=%#x, size=%dB", type, size);
            }
        }
        
//            SrsCodecSample sample;
//            if (srs_codec_->video_avc_demux(data, size, &sample) == ERROR_SUCCESS) {
//                if (srs_codec_->video_codec_id == SrsCodecVideoAVC) {    // Jus support H264
//                    //video_payload_->append(data+9, 2);
//                    GotVideoSample(timestamp, &sample);
//
//                } else {
//                    //LOG(LS_ERROR) << "Don't support video format!";
//                }
//            }
//        } else if (type == SRS_RTMP_TYPE_AUDIO) {
//            SrsCodecSample sample;
//            if (srs_codec_->audio_aac_demux(data, size, &sample) != ERROR_SUCCESS) {
//                if (sample.acodec == SrsCodecAudioMP3 && srs_codec_->audio_mp3_demux(data, size, &sample) != ERROR_SUCCESS) {
//                    free(data);
//                    return;
//                }
//                free(data);
//                return;    // Just support AAC.
//            }
//
//            SrsCodecAudio acodec = (SrsCodecAudio) srs_codec_->audio_codec_id;
//
//            // ts support audio codec: aac/mp3
//            if (acodec != SrsCodecAudioAAC && acodec != SrsCodecAudioMP3) {
//                free(data);
//                return;
//            }
//            // for aac: ignore sequence header
//            if (acodec == SrsCodecAudioAAC && sample.aac_packet_type == SrsCodecAudioTypeSequenceHeader
//                    || srs_codec_->aac_object == SrsAacObjectTypeReserved) {
//                free(data);
//                return;
//            }
//            GotAudioSample(timestamp, &sample);
//        } else if (type == SRS_RTMP_TYPE_SCRIPT) {
//            if (!srs_rtmp_is_onMetaData(type, data, size)) {
//                // LOG(LS_ERROR) << "No flv";
//                srs_human_trace("drop message type=%#x, size=%dB", type, size);
//            }
//        }
//
//        //if (srs_human_print_rtmp_packet(type, timestamp, data, size) != 0) {
//        //}
        free(data);
       
    }

    void EC_RtmpPuller::handleVideoPacket(char* data, int len, u_int32_t timestamp) {
        
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
                        // PrintConsole("[RTMP ERROR] %s unpack nalu error\n", __FUNCTION__);
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
                av_packet_cacher->CacheH264Data((uint8_t*)payloadData, payloadLen, timestamp);
            }
        }
    }
    
    void EC_RtmpPuller::HandleAuidoPacket(char* data, int length, u_int32_t timestamp)
    {
        unsigned voiceCodec = ((unsigned char)data[0]) >> 4;
        switch (voiceCodec) {
            case 0:
                PrintConsole("[RTMP INFO] %s Linear PCM\n", __FUNCTION__);
                break;
            case 10: //AAC
                if(av_packet_cacher) {
                     av_packet_cacher->CacheAacData((uint8_t*)data+2, length - 2, timestamp);
                }
                break;
            default:
                PrintConsole("[RTMP ERROR] %s codec id %d not support\n", __FUNCTION__,voiceCodec);
                break;
        }
        
        // printf("sample rate is %d KHZ  timestamp %d\n", sampleRate, packet->m_nTimeStamp);
    }
    
    
//    int EC_RtmpPuller::GotVideoSample(u_int32_t timestamp, SrsCodecSample *sample)
//    {
//        int ret = ERROR_SUCCESS;
//        // ignore info frame,
//        // @see https://github.com/simple-rtmp-server/srs/issues/288#issuecomment-69863909
//        if (sample->frame_type == SrsCodecVideoAVCFrameVideoInfoFrame) {
//            return ret;
//        }
//
//        // ignore sequence header
//        if (sample->frame_type == SrsCodecVideoAVCFrameKeyFrame
//                && sample->avc_packet_type == SrsCodecVideoAVCTypeSequenceHeader) {
//            return ret;
//        }
//
//        // when ts message(samples) contains IDR, insert sps+pps.
//        if (sample->has_idr) {
//            // fresh nalu header before sps.
//            if (srs_codec_->sequenceParameterSetLength > 0) {
////                video_payload_->append((const char*)fresh_nalu_header, 4);
//                // sps
//                video_payload_->append(srs_codec_->sequenceParameterSetNALUnit, srs_codec_->sequenceParameterSetLength);
//            }
//            // cont nalu header before pps.
//            if (srs_codec_->pictureParameterSetLength > 0) {
////                video_payload_->append((const char*)fresh_nalu_header, 4);
//                // pps
//                video_payload_->append(srs_codec_->pictureParameterSetNALUnit, srs_codec_->pictureParameterSetLength);
//            }
//        }
//
//        // all sample use cont nalu header, except the sps-pps before IDR frame.
//        for (int i = 0; i < sample->nb_sample_units; i++) {
//            SrsCodecSampleUnit* sample_unit = &sample->sample_units[i];
//            int32_t size = sample_unit->size;
//
//            if (!sample_unit->bytes || size <= 0) {
//                ret = -1;
//                return ret;
//            }
//
//
//            // 5bits, 7.3.1 NAL unit syntax,
//            // H.264-AVC-ISO_IEC_14496-10-2012.pdf, page 83.
//            SrsAvcNaluType nal_unit_type = (SrsAvcNaluType)(sample_unit->bytes[0] & 0x1f);
//
//            // ignore SPS/PPS/AUD
//            switch (nal_unit_type) {
//                case SrsAvcNaluTypeSPS:
//                case SrsAvcNaluTypePPS:
//                case SrsAvcNaluTypeSEI:
//                case SrsAvcNaluTypeAccessUnitDelimiter:
//                    continue;
//                default: {
//                    if (nal_unit_type == SrsAvcNaluTypeReserved) {
//                        RescanVideoframe(sample_unit->bytes, sample_unit->size, timestamp);
//                        continue;
//                    }
//                }
//                    break;
//            }
//
//            if (nal_unit_type == SrsAvcNaluTypeIDR) {
//                // insert cont nalu header before frame.
//#ifdef WEBRTC_IOS
////                video_payload_->append((const char*)fresh_nalu_header, 4);
//#else
////                 video_payload_->append((const char*)cont_nalu_header, 3);
//#endif
//            }
//            else {
////                 video_payload_->append((const char*)fresh_nalu_header, 4);
//            }
//            // sample data
//            video_payload_->append(sample_unit->bytes, sample_unit->size);
//        }
//        //* Fix for mutil nalu.
//        if (video_payload_->_data_len != 0) {
//             //callback_->onAvcDataComing((uint8_t*)video_payload_->_data, video_payload_->_data_len, timestamp);
//        }
//        video_payload_->reset();
//
//        return ret;
//    }
//    int EC_RtmpPuller::GotAudioSample(u_int32_t timestamp, SrsCodecSample *sample)
//    {
//        int ret = ERROR_SUCCESS;
//        for (int i = 0; i < sample->nb_sample_units; i++) {
//            SrsCodecSampleUnit* sample_unit = &sample->sample_units[i];
//            int32_t size = sample_unit->size;
//
//            if (!sample_unit->bytes || size <= 0 || size > 0x1fff) {
//                ret = -1;
//                return ret;
//            }
//
//            // the frame length is the AAC raw data plus the adts header size.
//            int32_t frame_length = size + 7;
//
//            // AAC-ADTS
//            // 6.2 Audio Data Transport Stream, ADTS
//            // in aac-iso-13818-7.pdf, page 26.
//            // fixed 7bytes header
//            u_int8_t adts_header[7] = { 0xff, 0xf9, 0x00, 0x00, 0x00, 0x0f, 0xfc };
//            /*
//            // adts_fixed_header
//            // 2B, 16bits
//            int16_t syncword; //12bits, '1111 1111 1111'
//            int8_t ID; //1bit, '1'
//            int8_t layer; //2bits, '00'
//            int8_t protection_absent; //1bit, can be '1'
//            // 12bits
//            int8_t profile; //2bit, 7.1 Profiles, page 40
//            TSAacSampleFrequency sampling_frequency_index; //4bits, Table 35, page 46
//            int8_t private_bit; //1bit, can be '0'
//            int8_t channel_configuration; //3bits, Table 8
//            int8_t original_or_copy; //1bit, can be '0'
//            int8_t home; //1bit, can be '0'
//
//            // adts_variable_header
//            // 28bits
//            int8_t copyright_identification_bit; //1bit, can be '0'
//            int8_t copyright_identification_start; //1bit, can be '0'
//            int16_t frame_length; //13bits
//            int16_t adts_buffer_fullness; //11bits, 7FF signals that the bitstream is a variable rate bitstream.
//            int8_t number_of_raw_data_blocks_in_frame; //2bits, 0 indicating 1 raw_data_block()
//            */
//            // profile, 2bits
//            SrsAacProfile aac_profile = srs_codec_aac_rtmp2ts(srs_codec_->aac_object);
//            adts_header[2] = (aac_profile << 6) & 0xc0;
//            // sampling_frequency_index 4bits
//            adts_header[2] |= (srs_codec_->aac_sample_rate << 2) & 0x3c;
//            // channel_configuration 3bits
//            adts_header[2] |= (srs_codec_->aac_channels >> 2) & 0x01;
//            adts_header[3] = (srs_codec_->aac_channels << 6) & 0xc0;
//            // frame_length 13bits
//            adts_header[3] |= (frame_length >> 11) & 0x03;
//            adts_header[4] = (frame_length >> 3) & 0xff;
//            adts_header[5] = ((frame_length << 5) & 0xe0);
//            // adts_buffer_fullness; //11bits
//            adts_header[5] |= 0x1f;
//
//            // copy to audio buffer
//            audio_payload_->append((const char*)adts_header, sizeof(adts_header));
//            audio_payload_->append(sample_unit->bytes, sample_unit->size);
//
//            // callback_.OnRtmpullAACData((uint8_t*)audio_payload_->_data, audio_payload_->_data_len, timestamp);
//            audio_payload_->reset();
//        }
//
//        return ret;
//    }
//
//    void EC_RtmpPuller::RescanVideoframe(const char*pdata, int len, uint32_t timestamp)
//    {
//        int nal_type = pdata[4] & 0x1f;
//        const char *p = pdata;
//        if (nal_type == 7)
//        {// keyframe
//            int find7 = 0;
//            const char* ptr7 = NULL;
//            int size7 = 0;
//            int find8 = 0;
//            const char* ptr8 = NULL;
//            int size8 = 0;
//            const char* ptr5 = NULL;
//            int size5 = 0;
//            int head01 = 4;
//            for (int i = 4; i < len - 4; i++)
//            {
//                if ((p[i] == 0x0 && p[i + 1] == 0x0 && p[i + 2] == 0x0 && p[i + 3] == 0x1) || (p[i] == 0x0 && p[i + 1] == 0x0 && p[i + 2] == 0x1))
//                {
//                    if (p[i + 2] == 0x01)
//                        head01 = 3;
//                    else
//                        head01 = 4;
//                    if (find7 == 0)
//                    {
//                        find7 = i;
//                        ptr7 = p;
//                        size7 = find7;
//                        i++;
//                    }
//                    else if (find8 == 0)
//                    {
//                        find8 = i;
//                        ptr8 = p + find7 ;
//                        size8 = find8 - find7;
//                        const char* ptr = p + i;
//                        if ((ptr[head01] & 0x1f) == 5)
//                        {
//                            ptr5 = p + find8 + head01;
//                            size5 = len - find8 - head01;
//                            break;
//                        }
//                    }
//                    else
//                    {
//                        ptr5 = p + i + head01;
//                        size5 = len - i - head01;
//                        break;
//                    }
//                }
//            }
//            video_payload_->append(ptr7, size7);
//            video_payload_->append(ptr8, size8);
////            video_payload_->append((const char*)fresh_nalu_header, 4);
//            video_payload_->append(ptr5, size5);
//            //callback_->onAvcDataComing((uint8_t*)video_payload_->_data, video_payload_->_data_len, timestamp);
//            video_payload_->reset();
//        }
//        else
//        {
//            video_payload_->append(pdata, len);
//            //callback_->onAvcDataComing((uint8_t*)video_payload_->_data, video_payload_->_data_len, timestamp);
//            video_payload_->reset();
//        }
//    }
}
