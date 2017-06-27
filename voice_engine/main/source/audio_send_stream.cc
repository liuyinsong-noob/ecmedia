#include "audio_send_stream.h"
#include "../system_wrappers/include/trace.h"
#include "../base/timeutils.h"

#include "voe_codec.h"
#include "voe_rtp_rtcp.h"
#include "voe_audio_processing.h"
#include "voe_volume_control.h"

#include <sstream>

extern char* filename_path;

namespace cloopenwebrtc{

	AudioSendStream::AudioSendStream(VoiceEngine* voe, int channel_id)
		: voe_(voe),
		  thread_(ThreadWrapper::CreateThread(AudioSendStream::AudioSendStatisticsThreadRun, this, kNormalPriority, "AudioSendStatisticsThread")),
		  channel_id_(channel_id),
		  clock_(Clock::GetRealTimeClock()),
		  last_process_time_(clock_->TimeInMilliseconds()),
		  crit_(CriticalSectionWrapper::CreateCriticalSection())
	{
		unsigned int id;
		updateEvent_ = EventWrapper::Create();
		thread_->Start(id);
	}

	AudioSendStream::~AudioSendStream()
	{
		thread_->SetNotAlive();
		updateEvent_->Set();

		if(thread_->Stop()) {
			delete updateEvent_;
			updateEvent_ = NULL;
			delete thread_;
			thread_ = NULL;
		}
		else {
			WEBRTC_TRACE(kTraceError, kTraceVoice, -1,
				"~AudioSendStream() failed to stop thread");
		}
	}

	bool AudioSendStream::AudioSendStatisticsThreadRun(void* obj)
	{
		 return static_cast<AudioSendStream*>(obj)->ProcessSendStatistics();
	}

	bool AudioSendStream::ProcessSendStatistics()
	{
		updateEvent_->Wait(100);
		const int64_t now = clock_->TimeInMilliseconds();
		const int64_t kUpdateIntervalMs = 1000; //1000ms定时器
		if (now >= last_process_time_ + kUpdateIntervalMs) {
			//统计各种发送信息
			last_process_time_ = now;
			CriticalSectionScoped lock(crit_.get());
			UpdateStats();
		}
		return true;
	}

	AudioSendStream::Stats AudioSendStream::GetStats(int64_t &timestamp) const
	{
		CriticalSectionScoped lock(crit_.get());
		timestamp = clock_->TimeInMilliseconds();
		return audioSendStats_;
	}

	void AudioSendStream::UpdateStats()
	{
		VoECodec *voe_codec = VoECodec::GetInterface(voe_);
		VoERTP_RTCP *voe_rtprtcp = VoERTP_RTCP::GetInterface(voe_);
		VoEAudioProcessing *voe_process = VoEAudioProcessing::GetInterface(voe_);
		VoEVolumeControl *voe_volume = VoEVolumeControl::GetInterface(voe_);

		if (voe_codec)
		{
			CodecInst sendcodec;
			voe_codec->GetSendCodec(channel_id_, sendcodec);
			audioSendStats_.codec_name = sendcodec.plname;
			voe_codec->Release();
		}

		if (voe_rtprtcp)
		{
			CallStatistics callstats;
			voe_rtprtcp->GetLocalSSRC(channel_id_, audioSendStats_.local_ssrc);
			voe_rtprtcp->GetRTCPStatistics(channel_id_, callstats);
			audioSendStats_.bytes_sent = callstats.bytesSent;
			audioSendStats_.packets_sent = callstats.packetsSent;
			audioSendStats_.jitter_ms = callstats.jitterSamples;
			audioSendStats_.ext_seqnum = callstats.extendedMax;
			audioSendStats_.packets_lost = callstats.cumulativeLost;
			audioSendStats_.fraction_lost = callstats.fractionLost;
			audioSendStats_.rtt_ms = callstats.rttMs;
			voe_rtprtcp->Release();
		}

		if (voe_process)
		{
			bool enabled = false;
			EcModes mode;
			voe_process->GetEcStatus(enabled, mode);
			if (enabled)
			{
				int dummy1=0;
				int dummy2=0;
				voe_process->GetEchoMetrics(audioSendStats_.echo_return_loss, audioSendStats_.echo_return_loss_enhancement, dummy1, dummy2);
				voe_process->GetEcDelayMetrics(audioSendStats_.echo_delay_median_ms, audioSendStats_.echo_delay_std_ms);

			}			
			voe_process->Release();
		}
		if (voe_volume)
		{
			uint32_t level = 0;
			voe_volume->GetSpeechInputLevelFullRange(level); //[0, 32768]
			audioSendStats_.audio_level = static_cast<int32_t>(level);
			voe_volume->Release();
		}
	}
}
