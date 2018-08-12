#ifndef __servicecoreVideo__RecordLocal__
#define __servicecoreVideo__RecordLocal__

#include "../system_wrappers/include/event_wrapper.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include "../system_wrappers/include/list_wrapper.h"
#include "video_coding_defines.h"
#include "h264_record.h"
#include "voe_base.h"
#include "vie_base.h"
#include "sdk_common.h"

using namespace yuntongxunwebrtc;

namespace yuntongxunwebrtc {

	class RecordLocal : public Transport, 
		public AudioPacketizationCallback,
		public VCMPacketizationCallback
	{
	public:
		RecordLocal();
		~RecordLocal();

		void Stop();
		int Start(const char* filename, void *localview=NULL);
		static bool RecordVideoThreadRun(void* obj);
		static bool RecordAudioThreadRun(void* obj);

	protected:
       virtual int SendRtp(int channelId, const uint8_t* packet, size_t length, const PacketOptions* options = NULL) { return 0; }
       virtual int SendRtcp(int channelId, const uint8_t* packet, size_t length) { return 0; }

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

	private:
		bool isRecording();

		bool InitCapture();
		void UnInitCapture();
		int startCapture();
		int stopCapture();
		int startCameraCapture();
		bool GetAllCameraInfo();
		bool RegisteVideoCodec(const char * plname);
		bool RegisterAudioCodec(const char * plname, int plfreq, int channels);

		bool InitRecord();
		void UnInitRecord();
		bool StartRecord();
		void StopRecord();
		bool ProcessVideoData();
		bool ProcessAudioData();

	private:
		void *local_view_;
		VoiceEngine *voe_;
		VideoEngine *vie_;
		char localFileName_[256];

		int push_camera_index_;
		std::vector<CameraInfo*> cameras_;

		int capture_id_;
		int audio_channel_;
		int video_channel_;
		bool isRecording_;

		int push_video_width_;
		int push_video_height_;
		int push_video_bitrates_;
		int push_video_fps_;

		int audioDataLen_;   
		int audioSamplingFreq_;
		ListWrapper videoList_;
		ListWrapper audioList_;
		h264_record *h264RecordLocal_;
		ThreadWrapper *videoThread_;
		ThreadWrapper *audioThread_;
		EventWrapper *videoEvent_;
		EventWrapper *audioEvent_;
		CriticalSectionWrapper* recordCrit_;
        
	};

}

#endif /* defined(__servicecoreVideo__RecordLocal__) */
