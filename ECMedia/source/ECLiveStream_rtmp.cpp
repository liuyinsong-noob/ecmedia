//
//  rtmpsession.cpp
//  ECMedia
//
//  Created by james on 16/9/20.
//  Copyright 2016年 Cloopen. All rights reserved.
//

#include <stdio.h>
#include "ECMedia.h"
#include "ECLiveStream.h"
#include "librtmp/rtmp.h"
//#include "rtmp_sys.h"
#include "librtmp/log.h"
#include "thread_wrapper.h"
#include "voe_base.h"
#include "trace.h"
#include "voe_file.h"
#include "voe_encryption.h"
#include "voe_network.h"
#include "voe_audio_processing.h"
#include "voe_codec.h"
#include "voe_rtp_rtcp.h"
#include "voe_hardware.h"
#include "sleep.h"
#ifdef WIN32
#include "codingHelper.h"
#endif
#ifdef VIDEO_ENABLED
#include "vie_network.h"
#include "vie_base.h"
#include "vie_capture.h"
#include "vie_file.h"
#include "vie_render.h"
#include "vie_codec.h"
#include "vie_rtp_rtcp.h"
#include "webrtc_libyuv.h"
#include "vie_file_impl.h"
#include "vie_desktop_share_impl.h"
#include "vie_image_process_impl.h"
#endif

#include "clock.h"
#include "sdk_common.h"
#include "faaccodec.h"

#include "push_resampler.h"
//#include "base64.h"
#include "librtmp/rtmp.h"


namespace cloopenwebrtc {

	static RTMPLiveSession *g_rtmpLiveSession = NULL;
	int onReturnVideoWidthHeight(int width, int height, int channelid)
	{
		if (g_rtmpLiveSession && channelid == g_rtmpLiveSession->video_channel_
				&& g_rtmpLiveSession->remote_video_resoution_callback_)
			g_rtmpLiveSession->remote_video_resoution_callback_(g_rtmpLiveSession, width, height);

		return 0;
	}
    
    RTMPLiveSession::RTMPLiveSession(VoiceEngine * voe,VideoEngine *vie)
    :audio_channel_(-1)
	, video_channel_(-1)
	, voe_(voe), vie_(vie)
	, audio_data_cb_(nullptr)
	, video_data_cb_(nullptr)
	, playnetworkThread_(nullptr)
	, pushnetworkThread_(nullptr)
	, audio_rtp_seq_(0)
	, video_rtp_seq_(0)
	, faac_decode_handle_(nullptr)
	, faac_encode_handle_(nullptr)
	, video_window_(nullptr)
	, rtmph_(nullptr)
	, capture_id_(-1)
	, desktop_capture_id_(-1)
    , share_window_id_(-1)
	, live_mode_(MODE_LIVE_UNKNOW)
	, video_source_(VIDEO_SOURCE_CAMERA)
	, desktop_share_type_(ShareScreen)
	, local_view_(NULL)
	, clock_(Clock::GetRealTimeClock())
	, push_audio_(true)
	, push_video_(true)
	, push_video_bitrates_(2000)
	, push_video_width_(640)
	, push_video_height_(480)
	, push_video_fps_(15)
	, push_camera_index_(0)
	, last_receive_time_(0)
	, packet_timeout_ms_(10000) //10s
	, network_status_callbck_(nullptr)
	, remote_video_resoution_callback_(nullptr)
    {
#ifdef __APPLE__
        push_video_height_ = 640;
        push_video_width_ = 480;
        push_camera_index_ =1;
#endif
		rtmp_lock_ = CriticalSectionWrapper::CreateCriticalSection();
		g_rtmpLiveSession = this;
    }

	RTMPLiveSession::~RTMPLiveSession()
	{
		g_rtmpLiveSession = nullptr;
		if (rtmp_lock_) {
			delete rtmp_lock_;
			rtmp_lock_ = nullptr;
		}
		UnInit();

		int i;
		CameraInfo *camera;

		if (cameras_.size() != 0)
		{
			for (i = 0; i < cameras_.size(); i++)
			{
				camera = cameras_[i];
				delete camera;
				camera = NULL;
			}

			cameras_.clear();
		}
	}

    bool RTMPLiveSession::Init()
    { 
		PrintConsole("[RTMP INFO] %s begins...\n", __FUNCTION__);
		if (voe_ == NULL || vie_ == NULL) {
			PrintConsole("[RTMP ERROR] %s voe or vie is NULL\n", __FUNCTION__);
			return false;
		}
		if (audio_channel_ != -1) {
			PrintConsole("[RTMP ERROR] %s already init\n", __FUNCTION__);
			return false;
		}
        VoEBase *base = VoEBase::GetInterface(voe_);
        audio_channel_ = base->CreateChannel();
        base->Release();
		//AEC only support below 16000 k
		RegisterReceiveAudioCodec("L16", 16000, 2);

        VoENetwork *network = VoENetwork::GetInterface(voe_);
        network->RegisterExternalTransport(audio_channel_, *this);
		network->RegisterExternalPacketization(audio_channel_,this);
        network->Release();
        
        ViEBase *vbase = ViEBase::GetInterface(vie_);
        vbase->CreateChannel(video_channel_);
        vbase->Release();
        
        ViENetwork *vnetwork = ViENetwork::GetInterface(vie_);
        vnetwork->RegisterSendTransport(video_channel_, *this);
		vnetwork->RegisterExternalPacketization(video_channel_, this);
        vnetwork->Release();
 
        RegisterReceiveVideoCodec("H264",90000);   
        faac_encode_handle_ = faac_encoder_crate(16000, 2, &faac_encode_input_samples_);
        return true;
    }
	void RTMPLiveSession::UnInit()
	{
		if (audio_channel_ == -1 || video_channel_ == -1)
			return;
		
		audio_data_cb_ = nullptr;
		video_data_cb_ = nullptr;

		VoENetwork *network = VoENetwork::GetInterface(voe_);
		network->DeRegisterExternalTransport(audio_channel_);

		network->DeRegisterExternalPacketization(audio_channel_);
		network->Release();

		ViENetwork *vnetwork = ViENetwork::GetInterface(vie_);
		vnetwork->DeregisterSendTransport(video_channel_);
		vnetwork->Release();

		VoEBase *base = VoEBase::GetInterface(voe_);
		base->DeleteChannel(audio_channel_);
		base->Release();

		ViEBase *vbase = ViEBase::GetInterface(vie_);
		vbase->DeleteChannel(video_channel_);
		vbase->Release();

		audio_channel_ = -1;
		video_channel_ = -1;
		audio_rtp_seq_ = 0;
		video_rtp_seq_ = 0;

		playbuffer_.Clear();

		if (faac_decode_handle_) {
			faad_decoder_close(faac_decode_handle_);
			faac_decode_handle_ = nullptr;
		}
		if (faac_encode_handle_) {
			faac_encoder_close(faac_encode_handle_);
			faac_encode_handle_ = nullptr;
		}

	}

