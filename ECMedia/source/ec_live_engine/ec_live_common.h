//
// Created by gezhaoyou on 17/7/23.
// Copyright (c) 2017 Sean Lee. All rights reserved.
//

#ifndef ECMEDIA_EC_LIVE_COMMON_H
#define ECMEDIA_EC_LIVE_COMMON_H

#include "sdk_common.h"
#include "clock.h"
#include "thread_wrapper.h"
#include "critical_section_wrapper.h"
#include "ECMedia.h"

namespace cloopenwebrtc {
    template <class T> class RingBuffer {
    public:
        RingBuffer(uint32_t size=8192);
        void PushData(const T* , uint32_t size );
        T* ConsumeData( uint32_t size);
        void Clear() {
            data_length_ = 0;
            data_start_ = 0;
        };
    private:
        T *buffer_;
        uint32_t data_length_;
        uint32_t data_start_;
        uint32_t buffer_legnth_;
    };
    template <class T>
    RingBuffer<T>::RingBuffer( uint32_t size)
    {
        buffer_ = new T[size];
        data_length_ = 0;
        data_start_ = 0;
        buffer_legnth_ = size;
        
    };
    
    template<class T>
    T* RingBuffer<T>::ConsumeData(uint32_t size)
    {
        if( data_length_ < size)
            return NULL;
        T* p = buffer_ + data_start_;
        data_length_ -= size;
        data_start_ += size;
        return p;
        
    };
    template<class T>
    void RingBuffer<T>::PushData(const T * data, uint32_t size)
    {
        if( size > buffer_legnth_ - data_length_) {
            //Todo
            T*p = new T[size+ data_length_];
            memcpy(p,buffer_+data_start_ , data_length_);
            memcpy(p+data_length_, data, size);
            data_length_ += size;
            data_start_ = 0;
            delete buffer_;
            buffer_ = p;
        }
        else {
            
            if (data_length_ + size > buffer_legnth_) {
                int newbuffer_legnth_ = data_length_ + size + buffer_legnth_;
                T *newbuffer = new T[newbuffer_legnth_];
                memmove(newbuffer, buffer_, buffer_legnth_);
                delete buffer_;
                buffer_legnth_ = newbuffer_legnth_;
                buffer_ = newbuffer;
                
            }
            else {
                memmove(buffer_, buffer_ + data_start_, data_length_);
                memcpy(buffer_ + data_length_, data, size);
                data_length_ += size;
                data_start_ = 0;
            }
        }
    };
    
    enum LIVE_MODE  {
        MODE_LIVE_UNKNOW = 0,
        MODE_LIVE_PLAY,
        MODE_LIVE_PUSH,
    } ;
    
    enum VIDEO_SOURCE {
        VIDEO_SOURCE_CAMERA = 0,
        VIDEO_SOURCE_DESKTOP
    };
    
    struct ShareWindowInfo {
        int id;
        int type;
        std::string name;
    };


    enum EC_RTMP_DATA_TYPE{
        VIDEO_DATA,
        AUDIO_DATA,
        META_DATA
    };

    typedef struct EncData
    {
        EncData(void) :_data(NULL), _dataLen(0),
                       _isVideo(false), _dts(0) {

        }

        uint8_t*            _data;
        int                 _dataLen;
        bool                _isVideo;
        bool                _isKeyfream;
        uint32_t            _dts;
        EC_RTMP_DATA_TYPE   _type;
    } EncodedData;

    class EC_CapturerCallback {
    public:
        EC_CapturerCallback(void){};
        virtual ~EC_CapturerCallback(void){};
        virtual void OnCapturerAacDataReady(uint8_t* pData, int nLen, uint32_t ts) = 0;
        virtual void OnCapturerAvcDataReady(uint8_t* pData, int nLen, uint32_t ts) = 0;
    };


    class EC_ReceiverCallback {
    public:
        EC_ReceiverCallback(void){};
        virtual ~EC_ReceiverCallback(void){};
        virtual void onAvcDataComing(void* nalu_data, int len, uint32_t timestamp) = 0;
        virtual void onAacDataComing(uint8_t* pData, int nLen, uint32_t ts, uint32_t sample_rate, int audio_channels) = 0;
    };

}


#endif //ECMEDIA_EC_LIVE_COMMON_H
