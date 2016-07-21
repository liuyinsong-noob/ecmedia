/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "send_statistics_proxy.h"

#include <sstream>
#include <map>
#include <string>

#include "timeutils.h"

#include "critical_section_wrapper.h"
#include "trace.h"

extern char* filename_path;


namespace cloopenwebrtc {

std::string SendStatisticsProxy::GenerateFileName(int video_channel)
	{
		std::string szRet = "";
		char timeBuffer[128];
// 		time_t nowtime = time(NULL);
// 		tm timeTemp;
// 		localtime_s(&timeTemp, &nowtime);
// 		sprintf(timeBuffer, "videochannel%d_SendStats-%04d-%02d-%02d_%02d-%02d-%02d.data",video_channel,
// 			timeTemp.tm_year + 1900, timeTemp.tm_mon + 1,
// 			timeTemp.tm_mday, timeTemp.tm_hour, timeTemp.tm_min, timeTemp.tm_sec);

#ifdef _WIN32
		sprintf(timeBuffer, "videochannel%d_SendStats.data",video_channel);
#else
		sprintf(timeBuffer, "%s/videochannel%d_SendStats.data",filename_path, video_channel);
#endif
		

		szRet = timeBuffer;
		return szRet;
	}

const int SendStatisticsProxy::kStatsTimeoutMs = 5000;

SendStatisticsProxy::SendStatisticsProxy(int video_channel)
	   :crit_(CriticalSectionWrapper::CreateCriticalSection()),
		clock_(Clock::GetRealTimeClock()),
		last_process_time_(clock_->TimeInMilliseconds()),
		trace_file_(*FileWrapper::Create()),
	    updateEvent_(EventWrapper::Create()),
		call_stats_(NULL),
		bitrate_controller_(NULL),
		remote_bitrate_estimator_(NULL),
		overuse_detector_(NULL),
		paced_sender_(NULL){
}

SendStatisticsProxy::~SendStatisticsProxy() {

	updateEvent_->Set();
	delete updateEvent_;

	if (trace_file_.Open())
	{
		trace_file_.Flush();
		trace_file_.CloseFile();
	}	
	delete &trace_file_;
}

void SendStatisticsProxy::OutgoingRate(const int video_channel,
                                       const unsigned int framerate,
                                       const unsigned int bitrate) {
  CriticalSectionScoped lock(crit_.get());
  stats_.encode_frame_rate = framerate;
  stats_.media_bitrate_bps = bitrate;
}

void SendStatisticsProxy::SuspendChange(int video_channel, bool is_suspended) {
  CriticalSectionScoped lock(crit_.get());
  stats_.suspended = is_suspended;
}

VideoSendStream::Stats SendStatisticsProxy::GetStats() {
  CriticalSectionScoped lock(crit_.get());
  return stats_;
}

void SendStatisticsProxy::OnSendEncodedImage(
	const EncodedImage& encoded_image,
	const RTPVideoHeader* rtp_video_header) {
		//统计与ssrc相关的信息，暂时没有实现
	CriticalSectionScoped lock(crit_.get());
	stats_.sent_width = encoded_image._encodedWidth;
	stats_.sent_height = encoded_image._encodedHeight;
	if (rtp_video_header)
	{
		if (rtp_video_header->codec == kRtpVideoH264)
		{
			stats_.encoder_implementation_name = "h264";
		}
		else if (rtp_video_header->codec == kRtpVideoVp8)
		{
			stats_.encoder_implementation_name = "vp8";
		}
	}
}

void SendStatisticsProxy::OnSetRates(uint32_t bitrate_bps, int framerate) {
	CriticalSectionScoped lock(crit_.get());
	stats_.target_media_bitrate_bps = bitrate_bps;
	stats_.input_frame_rate = framerate;
}

void SendStatisticsProxy::StatisticsUpdated(const RtcpStatistics& statistics,
											uint32_t ssrc) {
	CriticalSectionScoped lock(crit_.get());		
	stats_.stream.rtcp_stats = statistics;
	//uma
}

void SendStatisticsProxy::CNameChanged(const char* cname, uint32_t ssrc) {}


void SendStatisticsProxy::RtcpPacketTypesCounterUpdated(
														uint32_t ssrc,
														const RtcpPacketTypeCounter& packet_counter) {
	CriticalSectionScoped lock(crit_.get());
	stats_.stream.rtcp_packet_type_counts = packet_counter;
}


void SendStatisticsProxy::FrameCountUpdated(const FrameCounts& frame_counts,
											uint32_t ssrc) {
		CriticalSectionScoped lock(crit_.get());
		stats_.stream.frame_counts = frame_counts;
}

void SendStatisticsProxy::DataCountersUpdated(
												const StreamDataCounters& counters,
												uint32_t ssrc) {
		CriticalSectionScoped lock(crit_.get());
		stats_.stream.rtp_stats = counters;
}

void SendStatisticsProxy::Notify(const BitrateStatistics& total_stats,
								const BitrateStatistics& retransmit_stats,
								uint32_t ssrc) {
		CriticalSectionScoped lock(crit_.get());
		stats_.stream.total_bitrate_bps = total_stats.bitrate_bps;
		stats_.stream.retransmit_bitrate_bps = retransmit_stats.bitrate_bps;
}

void SendStatisticsProxy::SendSideDelayUpdated(int avg_delay_ms,
												int max_delay_ms,
												uint32_t ssrc) {
		CriticalSectionScoped lock(crit_.get());
// 		VideoSendStream::StreamStats* stats = GetStatsEntry(ssrc);
// 		if (!stats)
// 			return;

		stats_.stream.avg_delay_ms = avg_delay_ms;
		stats_.stream.max_delay_ms = max_delay_ms;

// 		uma_container_->delay_counter_.Add(avg_delay_ms);
// 		uma_container_->max_delay_counter_.Add(max_delay_ms);
}

int64_t SendStatisticsProxy::TimeUntilNextProcess()
{
//	return 0;
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000;
	return kUpdateIntervalMs - (now - last_process_time_);
}

int32_t SendStatisticsProxy::Process()
{
	updateEvent_->Wait(100);
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000; //1000ms定时器
	if (now >= last_process_time_ + kUpdateIntervalMs) {
		//统计各种发送信息
		std::vector<uint32_t> ssrcs;
		last_process_time_ = now;
		if (call_stats_)
		{
		//	call_stats_->rtcp_rtt_stats()->LastProcessedRtt();
			call_.rtt_ms = call_stats_->rtcp_rtt_stats()->LastProcessedRtt();
			if (bitrate_controller_)
			{
				bitrate_controller_->AvailableBandwidth(&call_.send_bandwidth_bps);
			}
			if (remote_bitrate_estimator_)
			{
				remote_bitrate_estimator_->LatestEstimate(&ssrcs, &call_.recv_bandwidth_bps);
			}
			if (paced_sender_)
			{
				call_.pacer_delay_ms = paced_sender_->QueueInMs();
			}
		}

		if (overuse_detector_)
		{
			CpuOveruseMetrics metrics;
			overuse_detector_->GetCpuOveruseMetrics(&metrics);
			stats_.avg_encode_time_ms = metrics.avg_encode_time_ms;
			stats_.encode_usage_percent = metrics.encode_usage_percent;
		}
	}
	return 0;
}

void SendStatisticsProxy::SetCallStats(CallStats *stats)
{
	call_stats_ = stats;
}
void SendStatisticsProxy::SetBitrateController(BitrateController *controller)
{
	bitrate_controller_ = controller;
}
void SendStatisticsProxy::SetRemoteBitrateEstimator(RemoteBitrateEstimator *remote_bitrate_estimator)
{
	remote_bitrate_estimator_ = remote_bitrate_estimator;
}

void SendStatisticsProxy::SetOverUseDetector(OveruseFrameDetector *overuse_detector)
{
	overuse_detector_ = overuse_detector;
}

void SendStatisticsProxy::SetPacedSender(PacedSender *paced_sender)
{
	paced_sender_ = paced_sender;
}

void SendStatisticsProxy::UpdateInputSize(int width, int height)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.input_width = width;
	stats_.input_height = height;
}

