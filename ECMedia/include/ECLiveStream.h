//
//  ECLiveStream.h
//  ECMedia
//
//  Created by james on 16/9/23.
//  Copyright © 2016年 Cloopen. All rights reserved.
//

#ifndef ECLiveStream_h
#define ECLiveStream_h

#include "video_encoder.h"
#include "video_coding_defines.h"

struct RTMP;
struct RTMPPacket;
typedef int(*ReturnVideoWidthHeightM)(int width, int height, int channelid);

namespace cloopenwebrtc {
    
    class RtpData;
    class VoiceEngine;
    class VideoEngine;
    class ThreadWrapper;
	class Clock;

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
            memmove(buffer_ , buffer_+data_start_ , data_length_);
            memcpy(buffer_+data_length_, data, size);
            data_length_ += size;
            data_start_ = 0;
        }
    };
    
    
    class RTMPLiveSession 
	: public Transport ,
	  public AudioPacketizationCallback,
	  public VCMPacketizationCallback {
    public:
        RTMPLiveSession(VoiceEngine * voe,VideoEngine *vie);
		virtual ~RTMPLiveSession();

        int PlayStream(const std::string &url,void *view, ReturnVideoWidthHeightM callback);
        int PushStream(const std::string &url,void *loadview);
        void StopPlay();
		void StopPush();

		void setPushVideoBitrates(int bitrates);
		int setVideoProfile(int index, CameraCapability cam, int bitRates);
		void setNetworkStatusCallBack(onLiveStreamNetworkStatusCallBack callbck);

        static bool NetworkThreadRun(void *pThis);
        bool NetworkThread();
      
    protected:
		bool Init();
		void UnInit();
        virtual int SendPacket(int channel, const void *data, size_t len, int sn=0)  { return 0;};
        virtual int SendRTCPPacket(int channel, const void *data, size_t len)  { return 0;};
        virtual void SetRtpData(int channel,void *,int type);

		virtual int32_t SendData(FrameType frame_type,
			uint8_t payload_type,
			uint32_t timestamp,
			const uint8_t* payload_data,
			size_t payload_len_bytes,
			const RTPFragmentationHeader* fragmentation);


		virtual int32_t SendData(uint8_t payloadType,
			const EncodedImage& encoded_image,
			const RTPFragmentationHeader& fragmentationHeader,
			const RTPVideoHeader* rtpVideoHdr);

		int Send_AAC_SPEC();
		int SendAudioPacket(unsigned char * aac_data, int aac_data_len);
		int Send_SPS_PPS(char *sps,int sps_len, char *pps,int pps_len);
		int SendVideoPacket(std::vector<uint8_t> &nalus);

		int startCaputre();
		bool RegisterReceiveAudioCodec(const char * plname , int plfreq, int channels);
        bool RegisterReceiveVideoCodec(const char * plname , int plfreq);
        bool UnpackSpsPps(const char *data , std::vector<uint8_t> &sps_pps);
        bool UnPackNAL(const char *data, int data_size, std::vector<uint8_t> & nal);
		bool GetAllCameraInfo();
        
    private:
		CriticalSectionWrapper* crit_;
        ThreadWrapper* networkThread_;
        void HandleAuidoPacket(RTMPPacket *packet);
        void HandleVideoPacket(RTMPPacket *packet);
        
	private:	
		bool playing_;
        int audio_channel_;
        int video_channel_;
        uint16_t audio_rtp_seq_;
        uint16_t video_rtp_seq_;
		int capture_id_;

        VoiceEngine *voe_;
        VideoEngine *vie_;
        RtpData* audio_data_cb_;
        RtpData* video_data_cb_;
        
		void *faac_decode_handle_;
		void *faac_encode_handle_;
		unsigned long faac_encode_input_samples_;
        void *video_window_;
		void *local_view_;
        RTMP *rtmph_;
        RingBuffer<uint8_t> playbuffer_;
		RingBuffer<uint8_t> recordbuffer_;
		std::vector<CameraInfo*> cameras_;
		bool hasSend_SPS_PPS_;
		int64_t last_receive_time_;
		int64_t packet_timeout_ms_;
		Clock*	clock_;
		CriticalSectionWrapper* rtmp_lock_;

		int push_video_bitrates_;
		int push_video_width_;
		int push_video_height_;
		int push_video_fps_;
		int push_camera_index_;
		onLiveStreamNetworkStatusCallBack network_status_callbck_;
    };
    
    class ECMedia_LiveStream {
    public:
        ECMedia_LiveStream();
        static int Init();
        static RTMPLiveSession *CreateLiveStream( int type =0);
        
    };
}
#endif /* ECLiveStream_h */
