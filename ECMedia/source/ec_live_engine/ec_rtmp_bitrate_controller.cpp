//
//  ec_rtmp_bitrate_controller.cpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//
#include <math.h>
#include <iostream>
#if defined(_WIN32)
#include <cstdint>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "ec_rtmp_bitrate_controller.h"
#include "ec_live_utility.h"
#include "event_wrapper.h"


namespace yuntongxunwebrtc{
    EC_RTMP_BitrateController::EC_RTMP_BitrateController() {
        bit_rate_controller_pthread_ = ThreadWrapper::CreateThread(EC_RTMP_BitrateController::controllerThreadRun,
                this,
                kNormalPriority,
                "bit_rate_controller_pthread_");

        base_time_ = 0;
        callback_ = nullptr;
        runing_ = false;
        cacher_data_size_ = 0;
        
        input_data_total_size_ = 0;
        output_data_total_size_ = 0;
        
        last_input_data_total_size_ = 0;
        last_output_data_total_size_ = 0;
        
        timer_event_ = EventTimerWrapper::Create();
        
    }

    EC_RTMP_BitrateController::~EC_RTMP_BitrateController() {
        runing_ = false;
    }

    void EC_RTMP_BitrateController::inputDataCount(int data_size) {
        input_data_total_size_ += data_size;
    }

    void EC_RTMP_BitrateController:: outputDataCount(int data_size) {
        output_data_total_size_ += data_size;
    }

    void EC_RTMP_BitrateController::setBitrateControllerCallback(EC_RTMP_BitrateControllerCallback* cb) {
        callback_ = cb;
    }

    void EC_RTMP_BitrateController::start() {
        if(!runing_) {
            runing_ = true;
            unsigned int pthread_id;
            bit_rate_controller_pthread_->Start(pthread_id);
            timer_event_->StartTimer(true, 1000); //1000ms
        }
    }

    void EC_RTMP_BitrateController::shutdown() {
        if(runing_) {
            runing_ = false;
            timer_event_->Set();
            timer_event_->StopTimer();
            bit_rate_controller_pthread_->Stop();
        }
    }
    
    bool EC_RTMP_BitrateController::controllerThreadRun(void *pThis) {
        return static_cast<EC_RTMP_BitrateController*>(pThis)->run();
    }

    bool EC_RTMP_BitrateController::run() {
        while(runing_) {
            timer_event_->Wait(1000); // wait 1000ms

            // for input data.
            int intput_size_diff = input_data_total_size_ - last_input_data_total_size_;
            input_bitrate_vec_.push_back(intput_size_diff);
            last_input_data_total_size_ = input_data_total_size_;

            int output_size_diff = output_data_total_size_ - last_output_data_total_size_;
            output_bitrate_vec_.push_back(output_size_diff);
            last_output_data_total_size_ = output_data_total_size_;
            
            cacher_data_size_ += (intput_size_diff - output_size_diff) > 0 ? (intput_size_diff - output_size_diff) : 0;
            
            static int statistics_times = 0;
            statistics_times++;
            // 缓存大于3MB
            if(cacher_data_size_ > 3000000) {
                if(callback_) {
                    statistics_times = 0;
                    cacher_data_size_ = 0;
                    callback_->onNeedClearBuffer();
                    adjustBandwidth();
                }
            }

            if(statistics_times > 60) {
                statistics_times = 0;
                adjustBandwidth();
            }
            return true;
        }
        return false;
    }

    void EC_RTMP_BitrateController::adjustBandwidth() {
        int input_bitrate_average = avg(input_bitrate_vec_);
        int input_bitrate_sum = input_bitrate_average*input_bitrate_vec_.size();
        int input_bitrate_variance = standardDeviation(input_bitrate_vec_, input_bitrate_average);
        input_bitrate_vec_.clear();
        
        int output_bitrate_average = avg(output_bitrate_vec_);
        int output_bitrate_sum = output_bitrate_average*output_bitrate_vec_.size();
        int output_bitrate_variance = standardDeviation(output_bitrate_vec_, output_bitrate_average);
        output_bitrate_vec_.clear();
        
        if(input_bitrate_average > output_bitrate_average + output_bitrate_variance/10) {
            if(callback_) {
                int coefficient = 1/3;
                callback_->onOutputBitrateChanged(output_bitrate_average + (int) (coefficient * output_bitrate_variance));
            }
        }
    }

    //Function for average
    double EC_RTMP_BitrateController::avg(std::vector<int> v)
    {
        double return_value = 0.0;
        int n = v.size();

        for ( int i=0; i < n; i++)
        {
            return_value += v[i];
        }
        return (return_value / n);
    }

    double EC_RTMP_BitrateController::standardDeviation(std::vector<int> v , int mean)
    {
        double sum = 0.0;
        double temp =0.0;

        for ( int j =0; j < v.size(); j++)
        {
            temp =  pow((v[j] - mean), 2);
            sum += temp;
        }

        return sqrt(sum/v.size());
    }
}
