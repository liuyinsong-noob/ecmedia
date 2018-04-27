//
//  ec_rtmp_ utility.cpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 gezhaoyou. All rights reserved.
//

#include "ec_live_utility.h"
namespace cloopenwebrtc {
    static uint32_t base_time_ = 0;
    static Clock*	clock_ = NULL;
    EC_Live_Utility::EC_Live_Utility() {
 
    }
    
    EC_Live_Utility::~EC_Live_Utility() {
       
    }
    
    uint32_t EC_Live_Utility::getTimestamp() {
        if(!clock_) {
            clock_ = Clock::GetRealTimeClock();
        }
        
        if(base_time_ == 0) {
            base_time_ = clock_->TimeInMilliseconds();
        }
        
        return clock_->TimeInMilliseconds() - base_time_;

    }

    // get current time
    uint32_t EC_Live_Utility:: Time() {
        if(!clock_) {
            clock_ = Clock::GetRealTimeClock();
        }
        return clock_->TimeInMilliseconds();
    }

    
    void EC_Live_Utility::pcm_s16le_to_s16be(short *data, int len)
    {
        if(data) {
            for(int i = 0; i< len; i++) {
                short tmp = ((data[i] & 0xFF00) >> 8) + ((data[i] & 0XFF) << 8);
                data[i] = tmp;
            }
        }
    }
  
//    uint32_t EC_Live_Utility::createTimestamp() {
//        return 0;
//    }
}
