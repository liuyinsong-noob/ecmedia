/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_SEND_STATISTICS_PROXY_H_
#define WEBRTC_VIDEO_SEND_STATISTICS_PROXY_H_

#include <string>
#ifdef WIN32
#include "initializer_list"
#endif
#include "module.h"

#include "../base/thread_annotations.h"
#include "common_types.h"
#ifdef VIDEO_ENABLED
#include "video_codec_interface.h"
#include "../system_wrappers/include/clock.h"
#include "../system_wrappers/include/scoped_ptr.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "video_send_stream.h"
#include "video_coding_defines.h"
#endif
//#include "clock.h"
//#include "scoped_ptr.h"


#include "../system_wrappers/include/event_wrapper.h"
#include "../system_wrappers/include/file_wrapper.h"
#include "call_stats.h"
#include "bitrate_controller.h"
#include "remote_bitrate_estimator.h"
#include "rtp_rtcp_defines.h"
#include "overuse_frame_detector.h"
//#include "constructormagic.h"
#include "../system_wrappers/include/stats_types.h"

namespace yuntongxunwebrtc {

class ViEEncoderObserver;
class VideoEncoderRateObserver;
class ViECaptureObserver;
class CriticalSectionWrapper;

class SendStatisticsProxy : public Module,
#ifdef VIDEO_ENABLED
							public ViEEncoderObserver,
							public VideoEncoderRateObserver,
#endif
							public RtcpStatisticsCallback,
							public RtcpPacketTypeCounterObserver,
							public FrameCountObserver,
							public StreamDataCountersCallback,
							public BitrateStatisticsObserver,
							public SendSideDelayObserver,
#ifdef VIDEO_ENABLED
							public ViECaptureObserver,
#endif
							public CpuOveruseObserver,
							public SendsideBweObserver,
							public CallStatsObserver{
 public:
  SendStatisticsProxy(int video_channel_);
  virtual ~SendStatisticsProxy();

  //Implement module
  virtual int64_t TimeUntilNextProcess();
  virtual int32_t Process();
#ifdef VIDEO_ENABLED
  VideoSendStream::Stats GetStats(bool isAvg, int64_t& timestamp);
  int NumberOfSimulcastStreams();
#endif


public:
	static const int kStatsTimeoutMs;
	// Implements ViEEncoderObserver.
	//int32_t RegisterEncoderObserver(ViEEncoderObserver* observer);
#ifdef VIDEO_ENABLED
	virtual void OutgoingRate(const int video_channel,
		const unsigned int framerate,
		const unsigned int bitrate) OVERRIDE;

	virtual void SuspendChange(int video_channel, bool is_suspended) OVERRIDE;

	// Implements VideoEncoderRateObserver.
	//int32_t RegisterEncoderRateObserver(VideoEncoderRateObserver *observer);
	virtual void OnSetRates(uint32_t bitrate_bps, int framerate) override;
#endif
	// From RtcpStatisticsCallback.
	// virtual int RegisterSendChannelRtcpStatisticsCallback( int channel, RtcpStatisticsCallback* callback);
	void StatisticsUpdated(const RtcpStatistics& statistics,
		uint32_t ssrc) override;
	void CNameChanged(const char* cname, uint32_t ssrc) override;

	// From RtcpPacketTypeCounterObserver. 
	// void RegisterReceiveRtcpPacketTypeCounterObserver(RtcpPacketTypeCounterObserver* observer); //vie_channel
	void RtcpPacketTypesCounterUpdated(
		uint32_t ssrc,
		const RtcpPacketTypeCounter& packet_counter) override;

	// From FrameCountObserver.
	//void RegisterSendFrameCountObserver(FrameCountObserver* observer);
	void FrameCountUpdated(const FrameCounts& frame_counts,
		uint32_t ssrc) override {};

	// From StreamDataCountersCallback.
	// RegisterSendChannelRtpStatisticsCallback(int channel, RtcpStatisticsCallback* callback);
	void DataCountersUpdated(const StreamDataCounters& counters,
		uint32_t ssrc) override;

	// From BitrateStatisticsObserver.
	//RegisterSendBitrateObserver(int channel, BitrateStatisticsObserver* callback);
	void Notify(uint32_t total_stats,
		uint32_t retransmit_stats,
		uint32_t ssrc);

	//From SendSideDelayObserver
	void SendSideDelayUpdated(int avg_delay_ms,
		int max_delay_ms,
		uint32_t ssrc) override;

	//Implement ViECaptureObserver
	// This method is called if a bright or dark captured image is detected.
#ifdef VIDEO_ENABLED
	void BrightnessAlarm(const int capture_id,
		const Brightness brightness) override {}

	// This method is called periodically telling the capture device frame rate.
	void CapturedFrameRate(const int capture_id,
						const unsigned char frame_rate) override;

	// This method is called if the capture device stops delivering images to
	// VideoEngine.
	void NoPictureAlarm(const int capture_id,
						const CaptureAlarm alarm) override{}
#endif
	//Implement CpuOveruseObserver
	// Called as soon as an overuse is detected.
	void OveruseDetected()  override {}
	// Called periodically when the system is not overused any longer.
	void NormalUsage() override {}

	void FrameSizeChanged(const int width,
						  const int height) override;

	void CpuOveruseMetricsMeasurements(const CpuOveruseMetrics &metrics) override;
	//Implement BandwidthEstimationObserver
	virtual void OnSendsideBwe(uint32_t* estimated_bandwidth,
								uint8_t *loss,
								int64_t *rtt) override;


	//from QMsetting
	void OnQMSettingChange(const uint32_t frame_rate,
							const uint32_t width,
							const uint32_t height);

	//Implement CallStatsObserver
	virtual void OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms);

