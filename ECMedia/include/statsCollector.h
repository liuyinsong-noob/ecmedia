#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H
#include <set>
#include "send_statistics_proxy.h"
#include "receive_statistics_proxy.h"
#include "audio_send_stream.h"
#include "audio_receive_stream.h"
#include "event_wrapper.h"
#include "file_wrapper.h"
#include "clock.h"
#include "stats_types.h"

#include "MediaStatisticsData.pb.h"

#ifdef VIDEO_ENABLED
#include "vie_base.h"
#endif

#include "voe_base.h"

using namespace cloopenwebrtc;
class StatsCollector{
public:
	StatsCollector();
	~StatsCollector();
	static bool StatsCollectorThreadRun(void* obj);
	bool ProcessStatsCollector();

	bool AddVideoSendStatsProxy(int channelid);
	void DeleteVideoSendStatsProxy(int channelid);
	bool AddVideoRecvStatsProxy(int channelid);
	void DeleteVideoRecvStatsProxy(int channelid);
	bool AddAudioSendStatsProxy(int channelid);
	void DeleteAudioSendStatsProxy(int channelid);
	bool AddAudioRecvStatsProxy(int channelid);
	void DeleteAudioRecvStatsProxy(int channelid);

	bool SetVideoEngin(VideoEngine* vie);
	bool SetVoiceEngin(VoiceEngine* voe);
	void SetVideoCaptureDeviceId(int capDevId) { capDevId_ = capDevId; }
	void Config(char* logFile, int logIntervalMs);


	void GetStats(StatsContentType type, char* callid, MediaStatisticsDataInner **mediaStatisticsDataInner);
	void DeletePbData(MediaStatisticsDataInner *mediaStatisticsDataInner);

private:
	void AddToReports(StatsReport::StatsReportType type, int channel_id);
	void DeleteFromReports(StatsReport::StatsReportType type, int channel_id);

	StatsReport* FindReport(bool isFullStats, int reportType, int channel_id);

	void ExtractVideoSenderInfo(bool isFullStats);
	void ExtractVideoReceiverInfo(bool isFullStats);
	void ExtractAudioSenderInfo(bool isFullStats);
	void ExtractAudioReceiverInfo(bool isFullStats);

	void Report_AddCommonFiled(StatsReport *report, int64_t ts);

	void VideoSenderInfo_AddEncoderSetting(const VideoSendStream::Stats info,
											StatsReport *report); //size = 15bytes
	void VideoSenderInfo_AddQMSetting(const VideoSendStream::Stats info,
										StatsReport *report); //size = 3bytes
	void VideoSenderInfo_AddCpuMetrics(const VideoSendStream::Stats info,
										StatsReport *report); //size = 2bytes
	void VideoSenderInfo_AddBweStats(const VideoSendStream::Stats info,
										StatsReport *report); //size = 8bytes
	void VideoSenderInfo_AddNetworkStats(const VideoSendStream::Stats info,
										StatsReport *report); //size = 7bytes
	void VideoSenderInfo_AddRtpStats(const VideoSendStream::Stats info,
										StatsReport *report); //size = 10bytes
	void VideoSenderInfo_AddRtcpStats(const VideoSendStream::Stats info,
										StatsReport *report); //size = 20bytes

	void VideoReciverInfo_AddReceiveBasic(const VideoReceiveStream::Stats info,
										StatsReport *report); //size = 3 bytes
	void VideoReciverInfo_AddReceiveStats(const VideoReceiveStream::Stats info,
										StatsReport *report); //size = 5 bytes
	void VideoReciverInfo_AddDecoderStats(const VideoReceiveStream::Stats info,
										StatsReport *report); //size = 3 bytes
	void VideoReciverInfo_AddRenderStats(const VideoReceiveStream::Stats info,
										StatsReport *report); //size = 5 bytes
	void VideoReciverInfo_AddRtpStats(const VideoReceiveStream::Stats info,
										StatsReport *report); //size = 12 bytes
	void VideoReciverInfo_AddRtcpStats(const VideoReceiveStream::Stats info,
										StatsReport *report); //size = 16 bytes
	void VideoReciverInfo_AddLossModeStats(const VideoReceiveStream::Stats info,
										StatsReport *report); //size = 32 bytes

	void AudioSenderInfo_AddBasic(const AudioSendStream::Stats info,
										StatsReport *report); //size = 9 bytes
	void AudioSenderInfo_AddEchoStats(const AudioSendStream::Stats info,
										StatsReport *report); //size = 16 bytes
	void AudioSenderInfo_AddNetworkStats(const AudioSendStream::Stats info,
										StatsReport *report); //size = 5 bytes


	void AudioReceiverInfo_AddBasic(const AudioReceiveStream::Stats info,
									StatsReport *report); //size = 9 bytes
	void AudioReceiverInfo_AddDecoderStats(const AudioReceiveStream::Stats info,
									StatsReport *report); //size = 16 bytess
	void AudioReceiverInfo_AddNetworkStats(const AudioReceiveStream::Stats info,
									StatsReport *report); //size = 23 bytess


	void LogToFile(bool isFullStats);

	void UpdateStats(bool isFullStats);
	void LoadReportsToPbBuffer(StatsContentType type, MediaStatisticsInner *mediaStatsInner);
	void LoadAudioSenderReportToPbBuffer(StatsContentType type,
										StatsReport report, 
										AudioSenderStatisticsInner *statsData);
	void LoadAudioReceiverReportToPbBuffer(StatsContentType type,
										StatsReport report,
										AudioReceiverStatisticsInner *statsData);
	void LoadVideoSenderReportToPbBuffer(StatsContentType type,
										StatsReport report,
										VideoSenderStatisticsInner *statsData);
	void LoadVideoReceiverReportToPbBuffer(StatsContentType type,
										StatsReport report,
										VideoReceiverStatisticsInner *statsData);
private:
	int		capDevId_;
	ThreadWrapper *thread_;
	char*		file_name_;
	FileWrapper *trace_file_;
	EventWrapper *updateEvent_;

	cloopenwebrtc::Clock*		clock_;
	uint32_t	updateIntervalMs_;
	int64_t		last_time_update_fullStats_;
	int64_t		last_time_update_simplifiedStats_;

	scoped_ptr<CriticalSectionWrapper> stream_crit_;
	std::set<SendStatisticsProxy*> video_send_stats_proxies_;
	std::set<ReceiveStatisticsProxy*> video_receive_stats_proxies_;
	std::set<AudioSendStream*> audio_send_stats_proxies_;
	std::set<AudioReceiveStream*> audio_receive_stats_proxies_;


	std::string account_id_; //like userid, set by the caller
	int64_t stats_gathering_started;
	StatsCollection reports_full_prev_;
	StatsCollection reports_full_;
	StatsCollection reports_simplifed_;
	//StatsCollection reports_;	
public:
	static const int kFullStatsUpdateIntervalMs;
	static const int kSimplifiedStatsUpdateIntervalMs;
#ifdef VIDEO_ENABLED
	cloopenwebrtc::VideoEngine *m_vie;
#endif
	cloopenwebrtc::VoiceEngine *m_voe;
};

#endif