std::string SendStatisticsProxy::ToString() const
{
	char timeBuffer[128];
	char formatString[128];
	time_t nowtime = time(NULL);
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
	ss << "videoSend timestamp=" << timeBuffer;
	ss << "\tCodecName=" << stats_.encoder_implementation_name;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.avg_encode_time_ms);
	ss << "\tAvgEncodeMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.encode_usage_percent);
	ss << "\tEncodeUsagePercent=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.input_frame_rate);
	ss << "\tFrameRateInput=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.encode_frame_rate);
	ss << "\tFrameRateSent=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.input_width);
	ss << "\tFrameWidthInput=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.input_height);
	ss << "\tFrameHeightInput=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.sent_width);
	ss << "\tFrameWidthSent=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.sent_height);
	ss << "\tFrameHeightSent=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.target_media_bitrate_bps);
	ss << "\tBitrateTargetBps=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.media_bitrate_bps);
	ss << "\tBitrateEncodeBps=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.stream.rtcp_stats.cumulative_lost); 
	ss << "\tpacketsLost=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.stream.rtp_stats.packets); 
	ss << "\tpacketsSent=" << formatString;

	size_t bytsSent = stats_.stream.rtp_stats.bytes 
					+ stats_.stream.rtp_stats.header_bytes
					+ stats_.stream.rtp_stats.padding_bytes;
	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", bytsSent); 
	ss << "\tbytesSent=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.stream.rtcp_packet_type_counts.nack_packets); 
	ss << "\tNacksReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.stream.rtcp_packet_type_counts.fir_packets); 
	ss << "\tFirsReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", call_.rtt_ms); 
	ss << "\tRtt=" << formatString;

	//bweCompound
	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", call_.send_bandwidth_bps); 
	ss << "\tAvailabeSendBandwidth=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.media_bitrate_bps); 
	ss << "\tActualEncBitrate=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-09d", stats_.target_media_bitrate_bps); 
	ss << "\tTargetEncBitrate=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.stream.total_bitrate_bps); 
	ss << "\tTransmitBitrate=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.stream.retransmit_bitrate_bps); 
	ss << "\tReransmitBitrate=" << formatString;


	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", call_.recv_bandwidth_bps); 
	ss << "\tAvailableReceiveBandwidth=" << formatString;


	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", call_.pacer_delay_ms); 
	ss << "\tBucketDelay=" << formatString;

	ss << "\r\n";

	return ss.str();
}

