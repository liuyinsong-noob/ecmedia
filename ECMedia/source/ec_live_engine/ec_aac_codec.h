//
//  ec_aac_codec.hpp
//  ECMedia
//
//  Created by gezhaoyou on 17/7/23.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_aac_codec_hpp
#define ec_aac_codec_hpp
#include "faaccodec.h"
#include "ec_live_common.h"
#include "acm_resampler.h"

namespace yuntongxunwebrtc {
    class EC_AAC_Codec {

    public:
        EC_AAC_Codec();
        ~EC_AAC_Codec();
        int encodeFrame(void *audio_data, int audio_record_samples, int audio_record_channels_, int audio_record_sample_hz_);
        int decodeFrame();
        int32_t resmapleAudioData(const void* audio_data, const size_t nSamples, const uint32_t samplesRate, const size_t nChannels);
        void setEncodedDataCallback(EC_CapturerCallback* callback);
    private:
        void *faac_decode_handle_;
        void *faac_encode_handle_;
        
        unsigned long faac_encode_input_samples_;
        acm2::ACMResampler resampler_record_;
        RingBuffer<uint8_t> recordbuffer_;
        EC_CapturerCallback *aac_data_callback;
    };
}

#endif /* ec_aac_codec_hpp */
