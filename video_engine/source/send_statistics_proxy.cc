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

#ifdef ENABLE_LIB_CURL
#ifdef	WIN32
#include "curl_post.h"
extern CurlPost *g_curlpost;
#endif
#endif

#include "stats_types.h"
#define REPORT_TYPE "video_send_statistics"

#define kKiloBits 1000

namespace cloopenwebrtc {

const int SendStatisticsProxy::kStatsTimeoutMs = 5000;

const int SendStatisticsProxy::RateCounter::kStatsRateTimeoutMs = 5000;
SendStatisticsProxy::RateCounter::RateCounter() {

}

SendStatisticsProxy::RateCounter::~RateCounter() {
	sample_list_.clear();
}

void SendStatisticsProxy::RateCounter::AddSample(uint32_t rate)
{
	Sample sample(std::make_pair(Time(), rate));
	PurgeOldStats();
	sample_list_.push_back(sample);
}

uint32_t SendStatisticsProxy::RateCounter::AvgRate()
{
	int num = 0;
	uint32_t sum = 0;
	for (auto it: sample_list_)
	{
		sum += it.second;
		num++;
	}
	if (num == 0)
	{
		return 0;
	}
	return sum / num + 0.5;
}

void SendStatisticsProxy::RateCounter::PurgeOldStats()
{
	int64_t old_stats_ms = cloopenwebrtc::Time() - kStatsTimeoutMs;
	std::list<Sample>::iterator it = sample_list_.begin();
	while(it!= sample_list_.end())
	{
		int64_t sample_time = it->first;
		if (sample_time < old_stats_ms)
		{
			it++;
			sample_list_.pop_front();
			if (sample_list_.size() == 0)
			{
				break;
			}
		}
		else
			it++;
	}
}

void SendStatisticsProxy::RtcpBlocksCounter::AddSample(const RtcpStatistics& rtcp_stats)
{
	RtcpSample sample(std::make_pair(Time(), rtcp_stats));
	PurgeOldStats();
	sample_list_.push_back(sample);
}

void SendStatisticsProxy::RtcpBlocksCounter::PurgeOldStats()
{
	int64_t old_stats_ms = cloopenwebrtc::Time() - kStatsTimeoutMs;
	std::list<RtcpSample>::iterator it = sample_list_.begin();
	while(it != sample_list_.end())
	{
		int64_t sample_time = it->first;
		if (sample_time < old_stats_ms)
		{
			it++;
			sample_list_.pop_front();
			if (sample_list_.size() == 0)
			{
				break;
			}
		}
		else
			it++;
	}
}

uint8_t SendStatisticsProxy::RtcpBlocksCounter::AvgFractionLost() const
{
	uint32_t sum = 0;
	uint32_t num = 0;
	for (auto it:sample_list_)
	{
		sum += it.second.fraction_lost;
		num++;
	}
	return sum / num +0.5;
}

uint8_t SendStatisticsProxy::RtcpBlocksCounter::AvgJitter() const
{
	uint32_t sum = 0;
	uint32_t num = 0;
	for (auto it : sample_list_)
	{
		sum += it.second.jitter;
		num++;
	}
	return sum / num + 0.5;
}



void SendStatisticsProxy::BitrateStatsCounter::AddSample(const BitrateStatistics& stats)
{
	BitrateSample sample(std::make_pair(Time(), stats));
	PurgeOldStats();
	sample_list_.push_back(sample);
}

void SendStatisticsProxy::BitrateStatsCounter::PurgeOldStats()
{
	int64_t old_stats_ms = cloopenwebrtc::Time() - kStatsTimeoutMs;
	std::list<BitrateSample>::iterator it = sample_list_.begin();
	while(it != sample_list_.end())
	{
		int64_t sample_time = it->first;
		if (sample_time < old_stats_ms)
		{
			it++;
			sample_list_.pop_front();
			if (sample_list_.size() == 0)
			{
				break;
			}
		}
		else
			it++;
	}
}

uint32_t SendStatisticsProxy::BitrateStatsCounter::AvgBitbitRate()
{
	int num = 0;
	uint32_t sum = 0;
	for (auto it : sample_list_)
	{
		sum += it.second.bitrate_bps;
		num++;
	}
	if (num == 0)
	{
		return 0;
	}
	return sum / num + 0.5;
}

uint32_t SendStatisticsProxy::BitrateStatsCounter::AvgPacketRate()
{
	int num = 0;
	uint32_t sum = 0;
	for (auto it : sample_list_)
	{
		sum += it.second.packet_rate;
		num++;
	}
	if (num == 0)
	{
		return 0;
	}
	return sum / num + 0.5;
}

SendStatisticsProxy::SendStatisticsProxy(int video_channel)
	: channel_id_(video_channel),
	crit_(CriticalSectionWrapper::CreateCriticalSection()),
	clock_(Clock::GetRealTimeClock()),
	last_process_time_(clock_->TimeInMilliseconds()),
	updateEvent_(EventWrapper::Create()),
	remote_bitrate_estimator_(NULL),
	new_config_(false){
}

SendStatisticsProxy::~SendStatisticsProxy() {
	updateEvent_->Set();
	delete updateEvent_;
}

void SendStatisticsProxy::OutgoingRate(const int video_channel,
	const unsigned int framerate,
	const unsigned int bitrate) {
	CriticalSectionScoped lock(crit_.get());
	stats_.actual_enc_framerate = framerate;
	stats_.actual_enc_bitrate_bps = bitrate;

	avg_rate_stats_.actual_enc_framerate.AddSample(framerate);
	avg_rate_stats_.actual_enc_bitrate_bps.AddSample(bitrate);
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameActualEncFrameRate,stats_.actual_enc_framerate, StatsReport::Value::kUInt8),
		StatsReport::Value(StatsReport::kStatsValueNameActualEncBitrate,stats_.actual_enc_bitrate_bps, StatsReport::Value::kUInt16)
	});
