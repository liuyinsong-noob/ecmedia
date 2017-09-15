//
//  ec_rtmp_bitrate_controller.hpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_rtmp_bitrate_controller_hpp
#define ec_rtmp_bitrate_controller_hpp

#include <list>
#include <vector>
#include "ec_live_common.h"

namespace cloopenwebrtc {
    class EventTimerWrapper;
    class EC_RTMP_BitrateControllerCallback {
    public:
        EC_RTMP_BitrateControllerCallback(){};
        virtual ~EC_RTMP_BitrateControllerCallback(){};

        virtual void onOutputBitrateChanged(int bitrate) = 0;
        virtual void onNeedClearBuffer() = 0;
    };


    class EC_RTMP_BitrateController{
    public:
        EC_RTMP_BitrateController();
        ~EC_RTMP_BitrateController();

        void start();
        void shutdown();
        void inputDataCount(int data_size);
        void outputDataCount(int data_size);

        static bool controllerThreadRun(void *pThis);
        void setBitrateControllerCallback(EC_RTMP_BitrateControllerCallback* cb);
    private:
        uint32_t input_data_total_size_;
        uint32_t output_data_total_size_;

        uint32_t last_input_data_total_size_;
        uint32_t last_output_data_total_size_;

        uint32_t cacher_data_size_;

        ThreadWrapper* bit_rate_controller_pthread_;

        std::vector<int> input_bitrate_vec_;
        std::vector<int> output_bitrate_vec_;

        uint32_t base_time_ ;
        EC_RTMP_BitrateControllerCallback *callback_;
        EventTimerWrapper *timer_event_;
    private:
        bool runing_;
        bool run();

        void adjustBandwidth();
        double avg(std::vector<int> v );

        double standardDeviation(std::vector<int> v , int mean);

    };


}

#endif /* ec_rtmp_bitrate_controller_hpp */
