#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "recordLocal.h"
#include "voe_network.h"
#include "voe_codec.h"
#include "vie_network.h"
#include "vie_capture.h"
#include "vie_rtp_rtcp.h"
#include "vie_renderer.h"
#include "vie_codec.h"
#include "ECMedia.h"

using namespace yuntongxunwebrtc;

extern yuntongxunwebrtc::VoiceEngine* m_voe;
extern yuntongxunwebrtc::VideoEngine* m_vie;


namespace yuntongxunwebrtc {

	RecordLocal::RecordLocal()
		:local_view_(NULL),
		capture_id_(-1),
		audio_channel_(-1),
		video_channel_(-1),
		isRecording_(false),
		push_video_width_(320),
		push_video_height_(240),
		push_video_fps_(15),
		push_video_bitrates_(2000),
		push_camera_index_(0),
		audioDataLen_(0),
		audioSamplingFreq_(-1),
		videoList_(),
		audioList_(),
		h264RecordLocal_(NULL),
		videoThread_(NULL),
		audioThread_(NULL),
		recordCrit_(CriticalSectionWrapper::CreateCriticalSection())
	{
#ifdef __APPLE__
		push_video_height_ = 640;
		push_video_width_ = 480;
		push_camera_index_ = 1;
#endif

#ifndef _WIN32
		signal(SIGPIPE, SIG_IGN);
#endif

		if (!m_voe) {
			ECMedia_init_audio();
		}
		if (!m_vie) {
			ECMedia_init_video();
		}

		voe_ = m_voe;
		vie_ = m_vie;
	}

	RecordLocal::~RecordLocal()
	{
		Stop();
	}

	bool RecordLocal::isRecording()
	{
		return isRecording_;
	}

	/**
	 * start record local camera video as MP4 file.
	 *
	 * @filename:  MP4 file path
	 * @localview: video preview's parent view.
	 */
	int RecordLocal::Start(const char* filename, void *localview)
	{
		if (!filename) {
			return -1;
		}
		sprintf(localFileName_, "%s", filename);
		local_view_ = localview;

		InitRecord();
		InitCapture();
		startCapture();

		isRecording_ = true;
		return 0;
	}

	/**
	 * stop record local camera video as MP4 file.
	 */
	void RecordLocal::Stop()
	{
		if (!isRecording())
			return;

		stopCapture();
		UnInitCapture();
		UnInitRecord();

		isRecording_ = false;
	}

	bool RecordLocal::InitCapture()
	{
		PrintConsole("[RECORD_LOCAL INFO] %s begins...\n", __FUNCTION__);
		if (voe_ == NULL || vie_ == NULL) {
			PrintConsole("[RECORD_LOCAL ERROR] %s voe or vie is NULL\n", __FUNCTION__);
			return false;
		}
		if (audio_channel_ != -1) {
			PrintConsole("[RECORD_LOCAL ERROR] %s already init\n", __FUNCTION__);
			return false;
		}
		VoEBase *base = VoEBase::GetInterface(voe_);
		audio_channel_ = base->CreateChannel();
		base->Release();

		VoENetwork *network = VoENetwork::GetInterface(voe_);
		network->RegisterExternalTransport(audio_channel_, *this);
		network->RegisterExternalPacketization(audio_channel_, this);
		network->Release();

		RegisterAudioCodec("L16", 8000, 1);

		ViEBase *vbase = ViEBase::GetInterface(vie_);
		vbase->CreateChannel(video_channel_);
		vbase->Release();

		ViENetwork *vnetwork = ViENetwork::GetInterface(vie_);
		vnetwork->RegisterSendTransport(video_channel_, *this);
		vnetwork->RegisterExternalPacketization(video_channel_, this);
		vnetwork->Release();

		RegisteVideoCodec("H264");
		return true;
	}

	void RecordLocal::UnInitCapture()
	{
		if (audio_channel_ == -1 || video_channel_ == -1)
			return;

		VoENetwork *network = VoENetwork::GetInterface(voe_);
		network->DeRegisterExternalTransport(audio_channel_);
		network->DeRegisterExternalPacketization(audio_channel_);
		network->Release();

		ViENetwork *vnetwork = ViENetwork::GetInterface(vie_);
		vnetwork->DeregisterSendTransport(video_channel_);
		vnetwork->DeRegisterExternalPacketization(video_channel_);
		vnetwork->Release();

		VoEBase *base = VoEBase::GetInterface(voe_);
		base->DeleteChannel(audio_channel_);
		base->Release();

		ViEBase *vbase = ViEBase::GetInterface(vie_);
		vbase->DeleteChannel(video_channel_);
		vbase->Release();

		audio_channel_ = -1;
		video_channel_ = -1;
	}