	int RTMPLiveSession::startCapture()
	{
		if (video_source_ == VIDEO_SOURCE_DESKTOP)
			startDesktopCapture();
		else
			startCameraCapture();
		return 0;
	}

	int RTMPLiveSession::stopCapture()
	{
		if (video_source_ == VIDEO_SOURCE_DESKTOP) {
			ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
#ifdef _WIN32
			ViERender* render = ViERender::GetInterface(vie_);
			render = ViERender::GetInterface(vie_);
			render->StopRender(desktop_capture_id_);
			render->RemoveRenderer(desktop_capture_id_);
#endif
			desktopShare->DisConnectDesktopCaptureDevice(video_channel_);
			desktopShare->StopDesktopShareCapture(desktop_capture_id_);
			desktopShare->ReleaseDesktopShareCapturer(desktop_capture_id_);
			desktopShare->Release();
			desktop_capture_id_ = -1;
			share_window_id_ = -1;
		}
		else {
#ifdef _WIN32
			ViERender *render = ViERender::GetInterface(vie_);
			render->StopRender(capture_id_);
			render->RemoveRenderer(capture_id_);
			render->Release();
#endif
			ViECapture *capture = ViECapture::GetInterface(vie_);
			capture->DisconnectCaptureDevice(video_channel_);
			capture->StopCapture(capture_id_);
			capture->ReleaseCaptureDevice(capture_id_);
			capture->Release();
			capture_id_ = -1;

		}
		return 0;
	}

	int RTMPLiveSession::startDesktopCapture()
	{
		ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
		if (desktop_capture_id_ >= 0) {
			desktopShare->StopDesktopShareCapture(desktop_capture_id_);
		}
		int ret = desktopShare->AllocateDesktopShareCapturer(desktop_capture_id_, desktop_share_type_);

		if (desktop_share_type_ == ShareWindow) {
			if (share_window_id_ == -1) {
				WindowList windows;
				desktopShare->GetWindowList(desktop_capture_id_, windows);
				for (int i = 0; i < windows.size();i++) {
					if (windows[i].title.find("cdr") != std::string::npos) {
						share_window_id_ = windows[i].id;
					}
				}

			}
			desktopShare->SelectWindow(desktop_capture_id_, share_window_id_);
		}
		else if (desktop_share_type_ == ShareScreen) {
			if (share_window_id_ == -1) {
				ScreenList screens;
				desktopShare->GetScreenList(desktop_capture_id_, screens);
                if (!screens.empty()) {
                   share_window_id_ = screens[0];
                } else {
                    /*iOS 上没有实现获取窗口list的代码，获取为空时，给share_window_id_随机赋值一个正值，防止访问screens[0] 崩溃*/
                    share_window_id_ = 1;
                }
			}
			desktopShare->SelectScreen(desktop_capture_id_, share_window_id_);
		}
		int width =0 , heigth = 0;
		desktopShare->GetDesktopShareCaptureRect(desktop_capture_id_, push_video_width_, push_video_height_);
		//push_video_width_ = 640;// push_video_width_ / 4 * 4;
		//push_video_height_ = 480;//;push_video_height_ / 4 * 4;
		push_video_bitrates_ = push_video_width_ * push_video_height_ * 0.07 *15 /1000;
		PrintConsole("desktop share width is %d heigth is %d \n", push_video_width_, push_video_height_);
		RegisterReceiveVideoCodec("H264", 90000);
		
		ret = desktopShare->StartDesktopShareCapture(desktop_capture_id_, 15);
		ret = desktopShare->ConnectDesktopCaptureDevice(desktop_capture_id_, video_channel_);

#ifdef WIN32
		ViERender* render = ViERender::GetInterface(vie_);
		ret = render->AddRenderer(desktop_capture_id_, local_view_, 1, 0, 0, 1, 1, NULL);
		if (ret) {
			render->Release();
			return ret;
		}
		ret = render->StartRender(desktop_capture_id_);
		render->Release();
		
#else
		//ret = desktopShare->SetLocalVideoWindow(desktop_capture_id_, local_view_);
#endif
		desktopShare->Release();
		return 0;
	}

