//
//  ec_aac_codec.cpp
//  ECMedia
//
//  Created by gezhaoyou on 17/7/23.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#include "ec_aac_codec.h"
#include "ec_live_utility.h"

namespace cloopenwebrtc {
#define AAC_CODEC_SAMPLE_RATE 44100
#define AUDIO_CHANNEL_COUNT 2
    
    EC_AAC_Codec::EC_AAC_Codec() {
//        faac_decode_handle_   = faac_encoder_crate(16000, 2, &faac_encode_input_samples_);
//        faac_encode_handle_ = faac_encoder_crate(16000, 2, &faac_encode_input_samples_);
    }

    EC_AAC_Codec::~EC_AAC_Codec(){

    }

    void EC_AAC_Codec::setEncodedDataCallback(EC_CapturerCallback* callback) {
        aac_data_callback = callback;
    }
    
    int EC_AAC_Codec::encodeFrame(void *audio_data, int audio_record_samples, int audio_record_channels_, int audio_record_sample_hz_) {
        if(!faac_encode_handle_) {
            if(!faac_encode_handle_) {
                faac_encode_input_samples_ = 0;
                faac_encode_handle_ = faac_encoder_crate(AAC_CODEC_SAMPLE_RATE, 2, &faac_encode_input_samples_);
            }
            
            uint8_t *pcmdata , *aac_data;
            int aac_data_len = 0;
            
            recordbuffer_.PushData((unsigned char *)audio_data, 2*audio_record_channels_*audio_record_sample_hz_/100);
            
            while (pcmdata = recordbuffer_.ConsumeData(faac_encode_input_samples_*2))
            {
                aac_data_len = 0;
                faac_encode_frame(faac_encode_handle_, pcmdata, &aac_data, &aac_data_len);
                if (aac_data_len > 0) {
                    if(aac_data_callback) {
                        //aac_data_callback->OnAACEncodeData(aac_data, aac_data_len, EC_Live_Utility::getTimestamp());
                    }
                }
            }
        }
        return 0;
    }

    int EC_AAC_Codec::decodeFrame() {
        return 0;
    }

    int32_t EC_AAC_Codec::resmapleAudioData(const void* audio_data, const size_t nSamples, const uint32_t samplesRate, const size_t nChannels)
    {
        // rtc::CritScope cs(&cs_audio_record_);
        int audio_record_sample_hz_ = AAC_CODEC_SAMPLE_RATE;
        int audio_record_channels_ = 2;
        size_t kMaxDataSizeSamples = 3840;
        
        if (audio_record_sample_hz_ != samplesRate || nChannels != audio_record_channels_) {
            int16_t temp_output[kMaxDataSizeSamples];
            int samples_per_channel_int = resampler_record_.Resample10Msec((int16_t*)audio_data, samplesRate * nChannels,
                                                                           audio_record_sample_hz_ * audio_record_channels_, 1, kMaxDataSizeSamples, temp_output);

            encodeFrame(temp_output, audio_record_sample_hz_ / 100, audio_record_channels_, audio_record_sample_hz_);
        }
        else {
            encodeFrame((void *)audio_data, audio_record_sample_hz_ / 100, audio_record_channels_, audio_record_sample_hz_);
        }
        return 0;
    }
}