int SendStatisticsProxy::ToString(FileWrapper *pFile)
{
	pFile->Write(ToString().c_str(), ToString().length());
	return 0;
}

  void SendStatisticsProxy::FillUploadStats()
  {
	  CriticalSectionScoped lock(crit_.get());
	  memset(&UploadStats, 0, sizeof(UploadStats));
	  memcpy(UploadStats.WhichStats,"VS",2);
	  memcpy(UploadStats.Version,"1", 2);
	  memcpy(UploadStats.CodecName, stats_.encoder_implementation_name.c_str(), 4);
	  UploadStats.AvgEncodeMs = stats_.avg_encode_time_ms;
	  UploadStats.EncodeUsagePercent = stats_.encode_usage_percent;
	  UploadStats.FrameRateInput = stats_.input_frame_rate;
	  UploadStats.FrameRateSent = stats_.encode_frame_rate;
	  UploadStats.FrameWidthInput = stats_.input_width;
	  UploadStats.FrameHeightInput = stats_.input_height;
	  UploadStats.FrameWidthSent = stats_.sent_width;
	  UploadStats.FrameHeightSent = stats_.sent_height;

	  UploadStats.BitrateTargetBps = stats_.target_media_bitrate_bps;
	  UploadStats.BitrateEncodeBps = stats_.media_bitrate_bps;

	  UploadStats.PacketsLost = stats_.stream.rtcp_stats.cumulative_lost; 
	  UploadStats.PacketsSent =  stats_.stream.rtp_stats.packets; 
	  UploadStats.BytesSent = stats_.stream.rtp_stats.bytes 
		  + stats_.stream.rtp_stats.header_bytes
		  + stats_.stream.rtp_stats.padding_bytes;

	  UploadStats.NacksReceived = stats_.stream.rtcp_packet_type_counts.nack_packets; 
	  UploadStats.FirsReceived = stats_.stream.rtcp_packet_type_counts.fir_packets; 
	  UploadStats.Rtt = call_.rtt_ms; 
	  //bweCompound
	  UploadStats.AvailabeSendBandwidth = call_.send_bandwidth_bps; 
	  UploadStats.ActualEncBitrate = stats_.media_bitrate_bps;
	  UploadStats.TargetEncBitrate = stats_.target_media_bitrate_bps;

	  UploadStats.TransmitBitrate = stats_.stream.total_bitrate_bps;
	  UploadStats.ReransmitBitrate = stats_.stream.retransmit_bitrate_bps; 
	  UploadStats.AvailableReceiveBandwidth = call_.recv_bandwidth_bps; 

	  UploadStats.BucketDelay = call_.pacer_delay_ms; 
  }

int SendStatisticsProxy::ToBinary(char* buffer, int buffer_length)
{	
	FillUploadStats();
	int length = sizeof(UploadStats);
	memset(buffer, 0, buffer_length);
	if (buffer_length < length)
	{
		return -1;
	}
	memcpy(buffer, &UploadStats, length);
	return length;
}