	int RTMPLiveSession::startCameraCapture()
	{
		if (cameras_.size() == 0)
			GetAllCameraInfo();

		if (cameras_.size() <= push_camera_index_) {
			return -1;
		}
		ViECapture *capture = ViECapture::GetInterface(vie_);
		if (capture_id_ >= 0) {
			capture->StopCapture(capture_id_);
		}
		CameraInfo *camera = cameras_[push_camera_index_];
		int ret = capture->AllocateCaptureDevice(camera->id, sizeof(camera->id), capture_id_);

		RotateCapturedFrame tr;
		ret = capture->GetOrientation(camera->id, tr);
		capture->SetRotateCapturedFrames(capture_id_, tr);

		CaptureCapability cap;
		cap.height = push_video_height_;
		cap.width = push_video_width_;
		cap.maxFPS = push_video_fps_;
		ret = capture->StartCapture(capture_id_, cap);
		ret = capture->ConnectCaptureDevice(capture_id_, video_channel_);

#if true
		ViERender* render = ViERender::GetInterface(vie_);
		ret = render->AddRenderer(capture_id_, local_view_, 1, 0, 0, 1, 1, NULL);
		if (ret) {
			render->Release();
			return ret;
		}
		ret = render->StartRender(capture_id_);
		render->Release();
#else
		ret = capture->SetLocalVideoWindow(capture_id_, local_view_);
#endif
		capture->Release();

		return 0;
	}
    bool RTMPLiveSession::RegisterReceiveAudioCodec(const char * plname , int plfreq, int channels)
    {
        VoECodec *codec = VoECodec::GetInterface(voe_);
        CodecInst audioCodec;
        strcpy(audioCodec.plname,plname);
        audioCodec.pltype = 113;
        audioCodec.plfreq = plfreq;
        audioCodec.channels =channels;
        audioCodec.fecEnabled = false;
        audioCodec.pacsize = plfreq /100;
		audioCodec.rate = plfreq *16;
        int ret = codec->SetRecPayloadType(audio_channel_, audioCodec);
        ret = codec->SetSendCodec(audio_channel_, audioCodec);
        codec->Release();
        return (ret == 0 );

    }
    
    bool RTMPLiveSession::RegisterReceiveVideoCodec(const char * plname , int plfreq)
    {
        ViECodec *vcodec = ViECodec::GetInterface(vie_);
        VideoCodec videoCodec;
        memset(&videoCodec,0,sizeof(videoCodec));
        strcpy(videoCodec.plName,plname);
        videoCodec.plType = 98;
        videoCodec.codecType = kVideoCodecH264;
        videoCodec.width = push_video_width_;
        videoCodec.height = push_video_height_;
        videoCodec.startBitrate = push_video_bitrates_;
        videoCodec.maxBitrate = push_video_bitrates_*2;
        videoCodec.minBitrate = push_video_bitrates_/2;
        videoCodec.targetBitrate = push_video_bitrates_;
        videoCodec.maxFramerate = push_video_fps_;
		videoCodec.mode = kRealtimeVideo;

        int ret = vcodec->SetReceiveCodec(video_channel_, videoCodec);
        ret = vcodec->SetSendCodec(video_channel_, videoCodec);
        vcodec->Release();
        return (ret == 0 );
    }
    
    
    void RTMPLiveSession::SetRtpData(int channel,void *rtpdata,int type)
    {
        if( 0 == type  )
            audio_data_cb_ = (RtpData*)rtpdata;
        else
            video_data_cb_ = (RtpData*)rtpdata;
        
    }
    