	int RecordLocal::startCapture()
	{
		startCameraCapture();

		ViEBase *vbase = ViEBase::GetInterface(vie_);
		vbase->StartSend(video_channel_);
		vbase->Release();

		VoEBase *base = VoEBase::GetInterface(voe_);
		base->StartRecord();
		base->StartSend(audio_channel_);
		base->Release();

		ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
		rtp_rtcp->RequestKeyFrame(video_channel_);
		rtp_rtcp->Release();

		return 0;
	}

	int RecordLocal::stopCapture()
	{
		if (local_view_)
		{
#ifdef _WIN32
			ViERender *render = ViERender::GetInterface(vie_);
			render->StopRender(capture_id_);
			render->RemoveRenderer(capture_id_);
			render->Release();
#endif
		}
		ViECapture *capture = ViECapture::GetInterface(vie_);
		capture->DisconnectCaptureDevice(video_channel_);
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

		return 0;
	}

	int RecordLocal::startCameraCapture()
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
		
		if (local_view_)
		{
#ifdef true
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
		}
		capture->Release();
		return 0;
	}

	bool RecordLocal::RegisterAudioCodec(const char * plname, int plfreq, int channels)
	{

		VoECodec *codec = VoECodec::GetInterface(voe_);
		CodecInst audioCodec;
		strcpy(audioCodec.plname, plname);
		audioCodec.pltype = 107;
		audioCodec.plfreq = plfreq;
		audioCodec.channels = channels;
		audioCodec.fecEnabled = false;
		audioCodec.pacsize = 160;
		audioCodec.rate = 128000;
		int ret = codec->SetSendCodec(audio_channel_, audioCodec);
		codec->Release();
		return (ret == 0);
	}

	bool RecordLocal::RegisteVideoCodec(const char * plname)
	{
		ViECodec *vcodec = ViECodec::GetInterface(vie_);
		VideoCodec videoCodec;
		memset(&videoCodec, 0, sizeof(videoCodec));
		strcpy(videoCodec.plName, plname);
		videoCodec.plType = 98;
		videoCodec.codecType = kVideoCodecH264;
		videoCodec.width = push_video_width_;
		videoCodec.height = push_video_height_;
		videoCodec.startBitrate = push_video_bitrates_;
		videoCodec.maxBitrate = push_video_bitrates_ * 2;
		videoCodec.minBitrate = push_video_bitrates_ / 2;
		videoCodec.targetBitrate = push_video_bitrates_;
		videoCodec.maxFramerate = push_video_fps_;
		videoCodec.mode = kRealtimeVideo;

		int ret = vcodec->SetSendCodec(video_channel_, videoCodec);
		vcodec->Release();
		return (ret == 0);
	}

	bool RecordLocal::GetAllCameraInfo()
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

	/* a callback of audio*/
	int32_t RecordLocal::SendData(FrameType frame_type,
		uint8_t payload_type,
		uint32_t timestamp,
		const uint8_t* payload_data,
		size_t payload_len_bytes,
		const RTPFragmentationHeader* fragmentation)
	{
		if (payload_data && payload_len_bytes > 0) {//payload_len_bytes=320
			audioDataLen_ = payload_len_bytes/2;//change to short
			audioSamplingFreq_ = 8000;
			int dataLen = payload_len_bytes;
			WebRtc_Word16 *data = (WebRtc_Word16 *)malloc(dataLen);
			memcpy(data, payload_data, dataLen);

			if (audioList_.GetSize() <= 100)
				audioList_.PushBack(data);

			audioEvent_->Set();
		}
		return 0;
	}

	/* A callback for video*/
	int32_t RecordLocal::SendData(uint8_t payloadType,
		const EncodedImage& encoded_image,
		const RTPFragmentationHeader& fragmentationHeader,
		const RTPVideoHeader* rtpVideoHdr)
	{
		//PrintConsole("[RECORD_LOCAL ERROR] %s video\n", __FUNCTION__);
		if (encoded_image._buffer && encoded_image._size > 0 && encoded_image._completeFrame) {
			EncodedVideoData *cloneData = new EncodedVideoData();
			cloneData->timeStamp = encoded_image._timeStamp;
			cloneData->encodedWidth = encoded_image._encodedWidth;
			cloneData->encodedHeight = encoded_image._encodedHeight;
			cloneData->completeFrame = encoded_image._completeFrame;
			cloneData->payloadSize = encoded_image._length + fragmentationHeader.fragmentationVectorSize * 4;
			cloneData->fragmentationHeader = fragmentationHeader;
			cloneData->payloadData = new WebRtc_UWord8[encoded_image._size + fragmentationHeader.fragmentationVectorSize * 4];
			uint8_t *data = encoded_image._buffer;

			uint8_t startcode[4] = { 0x0, 0x0, 0x0, 0x1 };
			uint32_t offset = 0;
			for (int i = 0; i < fragmentationHeader.fragmentationVectorSize; i++) {
				uint32_t nalu_size = fragmentationHeader.fragmentationLength[i];
				uint8_t * nalu = data + fragmentationHeader.fragmentationOffset[i];
				memcpy(cloneData->payloadData + offset, &startcode, 4);
				offset += 4;
				memcpy(cloneData->payloadData + offset, nalu, nalu_size);
				offset += nalu_size;
			}

			if (videoList_.GetSize() <= 100)
				videoList_.PushBack(cloneData);

			videoEvent_->Set();
		}

		return 0;
	}

