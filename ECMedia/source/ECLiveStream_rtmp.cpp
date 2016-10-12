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
#include "vie_image_process.h"
#endif

#include "clock.h"
#include "sdk_common.h"
#include "faaccodec.h"
#include "acm_resampler.h"
#include "push_resampler.h"
//#include "base64.h"
#include "rtmp.h"


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
	, networkThread_(nullptr)
	, audio_rtp_seq_(0)
	, video_rtp_seq_(0)
	, faac_decode_handle_(nullptr)
	, video_window_(nullptr)
	, rtmph_(nullptr)
	, capture_id_(-1)
	, clock_(Clock::GetRealTimeClock())
	, push_video_bitrates_(10000)
	, push_video_width_(640)
	, push_video_height_(480)
	, push_video_fps_(15)
	, push_camera_index_(0)
	, last_receive_time_(0)
	, packet_timeout_ms_(10000) //10s
	, network_status_callbck_(nullptr)
	, remote_video_resoution_callback_(nullptr)
    {
		g_rtmpLiveSession = this;
    }

	RTMPLiveSession::~RTMPLiveSession()
	{
		g_rtmpLiveSession = nullptr;
		UnInit();
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
		RegisterReceiveAudioCodec("L16", 16000, 2);

		faac_encode_handle_ = faac_encoder_crate(16000, 2, &faac_encode_input_samples_);
		rtmp_lock_ = CriticalSectionWrapper::CreateCriticalSection();

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

		if (rtmp_lock_) {
			delete rtmp_lock_;
			rtmp_lock_ = nullptr;
		}
	}

	int RTMPLiveSession::startCaputre()
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
		CaptureCapability cap;
		cap.height = push_video_height_;
		cap.width = push_video_width_;
		cap.maxFPS = push_video_fps_;
		ret = capture->StartCapture(capture_id_, cap);
		ret = capture->ConnectCaptureDevice(capture_id_, video_channel_);

#ifdef WIN32
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
		capture->Release();