	static PushResampler<int16_t> resampler;
    void RTMPLiveSession::HandleAuidoPacket(RTMPPacket *packet)
    {
        if(!audio_data_cb_)
            return;
        
        unsigned voiceCodec = ((unsigned char)packet->m_body[0]) >> 4;
        unsigned char pcmdata[40960];
        unsigned int payloadLen = packet->m_nBodySize-2 ;
        
        const uint8_t * payloadData = (const uint8_t*)packet->m_body+2;
        //static PushResampler<int16_t> resampler;
        int len = 0,ret = 0;
        uint8_t *audio_data_10ms;
        static int num = 0;
        switch (voiceCodec) {
            case 0:
				PrintConsole("[RTMP INFO] %s Linear PCM\n", __FUNCTION__);
                break;
            case 10: //AAC
                if ( !faac_decode_handle_ ) {
                    
					faad_decoder_getinfo(packet->m_body + 2, audio_sampleRate_, audio_channels_);
					faac_decode_handle_ = faad_decoder_create(audio_sampleRate_, audio_channels_,64000);
					faad_decoder_init(faac_decode_handle_, (unsigned char *)packet->m_body + 2, packet->m_nBodySize - 2, audio_sampleRate_, audio_channels_);
                    if( !RegisterReceiveAudioCodec("L16",32000, audio_channels_) ) {
						PrintConsole("[RTMP ERROR] %s register codec failed\n", __FUNCTION__);
                    }
                    VoEBase *base = VoEBase::GetInterface(voe_);
                    if (!base)
                        return ;
                    base->StartPlayout(audio_channel_);
                    base->Release();
                    resampler.InitializeIfNeeded(audio_sampleRate_, 32000, audio_channels_);
                }
                if( payloadLen <= 7 )
                    return;

                 ret = faad_decode_frame(faac_decode_handle_, (unsigned char *)packet->m_body+2, packet->m_nBodySize-2, pcmdata,&payloadLen);
				 if (ret < 0)
					 return;

                 if( payloadLen == 0  )
                     return;
                
                 playbuffer_.PushData( pcmdata, payloadLen);
                
                 while( audio_data_10ms = playbuffer_.ConsumeData(audio_sampleRate_ /100*4) )
                 {
			          len = resampler.Resample( (int16_t*)audio_data_10ms, audio_sampleRate_ /100* audio_channels_, (int16_t*)pcmdata,sizeof(pcmdata)/2);
					  if (len < 0) {
						  PrintConsole("[RTMP ERROR] %s resample error\n", __FUNCTION__);
						  return;
					  }
                     WebRtcRTPHeader rtpHeader;
                     rtpHeader.header.sequenceNumber = audio_rtp_seq_++;
                     rtpHeader.header.ssrc = 1;
                     rtpHeader.header.payloadType = 113;
                     rtpHeader.header.timestamp = 320 * rtpHeader.header.sequenceNumber;
                     audio_data_cb_->OnReceivedPayloadData((const uint8_t*)pcmdata, len*2, &rtpHeader);
					 //PrintConsole("[RTMP INFO] play audio\n");
                 }
                payloadData = pcmdata;
       
                break;
            default:
				PrintConsole("[RTMP ERROR] %s codec id %d not support\n", __FUNCTION__,voiceCodec);
                break;
        }

       // printf("sample rate is %d KHZ  timestamp %d\n", sampleRate, packet->m_nTimeStamp);
    }
    bool RTMPLiveSession::UnpackSpsPps(const char *data , std::vector<uint8_t> & sps_pps)
    {
        if(data[0]!= 1) {
			PrintConsole("[RTMP ERROR] %s SPS PPS version not correct\n", __FUNCTION__);
            return false;
        }
        int index = 5;
        int sps_num = (unsigned char) ( data[index++] & 0x1F );
        for (int i = 0 ; i < sps_num ; i++ ){
            int sps_len = ntohs(  *(unsigned short*)( data+index) );
            index += 2;
            sps_pps.insert(sps_pps.end(), data+index,  data+index+sps_len);
            index += sps_len;
        }
        sps_pps.push_back(0);
        sps_pps.push_back(0);
        sps_pps.push_back(0);
        sps_pps.push_back(1);
        int pps_num = (unsigned char) ( data[index++] );
        for (int i = 0 ; i < pps_num ; i++ ){
            int pps_len = ntohs(  *(unsigned short*)( data+index) );
            index += 2;
            sps_pps.insert(sps_pps.end(), data+index,  data+index+pps_len);
            index += pps_len;
        }
        return true;
    }
    bool RTMPLiveSession::UnPackNAL(const char *data, int data_size, std::vector<uint8_t> & nal)
    {
        int index =0 ;
        int count = 0;
        do {
            int nalu_size = ntohl( *(int*)(data+index));
            index += 4;
            if( index > 4 ) {
                nal.push_back(0);
                nal.push_back(0);
                nal.push_back(0);
                nal.push_back(1);
            }

            nal.insert(nal.end(), data+index, data+index+nalu_size);
            index += nalu_size;
        } while( index < data_size);
        
        if( index != data_size)
            return false;
        
        return true;
    }
    void RTMPLiveSession::HandleVideoPacket(RTMPPacket *packet)
    {
        unsigned frameType = ((unsigned char)packet->m_body[0]) >> 4;
        unsigned codecId = packet->m_body[0] &0xF;
        RTPHeader rtpHeader;
        unsigned char *payloadData;
        unsigned int payloadLen = 0;

        if(codecId == 7 ) {
            std::vector<uint8_t> sps_pps;
            std::vector<uint8_t> nal;
            switch( packet->m_body[1] ) {
                case 0:
                    UnpackSpsPps(packet->m_body+5, sps_pps);
                    payloadData = &sps_pps[0];
                    payloadLen = sps_pps.size();
                    break;
                case 1:
					if (!UnPackNAL(packet->m_body + 5, packet->m_nBodySize - 5, nal)) {
						PrintConsole("[RTMP ERROR] %s unpack nalu error\n", __FUNCTION__);
						return;
					}
                    payloadData = &nal[0];
                    payloadLen = nal.size();
                   /* if(frameType == 1) {
						PrintConsole("[RTMP INFO] %s key frame nalu  %d timestamp %d\n", __FUNCTION__, packet->m_nBodySize,packet->m_nTimeStamp);
                    }
					else {
						PrintConsole("[RTMP INFO] %s intra frame nalu len %d timestamp %d\n", __FUNCTION__, packet->m_nBodySize,packet->m_nTimeStamp);
					}*/
                    break;
                default:
					PrintConsole("[RTMP ERROR] %s codec %d not supported\n", __FUNCTION__, packet->m_body[1]);
                    return;
            }
            if( video_data_cb_) {
                rtpHeader.sequenceNumber = video_rtp_seq_++;
                rtpHeader.ssrc = 1;
                rtpHeader.payloadType = 98;
                rtpHeader.timestamp = packet->m_nTimeStamp *90;
                if (packet->m_body[1] == 1)
                {
                    rtpHeader.markerBit = true;
                }
                else
                {
                    rtpHeader.markerBit = false;
                }
                
                video_data_cb_->ReceivePacket((const uint8_t*) payloadData, payloadLen, rtpHeader,true);
            }
        }
    }
    