int SendStatisticsProxy::ToFile(FileWrapper *pFile)
{
	if (pFile)
	{
		FillUploadStats();
		pFile->Write(UploadStats.WhichStats, sizeof(UploadStats.WhichStats));
		pFile->Write(UploadStats.Version, sizeof(UploadStats.Version));
		pFile->Write(&UploadStats.CodecName, sizeof(UploadStats.CodecName));
		pFile->Write(&UploadStats.AvgEncodeMs, sizeof(UploadStats.AvgEncodeMs));
		pFile->Write(&UploadStats.EncodeUsagePercent, sizeof(UploadStats.EncodeUsagePercent));
		pFile->Write(&UploadStats.FrameRateInput, sizeof(UploadStats.FrameRateInput));
		pFile->Write(&UploadStats.FrameRateSent, sizeof(UploadStats.FrameRateSent));
		pFile->Write(&UploadStats.FrameWidthInput, sizeof(UploadStats.FrameWidthInput));
		pFile->Write(&UploadStats.FrameHeightInput, sizeof(UploadStats.FrameHeightInput));
		pFile->Write(&UploadStats.FrameWidthSent, sizeof(UploadStats.FrameWidthSent));
		pFile->Write(&UploadStats.FrameHeightSent, sizeof(UploadStats.FrameHeightSent));
		pFile->Write(&UploadStats.BitrateTargetBps, sizeof(UploadStats.BitrateTargetBps));
		pFile->Write(&UploadStats.BitrateEncodeBps, sizeof(UploadStats.BitrateEncodeBps));
		pFile->Write(&UploadStats.PacketsLost, sizeof(UploadStats.PacketsLost));
		pFile->Write(&UploadStats.PacketsSent, sizeof(UploadStats.PacketsSent));
		pFile->Write(&UploadStats.BytesSent, sizeof(UploadStats.BytesSent));
		pFile->Write(&UploadStats.NacksReceived, sizeof(UploadStats.NacksReceived));
		pFile->Write(&UploadStats.FirsReceived, sizeof(UploadStats.FirsReceived));
		pFile->Write(&UploadStats.Rtt, sizeof(UploadStats.Rtt));
		pFile->Write(&UploadStats.AvailabeSendBandwidth, sizeof(UploadStats.AvailabeSendBandwidth));
		pFile->Write(&UploadStats.ActualEncBitrate, sizeof(UploadStats.ActualEncBitrate));
		pFile->Write(&UploadStats.TargetEncBitrate, sizeof(UploadStats.TargetEncBitrate));
		pFile->Write(&UploadStats.TransmitBitrate, sizeof(UploadStats.TransmitBitrate));
		pFile->Write(&UploadStats.ReransmitBitrate, sizeof(UploadStats.ReransmitBitrate));
		pFile->Write(&UploadStats.AvailableReceiveBandwidth, sizeof(UploadStats.AvailableReceiveBandwidth));
		pFile->Write(&UploadStats.BucketDelay, sizeof(UploadStats.BucketDelay));


		int length = 0;
		length = sizeof(UploadStats.WhichStats)
			+sizeof(UploadStats.Version)
			+sizeof(UploadStats.CodecName)
			+sizeof(UploadStats.AvgEncodeMs)
			+sizeof(UploadStats.EncodeUsagePercent)
			+sizeof(UploadStats.FrameRateInput)
			+sizeof(UploadStats.FrameRateSent)
			+sizeof(UploadStats.FrameWidthInput)
			+sizeof(UploadStats.FrameHeightInput)
			+sizeof(UploadStats.FrameWidthSent)
			+sizeof(UploadStats.FrameHeightSent)
			+sizeof(UploadStats.BitrateTargetBps)
			+sizeof(UploadStats.BitrateEncodeBps)
			+sizeof(UploadStats.PacketsLost)
			+sizeof(UploadStats.PacketsSent)
			+sizeof(UploadStats.BytesSent)
			+sizeof(UploadStats.NacksReceived)
			+sizeof(UploadStats.FirsReceived)
			+sizeof(UploadStats.Rtt)
			+sizeof(UploadStats.AvailabeSendBandwidth)
			+sizeof(UploadStats.ActualEncBitrate)
			+sizeof(UploadStats.TargetEncBitrate)
			+sizeof(UploadStats.TransmitBitrate)
			+sizeof(UploadStats.ReransmitBitrate)
			+sizeof(UploadStats.AvailableReceiveBandwidth)
			+sizeof(UploadStats.BucketDelay);
		return length;	
	}

	return -1;
}

}  // namespace webrtc
