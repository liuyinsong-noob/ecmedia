#include "audio_send_stream.h"
#include "trace.h"
#include "timeutils.h"

#include "voe_codec.h"
#include "voe_rtp_rtcp.h"
#include "voe_audio_processing.h"

#include <sstream>

extern char* filename_path;

namespace cloopenwebrtc{

	AudioSendStream::AudioSendStream(VoiceEngine* voe, int channel)
		: voe_(voe),
		  thread_(ThreadWrapper::CreateThread(AudioSendStream::AudioSendStatisticsThreadRun, this, kNormalPriority, "AudioSendStatisticsThread")),
			trace_file_(*FileWrapper::Create()),
			channel_(channel),
		  clock_(Clock::GetRealTimeClock()),
		  last_process_time_(clock_->TimeInMilliseconds()),
		  crit_(CriticalSectionWrapper::CreateCriticalSection())
	{
		unsigned int id;
		thread_->Start(id);
	}

	AudioSendStream::~AudioSendStream()
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

	bool AudioSendStream::AudioSendStatisticsThreadRun(void* obj)
	{
		 return static_cast<AudioSendStream*>(obj)->ProcessSendStatistics();
	}

	bool AudioSendStream::ProcessSendStatistics()
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

	std::string AudioSendStream::GenerateFileName(int channel)
	{
		std::string szRet = "";
		char timeBuffer[128];
#ifdef _WIN32
		sprintf(timeBuffer, "audiochannel%d_SendStats.data",channel);
#else
		sprintf(timeBuffer, "%s/audiochannel%d_SendStats.data",filename_path, channel);
#endif
		szRet = timeBuffer;
		return szRet;
	}

	std::string AudioSendStream::ToString() const
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
		ss << "audioSend timestamp=" << timeBuffer;
		ss << "\tCodecName=" << audioSendStats_.codec_name;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-9d", audioSendStats_.bytes_sent);
		ss << "\tbytesSent=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-9d", audioSendStats_.packets_sent);
		ss << "\tpacketsSent=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-9d", audioSendStats_.rtt_ms);
		ss << "\tRtt=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-9d", audioSendStats_.jitter_ms);
		ss << "\tJitterReceived=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-9d", audioSendStats_.packets_lost);
		ss << "\tpacketsLost=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-4d", audioSendStats_.echo_delay_median_ms);
		ss << "\tEcEchoDelayMedian=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-4d", audioSendStats_.echo_delay_std_ms);
		ss << "\tEcEchoDelayStdDev=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-4d", audioSendStats_.echo_return_loss);
		ss << "\tEcReturnLoss=" << formatString;

		memset(formatString, 0, 128);
		sprintf(formatString, "%-4d", audioSendStats_.echo_return_loss_enhancement);
		ss << "\tEcRetrunLossEnhancement=" << formatString;

		ss << "\r\n";

		return ss.str();
	}

	int AudioSendStream::ToString(FileWrapper *pFile)
	{
		pFile->Write(ToString().c_str(), ToString().length());
		return 0;
	}

	void AudioSendStream::UpdateStats()
	{
		VoECodec *voe_codec = VoECodec::GetInterface(voe_);
		VoERTP_RTCP *voe_rtprtcp = VoERTP_RTCP::GetInterface(voe_);
		VoEAudioProcessing *voe_process = VoEAudioProcessing::GetInterface(voe_);
		if (voe_codec)
		{
			CodecInst sendcodec;
			voe_codec->GetSendCodec(channel_, sendcodec);
			audioSendStats_.codec_name = sendcodec.plname;
			voe_codec->Release();
		}

		if (voe_rtprtcp)
		{
			CallStatistics callstats;
			voe_rtprtcp->GetLocalSSRC(channel_, audioSendStats_.local_ssrc);
			voe_rtprtcp->GetRTCPStatistics(channel_, callstats);
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
	}

	void AudioSendStream::FillUploadStats()
	{
		CriticalSectionScoped lock(crit_.get());
		memset(&UploadStats, 0, sizeof(UploadStats));
		memcpy(UploadStats.WhichStats, "AS", 2);
		memcpy(UploadStats.Version, "1", 2);
		memcpy(UploadStats.CodecName, audioSendStats_.codec_name.c_str(), 4);
		UploadStats.BytesSent = audioSendStats_.bytes_sent;
		UploadStats.PacketsSent = audioSendStats_.packets_sent;
		UploadStats.Rtt = audioSendStats_.rtt_ms;
		UploadStats.JitterReceived = audioSendStats_.jitter_ms;
		UploadStats.PacketsLost = audioSendStats_.packets_lost;
		UploadStats.EcEchoDelayMedian = audioSendStats_.echo_delay_median_ms;
		UploadStats.EcEchoDelayStdDev = audioSendStats_.echo_delay_std_ms;
		UploadStats.EcReturnLoss = audioSendStats_.echo_return_loss;
		UploadStats.EcRetrunLossEnhancement = audioSendStats_.echo_return_loss_enhancement;
	}
	int AudioSendStream::ToBinary(char* buffer, int buffer_length)
	{		
		FillUploadStats();
		int length = sizeof(UploadStats);
		if(buffer_length < length)
			return -1;
		memcpy(buffer, &UploadStats, length);
		return length;
	}

	int AudioSendStream::ToFile(FileWrapper *pFile)
	{
		if (pFile)
		{
			FillUploadStats();
			pFile->Write(UploadStats.WhichStats, sizeof(UploadStats.WhichStats));
			pFile->Write(UploadStats.Version, sizeof(UploadStats.Version));
			pFile->Write(&UploadStats.CodecName, sizeof(UploadStats.CodecName));
			pFile->Write(&UploadStats.BytesSent, sizeof(UploadStats.BytesSent));
			pFile->Write(&UploadStats.PacketsSent, sizeof(UploadStats.PacketsSent));
			pFile->Write(&UploadStats.Rtt, sizeof(UploadStats.Rtt));
			pFile->Write(&UploadStats.JitterReceived, sizeof(UploadStats.JitterReceived));
			pFile->Write(&UploadStats.PacketsLost, sizeof(UploadStats.PacketsLost));
			pFile->Write(&UploadStats.EcEchoDelayMedian, sizeof(UploadStats.EcEchoDelayMedian));
			pFile->Write(&UploadStats.EcEchoDelayStdDev, sizeof(UploadStats.EcEchoDelayStdDev));
			pFile->Write(&UploadStats.EcReturnLoss, sizeof(UploadStats.EcReturnLoss));
			pFile->Write(&UploadStats.EcRetrunLossEnhancement, sizeof(UploadStats.EcRetrunLossEnhancement));

			int length = 0;
			length = sizeof(UploadStats.WhichStats)
				+sizeof(UploadStats.Version)
				+sizeof(UploadStats.CodecName)
				+sizeof(UploadStats.BytesSent)
				+sizeof(UploadStats.PacketsSent)
				+sizeof(UploadStats.Rtt)
				+sizeof(UploadStats.JitterReceived)
				+sizeof(UploadStats.PacketsLost)
				+sizeof(UploadStats.EcEchoDelayMedian)
				+sizeof(UploadStats.EcEchoDelayStdDev)
				+sizeof(UploadStats.EcReturnLoss)
				+sizeof(UploadStats.EcRetrunLossEnhancement);

			return length;
		}
		return -1;
	}
}