#endif
}

void SendStatisticsProxy::SuspendChange(int video_channel, bool is_suspended) {
	CriticalSectionScoped lock(crit_.get());
	stats_.suspended = is_suspended;	
	if (is_suspended)
	{
		//TODO: alert
	} 
}

VideoSendStream::Stats SendStatisticsProxy::GetStats(bool isAvg, int64_t& timestamp) {
	CriticalSectionScoped lock(crit_.get());
	timestamp = cloopenwebrtc::Time();
	if (isAvg)
	{
		//TODO: first copy stats_, then copy stats_avg
		GenerateAvgStats();		
		return stats_average_;
	}
	else {
		return  stats_;
	}
}

void SendStatisticsProxy::GenerateAvgStats()
{
	stats_average_ = stats_;
	stats_average_.actual_enc_bitrate_bps = avg_rate_stats_.actual_enc_bitrate_bps.AvgRate();
	stats_average_.actual_enc_framerate = avg_rate_stats_.actual_enc_framerate.AvgRate();
	stats_average_.avg_encode_time_ms = avg_rate_stats_.avg_encode_time_ms.AvgRate();
	stats_average_.encode_usage_percent = avg_rate_stats_.encode_usage_percent.AvgRate();
	stats_average_.target_enc_bitrate_bps = avg_rate_stats_.target_enc_bitrate_bps.AvgRate();
	stats_average_.target_enc_framerate = avg_rate_stats_.target_enc_framerate.AvgRate();
	stats_average_.call.rtt_ms = avg_rate_stats_.rtt_ms.AvgRate();
	stats_average_.call.pacer_delay_ms = avg_rate_stats_.pacer_delay_ms.AvgRate();
	stats_average_.call.sendside_bwe_bps = avg_rate_stats_.sendside_bwe_bps.AvgRate();

	for (auto it : stats_average_.ssrc_streams)
	{
		uint32_t ssrc = it;
		VideoSendStream::StreamStats *stream = GetStatsEntry(true, ssrc);
		std::map<uint32_t, RtcpBlocksCounter>::iterator it_rtcp = avg_rate_stats_.rtcp_blocks_map.find(ssrc);
		std::map<uint32_t,BitrateStatsCounter>::iterator it_total = avg_rate_stats_.total_stats_map.find(ssrc);
		std::map<uint32_t, BitrateStatsCounter>::iterator it_retransmit = avg_rate_stats_.total_stats_map.find(ssrc);
		if (stream && it_total!= avg_rate_stats_.total_stats_map.end())
		{
			stream->total_stats.bitrate_bps = it_total->second.AvgBitbitRate();
			stream->total_stats.packet_rate = it_total->second.AvgPacketRate();
			stream->retransmit_stats.bitrate_bps = it_retransmit->second.AvgBitbitRate();
			stream->retransmit_stats.packet_rate = it_retransmit->second.AvgPacketRate();
		}
		if (stream && it_rtcp != avg_rate_stats_.rtcp_blocks_map.end())
		{
			stream->rtcp_stats.fraction_lost = it_rtcp->second.AvgFractionLost();
			stream->rtcp_stats.jitter = it_rtcp->second.AvgJitter();
		}
	}	
}

