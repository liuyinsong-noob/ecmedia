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

#include "../system_wrappers/include/clock.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/trace.h"
//#include "time.h"
#include "../base/timeutils.h"

namespace yuntongxunwebrtc {

ReceiveStatisticsProxy::ReceiveStatisticsProxy(int channel_id)
    : channel_id_(channel_id),
      clock_(Clock::GetRealTimeClock()),
      crit_(CriticalSectionWrapper::CreateCriticalSection()),
	  last_process_time_(clock_->TimeInMilliseconds()),
      // 1000ms window, scale 1000 for ms to s.
      decode_fps_estimator_(1000, 1000),
      renders_fps_estimator_(1000, 1000) {
	updateEvent_ = EventWrapper::Create();
}

ReceiveStatisticsProxy::~ReceiveStatisticsProxy() {
	updateEvent_->Set();
	delete updateEvent_;
    updateEvent_ = NULL;
}

VideoReceiveStream::Stats ReceiveStatisticsProxy::GetStats(int64_t &timestamp) const {
  CriticalSectionScoped lock(crit_.get());
  timestamp = clock_->TimeInMilliseconds();
  return stats_;
}

void ReceiveStatisticsProxy::IncomingCodecChanged(const int channel_id,
												const VideoCodec& video_codec)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.last_height_ = video_codec.height;
	stats_.last_width_ = video_codec.width;
	stats_.decoder_implementation_name = video_codec.plName;
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoRecv,
	{
		StatsReport::Value(StatsReport::kStatsValueNameFrameWidthReceived, video_codec.width, StatsReport::Value::kUInt16),
		StatsReport::Value(StatsReport::kStatsValueNameFrameHeightReceived, video_codec.height, StatsReport::Value::kUInt16),
		//StatsReport::Value(StatsReport::kStatsValueNameCodecImplementationName, stats_.decoder_implementation_name,StatsReport::Value::kString)
	});
#endif
}

void ReceiveStatisticsProxy::IncomingRate(const int channel_id,
                                          const unsigned int framerate,
                                          const unsigned int bitrate_bps) {
  CriticalSectionScoped lock(crit_.get());
  stats_.received_framerate = framerate;
  stats_.total_bitrate_bps = bitrate_bps;
#ifdef WIN32
  post_message(StatsReport::kStatsReportTypeVideoRecv,
  {
	  StatsReport::Value(StatsReport::kStatsValueNameReceivedTotalBitrate, bitrate_bps, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNameReceivedFrameRate, framerate, StatsReport::Value::kUInt32),
  });
#endif
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
#ifdef WIN32
  post_message(StatsReport::kStatsReportTypeVideoRecv,
  {
	  StatsReport::Value(StatsReport::kStatsValueNameDecodeMs, decode_ms, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNameMaxDecodeMs, max_decode_ms, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNameCurrentDelayMs, current_delay_ms, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNameTargetDelayMs, target_delay_ms, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNameJitterBufferMs, target_delay_ms, StatsReport::Value::kUInt32),
  });
#endif
}

void ReceiveStatisticsProxy::RtcpPacketTypesCounterUpdated(uint32_t ssrc,
															const RtcpPacketTypeCounter& packet_counter)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.rtcp_packet_type_counts = packet_counter;
//    printf("seansean nack_packets:%u, nack_requests:%u\n", packet_counter.nack_packets, packet_counter.nack_requests);
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoRecv,
	{
		StatsReport::Value(StatsReport::kStatsValueNameSsrc, ssrc, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameNacksSent, (packet_counter.nack_packets), StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameNacksRequestsSent, (packet_counter.nack_requests), StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameNacksUniqueRequestsSent, (packet_counter.unique_nack_requests), StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameFirsSent, (packet_counter.fir_packets), StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNamePlisSent, (packet_counter.pli_packets), StatsReport::Value::kUInt32),
	});
#endif
}

void ReceiveStatisticsProxy::StatisticsUpdated(
    const yuntongxunwebrtc::RtcpStatistics& statistics,
    uint32_t ssrc) {
  CriticalSectionScoped lock(crit_.get());
  stats_.rtcp_stats = statistics; 
  stats_.ssrc = ssrc;
#ifdef WIN32
  post_message(StatsReport::kStatsReportTypeVideoRecv,
  {
	  StatsReport::Value(StatsReport::kStatsValueNameSsrc, ssrc, StatsReport::Value::kUInt32),
//    StatsReport::Value(StatsReport::kStatsValueNameLossFractionInPercent, statistics.fraction_lost, StatsReport::Value::kUInt8),
    StatsReport::Value(StatsReport::kStatsValueNameLossFractionInPercent, statistics.real_fraction_lost, StatsReport::Value::kUInt8),
	  StatsReport::Value(StatsReport::kStatsValueNamePacketsLost, statistics.cumulative_lost, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNameJitterReceived, statistics.jitter, StatsReport::Value::kUInt32),
  });
#endif
}

