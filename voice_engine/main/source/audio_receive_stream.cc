#include "audio_receive_stream.h"

#include "timeutils.h"
#include "trace.h"
#include "voe_codec.h"
#include "voe_rtp_rtcp.h"
#include "voe_neteq_stats.h"
#include "voe_video_sync.h"
#include "voe_volume_control.h"

#include <sstream>

// Convert fixed point number with 8 bit fractional part, to floating point.
inline float Q8ToFloat(uint32_t v) {
	return static_cast<float>(v) / (1 << 8);
}

// Convert fixed point number with 14 bit fractional part, to floating point.
inline float Q14ToFloat(uint32_t v) {
	return static_cast<float>(v) / (1 << 14);
}

extern char* filename_path;
namespace cloopenwebrtc {

AudioReceiveStream::AudioReceiveStream(VoiceEngine* voe, int channelid)
	:  voe_(voe),
	   thread_(ThreadWrapper::CreateThread(AudioReceiveStream::AudioRecvStatisticsThreadRun, this, kNormalPriority, "AudioRecvStatisticsThread")),
       channel_id_(channelid),
	   clock_(Clock::GetRealTimeClock()),
	   last_process_time_(clock_->TimeInMilliseconds()),
	   crit_(CriticalSectionWrapper::CreateCriticalSection())
{
	unsigned int id;
	updateEvent_ = EventWrapper::Create();
	thread_->Start(id);
}

AudioReceiveStream::~AudioReceiveStream()
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

bool AudioReceiveStream::AudioRecvStatisticsThreadRun(void* obj)
{
	return static_cast<AudioReceiveStream*>(obj)->ProcessRecvStatistics();
}

bool AudioReceiveStream::ProcessRecvStatistics()
{
	updateEvent_->Wait(100);
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000; //1000ms定时器
	if (now >= last_process_time_ + kUpdateIntervalMs) {
		//统计各种发送信息
		last_process_time_ = now;
		UpdateStats();
	}
	return true;
}

AudioReceiveStream::Stats AudioReceiveStream::GetStats(int64_t &timestamp) const
{
	CriticalSectionScoped lock(crit_.get());
	timestamp = clock_->TimeInMilliseconds();
	return audioRecvStats_;
}

void AudioReceiveStream::UpdateStats()
{
	VoEBase *voe_base = VoEBase::GetInterface(voe_);
	if (voe_base)
	{
		if (!voe_base->GetChannel(channel_id_))
		{
			voe_base->Release();
			return;
		}
		voe_base->Release();
	}	
	
	VoECodec *voe_codec = VoECodec::GetInterface(voe_);
	VoERTP_RTCP *voe_rtprtcp = VoERTP_RTCP::GetInterface(voe_);
	VoENetEqStats* voe_neteq = VoENetEqStats::GetInterface(voe_);
	VoEVideoSync *voe_sync = VoEVideoSync::GetInterface(voe_);
	VoEVolumeControl *voe_volume = VoEVolumeControl::GetInterface(voe_);

	CodecInst codec;

	CriticalSectionScoped lock(crit_.get());
	if (voe_codec)
	{
		if (voe_codec->GetRecCodec(channel_id_, codec) < 0) {
			voe_codec->Release();
			return;
		}
		audioRecvStats_.codec_name = codec.plname;
		voe_codec->Release();
	}

	if (voe_rtprtcp)
	{
		CallStatistics callstats;
		voe_rtprtcp->GetRemoteSSRC(channel_id_, audioRecvStats_.remote_ssrc);
		voe_rtprtcp->GetRTCPStatistics(channel_id_, callstats);
		audioRecvStats_.bytes_rcvd = callstats.bytesReceived;
		audioRecvStats_.packets_rcvd = callstats.packetsReceived;
		audioRecvStats_.packets_lost = callstats.cumulativeLost;
		audioRecvStats_.fraction_lost = Q8ToFloat(callstats.fractionLost);
		audioRecvStats_.capture_start_ntp_time_ms = callstats.capture_start_ntp_time_ms_;
		audioRecvStats_.ext_seqnum = callstats.extendedMax;

		if (codec.plfreq / 1000 > 0) {
			audioRecvStats_.jitter_ms = callstats.jitterSamples / (codec.plfreq / 1000);
		}

		voe_rtprtcp->Release();
	}

	if (voe_neteq)
	{
		AudioDecodingCallStats decodingStats;
		NetworkStatistics networkStats;
		voe_neteq->GetDecodingCallStatistics(channel_id_, &decodingStats);
		voe_neteq->GetNetworkStatistics(channel_id_, networkStats);
		audioRecvStats_.decoding_calls_to_silence_generator = decodingStats.calls_to_silence_generator;
		audioRecvStats_.decoding_calls_to_neteq = decodingStats.calls_to_neteq;
		audioRecvStats_.decoding_normal = decodingStats.decoded_normal;
		audioRecvStats_.decoding_plc = decodingStats.decoded_plc;
		audioRecvStats_.decoding_cng = decodingStats.decoded_cng;
		audioRecvStats_.decoding_plc_cng = decodingStats.decoded_plc_cng;
		
		audioRecvStats_.jitter_buffer_ms = networkStats.currentBufferSize;
		audioRecvStats_.jitter_buffer_preferred_ms = networkStats.preferredBufferSize;
		audioRecvStats_.expand_rate = Q14ToFloat(networkStats.currentExpandRate);
		audioRecvStats_.accelerate_rate = Q14ToFloat(networkStats.currentAccelerateRate);
		audioRecvStats_.preemptive_expand_rate = Q14ToFloat(networkStats.currentPreemptiveRate);
		voe_neteq->Release();
	}

	if (voe_sync)
	{
		int jitter_delay_ms=0;
		int playout_delay_ms=0;
		voe_sync->GetDelayEstimate(channel_id_, &jitter_delay_ms, &playout_delay_ms);
		audioRecvStats_.delay_estimate_ms = jitter_delay_ms+playout_delay_ms;
		voe_sync->Release();
	}

	if (voe_volume)
	{
		uint32_t level = 0;
		voe_volume->GetSpeechOutputLevelFullRange(channel_id_, level); //[0, 32768]
		audioRecvStats_.audio_level = static_cast<int32_t>(level);
		voe_volume->Release();
	}
}
}
