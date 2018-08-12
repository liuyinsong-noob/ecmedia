//
//  ec_rtmp_ utility.hpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_rtmp__utility_hpp
#define ec_rtmp__utility_hpp

#include "ec_live_common.h"
namespace yuntongxunwebrtc {
    class Clock;
    
    class EC_Live_Utility {
    public:
        EC_Live_Utility();
        ~EC_Live_Utility();
        static uint32_t getTimestamp();
        static uint32_t Time();
        static void pcm_s16le_to_s16be(short *data, int len);
    private:
  
    };
}



#endif /* ec_rtmp__utility_hpp */
