/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "receive_statistics_proxy.h"

#include <sstream>

#include "clock.h"
#include "critical_section_wrapper.h"
//#include "time.h"
#include "timeutils.h"

extern char* filename_path;

namespace cloopenwebrtc {

std::string ReceiveStatisticsProxy::GenerateFileName(int video_channel)
	{
		std::string szRet = "";
		char timeBuffer[128];
//		time_t nowtime = time(NULL);
//		tm timeTemp;
//		localtime_s(&timeTemp, &nowtime);
// 		sprintf(timeBuffer, "videochannel%d_RcvStats-%04d-%02d-%02d_%02d-%02d-%02d.data",video_channel,
// 			timeTemp.tm_year + 1900, timeTemp.tm_mon + 1,
// 			timeTemp.tm_mday, timeTemp.tm_hour, timeTemp.tm_min, timeTemp.tm_sec);

#ifdef _WIN32
		sprintf(timeBuffer, "videochannel%d_RcvStats.data",video_channel);
#else
		sprintf(timeBuffer, "%s/videochannel%d_RcvStats.data",filename_path, video_channel);
#endif
		szRet = timeBuffer;
		return szRet;
	}	

ReceiveStatisticsProxy::ReceiveStatisticsProxy(int video_channel)
    : clock_(Clock::GetRealTimeClock()),
      crit_(CriticalSectionWrapper::CreateCriticalSection()),
	  last_process_time_(clock_->TimeInMilliseconds()),
	  trace_file_(*FileWrapper::Create()),
      // 1000ms window, scale 1000 for ms to s.
      decode_fps_estimator_(1000, 1000),
      renders_fps_estimator_(1000, 1000) {
}

ReceiveStatisticsProxy::~ReceiveStatisticsProxy() {

	if (trace_file_.Open())
	{
		trace_file_.Flush();
		trace_file_.CloseFile();
	}
	delete &trace_file_;
}



VideoReceiveStream::Stats ReceiveStatisticsProxy::GetStats() const {
  CriticalSectionScoped lock(crit_.get());
  return stats_;
}

void ReceiveStatisticsProxy::IncomingCodecChanged(const int video_channel,
												const VideoCodec& video_codec)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.last_height_ = video_codec.height;
	stats_.last_width_ = video_codec.width;
	if (video_codec.codecType == kVideoCodecVP8)
	{
		stats_.decoder_implementation_name = "vp8";
	}else if (video_codec.codecType == kVideoCodecH264)
	{
		stats_.decoder_implementation_name = "h264";
	}
}

void ReceiveStatisticsProxy::IncomingRate(const int video_channel,
                                          const unsigned int framerate,
                                          const unsigned int bitrate_bps) {
  CriticalSectionScoped lock(crit_.get());
  stats_.network_frame_rate = framerate;
  stats_.total_bitrate_bps = bitrate_bps;
}

void ReceiveStatisticsProxy::DecoderTiming(int decode_ms,
                                           int max_decode_ms,
                                           int current_delay_ms,
                                           int target_delay_ms,
                                           int jitter_buffer_ms,
                                           int min_playout_delay_ms,
                                           int render_delay_ms) {
  CriticalSectionScoped lock(crit_.get());
  stats_.avg_delay_ms = target_delay_ms;

  stats_.decode_ms = decode_ms;
  stats_.max_decode_ms = max_decode_ms;
  stats_.current_delay_ms = current_delay_ms;
  stats_.target_delay_ms = target_delay_ms;
  stats_.jitter_buffer_ms = jitter_buffer_ms;
  stats_.min_playout_delay_ms = min_playout_delay_ms;
  stats_.render_delay_ms = render_delay_ms;

//   decode_time_counter_.Add(decode_ms);
//   jitter_buffer_delay_counter_.Add(jitter_buffer_ms);
//   target_delay_counter_.Add(target_delay_ms);
//   current_delay_counter_.Add(current_delay_ms);
}

void ReceiveStatisticsProxy::RtcpPacketTypesCounterUpdated(uint32_t ssrc,
															const RtcpPacketTypeCounter& packet_counter)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.rtcp_packet_type_counts = packet_counter;
}

void ReceiveStatisticsProxy::StatisticsUpdated(
    const cloopenwebrtc::RtcpStatistics& statistics,
    uint32_t ssrc) {
  CriticalSectionScoped lock(crit_.get());
  stats_.rtcp_stats = statistics; 
}

void ReceiveStatisticsProxy::CNameChanged(const char* cname, uint32_t ssrc) {
  CriticalSectionScoped lock(crit_.get());
  stats_.c_name = cname;
}

void ReceiveStatisticsProxy::DataCountersUpdated(
    const cloopenwebrtc::StreamDataCounters& counters,
    uint32_t ssrc) {
  CriticalSectionScoped lock(crit_.get());
  stats_.rtp_stats = counters;
}

