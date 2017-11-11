//
//  ec_media_ machine.hpp
//  ECMedia
//
//  Created by 葛昭友 on 2017/7/17.
//  Copyright © 2017年 Sean Lee. All rights reserved.
//

#ifndef ec_media__machine_hpp
#define ec_media__machine_hpp


#include <stdio.h>
#include <queue>
#include <list>
#include "video_encoder.h"
#include "video_coding_defines.h"
#include "vie_desktop_share.h"
#include "ec_live_common.h"
#include "acm_resampler.h"
#include "sdk_common.h"

typedef int(*ReturnVideoWidthHeightM)(int width, int height, int channelid);

namespace cloopenwebrtc {

    class RtpData;
    class VoiceEngine;
    class VideoEngine;
    class Clock;
    class AVCImageCallback;
    class EC_CapturerCallback;

    class ECMediaMachine
            : public Transport ,
              public AudioPacketizationCallback,
              public VCMPacketizationCallback,
              public EC_ReceiverCallback {
    public:
        static ECMediaMachine *getInstance();
        ECMediaMachine();
        virtual ~ECMediaMachine();
        // init
        bool Init();
        void UnInit();

        // device camera
        int switchCamera(int index);
        int setVideoCaptureInfo(int camera_index, int fps, int bitrate, int width, int height);
        int setVideoFrameProperty(int bitrate, int width, int height);
        int setBitrate(int bitrate);
        void setCapturerCallback(EC_CapturerCallback *callback);

        // share window
        void SelectShareWindow(int type, int id);
        void SetVideoCaptureSource(VIDEO_SOURCE video_source);
        void GetShareWindowList(std::vector<ShareWindowInfo> & list);

    protected:
        // Transport
        virtual int SendRtp(int channelId, const uint8_t* packet, size_t length, const PacketOptions* options = NULL) {return 0;};
        virtual int SendRtcp(int channelId, const uint8_t* packet, size_t length) {return 0;};
        virtual void SetRtpData(int channel,void *,int type);

        // video data callback
        virtual int32_t SendData(FrameType frame_type,
                uint8_t payload_type,
                uint32_t timestamp,
                const uint8_t* payload_data,
                size_t payload_len_bytes,
                const RTPFragmentationHeader* fragmentation);

        // audio data callback
        virtual int32_t SendData(uint8_t payloadType,
                const EncodedImage& encoded_image,
                const RTPFragmentationHeader& fragmentationHeader,
                const RTPVideoHeader* rtpVideoHdr);

        // EC_ReceiverCallback
        void onAvcDataComing(void* nalu_data, int len, uint32_t timestamp);
        void on10MsecPcmDataComing(uint8_t* pData, int nLen, uint32_t ts, uint32_t sample_rate, int audio_channels);


    private:
        // camera capture
        int doCameraCapture();
        int shutdownCameraCapture();


        // video data preview render.
        int doCameraPreviewRender(int render_id);
        int shutdownCameraPreviewRender(int render_id);
                  
        int doPlayingPreviewRender(int render_id);
        int shutdownPlayingPreviewRender(int render_id);

        // desktop capture.
        int doDesktopCapture();
        int shutdownDesktopCapture();

        // init audio and video network
        int initAudioNetwork();
        int initVideoNetwork();
        int uninitAudioNetwork();
        int uninitVideoNetwork();

        // init audio and video codec
        int initAudioTransportCodec(const char *plname, int channels);
        int  initVideoTransportCodec(const char *plname, int plfreq);

        // audio playout
        int doAudioPlayout();
        int shutdownAudioPlayout();

        // video send and rceive
        int doVideoDataSend();
        int shutdownVideoDataSend();
        int doVideoDataReceive();
        int shutdownVideoDataReceive();

        // audio data send and receive.
        int doAudioDataSend();
        int shutdownAudioDataSend();
        int doAudioDataReceive();
        int shutdownAudioDataReceive();

        // audio engine init
        int initAudioEngine();
        int initVideoEngine();

        // video engine init
        int uninitAudioEngine();
        int uninitVideoEngine();

        // init camera device , to find how manny camera device can be used.
        int initCameraDevice();
        void uninitCameraDevice();

        // resample pcm data
        int32_t resamplePCMData(const void *audio_data, const size_t nSamples, const uint32_t samplesRate, const size_t nChannels);

    public:
        // capture start and stop
        int startCapture();
        int stopCapture();

        // playout start and stop
        int startPlayout();
        int stopPlayout();

        // set video preview view.
        int setVideoPreview(void *view);

    private:
        // audio and video engine
        VoiceEngine *voe_;
        VideoEngine *vie_;

        // channel
        int audio_channel_;
        int video_channel_;

        // video capture source
        VIDEO_SOURCE video_source_; // default camera

        std::vector<CameraInfo*> cameras_; // camera device

        int share_window_id_;
        DesktopShareType desktop_share_type_;

        uint16_t audio_rtp_seq_;
        uint16_t video_rtp_seq_;
        int capture_id_;
        int desktop_capture_id_;

        RtpData* audio_data_cb_;
        RtpData* video_data_cb_;

        void *faac_decode_handle_;
        void *faac_encode_handle_;
        unsigned long faac_encode_input_samples_;

        void *local_view_;
        unsigned int audio_sampleRate_;
        unsigned int audio_channels_ ;

        RingBuffer<uint8_t> playbuffer_;
        RingBuffer<uint8_t> recordbuffer_;

        acm2::ACMResampler resampler_record_;
#ifdef __ANDROID__
        char render_viewer_id[128];
#endif
    private:
        // video capture info
        int info_video_bitrates_;
        int info_video_width_;
        int info_video_height_;
        int info_video_fps_;
        int info_camera_index_;

        EC_CapturerCallback *capturer_data_callback_;
    };
}



#endif /* ec_media__machine_hpp */
