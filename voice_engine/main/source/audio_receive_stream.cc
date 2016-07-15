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

AudioReceiveStream::AudioReceiveStream(VoiceEngine* voe, int channel)
	:  voe_(voe),
	   thread_(ThreadWrapper::CreateThread(AudioReceiveStream::AudioRecvStatisticsThreadRun, this, kNormalPriority, "AudioRecvStatisticsThread")),
	trace_file_(*FileWrapper::Create()),
	channel_(channel),
	   clock_(Clock::GetRealTimeClock()),
	   last_process_time_(clock_->TimeInMilliseconds()),
	   crit_(CriticalSectionWrapper::CreateCriticalSection())
{
	unsigned int id;
	thread_->Start(id);
}

AudioReceiveStream::~AudioReceiveStream()
{
	if (trace_file_.Open())
	{
		trace_file_.Flush();
		trace_file_.CloseFile();
	}
	delete &trace_file_;

	thread_->SetNotAlive();

	if(thread_->Stop()) {
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
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000; //1000ms定时器
	if (now >= last_process_time_ + kUpdateIntervalMs) {
		//统计各种发送信息
		last_process_time_ = now;
		UpdateStats();
	}
	return true;
}

std::string AudioReceiveStream::GenerateFileName(int channel)
{
	std::string szRet = "";
	char timeBuffer[128];
#ifdef _WIN32
	sprintf(timeBuffer, "audiochannel%d_RcvStats.data",channel);
#else
	sprintf(timeBuffer, "%s/audiochannel%d_RcvStats.data",filename_path, channel);
#endif
	szRet = timeBuffer;
	return szRet;
}

std::string AudioReceiveStream::ToString() const
{
	char timeBuffer[128];
	char formatString[128];
	//		time_t nowtime = time(NULL);
	tm timeTemp;
	//	localtime_s(&timeTemp, &nowtime);
	int microseconds;
	CurrentTmTime(&timeTemp, &microseconds);
	sprintf(timeBuffer, "%02d/%02d/%04d %02d:%02d:%02d",
		timeTemp.tm_mon + 1, timeTemp.tm_mday, 	
		timeTemp.tm_year + 1900,
		timeTemp.tm_hour, timeTemp.tm_min, timeTemp.tm_sec);

	CriticalSectionScoped lock(crit_.get());
	std::stringstream ss;
	ss << "audioRecv timestamp=" << timeBuffer;
	ss << "\tCodecName=" << audioRecvStats_.codec_name;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.audio_level);
	ss << "\taudioOutputLevel=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9I64d", audioRecvStats_.bytes_rcvd);
	ss << "\tbytesReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.packets_rcvd);
	ss << "\tpacketsReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.packets_lost);
	ss << "\tpacketsLost=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.jitter_ms);
	ss << "\tjitterReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.delay_estimate_ms);
	ss << "\tCurrentDelayMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.jitter_buffer_ms);
	ss << "\tjitterBufferMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.jitter_buffer_preferred_ms);
	ss << "\tPreferredjitterBufferMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%f", audioRecvStats_.accelerate_rate);
	ss << "\tAccelerateRate=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%f", audioRecvStats_.expand_rate);
	ss << "\tExpandRate=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%f", audioRecvStats_.preemptive_expand_rate);
	ss << "\tPreemptiveExpandRate=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.decoding_normal);
	ss << "\tDecodingNormal=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.decoding_plc);
	ss << "\tDecodingPLC=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.decoding_cng);
	ss << "\tDecodingCNG=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", audioRecvStats_.decoding_plc_cng);
	ss << "\tDecodingPLCCNG=" << formatString;

	ss << "\r\n";
	return ss.str();
}

int AudioReceiveStream::ToString(FileWrapper *pFile)
{
	pFile->Write(ToString().c_str(), ToString().length());
	return 0;
}


void AudioReceiveStream::UpdateStats()
{
	VoECodec *voe_codec = VoECodec::GetInterface(voe_);
	VoERTP_RTCP *voe_rtprtcp = VoERTP_RTCP::GetInterface(voe_);
	VoENetEqStats* voe_neteq = VoENetEqStats::GetInterface(voe_);
	VoEVideoSync *voe_sync = VoEVideoSync::GetInterface(voe_);
	VoEVolumeControl *voe_volume = VoEVolumeControl::GetInterface(voe_);

	CodecInst codec;

	CriticalSectionScoped lock(crit_.get());
	if (voe_codec)
	{
		voe_codec->GetRecCodec(channel_, codec);
		audioRecvStats_.codec_name = codec.plname;
		voe_codec->Release();
	}

	if (voe_rtprtcp)
	{
		CallStatistics callstats;
		voe_rtprtcp->GetRemoteSSRC(channel_, audioRecvStats_.remote_ssrc);
		voe_rtprtcp->GetRTCPStatistics(channel_, callstats);
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
		voe_neteq->GetDecodingCallStatistics(channel_, &decodingStats);
		voe_neteq->GetNetworkStatistics(channel_, networkStats);
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
		voe_sync->GetDelayEstimate(channel_, &jitter_delay_ms, &playout_delay_ms);
		audioRecvStats_.delay_estimate_ms = jitter_delay_ms+playout_delay_ms;
		voe_sync->Release();
	}

	if (voe_volume)
	{
		uint32_t level = 0;
		voe_volume->GetSpeechOutputLevelFullRange(channel_, level); //[0, 32768]
		audioRecvStats_.audio_level = static_cast<int32_t>(level);
		voe_volume->Release();
	}
}