void SendStatisticsProxy::OnQMSettingChange(const uint32_t frame_rate,
											const uint32_t width,
											const uint32_t height) {
	CriticalSectionScoped lock(crit_.get());
	stats_.qm_width = width;
	stats_.qm_height = height;	
	stats_.qm_framerate = frame_rate;
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameQMFrameWidth, stats_.qm_width, StatsReport::Value::kUInt8),
		StatsReport::Value(StatsReport::kStatsValueNameQMFrameHeight, stats_.qm_height, StatsReport::Value::kUInt8),
		StatsReport::Value(StatsReport::kStatsValueNameQMFrameRate, stats_.qm_framerate, StatsReport::Value::kUInt8)
	});
#endif
}

void SendStatisticsProxy::OnSetRates(uint32_t bitrate_bps, int framerate) {
	CriticalSectionScoped lock(crit_.get());
	stats_.target_enc_bitrate_bps = bitrate_bps;
	stats_.target_enc_framerate = framerate;
	avg_rate_stats_.target_enc_bitrate_bps.AddSample(bitrate_bps);
	avg_rate_stats_.target_enc_framerate.AddSample(framerate);
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameTargetEncBitrate, stats_.target_enc_bitrate_bps, StatsReport::Value::kUInt16),
		StatsReport::Value(StatsReport::kStatsValueNameTargetEncFrameRate, stats_.target_enc_framerate,	StatsReport::Value::kUInt8)
	});
#endif
}

void SendStatisticsProxy::StatisticsUpdated(const RtcpStatistics& statistics,
	uint32_t ssrc) {
	CriticalSectionScoped lock(crit_.get());	
	VideoSendStream::StreamStats *stream = GetStreamStats(ssrc);
	if (!stream)
	{
		return;
	}
	stream->rtcp_stats = statistics;
	
	std::map<uint32_t, RtcpBlocksCounter>::iterator it = avg_rate_stats_.rtcp_blocks_map.find(ssrc);
	if (it == avg_rate_stats_.rtcp_blocks_map.end())
	{
		RtcpBlocksCounter rtcp_block;
		rtcp_block.AddSample(statistics);
		avg_rate_stats_.rtcp_blocks_map[ssrc] = rtcp_block;
	}
	else {
		RtcpBlocksCounter rtcp_block = it->second;
		rtcp_block.AddSample(statistics);
	}
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameSsrc, ssrc, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNamePacketsLost, statistics.cumulative_lost,StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameLossFractionInPercent, statistics.fraction_lost,	StatsReport::Value::kUInt8),
		StatsReport::Value(StatsReport::kStatsValueNameJitterReceived, statistics.jitter,	StatsReport::Value::kUInt16)
	});
#endif
}

void SendStatisticsProxy::CNameChanged(const char* cname, uint32_t ssrc) {
}