void ReceiveStatisticsProxy::CNameChanged(const char* cname, uint32_t ssrc) {
  CriticalSectionScoped lock(crit_.get());
  stats_.c_name = cname;
  stats_.ssrc = ssrc;
}

void ReceiveStatisticsProxy::DataCountersUpdated(
    const yuntongxunwebrtc::StreamDataCounters& counters,
    uint32_t ssrc) {
  CriticalSectionScoped lock(crit_.get());
  stats_.rtp_stats = counters;
  stats_.ssrc = ssrc;
#ifdef WIN32
  post_message(StatsReport::kStatsReportTypeVideoRecv,
  {
	  StatsReport::Value(StatsReport::kStatsValueNameSsrc, ssrc, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNamePacketsReceived, counters.transmitted.packets, StatsReport::Value::kUInt32),
	  StatsReport::Value(StatsReport::kStatsValueNameBytesReceived, counters.transmitted.payload_bytes, StatsReport::Value::kUInt32),
  });
#endif
}

void ReceiveStatisticsProxy::OnDecodedFrame() {
  uint64_t now = clock_->TimeInMilliseconds();

  CriticalSectionScoped lock(crit_.get());
  decode_fps_estimator_.Update(1, now);
  stats_.decoded_framerate = decode_fps_estimator_.Rate(now).value_or(0);
#ifdef WIN32
  post_message(StatsReport::kStatsReportTypeVideoRecv,
  {
	  StatsReport::Value(StatsReport::kStatsValueNameDecoderFrameRate, stats_.decoded_framerate, StatsReport::Value::kUInt32)
  });
#endif
}

void ReceiveStatisticsProxy::OnRenderedFrame() {
  uint64_t now = clock_->TimeInMilliseconds();
  CriticalSectionScoped lock(crit_.get());
  renders_fps_estimator_.Update(1, now);
  //stats_.rendered_framerate = renders_fps_estimator_.Rate(now);
#ifdef WIN32
  post_message(StatsReport::kStatsReportTypeVideoRecv,
  {
	  StatsReport::Value(StatsReport::kStatsValueNameFrameRateRender, stats_.rendered_framerate, StatsReport::Value::kUInt32)
  });
#endif
}

void ReceiveStatisticsProxy::OnDiscardedPacketsUpdated(int discarded_packets) {
  CriticalSectionScoped lock(crit_.get());
  stats_.discarded_packets = discarded_packets;
#ifdef WIN32
  post_message(StatsReport::kStatsReportTypeVideoRecv,
  {
	  StatsReport::Value(StatsReport::kStatsValueNameDiscardedPackets, discarded_packets, StatsReport::Value::kUInt32)
  });
#endif
}

int64_t ReceiveStatisticsProxy::TimeUntilNextProcess()
{
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000;
	return kUpdateIntervalMs - (now - last_process_time_);
}

int32_t ReceiveStatisticsProxy::Process()
{
	updateEvent_->Wait(100);
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000; 

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
#ifdef WIN32
void ReceiveStatisticsProxy::post_message(int reportType, std::initializer_list<StatsReport::Value> values)
{
#ifdef ENABLE_LIB_CURL
	std::stringstream ss;
	for (auto it : values)
	{
		StatsReport::Value value = it;
		switch (value.type())
		{
		case StatsReport::Value::kUInt8:
			ss << "," << value.display_name() << ":" << value.uint8_val();
			break;
		case StatsReport::Value::kUInt16:
			ss << "," << value.display_name() << ":" << value.uint16_val();
			break;
		case StatsReport::Value::kUInt32:
			ss << "," << value.display_name() << ":" << value.uint32_val();
			break;
		case StatsReport::Value::kUInt64:
			ss << "," << value.display_name() << ":" << value.uint64_val();
			break;
		case StatsReport::Value::kString:
			ss << "," << value.display_name() << ":" << value.string_val();
			break;
		case StatsReport::Value::kStaticString:
			ss << "," << value.display_name() << ":" << value.static_string_val();
			break;
		case StatsReport::Value::kBool:
			ss << "," << value.display_name() << ":" << value.bool_val();
			break;
		default:
			break;
		}
	}
	//curl_post_message(reportType, ss.str().c_str());
#endif
}
#endif
}  // namespace webrtc