	//From vie::bucketDelay
	void OnBucketDelay(int64_t delayInMs);
	void SetRemoteBitrateEstimator(RemoteBitrateEstimator *remote_bitrate_estimator);
	

	int channelId() { return channel_id_; };
	int GetAvailableReceiveBandwidth();
	int GetAvailableSendBandwidth();
	int64_t GetBucketDelay();
	int GetRtt();

	void SetSsrcs(const std::list<unsigned int> &ssrcs);
#ifdef VIDEO_ENABLED
	VideoSendStream::StreamStats* GetStatsEntry(bool isAvg, const uint32_t ssrc);
	void ConfigEncoderSetting(const VideoCodec& video_codec);
	VideoSendStream::Config GetConfig() const;
#endif
	bool newConfig() { return new_config_; }
	void ResetNewConfig() { new_config_ = false; }
private:
#ifdef VIDEO_ENABLED
	VideoSendStream::StreamStats* GetStreamStats(uint32_t ssrc);
#endif
	void GenerateAvgStats();

private:
	class RateCounter {
	public:
		RateCounter();
		~RateCounter();
		static const int kStatsRateTimeoutMs;
		typedef std::pair<int64_t, uint32_t> Sample;
		typedef std::list<Sample> SampleList;
		void AddSample(uint32_t rate);
		uint32_t AvgRate();
	private:
		void PurgeOldStats();
		SampleList sample_list_;
	};

	class RtcpBlocksCounter {
	public:
		typedef std::pair<int64_t, RtcpStatistics> RtcpSample;
		typedef std::list<RtcpSample> RtcpSampleList;
		void AddSample(const RtcpStatistics& rtcp_stats);
		uint8_t AvgFractionLost() const;
		uint8_t AvgJitter() const;
	private:
		void PurgeOldStats();
		RtcpSampleList sample_list_;
	};

	class RtpBlocksCounter {
	public:
		typedef std::pair<int64_t, StreamDataCounters> RtpSample;
		typedef std::list<RtpSample> RtpSampleList;
		void AddSample(const StreamDataCounters& rtp_stats);
	private:
		void PurgeOldStats();
		RtpSampleList sample_list_;
		
	};

	class BitrateStatsCounter {
	public:
		typedef std::pair<int64_t, BitrateStatistics> BitrateSample;
		typedef std::list<BitrateSample> BitrateSampleList;
		void AddSample(const BitrateStatistics& sample);
		uint32_t AvgBitbitRate();
		uint32_t AvgPacketRate();
	private:
		void PurgeOldStats();
		BitrateSampleList sample_list_;
	};

	typedef std::map<uint32_t, RtcpBlocksCounter> RtcpBlocksCounterMap;
	typedef std::map<uint32_t, BitrateStatsCounter> BitrateStatsCounterMap;
#ifdef WIN32
	void post_message(int reportType, std::initializer_list<StatsReport::Value> values);
#endif

	struct AverageRateStats {
		RateCounter actual_enc_framerate;
		RateCounter actual_enc_bitrate_bps;
		RateCounter	target_enc_framerate;
		RateCounter target_enc_bitrate_bps;
		RateCounter avg_encode_time_ms;
		RateCounter encode_usage_percent;
		RateCounter rtt_ms;
		RateCounter pacer_delay_ms;
		RateCounter sendside_bwe_bps;
		RateCounter recv_bandwidth_bps;
	
		RtcpBlocksCounterMap rtcp_blocks_map;
		BitrateStatsCounterMap total_stats_map;
		BitrateStatsCounterMap retransmit_stats_map;
	};

private:
	int					channel_id_;
	Clock*              clock_;
	int64_t             last_process_time_;		
	EventWrapper*		 updateEvent_;
	scoped_ptr<CriticalSectionWrapper> crit_;
#ifdef VIDEO_ENABLED
	VideoSendStream::Stats		stats_ GUARDED_BY(crit_);  //for now
	VideoSendStream::Stats		stats_average_ GUARDED_BY(crit_); // for history
#endif
	AverageRateStats		avg_rate_stats_ GUARDED_BY(crit_);
	RemoteBitrateEstimator*	remote_bitrate_estimator_;  //TODO: remove or set as callback
	bool					new_config_;
	unsigned char			captured_framerate_;
	DISALLOW_COPY_AND_ASSIGN(SendStatisticsProxy);
};

}  // namespace webrtc
#endif  // WEBRTC_VIDEO_SEND_STATISTICS_PROXY_H_