#endif
		capture->Release();

		return 0;
	}
    bool RTMPLiveSession::RegisterReceiveAudioCodec(const char * plname , int plfreq, int channels)
    {
        VoECodec *codec = VoECodec::GetInterface(voe_);
        CodecInst audioCodec;
        strcpy(audioCodec.plname,plname);
        audioCodec.pltype = 108;
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
    
    void RTMPLiveSession::HandleAuidoPacket(RTMPPacket *packet)
    {
        if(!audio_data_cb_)
            return;
        
        unsigned voiceCodec = ((unsigned char)packet->m_body[0]) >> 4;
        unsigned sampleRate = ( 1 << ((packet->m_body[0]  &0xC ) >>2)  ) *5512.5;
        unsigned channels = (packet->m_body[0] &0x1) +1;
        unsigned char pcmdata[40960];
        unsigned int payloadLen = packet->m_nBodySize-2 ;
        
        const uint8_t * payloadData = (const uint8_t*)packet->m_body+2;
        static PushResampler<int16_t> resampler;
        int len = 0;
        uint8_t *audio_data_10ms;
        static FILE *fp1,*fp2;
        static int num = 0;
        switch (voiceCodec) {
            case 0:
				PrintConsole("[RTMP INFO] %s Linear PCM\n", __FUNCTION__);
                break;
            case 10: //AAC
                if ( !faac_decode_handle_ ) {
                    faac_decode_handle_ = faad_decoder_create(sampleRate,channels,64000);

                    if( !RegisterReceiveAudioCodec("L16",32000,channels) ) {
						PrintConsole("[RTMP ERROR] %s register codec failed\n", __FUNCTION__);
                    }
                    VoEBase *base = VoEBase::GetInterface(voe_);
                    if (!base)
                        return ;
                    base->StartPlayout(audio_channel_);
                    base->Release();
                    resampler.InitializeIfNeeded(sampleRate, 32000, channels);
                }
                if( payloadLen <= 7 )
                    return;

                 len = faad_decode_frame(faac_decode_handle_, (unsigned char *)packet->m_body+2, packet->m_nBodySize-2, pcmdata,&payloadLen);
                 if( payloadLen == 0 )
                     return;
                
                 playbuffer_.PushData( pcmdata, payloadLen);
                
                 while( audio_data_10ms = playbuffer_.ConsumeData(sampleRate/100*4) )
                 {
                      len = resampler.Resample( (int16_t*)audio_data_10ms, sampleRate/100*2, (int16_t*)pcmdata,sizeof(pcmdata)/2);
                     WebRtcRTPHeader rtpHeader;
                     rtpHeader.header.sequenceNumber = audio_rtp_seq_++;
                     rtpHeader.header.ssrc = 1;
                     rtpHeader.header.payloadType = 108;
                     rtpHeader.header.timestamp = 320 * rtpHeader.header.sequenceNumber;
                     audio_data_cb_->OnReceivedPayloadData((const uint8_t*)pcmdata, len*2, &rtpHeader);
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
        do {
            int nalu_size = ntohl( *(int*)(data+index));
            index += 4;
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
                    if(frameType == 1) {
						PrintConsole("[RTMP ERROR] %s key frame nalu ,len = %d (%d)\n", __FUNCTION__, payloadLen, packet->m_nBodySize);
                    }
					else {
						PrintConsole("[RTMP ERROR] %s intra frame nalu ,len = %d (%d)\n", __FUNCTION__, payloadLen, packet->m_nBodySize);
					}
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
                rtpHeader.markerBit = true;
                video_data_cb_->ReceivePacket((const uint8_t*) payloadData, payloadLen, rtpHeader,true);
            }
        }
    }
    
    bool RTMPLiveSession::NetworkThread()
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
            return true;
        }
        if(!RTMP_ReadPacket(rtmph_, &packet)){
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

			startCaputre();
		}
		RegisterReceiveVideoCodec("H264", 90000);
		return 0;
	}

	void RTMPLiveSession::setNetworkStatusCallBack(onLiveStreamNetworkStatusCallBack callbck)
	{
		network_status_callbck_ = callbck;
	}

	bool RTMPLiveSession::NetworkThreadRun(void* pThis)
    {
        return static_cast<RTMPLiveSession*>(pThis)->NetworkThread();
    }

	bool RTMPLiveSession::GetAllCameraInfo()
	{
		if (cameras_.size() != 0)
			return true;
		ViECapture *capture = ViECapture::GetInterface(vie_);
		for (int i = 0; i < capture->NumberOfCaptureDevices(); i++) {
			CameraInfo *camera = new CameraInfo;
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
			PrintConsole("[RTMP DEBUG] aac_data_len(%d)\n", aac_data_len);
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
		std::vector<char> sps, pps,body;
		std::vector<uint8_t> nalus;

		for (int i = 0; i < fragmentationHeader.fragmentationVectorSize; i++) {
			uint32_t nalu_size = fragmentationHeader.fragmentationLength[i];
			uint8_t * nalu = data + fragmentationHeader.fragmentationOffset[i];
			uint32_t naltype = nalu[0] & 0x1F;
			if (7 == naltype) { //sps
				sps.insert(sps.end(), nalu, nalu + nalu_size);
			}
			else if (8 == naltype) {//pps
				pps.insert(pps.end(), nalu, nalu + nalu_size);
				if (!hasSend_SPS_PPS_) {
					Send_SPS_PPS(&sps[0], sps.size(), &pps[0], pps.size());
					PrintConsole("Send sps pps \n");
					hasSend_SPS_PPS_ = true;
				}
			}
			else if( 5 == naltype || 1 == naltype ) {
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
		RTMPPacket * packet =new RTMPPacket;
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
		RTMP_SendPacket(rtmph_, packet, TRUE);
		rtmp_lock_->Leave();
		delete packet;
		return 0;
	}
	int RTMPLiveSession::SendAudioPacket(unsigned char *aac_data, int aac_data_len)
	{
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
		RTMP_SendPacket(rtmph_, packet, TRUE);
		rtmp_lock_->Leave();
		delete packet;
		return 0;

	}

	int RTMPLiveSession::Send_SPS_PPS(char *sps, int sps_len,  char *pps, int pps_len)
	{
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
		RTMP_SendPacket(rtmph_, packet, TRUE);
		rtmp_lock_->Leave();
		delete packet;
		return 0;
	}
	int RTMPLiveSession::SendVideoPacket(std::vector<uint8_t> &nalus)
	{
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
		PrintConsole("Send video data timestamp %d %d\n", timestamp, packet->m_nBodySize);
		
		//this->HandleVideoPacket(packet);
		rtmp_lock_->Enter();
		RTMP_SendPacket(rtmph_, packet, TRUE);
		rtmp_lock_->Leave();
		delete packet;
		return 0;
	}

	int  RTMPLiveSession::PushStream(const std::string &url, void *localview)
	{
		if (!Init())
			return -1;

		if (rtmph_ != NULL)  //already in playing state
			return -2;

		if (!GetAllCameraInfo())
			return -5;
		hasSend_SPS_PPS_ = false;
		RTMP_LogSetLevel(RTMP_LOGALL);
		rtmph_ = RTMP_Alloc();
		RTMP_Init(rtmph_);
		RTMP_SetupURL(rtmph_, (char*)url.c_str());
		RTMP_EnableWrite(rtmph_);
		rtmph_->Link.timeout = 3; //connection timeout
		rtmph_->Link.lFlags |= RTMP_LF_LIVE;
#ifdef _WIN32
		if (!RTMP_Connect(rtmph_, NULL))
#else
		if (!RTMP_ConnectEx(rtmph_, NULL, 1000))
#endif 
		{
			printf("connect error\n");
			return -3;
		}
		if (!RTMP_ConnectStream(rtmph_, 0))
		{
			printf("connect stream error\n");
			return -4;
		}
		RTMP_SetBufferMS(rtmph_, 3600 * 1000);
		Send_AAC_SPEC();

		local_view_ = localview;
		if (capture_id_ < 0)
			startCaputre();

		ViEBase *vbase = ViEBase::GetInterface(vie_);
		int ret = vbase->StartSend(video_channel_);
		vbase->Release();

		VoEBase *base = VoEBase::GetInterface(voe_);
		ret = base->StartRecord();
		ret = base->StartSend(audio_channel_);
		base->Release();

		return 0;
	}

    int RTMPLiveSession::PlayStream(const std::string &url,void *view, onLiveStreamVideoResolution callback)
    {
        if( !Init() )
            return -1;
        
        if( rtmph_ != NULL)  //already in playing state
            return -2;
		remote_video_resoution_callback_ = callback;

		//hasSend_SPS_PPS_ = false;
        //RTMP_debuglevel = RTMP_LOGALL;
        rtmph_ = RTMP_Alloc();
        RTMP_Init(rtmph_);
        RTMP_SetupURL(rtmph_, (char*)url.c_str());
        rtmph_->Link.timeout = 3; //connection timeout
        rtmph_->Link.lFlags |= RTMP_LF_LIVE;

#ifdef _WIN32
		if (!RTMP_Connect(rtmph_, NULL))
#else
		if (!RTMP_ConnectEx(rtmph_, NULL, 1000))
#endif 
        {
			StopPlay();
			PrintConsole("[RTMP ERROR] %s connect error\n", __FUNCTION__);
			return -3;
        }
        if (!RTMP_ConnectStream(rtmph_, 0))
        {
			StopPlay();
			PrintConsole("[RTMP ERROR] %s connect stream error\n", __FUNCTION__);
			return -4;
        }
        
		RTMP_SetBufferMS(rtmph_, 3600*1000);

        networkThread_ = ThreadWrapper::CreateThread(RTMPLiveSession::NetworkThreadRun,
                                                     this,
                                                     kHighestPriority,
                                                     "RtmpLiveSession");
		if (networkThread_ == NULL) {
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
		ViEBase *vbase = ViEBase::GetInterface(vie_);
		vbase->StartReceive(video_channel_);

        VoEHardware *hardware = VoEHardware::GetInterface(voe_);
        hardware->SetLoudspeakerStatus(true);
        hardware->Release();
        
        unsigned int thread_id = 0;
        bool success = networkThread_->Start(thread_id);
		if (!success) {
			StopPlay();
            return -1;
		}
		last_receive_time_ = clock_->TimeInMilliseconds();
        return 0;
    }
    
    void  RTMPLiveSession::StopPlay()
    {

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

		if(networkThread_)
			networkThread_->Stop();
		delete networkThread_;
		networkThread_ = NULL;
		
		UnInit(); //must before close rtmp connection
		if (rtmph_) {
			RTMP_Close(rtmph_);
			RTMP_Free(rtmph_);
			rtmph_ = NULL;
		}

    }
	
	void RTMPLiveSession::StopPush()
	{
#ifdef _WIN32
		ViERender *render = ViERender::GetInterface(vie_);
		render->StopRender(capture_id_);
		render->RemoveRenderer(capture_id_);
		render->Release();
#endif

		ViECapture *capture = ViECapture::GetInterface(vie_);
		capture->StopCapture(capture_id_);
		capture->ReleaseCaptureDevice(capture_id_);
		capture->Release();
		capture_id_ = -1;

		ViEBase *vbase = ViEBase::GetInterface(vie_);
	    vbase->StopSend(video_channel_);
		vbase->Release();


		VoEBase *base = VoEBase::GetInterface(voe_);
		base->StopRecord();
		base->StopSend(audio_channel_);
		base->Release();



		if (networkThread_)
			networkThread_->Stop();
		delete networkThread_;
		networkThread_ = NULL;
		
		UnInit(); //must before close rtmp connection
		if (rtmph_) {
			RTMP_Close(rtmph_);
			RTMP_Free(rtmph_);
			rtmph_ = NULL;
		}

	}

}
