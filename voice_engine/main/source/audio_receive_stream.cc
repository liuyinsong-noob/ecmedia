#include "audio_receive_stream.h"

#include "../base/timeutils.h"
#include "../system_wrappers/include/trace.h"
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
namespace yuntongxunwebrtc {

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
	const int64_t kUpdateIntervalMs = 1000; //1000ms��ʱ��
	if (now >= last_process_time_ + kUpdateIntervalMs) {
		//ͳ�Ƹ��ַ�����Ϣ
		last_process_time_ = now;
		UpdateStats();
	}
	return true;
}

AudioReceiveStream::Stats AudioReceiveStream::GetStats(int64_t &timestamp) const
{
	CriticalSectionScoped lock(crit_.get());
	timestamp = clock_->TimeInMilliseconds();
             
	/*WEBRTC_TRACE(kTraceStream, kTraceVoice, -1, "this:%ld channelid:%d audio received: remote_ssrc(%d) packets_lost(%d) fraction_lost(%d)\n \
	 bytes_rcvd(%d) \n\
     packets_rcvd(%d) \n \
     ext_seqnum(%d) \n \
     jitter_ms(%d) \n \
     jitter_buffer_ms(%d) \n \
     jitter_buffer_preferred_ms(%d) \n \
     delay_estimate_ms(%d) \n \
     audio_level(%d) \n \
     expand_rate(%f) \n \
     speech_expand_rate(%f) \n \
     secondary_decoded_rate(%f) \n \
     accelerate_rate(%f) \n \
     preemptive_expand_rate(%f) \n \
     decoding_calls_to_silence_generator(%d) \n \
     decoding_calls_to_neteq(%d) \n \
     decoding_normal(%d) \n \
     decoding_plc(%d) \n \
     decoding_cng(%d) \n \
     decoding_plc_cng(%d) \n \
     capture_start_ntp_time_ms(%d) \n", (long)this, channel_id_,
           audioRecvStats_.remote_ssrc,
		    audioRecvStats_.packets_lost,
		    audioRecvStats_.fraction_lost,
            audioRecvStats_.bytes_rcvd,
            audioRecvStats_.packets_rcvd,
           audioRecvStats_. ext_seqnum,
            audioRecvStats_.jitter_ms,
            audioRecvStats_.jitter_buffer_ms,
            audioRecvStats_.jitter_buffer_preferred_ms,
            audioRecvStats_.delay_estimate_ms,
            audioRecvStats_.audio_level,
            audioRecvStats_.expand_rate,
            audioRecvStats_.speech_expand_rate,
            audioRecvStats_.secondary_decoded_rate,
            audioRecvStats_.accelerate_rate,
            audioRecvStats_.preemptive_expand_rate,
            audioRecvStats_.decoding_calls_to_silence_generator,
            audioRecvStats_.decoding_calls_to_neteq,
            audioRecvStats_.decoding_normal,
            audioRecvStats_.decoding_plc,
            audioRecvStats_.decoding_cng,
            audioRecvStats_.decoding_plc_cng,
           audioRecvStats_.capture_start_ntp_time_ms);*/
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
        
        audioRecvStats_.bitrate = callstats.received_bitrate;
		audioRecvStats_.bytes_rcvd = callstats.bytesReceived;
		audioRecvStats_.packets_rcvd = callstats.packetsReceived;
		audioRecvStats_.packets_lost = callstats.cumulativeLost;
		audioRecvStats_.fraction_lost = (int)callstats.fractionLost;
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