void AudioReceiveStream::FillUploadStats()
{
	CriticalSectionScoped lock(crit_.get());
	memset(&UploadStats, 0, sizeof(UploadStats));
	memcpy(UploadStats.WhichStats,"AR",2);
	memcpy(UploadStats.Version,"1", 2);
	memcpy(UploadStats.CodecName, audioRecvStats_.codec_name.c_str(), 4);
	UploadStats.AudioOutputLevel = audioRecvStats_.audio_level;
	UploadStats.BytesReceived = audioRecvStats_.bytes_rcvd;
	UploadStats.PacketsReceived = audioRecvStats_.packets_rcvd;
	UploadStats.PacketsLost = audioRecvStats_.packets_lost;
	UploadStats.JitterReceived = audioRecvStats_.jitter_ms;
	UploadStats.CurrentDelayMs = audioRecvStats_.delay_estimate_ms;
	UploadStats.JitterBufferMs = audioRecvStats_.jitter_buffer_ms;
	UploadStats.PreferredjitterBufferMs = audioRecvStats_.jitter_buffer_preferred_ms;
	UploadStats.AccelerateRate = audioRecvStats_.accelerate_rate;
	UploadStats.ExpandRate = audioRecvStats_.expand_rate;
	UploadStats.PreemptiveExpandRate = audioRecvStats_.preemptive_expand_rate;
	UploadStats.DecodingNormal = audioRecvStats_.decoding_normal;
	UploadStats.DecodingPLC = audioRecvStats_.decoding_plc;
	UploadStats.DecodingCNG = audioRecvStats_.decoding_cng;
	UploadStats.DecodingPLCCNG = audioRecvStats_.decoding_plc_cng;
}

 int AudioReceiveStream::ToBinary(char* buffer, int buffer_length)
 {
	FillUploadStats();	 
	 int length = sizeof(UploadStats);
	 if (buffer_length < length)
	 {
		 return -1;
	 }
	 memcpy(buffer, &UploadStats, length);
	 return length;
}

 int AudioReceiveStream::ToFile(FileWrapper *pFile)
 {
	 if(pFile)
	 {
		 FillUploadStats();
		 pFile->Write(UploadStats.WhichStats, sizeof(UploadStats.WhichStats));
		 pFile->Write(UploadStats.Version, sizeof(UploadStats.Version));
		 pFile->Write(&UploadStats.CodecName, sizeof(UploadStats.CodecName));
		 pFile->Write(&UploadStats.AudioOutputLevel, sizeof(UploadStats.AudioOutputLevel));
		 pFile->Write(&UploadStats.BytesReceived, sizeof(UploadStats.BytesReceived));
		 pFile->Write(&UploadStats.PacketsReceived, sizeof(UploadStats.PacketsReceived));
		 pFile->Write(&UploadStats.PacketsLost, sizeof(UploadStats.PacketsLost));
		 pFile->Write(&UploadStats.JitterReceived, sizeof(UploadStats.JitterReceived));
		 pFile->Write(&UploadStats.CurrentDelayMs, sizeof(UploadStats.CurrentDelayMs));
		 pFile->Write(&UploadStats.JitterBufferMs, sizeof(UploadStats.JitterBufferMs));
		 pFile->Write(&UploadStats.PreferredjitterBufferMs, sizeof(UploadStats.PreferredjitterBufferMs));
		 pFile->Write(&UploadStats.AccelerateRate, sizeof(UploadStats.AccelerateRate));
		 pFile->Write(&UploadStats.ExpandRate, sizeof(UploadStats.ExpandRate));
		 pFile->Write(&UploadStats.PreemptiveExpandRate, sizeof(UploadStats.PreemptiveExpandRate));
		 pFile->Write(&UploadStats.DecodingNormal, sizeof(UploadStats.DecodingNormal));
		 pFile->Write(&UploadStats.DecodingPLC, sizeof(UploadStats.DecodingPLC));
		 pFile->Write(&UploadStats.DecodingCNG, sizeof(UploadStats.DecodingCNG));
		 pFile->Write(&UploadStats.DecodingPLCCNG, sizeof(UploadStats.DecodingPLCCNG));

		 int length = 0;
		 length = sizeof(UploadStats.WhichStats)
			 +sizeof(UploadStats.Version)
			 +sizeof(UploadStats.CodecName)
			 +sizeof(UploadStats.AudioOutputLevel)
			 +sizeof(UploadStats.BytesReceived)
			 +sizeof(UploadStats.PacketsReceived)
			 +sizeof(UploadStats.PacketsLost)
			 +sizeof(UploadStats.JitterReceived)
			 +sizeof(UploadStats.CurrentDelayMs)
			 +sizeof(UploadStats.JitterBufferMs)
			 +sizeof(UploadStats.PreferredjitterBufferMs)
			 +sizeof(UploadStats.AccelerateRate)
			 +sizeof(UploadStats.ExpandRate)
			 +sizeof(UploadStats.PreemptiveExpandRate)
			 +sizeof(UploadStats.DecodingNormal)
			 +sizeof(UploadStats.DecodingPLC)
			 +sizeof(UploadStats.DecodingCNG)
			 +sizeof(UploadStats.DecodingPLCCNG);

		 return length;
	 }

	 return -1;
 }
}
