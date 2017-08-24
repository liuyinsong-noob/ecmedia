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
#include "vie_desktop_share.h"

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
    class RTMPLiveSession 
	: public Transport ,
	  public AudioPacketizationCallback,
	  public VCMPacketizationCallback {
    public:
        static RTMPLiveSession *CreateRTMPSession(VoiceEngine * voe,VideoEngine *vie);
        RTMPLiveSession(VoiceEngine * voe,VideoEngine *vie);
		virtual ~RTMPLiveSession();

        int PlayStream(const std::string &url,void *view, onLiveStreamVideoResolution callback);
        int PushStream(const std::string &url,void *loadview);
        void StopPlay();
		void StopPush();

		void SetPushContent(bool push_audio, bool push_video);
		void SetVideoSource(VIDEO_SOURCE video_source);
		void SelectShareWindow(int type, int id);
		void GetShareWindowList(std::vector<ShareWindowInfo> & list);
		int setVideoProfile(int index, CameraCapability cam, int bitRates);
		void setNetworkStatusCallBack(onLiveStreamNetworkStatusCallBack callbck);


        static bool PlayNetworkThreadRun(void *pThis);
        bool PlayNetworkThread();
		
		static bool PushNetworkThreadRun(void *pThis);
		bool PushNetworkThread();

		void EnableBeauty();
		void DisableBeauty();

    protected:
		bool Init();
		void UnInit();
       virtual int SendRtp(int channelId, const uint8_t* packet, size_t length, const PacketOptions* options = NULL) { return 0; }
       virtual int SendRtcp(int channelId, const uint8_t* packet, size_t length) { return 0; }
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
		int Send_SPS_PPS();
		int SendVideoPacket(std::vector<uint8_t> &nalus);

		int startCameraCapture();
		int startDesktopCapture();
		bool RegisterReceiveAudioCodec(const char * plname , int plfreq, int channels);
        bool RegisterReceiveVideoCodec(const char * plname , int plfreq);
        bool UnpackSpsPps(const char *data , std::vector<uint8_t> &sps_pps);
        bool UnPackNAL(const char *data, int data_size, std::vector<uint8_t> & nal);
		bool GetAllCameraInfo();
        
    private:
        ThreadWrapper* playnetworkThread_;
		ThreadWrapper* pushnetworkThread_;
        void HandleAuidoPacket(RTMPPacket *packet);
        void HandleVideoPacket(RTMPPacket *packet);
		int startCapture();
		int stopCapture();
        void pcm_s16le_to_s16be(short *data, int len);

	public:
		int video_channel_;
		onLiveStreamVideoResolution remote_video_resoution_callback_;

	private:	
        int audio_channel_;
		LIVE_MODE live_mode_;
		VIDEO_SOURCE video_source_;
		int share_window_id_;
		DesktopShareType desktop_share_type_;

        uint16_t audio_rtp_seq_;
        uint16_t video_rtp_seq_;
		int capture_id_;
		int desktop_capture_id_;

        VoiceEngine *voe_;
        VideoEngine *vie_;
        RtpData* audio_data_cb_;
        RtpData* video_data_cb_;
        
		void *faac_decode_handle_;
		void *faac_encode_handle_;
		unsigned long faac_encode_input_samples_;
        void *video_window_;
		void *local_view_;
		unsigned int audio_sampleRate_;
		unsigned int audio_channels_ ;
        RTMP *rtmph_;
        RingBuffer<uint8_t> playbuffer_;
		RingBuffer<uint8_t> recordbuffer_;
		std::vector<CameraInfo*> cameras_;
		std::vector<char> sps_;
		std::vector<char> pps_;
		bool hasSend_SPS_PPS_;
		bool hasSend_AAC_SPEC_;
		int64_t last_receive_time_;
		int64_t packet_timeout_ms_;
		Clock*	clock_;
		CriticalSectionWrapper* rtmp_lock_;
		std::string stream_url_;

		bool push_video_;
		bool push_audio_;

		int push_video_bitrates_;
		int push_video_width_;
		int push_video_height_;
		int push_video_fps_;
		int push_camera_index_;
		onLiveStreamNetworkStatusCallBack network_status_callbck_;
      private:
          bool inited_;
          bool capture_started_;
          bool stoped_;
          static CriticalSectionWrapper *singleProtect_;
    };
    
    class ECMedia_LiveStream {
    public:
        ECMedia_LiveStream();
        static int Init();
        static RTMPLiveSession *CreateLiveStream( int type =0);
        
    };
}
#endif /* ECLiveStream_h */