/********************************************************************************************/

	bool RecordLocal::InitRecord()
	{		
		StartRecord();

		videoEvent_= EventWrapper::Create();
		audioEvent_= EventWrapper::Create();

		videoThread_ = ThreadWrapper::CreateThread(RecordLocal::RecordVideoThreadRun, this, kHighestPriority, "RecordVideo_Thread");
		audioThread_ = ThreadWrapper::CreateThread(RecordLocal::RecordAudioThreadRun, this, kHighestPriority, "RecordAudio_Thread");
		if (!videoThread_ || !audioThread_){
			PrintConsole("[RECORD_LOCAL ERROR] %s create video thread or audio thread failed", __FUNCTION__);
			return false;
		}

		unsigned int tid = 0;
		videoThread_->Start(tid);
		audioThread_->Start(tid);
		return true;
	}

	void RecordLocal::UnInitRecord()
	{
		StopRecord();

		videoThread_->SetNotAlive();
		videoEvent_->Set();

		audioThread_->SetNotAlive();
		audioEvent_->Set();

		if (videoThread_->Stop()) {
			delete videoEvent_;
			videoEvent_ = NULL;
			delete videoThread_;
			videoThread_ = NULL;
		}
		else {
			PrintConsole("[RECORD_LOCAL ERROR] %s RecordLocal failed to stop video thread, leaking\n", __FUNCTION__);
		}

		if (audioThread_->Stop()) {
			delete audioEvent_;
			audioEvent_ = NULL;
			delete audioThread_;
			audioThread_ = NULL;
		}
		else {
			PrintConsole("[RECORD_LOCAL ERROR] %s RecordLocal failed to stop audio thread, leaking\n", __FUNCTION__);
		}
	}
    bool isRecording = true;
	bool RecordLocal::StartRecord()
	{
		CriticalSectionScoped lock(recordCrit_);
		h264RecordLocal_ = new h264_record();
		if (!h264RecordLocal_) {
			PrintConsole("[RECORD_LOCAL ERROR] %s new h264_record failed", __FUNCTION__);
			return false;
		}
		h264RecordLocal_->init(localFileName_);
        return true;
	}

	void RecordLocal::StopRecord()
	{
		CriticalSectionScoped lock(recordCrit_);

		h264RecordLocal_->uninit();
		delete h264RecordLocal_;
		h264RecordLocal_ = NULL;
	}

	bool RecordLocal::RecordVideoThreadRun(void* obj)
	{
		return static_cast<RecordLocal*>(obj)->ProcessVideoData();
	}

	bool RecordLocal::RecordAudioThreadRun(void* obj)
	{
		return static_cast<RecordLocal*>(obj)->ProcessAudioData();
	}

	bool RecordLocal::ProcessVideoData()
	{
		if (videoEvent_->Wait(100) == kEventSignaled) {
			while (videoList_.GetSize()) {
				ListItem *storageItem = videoList_.First();
				if (storageItem) {
					EncodedVideoData *frame = (EncodedVideoData*)storageItem->GetItem();		
						CriticalSectionScoped lock(recordCrit_);
                    if (h264RecordLocal_){
                        h264RecordLocal_->wirte_video_data(frame->payloadData, frame->payloadSize, frame->timeStamp);
                        
                    }
					delete frame;
				}
				videoList_.PopFront();
			}
		}

		return true;
	}

	bool RecordLocal::ProcessAudioData()
	{
		if (audioEvent_->Wait(200) == kEventSignaled) {

			while (audioList_.GetSize()) {
				WebRtc_Word16 *recordingData = NULL;
				ListItem *recordItem = audioList_.First();
			
				if (recordItem) {
					recordingData = (WebRtc_Word16 *)recordItem->GetItem();
					audioList_.PopFront();
				}

				if (recordingData) {

					CriticalSectionScoped lock(recordCrit_);
                    if (h264RecordLocal_);
						h264RecordLocal_->write_audio_data(recordingData, audioDataLen_, audioSamplingFreq_);
				}
				free(recordingData);
			}
		}

		return true;
	}

}