void ReceiveStatisticsProxy::OnDecodedFrame() {
  uint64_t now = clock_->TimeInMilliseconds();

  CriticalSectionScoped lock(crit_.get());
  decode_fps_estimator_.Update(1, now);
  stats_.decode_frame_rate = decode_fps_estimator_.Rate(now);
}

void ReceiveStatisticsProxy::OnRenderedFrame() {
  uint64_t now = clock_->TimeInMilliseconds();

  CriticalSectionScoped lock(crit_.get());
  renders_fps_estimator_.Update(1, now);
  stats_.render_frame_rate = renders_fps_estimator_.Rate(now);
}

void ReceiveStatisticsProxy::OnReceiveRatesUpdated(uint32_t bitRate,
                                                   uint32_t frameRate) {
}

void ReceiveStatisticsProxy::OnFrameCountsUpdated(
    const FrameCounts& frame_counts) {
  CriticalSectionScoped lock(crit_.get());
  stats_.frame_counts = frame_counts;
}

void ReceiveStatisticsProxy::OnDiscardedPacketsUpdated(int discarded_packets) {
  CriticalSectionScoped lock(crit_.get());
  stats_.discarded_packets = discarded_packets;
}

int64_t ReceiveStatisticsProxy::TimeUntilNextProcess()
{
	//	return 0;
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000;
	return kUpdateIntervalMs - (now - last_process_time_);
}

int32_t ReceiveStatisticsProxy::Process()
{
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000; //1000ms?¡§¨º¡À?¡Â

	if (now >= last_process_time_ + kUpdateIntervalMs) {
		last_process_time_ = now;
	}
	
	return 0;
}

void ReceiveStatisticsProxy::FrameCallback(I420VideoFrame* video_frame)
{
	OnDecodedFrame();
}

int32_t ReceiveStatisticsProxy::RenderFrame(const WebRtc_UWord32 streamId,
						   I420VideoFrame& videoFrame)
{
	OnRenderedFrame();
	return 0;
}

std::string ReceiveStatisticsProxy::ToString() const
{
	char timeBuffer[128];
	char formatString[128];
//	time_t nowtime = time(NULL);
	tm timeTemp;
	int microseconds;

	CurrentTmTime(&timeTemp, &microseconds);
	sprintf(timeBuffer, "%02d/%02d/%04d %02d:%02d:%02d",
		timeTemp.tm_mon + 1,timeTemp.tm_mday, timeTemp.tm_year + 1900,
		timeTemp.tm_hour, timeTemp.tm_min, timeTemp.tm_sec);

	CriticalSectionScoped lock(crit_.get());

	std::stringstream ss;
	ss << "videoRecv timestamp=" << timeBuffer;
	ss << "\tCodecName=" << stats_.decoder_implementation_name;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.last_width_); 
	ss << "\tFrameWidthReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-4d", stats_.last_height_); 
	ss << "\tFrameHeightReceived=" << formatString;

	int64_t bytes_rcvd = stats_.rtp_stats.bytes +
						 stats_.rtp_stats.header_bytes +
						 stats_.rtp_stats.padding_bytes;
	memset(formatString, 0, 128);
	sprintf(formatString, "%-9lld", bytes_rcvd);
	ss << "\tbytesReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.rtp_stats.packets); 
	ss << "\tpacketsReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.rtcp_stats.cumulative_lost); 
	ss << "\tpacketsLost=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.current_delay_ms); 
	ss << "\tCurrentDelayMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.decode_ms); 
	ss << "\tDecodeMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.jitter_buffer_ms); 
	ss << "\tJitterBufferMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.max_decode_ms); 
	ss << "\tMaxDecodeMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.min_playout_delay_ms); 
	ss << "\tMinPlayoutDelayMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.render_delay_ms); 
	ss << "\tRenderDelayMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.target_delay_ms); 
	ss << "\tTargetDelayMs=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.rtcp_packet_type_counts.nack_packets); 
	ss << "\tNacksSent=" << formatString;

// 	memset(formatString, 0, 128);
// 	sprintf(formatString, "%-9d", stats_.rtcp_packet_type_counts.fir_packets); 
// 	ss << "\tFirsSent=" << formatString;

//	memset(formatString, 0, 128);
// 	sprintf(formatString, "%-9d", stats_.rtcp_packet_type_counts.pli_packets); 
// 	ss << "\tPlisSent=" << formatStrin


	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.network_frame_rate); 
	ss << "\tFrameRateReceived=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.decode_frame_rate); 
	ss << "\tFrameRateDecoded=" << formatString;

	memset(formatString, 0, 128);
	sprintf(formatString, "%-9d", stats_.render_frame_rate); 
	ss << "\tFrameRateOutput=" << formatString;

	ss << "\r\n";

	return ss.str();
}
int ReceiveStatisticsProxy::ToString(FileWrapper *pFile)
{
	pFile->Write(ToString().c_str(), ToString().length());
	return 0;
}