void SendStatisticsProxy::RtcpPacketTypesCounterUpdated(
	uint32_t ssrc,
	const RtcpPacketTypeCounter& packet_counter) {
	CriticalSectionScoped lock(crit_.get());
	VideoSendStream::StreamStats *stream = GetStreamStats(ssrc);
	if (!stream)
	{
		return;
	}
	stream->rtcp_packet_type_counts = packet_counter;
#ifdef WIN32	
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameSsrc, ssrc, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameFirsReceived, packet_counter.fir_packets, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameNacksReceived, packet_counter.nack_packets, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameNacksRequestsReceived, packet_counter.nack_requests, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameNacksUniqueRequestsReceived, packet_counter.unique_nack_requests, StatsReport::Value::kUInt32)
	});
#endif
}


void SendStatisticsProxy::DataCountersUpdated(const StreamDataCounters& counters,
	uint32_t ssrc) {
	CriticalSectionScoped lock(crit_.get());
	VideoSendStream::StreamStats *stream = GetStreamStats(ssrc);
	if (!stream)
	{
		return;
	}
	stream->rtp_stats = counters;
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameSsrc, ssrc, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameBytesSent, counters.bytes, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNamePacketsSent, counters.packets, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameRetransmittedBytes, counters.retransmitted_bytes, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameRetransmittedPackets, counters.retransmitted_packets, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameFecPackets, counters.fec_packets, StatsReport::Value::kUInt32)
	});
#endif
}

void SendStatisticsProxy::Notify(const BitrateStatistics& total_stats,
	const BitrateStatistics& retransmit_stats,
	uint32_t ssrc) {
	CriticalSectionScoped lock(crit_.get());
	VideoSendStream::StreamStats *stream = GetStreamStats(ssrc);
	if (!stream)
	{
		return;
	}
	stream->total_stats = total_stats;
	stream->retransmit_stats = retransmit_stats;

	
	std::map<uint32_t, BitrateStatsCounter>::iterator it_total =  avg_rate_stats_.total_stats_map.find(ssrc);
	std::map<uint32_t, BitrateStatsCounter>::iterator it_retransmit = avg_rate_stats_.retransmit_stats_map.find(ssrc);
	if (it_total == avg_rate_stats_.total_stats_map.end())
	{
		BitrateStatsCounter total;
		BitrateStatsCounter retransmit;
		total.AddSample(total_stats);
		retransmit.AddSample(retransmit_stats);
		avg_rate_stats_.total_stats_map[ssrc] = total;
		avg_rate_stats_.retransmit_stats_map[ssrc] = retransmit;
	}
	else {
		it_total->second.AddSample(total_stats);
		it_retransmit->second.AddSample(retransmit_stats);
	}
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameSsrc,ssrc, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameTransmitBitrate, total_stats.bitrate_bps, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameRetransmitBitrate, retransmit_stats.bitrate_bps, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameTransmitPacketsRate, total_stats.packet_rate, StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameRetransmitPacketsRate, retransmit_stats.packet_rate, StatsReport::Value::kUInt32),
	});
#endif
}

void SendStatisticsProxy::SendSideDelayUpdated(int avg_delay_ms,
												int max_delay_ms,
												uint32_t ssrc) {
	CriticalSectionScoped lock(crit_.get());
	VideoSendStream::StreamStats *stream = GetStreamStats(ssrc);
	if (!stream)
	{
		return;
	}
	stream->avg_delay_ms = avg_delay_ms;
	stream->max_delay_ms = max_delay_ms;
}

void SendStatisticsProxy::OnRttUpdate(int64_t rtt_ms)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.call.rtt_ms = rtt_ms;
	avg_rate_stats_.rtt_ms.AddSample(rtt_ms);
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameRttInMs, rtt_ms, StatsReport::Value::kUInt64)
	});
#endif
}