    bool RTMPLiveSession::PlayNetworkThread()
    {
		int64_t now = clock_->TimeInMilliseconds();
		if ((packet_timeout_ms_ != 0) &&
			(last_receive_time_ != 0) &&
			(now - last_receive_time_ > packet_timeout_ms_))
		{
			last_receive_time_ = 0;  // only one callback
			if (network_status_callbck_)
				network_status_callbck_(this, NET_STATUS_TIMEOUT);
			return true;
		}
	    RTMPPacket packet = { 0 };
        if(!RTMP_IsConnected(rtmph_) ) {

			PrintConsole("[RTMP ERROR] %s RTMP session not connected\n", __FUNCTION__);
			if(network_status_callbck_)
				network_status_callbck_(this, NET_STATUS_CONNECTING);
			RTMP_SetupURL(rtmph_, (char*)stream_url_.c_str());
			rtmph_->Link.timeout = 3; //connection timeout
			rtmph_->Link.lFlags |= RTMP_LF_LIVE;

			if (!RTMP_Connect(rtmph_, NULL)) {
				if( network_status_callbck_)
					network_status_callbck_(this, NET_STATUS_DISCONNECTED);
				PrintConsole("[RTMP ERROR] %s RTMP connected error\n", __FUNCTION__);
				SleepMs(1000); //try connect after 1s
				return true;
			}
			if (!RTMP_ConnectStream(rtmph_, 0)) {
				return true;
			}
			if(network_status_callbck_)
				network_status_callbck_(this, NET_STATUS_CONNECTED);

			RTMP_SetBufferMS(rtmph_, 3600 * 1000);
			ViEBase *vbase = ViEBase::GetInterface(vie_);
			vbase->StopReceive(video_channel_);
			vbase->StartReceive(video_channel_);
			vbase->Release();
        }
        if(!RTMP_ReadPacket(rtmph_, &packet)){
			RTMP_Close(rtmph_);
			PrintConsole("[RTMP ERROR] %s RTMP read packet failed\n", __FUNCTION__);
            return true;
        }
        if (!RTMPPacket_IsReady(&packet)) {
			PrintConsole("[RTMP ERROR] %s RTMP packet not ready\n", __FUNCTION__);
            return true;
        }
        if (!packet.m_nBodySize) {
			PrintConsole("[RTMP ERROR] %s RTMP packet not ready\n", __FUNCTION__);
			return true;
        }
		
		last_receive_time_ = clock_->TimeInMilliseconds();

		if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO) {
            HandleAuidoPacket(&packet);
        }
        else if(packet.m_packetType == RTMP_PACKET_TYPE_VIDEO){
            HandleVideoPacket(&packet);
        }
        else if(packet.m_packetType == RTMP_PACKET_TYPE_INFO){
            //printf("recv info packet\n");            
        }
        else {
            RTMP_ClientPacket(rtmph_, &packet);
        }
        RTMPPacket_Free(&packet);
        return true;
    }

	bool RTMPLiveSession::PushNetworkThread()
	{
		if (live_mode_ != MODE_LIVE_PUSH)
			return false;

		if (!RTMP_IsConnected(rtmph_)) {

			hasSend_SPS_PPS_ = false;
			hasSend_AAC_SPEC_= false;
			PrintConsole("[RTMP INFO] try to connect to server %s\n",stream_url_.c_str());
			if (network_status_callbck_)
				network_status_callbck_(this, NET_STATUS_CONNECTING);
			
			rtmp_lock_->Enter();
			RTMP_SetupURL(rtmph_, (char*)stream_url_.c_str());
			rtmph_->Link.timeout = 3; //connection timeout
			rtmph_->Link.lFlags |= RTMP_LF_LIVE;
			RTMP_EnableWrite(rtmph_);

			if (!RTMP_Connect(rtmph_, NULL)) {
				if (network_status_callbck_)
					network_status_callbck_(this, NET_STATUS_DISCONNECTED);
				rtmp_lock_->Leave();
				PrintConsole("[RTMP INFO] connect failed ,try after 1s\n");
				SleepMs(1000); // try connect after 1s
				return true;
			}
			if (!RTMP_ConnectStream(rtmph_, 0)) {
				rtmp_lock_->Leave();
				PrintConsole("[RTMP INFO] try to connect stream,but server close connection\n");
				return true;
			}
			rtmp_lock_->Leave();
			PrintConsole("[RTMP INFO] RTMP session connected\n");
			if (network_status_callbck_)
				network_status_callbck_(this, NET_STATUS_CONNECTED);
			RTMP_SetBufferMS(rtmph_, 3600 * 1000);
			Send_AAC_SPEC();
			Send_SPS_PPS();
		}
		SleepMs(1000);
		return true;
	}

	int RTMPLiveSession::setVideoProfile(int index, CameraCapability cam, int bitRates)
	{
		if( cameras_.size() == 0 )
			GetAllCameraInfo();

		if (cameras_.size() <= index) {
			return -1;
		}

		push_camera_index_ = index;
		push_video_width_ = cam.width;
		push_video_height_ = cam.height;
		push_video_fps_ = cam.maxfps;
		push_video_bitrates_ = bitRates;

		if (capture_id_ >= 0) {
			ViECapture *capture = ViECapture::GetInterface(vie_);
			capture->StopCapture(capture_id_);
			capture_id_ = -1;
			capture->Release();

			startCameraCapture();
		}
		RegisterReceiveVideoCodec("H264", 90000);
		return 0;
	}

	void RTMPLiveSession::setNetworkStatusCallBack(onLiveStreamNetworkStatusCallBack callbck)
	{
		network_status_callbck_ = callbck;
	}

	bool RTMPLiveSession::PlayNetworkThreadRun(void* pThis)
    {
        return static_cast<RTMPLiveSession*>(pThis)->PlayNetworkThread();
    }

	bool RTMPLiveSession::PushNetworkThreadRun(void* pThis)
	{
		return static_cast<RTMPLiveSession*>(pThis)->PushNetworkThread();
	}

	bool RTMPLiveSession::GetAllCameraInfo()
	{
		int i;
		CameraInfo *camera;

		if (cameras_.size() != 0)
		{
			for (i = 0; i < cameras_.size(); i++)
			{
				camera = cameras_[i];
				delete camera;
				camera = NULL;
			}

			cameras_.clear();
		}

		ViECapture *capture = ViECapture::GetInterface(vie_);
		for (i = 0; i < capture->NumberOfCaptureDevices(); i++) {
			camera = new CameraInfo;
			camera->index = i;
			capture->GetCaptureDevice(i, camera->name, sizeof(camera->name), camera->id, sizeof(camera->id));
			cameras_.push_back(camera);
		}
		capture->Release();
		return cameras_.size() != 0;
	}

	int32_t RTMPLiveSession::SendData(FrameType frame_type,
		uint8_t payload_type,
		uint32_t timestamp,
		const uint8_t* payload_data,
		size_t payload_len_bytes,
		const RTPFragmentationHeader* fragmentation)
	{
		//PrintConsole("[RTMP DEBUG] %s Send voice  data (%d) len(%d)\n", __FUNCTION__,frame_type,payload_len_bytes);
		uint8_t *pcmdata , *aac_data;
		int aac_data_len;

		recordbuffer_.PushData(payload_data, payload_len_bytes);

		while (pcmdata = recordbuffer_.ConsumeData(faac_encode_input_samples_*2))
		{
			aac_data_len = 0;
			faac_encode_frame(faac_encode_handle_, pcmdata, &aac_data, &aac_data_len);
			if (aac_data_len > 0) {
				SendAudioPacket(aac_data, aac_data_len);
			}
		}
		return 0;
	}

	int32_t RTMPLiveSession::SendData(uint8_t payloadType,
		const EncodedImage& encoded_image,
		const RTPFragmentationHeader& fragmentationHeader,
		const RTPVideoHeader* rtpVideoHdr)
	{
		//PrintConsole("[RTMP DEBUG] %s Send video  data (%d) len(%d)\n", __FUNCTION__, encoded_image._frameType , encoded_image._length);
		uint8_t *data = encoded_image._buffer;
		std::vector<char> body;
		std::vector<uint8_t> nalus;

		for (int i = 0; i < fragmentationHeader.fragmentationVectorSize; i++) {
			uint32_t nalu_size = fragmentationHeader.fragmentationLength[i];
			uint8_t * nalu = data + fragmentationHeader.fragmentationOffset[i];
			uint32_t naltype = nalu[0] & 0x1F;
			if (7 == naltype) { //sps
				sps_.clear();
				sps_.insert(sps_.end(), nalu, nalu + nalu_size);
			}
			else if (8 == naltype) {//pps
				pps_.clear();
				pps_.insert(pps_.end(), nalu, nalu + nalu_size);
				if (!hasSend_SPS_PPS_) {
					Send_SPS_PPS();
				}
			}
			else if( 5 == naltype || 1 == naltype || naltype > 23 ) {
				nalus.push_back((nalu_size >> 24) & 0xFF);
				nalus.push_back((nalu_size >> 16) & 0xFF);
				nalus.push_back((nalu_size >> 8) & 0xFF);
				nalus.push_back(nalu_size & 0xFF);
				nalus.insert(nalus.end(), nalu, nalu + nalu_size);
			}
		}
		if (nalus.size() > 0) {
			SendVideoPacket(nalus);
		}
		return 0;
	}
	
	int  RTMPLiveSession::Send_AAC_SPEC()
	{
		if (!push_audio_)
			return 0;

		RTMPPacket * packet = new RTMPPacket;
		memset(packet, 0, sizeof(RTMPPacket));

		std::vector<uint8_t> body;
		body.insert(body.end(), RTMP_MAX_HEADER_SIZE, 0);

		/*AF 00 + AAC Decode Spec*/
		body.push_back(0xAF);
		body.push_back(0x00);

		unsigned char *spec_buf;
		unsigned long  spec_len;
		if (faac_get_decoder_info(faac_encode_handle_, &spec_buf, &spec_len) < 0)
			return -1;

		body.insert(body.end(), spec_buf, spec_buf + spec_len);

		packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		packet->m_nChannel = 0x04;
		packet->m_nTimeStamp = 0;
		packet->m_hasAbsTimestamp = 0;
		packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
		packet->m_nInfoField2 = rtmph_->m_stream_id;
		packet->m_nBodySize = body.size() - RTMP_MAX_HEADER_SIZE;
		packet->m_body = ((char*)&body[0]) + RTMP_MAX_HEADER_SIZE;

		rtmp_lock_->Enter();
		if (RTMP_IsConnected(rtmph_) && RTMP_SendPacket(rtmph_, packet, TRUE)) {
			PrintConsole("[RTMP INFO] Send AAC SPEC to server ok!\n");
			hasSend_AAC_SPEC_ = true;
		}
		rtmp_lock_->Leave();
		delete packet;
		return 0;
	}
	int RTMPLiveSession::SendAudioPacket(unsigned char *aac_data, int aac_data_len)
	{
		if (!hasSend_AAC_SPEC_)
			return 0;

		static int64_t start_time = 0;
		if (start_time == 0)
			start_time = clock_->TimeInMilliseconds();

		uint32_t timestamp = clock_->TimeInMilliseconds() - start_time;

		RTMPPacket * packet = new RTMPPacket;
		memset(packet, 0, sizeof(RTMPPacket));

		std::vector<uint8_t> body;
		body.insert(body.end(), RTMP_MAX_HEADER_SIZE, 0);

		/*AF 00 + AAC Decode Spec*/
		body.push_back(0xAF);
		body.push_back(0x01);
		body.insert(body.end(), aac_data, aac_data + aac_data_len);
		packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		packet->m_nChannel = 0x04;
		packet->m_nTimeStamp = timestamp;
		packet->m_hasAbsTimestamp = 1;
		packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
		packet->m_nInfoField2 = rtmph_->m_stream_id;
		packet->m_nBodySize = body.size() - RTMP_MAX_HEADER_SIZE;
		packet->m_body = ((char*)&body[0]) + RTMP_MAX_HEADER_SIZE;

		rtmp_lock_->Enter();
		if (RTMP_IsConnected(rtmph_)) // put judge here in case of video/audio to close socket twice.
			RTMP_SendPacket(rtmph_, packet, TRUE);
		rtmp_lock_->Leave();
		delete packet;
		return 0;

	}

	int RTMPLiveSession::Send_SPS_PPS()
	{
		if (sps_.size() == 0 || pps_.size() == 0)
			return 0;
		
		char *sps = &sps_[0];
		int sps_len = sps_.size();

		char *pps = &pps_[0];
		int pps_len = pps_.size();

		RTMPPacket * packet = new RTMPPacket;
		memset(packet, 0, sizeof(RTMPPacket));
		std::vector<uint8_t> body;
		body.insert(body.end(), RTMP_MAX_HEADER_SIZE, 0);
		body.push_back(0x17);
		body.push_back(0x0);
		//composite time
		body.push_back(0x0);
		body.push_back(0x0);
		body.push_back(0x0);

		/*AVCDecoderConfigurationRecord*/
		body.push_back(0x1);
		body.push_back(sps[1]);
		body.push_back(sps[2]);
		body.push_back(sps[3]);
		body.push_back(0xff);

		/*sps*/
		body.push_back(0xe1);
		body.push_back((sps_len >> 8) & 0xff);
		body.push_back(sps_len & 0xff);
		body.insert(body.end(), sps, sps + sps_len);

		/*pps*/
		body.push_back(0x01);
		body.push_back((pps_len >> 8) & 0xff);
		body.push_back((pps_len) & 0xff);
		body.insert(body.end(), pps, pps + pps_len);

		packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
		packet->m_nChannel = 0x04;
		packet->m_nTimeStamp = 0;
		packet->m_hasAbsTimestamp = 0;
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
		packet->m_nInfoField2 = rtmph_->m_stream_id;
		packet->m_nBodySize = body.size() - RTMP_MAX_HEADER_SIZE;
		packet->m_body =((char*) &body[0]) + RTMP_MAX_HEADER_SIZE;

		//this->HandleVideoPacket(packet);
		
		rtmp_lock_->Enter();
		if (RTMP_IsConnected(rtmph_) && RTMP_SendPacket(rtmph_, packet, TRUE)) {
			PrintConsole("[RTMP INFO] Send SPS PPS to server ok!\n");
			hasSend_SPS_PPS_ = true;
		}
		rtmp_lock_->Leave();
		delete packet;
		return 0;
	}
	int RTMPLiveSession::SendVideoPacket(std::vector<uint8_t> &nalus)
	{
		if (!hasSend_SPS_PPS_)
			return 0;
		
		static int64_t start_time = 0;
		if (start_time == 0)
			start_time = clock_->TimeInMilliseconds();

		uint32_t timestamp = clock_->TimeInMilliseconds() - start_time;

		RTMPPacket * packet = new RTMPPacket;
		memset(packet, 0, sizeof(RTMPPacket));
		std::vector<uint8_t> body;
		body.insert(body.end(), RTMP_MAX_HEADER_SIZE, 0);
		int naltype = nalus[4] & 0x1F;
		if (naltype == 5)
			body.push_back(0x17); //key frame
		else if (naltype == 1)
			body.push_back(0x27);
		else
			return 0;
		body.push_back(0x01); // AVC NALU

		body.push_back(0x00); //composite time
		body.push_back(0x00);
		body.push_back(0x00);

		body.insert(body.end(), nalus.begin(),nalus.end());

		packet->m_hasAbsTimestamp = 1;
		packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
		packet->m_nInfoField2 = rtmph_->m_stream_id;
		packet->m_nChannel = 0x04;
		packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
		packet->m_nTimeStamp = timestamp;
		packet->m_nBodySize = body.size() - RTMP_MAX_HEADER_SIZE;
		packet->m_body = ((char*)&body[0]) + RTMP_MAX_HEADER_SIZE;

		
		//this->HandleVideoPacket(packet);
		rtmp_lock_->Enter();
		//PrintConsole("Send video data \n");
		if (RTMP_IsConnected(rtmph_) && RTMP_SendPacket(rtmph_, packet, TRUE)) {
			//PrintConsole("Send video data timestamp %d %d  \n", timestamp, packet->m_nBodySize);
		}
		rtmp_lock_->Leave();
		delete packet;
		return 0;
	}

	int  RTMPLiveSession::PushStream(const std::string &url, void *localview)
	{
		if (video_source_ == VIDEO_SOURCE_CAMERA)
			if (!GetAllCameraInfo())
				return -5;

		if (!Init())
			return -1;

		if (rtmph_ != NULL)  //already in playing state
			return -2;
		
		hasSend_SPS_PPS_ = false;
		hasSend_AAC_SPEC_= false;
		//RTMP_LogSetLevel(RTMP_LOGALL);
		live_mode_ = MODE_LIVE_PUSH;
		stream_url_ = url;
		rtmph_ = RTMP_Alloc();
		RTMP_Init(rtmph_);
		
		pushnetworkThread_ = ThreadWrapper::CreateThread(RTMPLiveSession::PushNetworkThreadRun,
			this,
			kHighestPriority,
			"RtmpPushThead");
		if (pushnetworkThread_ == NULL) {
			StopPush();
			PrintConsole("[RTMP ERROR] %s CreateThread failed\n", __FUNCTION__);
			return -1;
		}

		if (push_video_) {
			local_view_ = localview;
			startCapture();

			ViEBase *vbase = ViEBase::GetInterface(vie_);
			vbase->StartSend(video_channel_);
			vbase->Release();
		}

		if (push_audio_) {
			VoEBase *base = VoEBase::GetInterface(voe_);
			base->StartRecord();
			base->StartSend(audio_channel_);
			base->Release();
		}

		unsigned int thread_id = 0;
		pushnetworkThread_->Start(thread_id);

		return 0;
	}

    int RTMPLiveSession::PlayStream(const std::string &url,void *view, onLiveStreamVideoResolution callback)
    {
        if( !Init() )
            return -1;
        
        if( rtmph_ != NULL)  //already in playing state
            return -2;
		live_mode_ = MODE_LIVE_PLAY;
		remote_video_resoution_callback_ = callback;

		stream_url_ = url;
        rtmph_ = RTMP_Alloc();
        RTMP_Init(rtmph_);
        playnetworkThread_ = ThreadWrapper::CreateThread(RTMPLiveSession::PlayNetworkThreadRun,
                                                     this,
                                                     kHighestPriority,
                                                     "PrtmpPlayThread");
		if (playnetworkThread_ == NULL) {
			StopPlay();
			PrintConsole("[RTMP ERROR] %s CreateThread failed\n", __FUNCTION__);
			return -1;
		}
        video_window_ = view;
        if( video_window_ ) {
            ViERender *render = ViERender::GetInterface(vie_);
            render->AddRenderer(video_channel_, video_window_, 2, 0, 0, 1, 1, onReturnVideoWidthHeight);
            render->StartRender(video_channel_);
            render->Release();
        }

        VoEHardware *hardware = VoEHardware::GetInterface(voe_);
        hardware->SetLoudspeakerStatus(true);
        hardware->Release();      
        
        unsigned int thread_id = 0;
        bool success = playnetworkThread_->Start(thread_id);
		if (!success) {
			StopPlay();
            return -1;
		}
		last_receive_time_ = clock_->TimeInMilliseconds();
        return 0;
    }
   
    void  RTMPLiveSession::StopPlay()
    {
		if (live_mode_ != MODE_LIVE_PLAY)
			return;
		live_mode_ = MODE_LIVE_UNKNOW;

		VoEBase *base = VoEBase::GetInterface(voe_);
		base->StopPlayout(audio_channel_);
		base->StopReceive(audio_channel_);
		base->Release();

		ViERender *render = ViERender::GetInterface(vie_);
        render->StopRender(video_channel_);
		render->RemoveRenderer(video_channel_);
		render->Release();
		
		ViEBase *vbase = ViEBase::GetInterface(vie_);
		vbase->StopReceive(video_channel_);
		vbase->Release();

		if (playnetworkThread_) {
			playnetworkThread_->Stop();
			SleepMs(2000);
		}
		delete playnetworkThread_;
		playnetworkThread_ = NULL;

		UnInit(); //must before close rtmp connection

		if (rtmph_) {
			RTMP_Close(rtmph_);
			RTMP_Free(rtmph_);
			rtmph_ = NULL;
		}
    }
	
	void RTMPLiveSession::StopPush()
	{
		if (live_mode_ != MODE_LIVE_PUSH)
			return;
		live_mode_ = MODE_LIVE_UNKNOW;

		if (pushnetworkThread_)
			pushnetworkThread_->Stop();
		delete pushnetworkThread_;
		pushnetworkThread_ = NULL;
		
		hasSend_AAC_SPEC_ = false;
		hasSend_SPS_PPS_ = false;

		RTMPSockBuf_Close(&rtmph_->m_sb);
		rtmph_->m_sb.sb_socket = -1;

		stopCapture();

		ViEBase *vbase = ViEBase::GetInterface(vie_);
		vbase->StopSend(video_channel_);
		vbase->Release();

		VoEBase *base = VoEBase::GetInterface(voe_);
		base->StopRecord();
		base->StopSend(audio_channel_);
		base->Release();


		UnInit(); //must before close rtmp connection
		if (rtmph_) {
			RTMP_Close(rtmph_);
			RTMP_Free(rtmph_);
			rtmph_ = NULL;
		}
	}

	void RTMPLiveSession::SetPushContent(bool push_audio,bool push_video)
	{
		push_video_ = push_video;
		push_audio_ = push_audio;
	}

	void RTMPLiveSession::SetVideoSource(VIDEO_SOURCE video_source)
	{
		if(  MODE_LIVE_UNKNOW == live_mode_ )
			video_source_ = video_source;
	}

	void RTMPLiveSession::SelectShareWindow(int type, int id)
	{
		share_window_id_ = id;
		if (desktop_capture_id_ < 0)
			return;
		ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
		if (DesktopShareType::ShareScreen == type) {
			desktopShare->SelectScreen(desktop_capture_id_, id);
		}
		else if (DesktopShareType::ShareWindow == type) {
			desktopShare->SelectWindow(desktop_capture_id_, id);
		}
		desktopShare->Release();

	}
	void RTMPLiveSession::GetShareWindowList(std::vector<ShareWindowInfo> & list)
	{
		int capture_id;
		ViEDesktopShare *desktopShare = ViEDesktopShare::GetInterface(vie_);
		if (desktop_capture_id_ >= 0) {
			capture_id = desktop_capture_id_;
		}
		else {
			desktopShare->AllocateDesktopShareCapturer(capture_id, desktop_share_type_);
		}
		ShareWindowInfo win_info;
		ScreenList screens;
		desktopShare->GetScreenList(capture_id, screens);
		for (int i = 0; i < screens.size(); i++) {
			win_info.id = screens[i];
			win_info.name = "desktop";
			win_info.type = ShareScreen;
			list.push_back(win_info);
		}
		WindowList windows;
		desktopShare->GetWindowList(capture_id, windows);
		for (int i = 0; i < windows.size(); i++) {
			win_info.id = windows[i].id;
			win_info.name = windows[i].title;
			win_info.type = ShareWindow;
			list.push_back(win_info);
		}
		if (desktop_capture_id_ < 0) {
			desktopShare->ReleaseDesktopShareCapturer(capture_id);
		}
		desktopShare->Release();
	}

	void RTMPLiveSession::EnableBeauty()
	{
		ViECapture *capture = ViECapture::GetInterface(vie_);
		if (capture_id_ >= 0) {
			capture->EnableBeautyFilter(capture_id_, true);
		}
		capture->Release();
	}

	void RTMPLiveSession::DisableBeauty()
	{
		ViECapture *capture = ViECapture::GetInterface(vie_);
		if (capture_id_ >= 0) {
			capture->EnableBeautyFilter(capture_id_, false);
		}
		capture->Release();
	}
}