void ReceiveStatisticsProxy::FillUploadStats()
{
	CriticalSectionScoped lock(crit_.get());
	memset(&UploadStats, 0, sizeof(UploadStats));
	memcpy(UploadStats.WhichStats, "VR", 2);
	memcpy(UploadStats.Version,"1", 2);
	memcpy(UploadStats.CodecName, stats_.decoder_implementation_name.c_str(), 4);

	UploadStats.FrameWidthReceived = stats_.last_width_; 
	UploadStats.FrameHeightReceived = stats_.last_height_; 
	UploadStats.BytesReceived = stats_.rtp_stats.bytes 
		+ stats_.rtp_stats.header_bytes
		+ stats_.rtp_stats.padding_bytes;

	UploadStats.PacketsReceived = stats_.rtp_stats.packets; 
	UploadStats.PacketsLost = stats_.rtcp_stats.cumulative_lost; 
	UploadStats.CurrentDelayMs = stats_.current_delay_ms; 
	UploadStats.DecodeMs = stats_.decode_ms; 
	UploadStats.JitterBufferMs = stats_.jitter_buffer_ms; 
	UploadStats.MaxDecodeMs = stats_.max_decode_ms; 

	UploadStats.MinPlayoutDelayMs = stats_.min_playout_delay_ms; 
	UploadStats.RenderDelayMs = stats_.render_delay_ms; 
	UploadStats.TargetDelayMs = stats_.target_delay_ms; 
	UploadStats.NacksSent = stats_.rtcp_packet_type_counts.nack_packets; 
	UploadStats.FrameRateReceived =	stats_.network_frame_rate; 
	UploadStats.FrameRateDecoded = stats_.decode_frame_rate; 
	UploadStats.FrameRateOutput = stats_.render_frame_rate; 
}

int ReceiveStatisticsProxy::ToBinary(char* buffer, int buffer_length)
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

int ReceiveStatisticsProxy::ToFile(FileWrapper *pFile)
{
	if (pFile)
	{
		FillUploadStats();
		pFile->Write(UploadStats.WhichStats, sizeof(UploadStats.WhichStats));
		pFile->Write(UploadStats.Version, sizeof(UploadStats.Version));
		pFile->Write(&UploadStats.CodecName, sizeof(UploadStats.CodecName));
		pFile->Write(&UploadStats.FrameWidthReceived, sizeof(UploadStats.FrameWidthReceived));
		pFile->Write(&UploadStats.FrameHeightReceived, sizeof(UploadStats.FrameHeightReceived));
		pFile->Write(&UploadStats.BytesReceived, sizeof(UploadStats.BytesReceived));
		pFile->Write(&UploadStats.PacketsReceived, sizeof(UploadStats.PacketsReceived));
		pFile->Write(&UploadStats.PacketsLost, sizeof(UploadStats.PacketsLost));
		pFile->Write(&UploadStats.CurrentDelayMs, sizeof(UploadStats.CurrentDelayMs));
		pFile->Write(&UploadStats.DecodeMs, sizeof(UploadStats.DecodeMs));
		pFile->Write(&UploadStats.JitterBufferMs, sizeof(UploadStats.JitterBufferMs));
		pFile->Write(&UploadStats.MaxDecodeMs, sizeof(UploadStats.MaxDecodeMs));
		pFile->Write(&UploadStats.MinPlayoutDelayMs, sizeof(UploadStats.MinPlayoutDelayMs));
		pFile->Write(&UploadStats.RenderDelayMs, sizeof(UploadStats.RenderDelayMs));
		pFile->Write(&UploadStats.TargetDelayMs, sizeof(UploadStats.TargetDelayMs));
		pFile->Write(&UploadStats.NacksSent, sizeof(UploadStats.NacksSent));
		pFile->Write(&UploadStats.FrameRateReceived, sizeof(UploadStats.FrameRateReceived));
		pFile->Write(&UploadStats.FrameRateDecoded, sizeof(UploadStats.FrameRateDecoded));
		pFile->Write(&UploadStats.FrameRateOutput, sizeof(UploadStats.FrameRateOutput));

		int length = 0;
		length = sizeof(UploadStats.WhichStats)
			+sizeof(UploadStats.Version)
			+sizeof(UploadStats.CodecName)
			+sizeof(UploadStats.FrameWidthReceived)
			+sizeof(UploadStats.FrameHeightReceived)
			+sizeof(UploadStats.BytesReceived)
			+sizeof(UploadStats.PacketsReceived)
			+sizeof(UploadStats.PacketsLost)
			+sizeof(UploadStats.CurrentDelayMs)
			+sizeof(UploadStats.DecodeMs)
			+sizeof(UploadStats.JitterBufferMs)
			+sizeof(UploadStats.MaxDecodeMs)
			+sizeof(UploadStats.MinPlayoutDelayMs)
			+sizeof(UploadStats.RenderDelayMs)
			+sizeof(UploadStats.TargetDelayMs)
			+sizeof(UploadStats.NacksSent)
			+sizeof(UploadStats.FrameRateReceived)
			+sizeof(UploadStats.FrameRateDecoded)
			+sizeof(UploadStats.FrameRateOutput);

		return length;
	}

	return -1;
}
}  // namespace webrtc