void SendStatisticsProxy::OnBucketDelay(int64_t delayInMs)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.call.pacer_delay_ms = delayInMs;
	avg_rate_stats_.pacer_delay_ms.AddSample(delayInMs);
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameBucketDelayInMs, stats_.call.pacer_delay_ms, StatsReport::Value::kUInt64)
	});
#endif
}

int64_t SendStatisticsProxy::TimeUntilNextProcess()
{
	//return -1;
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000;
	return kUpdateIntervalMs - (now - last_process_time_);
}

int32_t SendStatisticsProxy::Process()
{
	//return -1;
	updateEvent_->Wait(100);
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = 1000; //1000ms定时器
	if (now >= last_process_time_ + kUpdateIntervalMs) {
		CriticalSectionScoped lock(crit_.get());
		//统计各种发送信息
		std::vector<uint32_t> ssrcs;
		last_process_time_ = now;
		
		if (remote_bitrate_estimator_)
		{
			remote_bitrate_estimator_->LatestEstimate(&ssrcs, &stats_.call.recv_bandwidth_bps);
			avg_rate_stats_.recv_bandwidth_bps.AddSample(stats_.call.recv_bandwidth_bps);
		}
		stats_average_ = stats_;
	}
	return 0;
}

void SendStatisticsProxy::SetRemoteBitrateEstimator(RemoteBitrateEstimator *remote_bitrate_estimator)
{
	remote_bitrate_estimator_ = remote_bitrate_estimator;
}

int SendStatisticsProxy::GetAvailableReceiveBandwidth() {
	CriticalSectionScoped lock(crit_.get());
	return stats_.call.recv_bandwidth_bps;
}
int SendStatisticsProxy::GetAvailableSendBandwidth() {
	CriticalSectionScoped lock(crit_.get());
	return stats_.call.sendside_bwe_bps;
}
int64_t SendStatisticsProxy::GetBucketDelay() {
	CriticalSectionScoped lock(crit_.get());
	return stats_.call.pacer_delay_ms;
}
int SendStatisticsProxy::GetRtt() {
	CriticalSectionScoped lock(crit_.get());
	return stats_.call.rtt_ms;
}

void SendStatisticsProxy::SetSsrcs(const std::list<unsigned int> &ssrcs)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.ssrc_streams.clear();
	for (std::list<unsigned int>::const_iterator it = ssrcs.begin(); it != ssrcs.end(); ++it)
	{
		unsigned int ssrc = *it;
		stats_.ssrc_streams.push_back(ssrc);
	}
}

void SendStatisticsProxy::CapturedFrameRate(const int capture_id,
											const unsigned char frame_rate)
{
	//TODO: compare capture_id?
	stats_.captured_framerate = frame_rate;
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameCapturedFrameRate, stats_.captured_framerate, StatsReport::Value::kUInt8),
	});
#endif
}

void SendStatisticsProxy::FrameSizeChanged(const int width,
										   const int height) {
	CriticalSectionScoped lock(crit_.get());
	stats_.captured_width = width;
	stats_.captured_height = height;	
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameCapturedFrameWidth, stats_.captured_width, StatsReport::Value::kUInt8),
		StatsReport::Value(StatsReport::kStatsValueNameCapturedFrameHeight, stats_.captured_height, StatsReport::Value::kUInt8)
	});
#endif
}

void SendStatisticsProxy::CpuOveruseMetricsMeasurements(const CpuOveruseMetrics &metrics)
{
	CriticalSectionScoped lock(crit_.get());
	stats_.avg_encode_time_ms = metrics.avg_encode_time_ms;
	stats_.encode_usage_percent = metrics.encode_usage_percent;

	avg_rate_stats_.avg_encode_time_ms.AddSample(metrics.avg_encode_time_ms);
	avg_rate_stats_.encode_usage_percent.AddSample(metrics.encode_usage_percent);
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameAvgEncodeMs, stats_.avg_encode_time_ms, StatsReport::Value::kUInt8),
		StatsReport::Value(StatsReport::kStatsValueNameEncodeUsagePercent, stats_.encode_usage_percent, StatsReport::Value::kUInt8)
	});
