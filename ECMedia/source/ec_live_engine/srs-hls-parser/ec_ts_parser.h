//
//  ec_hls_parser.h
//  ECMedia
//
//  Created by 葛昭友 on 2017/8/21.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_hls_parser_h
#define ec_hls_parser_h

#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <vector>
#include <map>
class TSMessage;
class AacMuxer;

namespace cloopenwebrtc {

    class EC_TS_ParserCallback {
    public:
        EC_TS_ParserCallback() {};
        virtual ~EC_TS_ParserCallback() {};
        // virtual void onTsPacketParsed() = 0;
        
        // avc data callback
        virtual void onGotAvcframe(const char* avc_data, int length, int64_t dts, int64_t pts) = 0;
        // aac data callback
        virtual void onGotAacframe(const char* aac_data, int length, int64_t dts, int64_t pts) = 0;
    };
    
    
    
    class EC_TS_Parser {
    public:
        EC_TS_Parser();
        ~EC_TS_Parser();
        int parse(u_int8_t* ts_data, int data_length);
        void setCallback(EC_TS_ParserCallback* cb);
    private:
        int consume(TSMessage* msg, AacMuxer* aac_muxer);
        
    private:
        EC_TS_ParserCallback *callback_;
    };
}

#endif /* ec_hls_parser_h */
