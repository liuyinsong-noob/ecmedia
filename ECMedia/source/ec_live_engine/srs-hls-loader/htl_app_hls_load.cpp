/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "htl_stdinc.hpp"

#include <inttypes.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>
#include <unistd.h>
using namespace std;

#include "htl_core_log.hpp"
#include "htl_core_error.hpp"
#include "htl_core_aggregate_ret.hpp"
#include "htl_app_http_client.hpp"

#include "htl_app_hls_load.hpp"

#include <algorithm>

#define DEFAULT_TS_DURATION 10

StHlsTask::StHlsTask(){
    target_duration = DEFAULT_TS_DURATION;
    callback_ = nullptr;
    running_ = false;
    ts_wait_event_ = cloopenwebrtc::EventWrapper::Create();
}

StHlsTask::~StHlsTask(){
    callback_ = nullptr;
}

int StHlsTask::Initialize(std::string http_url, bool vod, double startup, double delay, double error, int count, StHlsTaskCallback* callback){
    int ret = ERROR_SUCCESS;
    
    is_vod = vod;
    callback_ = callback;
    
    if((ret = InitializeBase(http_url, startup, delay, error, count)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}

Uri* StHlsTask::GetUri(){
    return &url;
}

int StHlsTask::ProcessTask(){
    int ret = ERROR_SUCCESS;
    
    Trace("start to process HLS task #%d, schema=%s, host=%s, port=%d, path=%s, startup=%.2f, delay=%.2f, error=%.2f, count=%d", 
        GetId(), url.GetSchema(), url.GetHost(), url.GetPort(), url.GetPath(), startup_seconds, delay_seconds, error_seconds, count);
    running_ = true;
    
    while(running_) {
        statistic->OnTaskStart(GetId(), url.GetUrl());
        
        StHttpClient client;
        if((ret = ProcessM3u8(client)) != ERROR_SUCCESS){
            statistic->OnTaskError(GetId(), 0);
            
            Error("http client process m3u8 failed. ret=%d", ret);
            if(running_) {
                ts_wait_event_->Wait(error_seconds * 1000);
            }
            
            continue;
        }
        
        Info("[HLS] %s download completed.", url.GetUrl());
    }
    
    return ret;
}

void StHlsTask::stopProcessTask(){
    running_ = false;
    ts_wait_event_->Set();
}


int StHlsTask::ProcessM3u8(StHttpClient& client){
    int ret = ERROR_SUCCESS;
    
    string m3u8;
    if((ret = client.DownloadString(&url, &m3u8)) != ERROR_SUCCESS){
        Error("http client get m3u8 failed. ret=%d", ret);
        return ret;
    }
    Trace("[HLS] get m3u8 %s get success, length=%" PRId64, url.GetUrl(), (int64_t)m3u8.length());
    
    string variant;
    vector<M3u8TS> ts_objects;
    if((ret = HlsM3u8Parser::ParseM3u8Data(&url, m3u8, ts_objects, target_duration, variant)) != ERROR_SUCCESS){
        Error("http client parse m3u8 content failed. ret=%d", ret);
        return ret;
    }
    
    if (!variant.empty()) {
        if ((ret = url.Initialize(variant)) != ERROR_SUCCESS) {
            Error("parse variant=%s failed, ret=%d", variant.c_str(), ret);
            return ret;
        }
        return ret;
    }
    
    if((ret = ProcessTS(client, ts_objects)) != ERROR_SUCCESS){
        Error("http client download m3u8 ts file failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int StHlsTask::ProcessTS(StHttpClient& client, vector<M3u8TS>& ts_objects){
    int ret = ERROR_SUCCESS;
    
    vector<M3u8TS>::iterator ite = ts_objects.begin();
    
    // if live(not vod), remember the last download ts object.
    // if vod(not live), always access from the frist ts.
    if(!is_vod){
        ite = find(ts_objects.begin(), ts_objects.end(), last_downloaded_ts);
        
        // not found, reset to begin to process all.
        if(ite == ts_objects.end()){
            ite = ts_objects.begin();
        }
        // fount, skip it.
        else{
            ite++;
        }
        
        // no ts now, wait for a segment
        if(ite == ts_objects.end()){
            int sleep_ms = StUtility::BuildRandomMTime((target_duration > 0)? target_duration:DEFAULT_TS_DURATION);
            Trace("[TS] no fresh ts, wait for a while. sleep %dms", sleep_ms);
            if(running_) {
                 ts_wait_event_->Wait(sleep_ms);
            }

            return ret;
        }
    }
    
    AggregateRet aggregate_ret;
    
    // to process from the specified ite
    for(; ite != ts_objects.end(); ++ite){
        if(!running_) {
            break;
        }
        
        M3u8TS ts_object = *ite;

        if(!is_vod){
            last_downloaded_ts = ts_object;
        }
        
        Info("start to process ts %s", ts_object.ts_url.c_str());
        
        aggregate_ret.Add(DownloadTS(client, ts_object));
    }
    
    return aggregate_ret.GetReturnValue();
}

int StHlsTask::DownloadTS(StHttpClient& client, M3u8TS& ts){
    int ret = ERROR_SUCCESS;
    
    HttpUrl url;
    
    if((ret = url.Initialize(ts.ts_url)) != ERROR_SUCCESS){
        Error("initialize ts url failed. ret=%d", ret);
        return ret;
    }
    
    Info("[TS] url=%s, duration=%.2f, delay=%.2f", url.GetUrl(), ts.duration, delay_seconds);
    statistic->OnSubTaskStart(GetId(), ts.ts_url);
    
    std::string ts_video_buffer_;
    if((ret = client.DownloadString(&url, &ts_video_buffer_)) != ERROR_SUCCESS){
        statistic->OnSubTaskError(GetId(), (int)ts.duration);
            
        Error("http client download ts file %s failed. ret=%d", url.GetUrl(), ret);
        return ret;
    }

    callback_->onTSDownloaded(ts_video_buffer_.data(), ts_video_buffer_.length());
    int sleep_ms = StUtility::BuildRandomMTime((delay_seconds >= 0)? delay_seconds:ts.duration);
    Trace("[TS] url=%s download, duration=%.2f, delay=%.2f, size=%" PRId64", sleep %dms", 
        url.GetUrl(), ts.duration, delay_seconds, client.GetResponseHeader()->content_length, sleep_ms);
    if(running_) {
        ts_wait_event_->Wait(sleep_ms);
    }

    statistic->OnSubTaskEnd(GetId(), (int)ts.duration);
    return ret;
}