#endif
}

void SendStatisticsProxy::OnSendsideBwe(uint32_t* estimated_bandwidth, uint8_t *loss, int64_t *rtt)
{
	stats_.call.sendside_bwe_bps = *estimated_bandwidth;
	avg_rate_stats_.sendside_bwe_bps.AddSample(*estimated_bandwidth);
#ifdef WIN32
	post_message(StatsReport::kStatsReportTypeVideoSend,
	{
		StatsReport::Value(StatsReport::kStatsValueNameAvailableSendBandwidth, (*estimated_bandwidth), StatsReport::Value::kUInt32),
		StatsReport::Value(StatsReport::kStatsValueNameLossFractionInPercent, (*loss), StatsReport::Value::kUInt8)
	});
#endif
}

VideoSendStream::StreamStats* SendStatisticsProxy::GetStatsEntry(bool isAvg, const uint32_t ssrc)
{
	VideoSendStream::Stats *pStats = &stats_;
	if (isAvg)
	{
		pStats = &stats_average_;
	}
	
	std::map<uint32_t, VideoSendStream::StreamStats>::iterator it = pStats->substreams.find(ssrc);
	if (it != pStats->substreams.end())
	{
		return &it->second;
	}
	if (std::find(pStats->ssrc_streams.begin(), pStats->ssrc_streams.end(), static_cast<unsigned int>(ssrc)) ==
		pStats->ssrc_streams.end())
	{
		return nullptr;
	}
	return &pStats->substreams[ssrc];
}

int SendStatisticsProxy::NumberOfSimulcastStreams()
{
	return stats_.config.encoder_settings.numberOfSimulcastStreams;
}
void SendStatisticsProxy::ConfigEncoderSetting(const VideoCodec& video_codec)
{
	VideoSendStream::Config &config_ = stats_.config;
	config_.encoder_settings.payload_name = video_codec.plName;
	config_.encoder_settings.payload_type = video_codec.plType;
	config_.encoder_settings.width = video_codec.width;
	config_.encoder_settings.height = video_codec.height;
	config_.encoder_settings.maxFramerate = video_codec.maxFramerate;
	config_.encoder_settings.startBitrate = video_codec.startBitrate;
	config_.encoder_settings.maxBitrate = video_codec.maxBitrate;
	config_.encoder_settings.minBitrate = video_codec.minBitrate;
	config_.encoder_settings.targetBitrate = video_codec.targetBitrate;
	config_.encoder_settings.codecSpecific = video_codec.codecSpecific;
	config_.encoder_settings.numberOfSimulcastStreams = video_codec.numberOfSimulcastStreams;
	for (int i = 0; i < video_codec.numberOfSimulcastStreams; i++) {
		config_.encoder_settings.simulcastStream[i] = video_codec.simulcastStream[i];
	}
	stats_.encoder_implementation_name = video_codec.plName;
	new_config_ = true;

#ifdef ENABLE_LIB_CURL
#ifdef	WIN32
	if (g_curlpost)
		g_curlpost->AddMessage(StatsReport::kStatsReportTypeVideoSend, config_.ToString());
#endif
#endif
}

VideoSendStream::Config SendStatisticsProxy::GetConfig() const
{
	return stats_.config;
}

VideoSendStream::StreamStats* SendStatisticsProxy::GetStreamStats(uint32_t ssrc)
{
	if (NumberOfSimulcastStreams() == 0) {
		return &stats_.stream;
	}
	else 
		return GetStatsEntry(false, ssrc);
}

#ifdef WIN32
void SendStatisticsProxy::post_message(int reportType,
											std::initializer_list<StatsReport::Value> values)
{
	std::stringstream ss;
	for (auto it:values)
	{
		StatsReport::Value value=it;
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
#ifdef ENABLE_LIB_CURL
#ifdef	WIN32
	if (g_curlpost)
		g_curlpost->AddMessage(reportType, ss.str());
#endif
#endif
}
#endif
}  // namespace webrtc

