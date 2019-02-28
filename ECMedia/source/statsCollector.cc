#include "statsCollector.h"
#ifdef VIDEO_ENABLED
#include "vie_render.h"
#endif

#include "../system_wrappers/include/trace.h"

#include <time.h>
#include <stdarg.h>
#include <sstream>

#include "../third_party/protobuf/src/google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "../third_party/protobuf/src/google/protobuf/io/coded_stream.h"

extern char* filename_path;
#define kUpdateIntervalMs 1000
#define kKiloBits	1000

template<typename ValueType>
struct TypeForAdd {
	const StatsReport::StatsValueName name;
	const ValueType& value;
};

typedef TypeForAdd<bool> BoolForAdd;
typedef TypeForAdd<float> FloatForAdd;
typedef TypeForAdd<uint8_t>	UInt8ForAdd;
typedef TypeForAdd<uint16_t> UInt16ForAdd;
typedef TypeForAdd<uint32_t> UInt32ForAdd;
typedef TypeForAdd<int32_t> Int32ForAdd;
typedef TypeForAdd<uint64_t> UInt64ForAdd;
typedef TypeForAdd<int64_t> Int64ForAdd;
typedef TypeForAdd<std::string> StringForAdd;

int GetLocalTime(char *timebuffer)
{
	struct tm *pt = NULL;
	time_t curr_time;
	curr_time = time(NULL);
#ifdef WIN32
	pt = localtime(&curr_time);
#else
	struct tm t1;
	pt = localtime_r(&curr_time, &t1);
#endif

	if (!pt)
		return -1;

#ifndef WIN32
	int count = sprintf(timebuffer, "%02d%02d %02d:%02d:%02d",
		pt->tm_mon + 1, pt->tm_mday,
		pt->tm_hour, pt->tm_min, pt->tm_sec);
#else
	int count = sprintf(timebuffer, "%02d%02d %02d:%02d:%02d",
		pt->tm_mon + 1, pt->tm_mday,
		pt->tm_hour, pt->tm_min, pt->tm_sec);
#endif

	return count;
}

const int StatsCollector::kFullStatsUpdateIntervalMs = 5000;
const int StatsCollector::kSimplifiedStatsUpdateIntervalMs = 1000;


StatsCollector::StatsCollector()
	  : thread_(ThreadWrapper::CreateThread(StatsCollector::StatsCollectorThreadRun, this ,kNormalPriority, "StatsCollectorThread")),
	  	file_name_(nullptr),	
		trace_file_(yuntongxunwebrtc::FileWrapper::Create()),
		clock_(Clock::GetRealTimeClock()),
		updateIntervalMs_(kUpdateIntervalMs),
		last_time_update_simplifiedStats_(clock_->TimeInMilliseconds()),
		last_time_update_fullStats_(clock_->TimeInMilliseconds()),
		capDevId_(-1),
#ifdef VIDEO_ENABLED
		m_vie(NULL),
#endif
		m_voe(NULL),
		stream_crit_(CriticalSectionWrapper::CreateCriticalSection())

{
	updateEvent_ = EventWrapper::Create();
	unsigned int id;
	thread_->Start(id);
}

StatsCollector::~StatsCollector()
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
			"~StatsCollector() failed to stop thread");
	}

	trace_file_->Flush();
	trace_file_->CloseFile();
	delete trace_file_;
	while (!pStatsPBList.empty())
	{
		pStatsPBList.erase(pStatsPBList.begin());
	}
}

void StatsCollector::Config(char* logFile, int logIntervalMs) {
	file_name_ = logFile; 
	updateIntervalMs_ = logIntervalMs; 
	std::stringstream ss;
	char timebuffer[100];
	GetLocalTime(timebuffer);
	trace_file_->OpenFile(file_name_, false);
	ss << "version: 1.0" << "\t" << timebuffer;
	trace_file_->Write(ss.str().c_str(), ss.str().length());
}

#ifdef VIDEO_ENABLED
bool StatsCollector::SetVideoEngin(VideoEngine *vie)
{
#ifdef VIDEO_ENABLED
//    printf("seansean StatsCollector:%p, vie:%p\n", this, vie);
	m_vie = vie;
	return true;
#endif 
	return false;
}
#endif
bool StatsCollector::SetVoiceEngin(VoiceEngine* voe)
{
	m_voe = voe;
	return true;
}

void StatsCollector::AddToReports(StatsReport::StatsReportType type, int channel_id)
{
	const int64_t id = (static_cast<int64_t>(type) << 32 | channel_id); 
	StatsReport *report;
	report = reports_full_.InsertNew(id);
	report->SetReportType(type);
	report->SetChannelId(channel_id);
	report = reports_simplifed_.InsertNew(id);
	report->SetReportType((StatsReport::StatsReportType)(type + StatsReport::kStatsReportTypeAudioSend_Simplified));
	report->SetChannelId(channel_id);
}

void StatsCollector::DeleteFromReports(StatsReport::StatsReportType type, int channel_id)
{
	const int64_t id = (static_cast<int64_t>(type) << 32 | channel_id);
	reports_full_.Delete(id);
	reports_simplifed_.Delete(id);
}
#ifdef VIDEO_ENABLED
bool StatsCollector::AddVideoSendStatsProxy(int channelid)
{
#ifdef VIDEO_ENABLED
//    printf("seansean AddVideoSendStatsProxy:%p, channelid:%d, m_vie:%p\n", this, channelid, m_vie);
	if (!m_vie)
	{
		return false;
	}
	CriticalSectionScoped lock(stream_crit_.get());
	ViEBase*	vie_base = ViEBase::GetInterface(m_vie);
	ViECapture *vie_capture = ViECapture::GetInterface(m_vie);
	if (vie_base)
	{
		SendStatisticsProxy *pStatsProxy = vie_base->GetSendStatisticsProxy(channelid);
		vie_base->RegisterSendStatisticsProxy(channelid, pStatsProxy);
		vie_base->RegisterCpuOveruseObserver(channelid, pStatsProxy);
		vie_base->Release();
		if (vie_capture)
		{
			vie_capture->RegisterObserver(capDevId_, *pStatsProxy);
			vie_capture->Release();
		}
		
		video_send_stats_proxies_.insert(pStatsProxy);		
		AddToReports(StatsReport::kStatsReportTypeVideoSend, channelid);
		return true;
	}
	return false;
#endif
	return true;
}

bool StatsCollector::AddVideoRecvStatsProxy(int channelid)
{
#ifdef VIDEO_ENABLED
	if (!m_vie)
	{
		return false;
	}
	CriticalSectionScoped lock(stream_crit_.get());
	ViEBase*	vie_base = ViEBase::GetInterface(m_vie);
	ViERender*	vie_render = ViERender::GetInterface(m_vie);

	if (vie_base)
	{
		ReceiveStatisticsProxy* pStatsProxy = vie_base->GetReceiveStatisticsProxy(channelid);
		vie_base->Release();
		if (!pStatsProxy)
		{
			return false;
		}
		if (vie_render)
		{
			vie_render->AddRenderCallback(channelid, pStatsProxy);
			vie_render->Release();
		}
		video_receive_stats_proxies_.insert(pStatsProxy);
		AddToReports(StatsReport::kStatsReportTypeVideoRecv, channelid);
		return true;
	}
#endif
	return false;
}

void StatsCollector::DeleteVideoSendStatsProxy(int channelid)
{
#ifdef VIDEO_ENABLED
	CriticalSectionScoped lock(stream_crit_.get());
	SendStatisticsProxy *pStatsProxy = nullptr;
	for (auto it : video_send_stats_proxies_)
	{
		if (it->channelId() == channelid)
		{
			pStatsProxy = it;
			break;
		}
	}
	if (!pStatsProxy)
	{
		return;
	}
	DeleteFromReports(StatsReport::kStatsReportTypeVideoSend, channelid);
	video_send_stats_proxies_.erase(pStatsProxy);
	ViEBase*	vie_base = ViEBase::GetInterface(m_vie);
	if (vie_base)
	{
		vie_base->DeregisterCpuOveruseObserver(channelid);
		vie_base->Release();
	}
#endif
}

void StatsCollector::DeleteVideoRecvStatsProxy(int channelid)
{
	CriticalSectionScoped lock(stream_crit_.get());
	ReceiveStatisticsProxy *pStatsProxy = nullptr;
	for (auto it : video_receive_stats_proxies_)
	{
		if (it->channelId() == channelid)
		{
			pStatsProxy = it;
			break;
		}
	}
	if (!pStatsProxy)
	{
		return;
	}
	DeleteFromReports(StatsReport::kStatsReportTypeVideoRecv, channelid);
	video_receive_stats_proxies_.erase(pStatsProxy);
}
#endif

bool StatsCollector::AddAudioSendStatsProxy(int channelid)
{
	CriticalSectionScoped lock(stream_crit_.get());
	if (!m_voe)
	{
		return false;
	}
	AudioSendStream* pStatsProxy = new yuntongxunwebrtc::AudioSendStream(m_voe, channelid);
	audio_send_stats_proxies_.insert(pStatsProxy);
	AddToReports(StatsReport::kStatsReportTypeAudioSend, channelid);
	return true;
}

bool StatsCollector::AddAudioRecvStatsProxy(int channelid)
{
	CriticalSectionScoped lock(stream_crit_.get());
	if (!m_voe)
	{
		return false;
	}
	AudioReceiveStream* pStatsProxy = new yuntongxunwebrtc::AudioReceiveStream(m_voe, channelid);
	audio_receive_stats_proxies_.insert(pStatsProxy);
	AddToReports(StatsReport::kStatsReportTypeAudioRecv, channelid);
	return true;
}

void StatsCollector::DeleteAudioSendStatsProxy(int channelid)
{
	CriticalSectionScoped lock(stream_crit_.get());
	AudioSendStream* pStatsProxy = nullptr;
	for (auto it:audio_send_stats_proxies_)
	{
		if (it->channelId() == channelid)
		{
			pStatsProxy = it;
			break;
		}
	}
	if (!pStatsProxy)
	{
		return;
	}
	DeleteFromReports(StatsReport::kStatsReportTypeAudioSend, channelid);
	audio_send_stats_proxies_.erase(pStatsProxy);
	delete pStatsProxy;
}

void StatsCollector::DeleteAudioRecvStatsProxy(int channelid)
{
	CriticalSectionScoped lock(stream_crit_.get());
	AudioReceiveStream* pStatsProxy = nullptr;
	for (auto it:audio_receive_stats_proxies_)
	{
		if (it->channelId() == channelid)
		{
			pStatsProxy = it;
			break;;
		}
	}
	if (!pStatsProxy)
	{
		return;
	}
	DeleteFromReports(StatsReport::kStatsReportTypeAudioRecv, channelid);
	audio_receive_stats_proxies_.erase(pStatsProxy);
	delete pStatsProxy;
}

bool StatsCollector::StatsCollectorThreadRun(void* obj)
{
	return static_cast<StatsCollector*>(obj)->ProcessStatsCollector();
}

bool StatsCollector::ProcessStatsCollector()
{
	updateEvent_->Wait(100);
	const int64_t now = clock_->TimeInMilliseconds();
	if (now > last_time_update_simplifiedStats_ + kSimplifiedStatsUpdateIntervalMs)
	{
		CriticalSectionScoped lock(stream_crit_.get());
		last_time_update_simplifiedStats_ = now;
		UpdateStats(false);
		//LogToFile(false);
	}
	if (now > last_time_update_fullStats_ + kFullStatsUpdateIntervalMs)
	{
		CriticalSectionScoped lock(stream_crit_.get());
		last_time_update_fullStats_ = now;
		UpdateStats(true);
		//LogToFile(true);
	}
	return true;
}

void StatsCollector::UpdateStats(bool isFullStats)
{
#ifdef VIDEO_ENABLED
	ExtractVideoSenderInfo(isFullStats);
	ExtractVideoReceiverInfo(isFullStats);
#endif
	ExtractAudioSenderInfo(isFullStats);
	ExtractAudioReceiverInfo(isFullStats);
}

void StatsCollector::GetStats(StatsContentType type, char* callid, void **pMediaStatisticsDataInnerArray, int *pArraySize)
{ 
	CriticalSectionScoped lock(stream_crit_.get());
	MediaStatisticsDataInner *pMediaStatisticsDataInner;
	MediaStatisticsInner *mediaStatsInner;
	
	pMediaStatisticsDataInner = new MediaStatisticsDataInner();
	mediaStatsInner = pMediaStatisticsDataInner->add_mediadata();
	pMediaStatisticsDataInner->set_callid(callid);
	LoadReportsToPbBuffer(type, mediaStatsInner);

	int size = pMediaStatisticsDataInner->ByteSize() + 8;
	*pMediaStatisticsDataInnerArray = new char[size];
	memset(*pMediaStatisticsDataInnerArray, 0, size);
	yuntongxun_google::protobuf::io::ArrayOutputStream array_stream(*pMediaStatisticsDataInnerArray, size);
	yuntongxun_google::protobuf::io::CodedOutputStream output_stream(&array_stream);
	output_stream.WriteVarint32(pMediaStatisticsDataInner->ByteSize());
	if (pMediaStatisticsDataInner->SerializeToCodedStream(&output_stream))
	{
		*pArraySize = output_stream.ByteCount();
	}
	else
	{
		*pArraySize = -1;
	}

	delete pMediaStatisticsDataInner;
	pStatsPBList.push_back(*pMediaStatisticsDataInnerArray);
}

void StatsCollector::DeletePbData(void *pMediaStatisticsDataInnerArray)
{
	std::list<void*>::iterator it = std::find(pStatsPBList.begin(),
												pStatsPBList.end(),
												pMediaStatisticsDataInnerArray);
	if (it == pStatsPBList.end())
		return;
	delete [] (char *)pMediaStatisticsDataInnerArray;
	pStatsPBList.erase(it);
}

void StatsCollector::LoadReportsToPbBuffer(StatsContentType type, MediaStatisticsInner *mediaStatsInner)
{
	StatsCollection *reports;
	if (type == kStatsContentFull)
	{
		reports = &reports_full_;
	}
	else if (type == kStatsContentSimplified)
	{
		reports = &reports_simplifed_;
	}

	for (const auto *r : *reports)
	{
		StatsReport report = *r;
		AudioSenderStatisticsInner *audioSenderStats;
		AudioReceiverStatisticsInner *audioReceiverStats;
#ifdef VIDEO_ENABLED
		VideoSenderStatisticsInner *videoSenderStats;
		VideoReceiverStatisticsInner *videoReceiverStats;
#endif
		switch (report.type())
		{
		case StatsReport::kStatsReportTypeAudioSend:
		case StatsReport::kStatsReportTypeAudioSend_Simplified:
			audioSenderStats = mediaStatsInner->add_audiosenderstats();
			LoadAudioSenderReportToPbBuffer(type, report, audioSenderStats);
			break;
		case StatsReport::kStatsReportTypeAudioRecv:
		case StatsReport::kStatsReportTypeAudioRecv_Simplified:
			audioReceiverStats = mediaStatsInner->add_audioreceiverstats();
			LoadAudioReceiverReportToPbBuffer(type, report, audioReceiverStats);
			break;
#ifdef VIDEO_ENABLED
		case StatsReport::kStatsReportTypeVideoSend:
		case StatsReport::kStatsReportTypeVideoSend_Simplified:
			videoSenderStats = mediaStatsInner->add_videosenderstats();
			LoadVideoSenderReportToPbBuffer(type, report, videoSenderStats);
			break;
		case StatsReport::kStatsReportTypeVideoRecv:
		case StatsReport::kStatsReportTypeVideoRecv_Simplified:
			videoReceiverStats = mediaStatsInner->add_videoreceiverstats();
			LoadVideoReceiverReportToPbBuffer(type, report, videoReceiverStats);
			break;
#endif
		}		
	}
}

void StatsCollector::LogToFile(bool isFullStats)
{
#ifdef VIDEO_ENABLED
	//log config to file 
	for (auto it : video_send_stats_proxies_)
	{
		SendStatisticsProxy* pStatsProxy = it;		
		if (pStatsProxy->newConfig())
		{
			std::stringstream ss;
			std::string config = pStatsProxy->GetConfig().ToString();
			pStatsProxy->ResetNewConfig();
			ss << "\nEncoderSetting: ";
			ss << config;
			/*ss << "\n";*/
			trace_file_->Write(ss.str().c_str(), ss.str().length());
		}	
	}
#endif
	// log all statistics items to file in new style
	StatsCollection *reports;
	if (isFullStats)
	{
		reports = &reports_full_;
	}
	else
		reports = &reports_simplifed_;
	for (const auto *r: *reports)
	{
		StatsReport::Values values = r->values();

		trace_file_->Write("\n", 1);
		for (const auto &v : values)
		{
			const StatsReport::Value *value = v.second.get();
			std::stringstream ss;
			std::string temp;
			ss << value->display_name() << ":" << value->ToString() << "\t";
			trace_file_->Write(ss.str().c_str(), ss.str().length());
		}
	}
}

#ifdef VIDEO_ENABLED
void StatsCollector::VideoSenderInfo_AddEncoderSetting(const VideoSendStream::Stats info,
												StatsReport *report)
{
	VideoSendStream::Config::EncoderSettings encoder_settings = info.config.encoder_settings;
	report->AddInt32(StatsReport::kStatsValueNameCapturedFrameWidth, info.captured_width);
	report->AddInt32(StatsReport::kStatsValueNameCapturedFrameHeight, info.captured_height);
	report->AddInt32(StatsReport::kStatsValueNameCapturedFrameRate, info.captured_framerate);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingFrameWidth, encoder_settings.width);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingFrameHeight, encoder_settings.height);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingFrameRate, encoder_settings.maxFramerate);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingSimulcastNum, encoder_settings.numberOfSimulcastStreams);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingStartBitrate, encoder_settings.startBitrate);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingMinBitrate, encoder_settings.minBitrate);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingMaxBitrate, encoder_settings.maxBitrate);
	report->AddInt32(StatsReport::kStatsValueNameCodecSettingTargetBitrate, encoder_settings.targetBitrate);
	report->AddString(StatsReport::kStatsValueNameCodecImplementationName, encoder_settings.payload_name);
}

void StatsCollector::VideoSenderInfo_AddQMSetting(const VideoSendStream::Stats info,
												StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameQMFrameWidth, info.qm_width);
	report->AddInt32(StatsReport::kStatsValueNameQMFrameHeight, info.qm_height);
	report->AddInt32(StatsReport::kStatsValueNameQMFrameRate, info.qm_framerate);
}

void StatsCollector::VideoSenderInfo_AddCpuMetrics(const VideoSendStream::Stats info,
													StatsReport *report) {
	report->AddInt32(StatsReport::kStatsValueNameAvgEncodeMs, info.avg_encode_time_ms);
	report->AddInt32(StatsReport::kStatsValueNameEncodeUsagePercent, info.encode_usage_percent);
}

void StatsCollector::VideoSenderInfo_AddBweStats(const VideoSendStream::Stats info,
												StatsReport *report)
{
	if (info.config.encoder_settings.numberOfSimulcastStreams == 0)
	{
		report->AddInt32(StatsReport::kStatsValueNameTargetEncBitrate, info.target_enc_bitrate_bps);
		report->AddInt32(StatsReport::kStatsValueNameActualEncBitrate, info.actual_enc_bitrate_bps);
		report->AddInt32(StatsReport::kStatsValueNameAvailableSendBandwidth, info.call.sendside_bwe_bps);
		report->AddInt32(StatsReport::kStatsValueNameAvailableReceiveBandwidth, info.call.recv_bandwidth_bps);
        //zhangn added 20181210
        report->AddInt32(StatsReport::kStatsValueNameTargetEncFrameRate, info.target_enc_framerate);
        report->AddInt32(StatsReport::kStatsValueNameActualEncFrameRate, info.actual_enc_framerate);
	}
	else { //TODO: simulcast
	}
}

void StatsCollector::VideoSenderInfo_AddNetworkStats(const VideoSendStream::Stats info,
													StatsReport *report)
{
	if (info.config.encoder_settings.numberOfSimulcastStreams == 0)
	{
		report->AddInt32(StatsReport::kStatsValueNameLossFractionInPercent, info.stream.rtcp_stats.fraction_lost*100/255);
		report->AddInt32(StatsReport::kStatsValueNameJitterReceived, static_cast<int32_t>(info.stream.rtcp_stats.jitter));
		report->AddInt32(StatsReport::kStatsValueNameRttInMs, static_cast<int32_t>(info.call.rtt_ms));
		report->AddInt32(StatsReport::kStatsValueNameBucketDelayInMs, info.call.pacer_delay_ms);
	}
	else { //TODO: simulcast
	}
}

void StatsCollector::VideoSenderInfo_AddRtpStats(const VideoSendStream::Stats info,
												StatsReport *report)
{
	if (info.config.encoder_settings.numberOfSimulcastStreams == 0)
	{
		report->AddInt32(StatsReport::kStatsValueNameTransmitBitrate, info.stream.total_stats.bitrate_bps);
		report->AddInt32(StatsReport::kStatsValueNameRetransmitBitrate, info.stream.retransmit_stats.bitrate_bps);
		report->AddInt32(StatsReport::kStatsValueNameTransmitPacketsRate, info.stream.total_stats.packet_rate);
		report->AddInt32(StatsReport::kStatsValueNameRetransmitPacketsRate, info.stream.retransmit_stats.packet_rate);
		report->AddInt32(StatsReport::kStatsValueNameFecBitrate, 0);
		report->AddInt32(StatsReport::kStatsValueNameSsrc, *info.ssrc_streams.begin());
	}
	else { //TODO: simulcast
	}
}

void StatsCollector::VideoSenderInfo_AddRtcpStats(const VideoSendStream::Stats info,
													StatsReport *report)
{
	if (info.config.encoder_settings.numberOfSimulcastStreams == 0)
	{
		report->AddInt32(StatsReport::kStatsValueNamePacketsLost, info.stream.rtcp_stats.cumulative_lost);
		report->AddInt32(StatsReport::kStatsValueNameFirsReceived, info.stream.rtcp_packet_type_counts.fir_packets);
		report->AddInt32(StatsReport::kStatsValueNameNacksReceived, info.stream.rtcp_packet_type_counts.nack_packets);
		report->AddInt32(StatsReport::kStatsValueNameNacksRequestsReceived, info.stream.rtcp_packet_type_counts.nack_requests);
		report->AddInt32(StatsReport::kStatsValueNameNacksUniqueRequestsReceived, info.stream.rtcp_packet_type_counts.unique_nack_requests);
	}
	else { //TODO: simulcast
	}
}
#endif

void StatsCollector::Report_AddCommonFiled(StatsReport *report, int64_t ts)
{
	assert(report);
	report->AddInt32(StatsReport::kStatsValueNameReportType, report->type());
	report->AddInt32(StatsReport::kStatsValueNameChannelId, report->channel_id());
	report->AddUInt64(StatsReport::kStatsValueNameTimestamp, static_cast<uint64_t> (ts));
}

#ifdef VIDEO_ENABLED
void StatsCollector::ExtractVideoSenderInfo(bool isFullStats)
{
	for (auto it : video_send_stats_proxies_)
	{
		int64_t timestamp;
		SendStatisticsProxy* pStatsProxy = it;
		VideoSendStream::Stats info = pStatsProxy->GetStats(isFullStats, timestamp);
		pStatsProxy->TimeUntilNextProcess();
		StatsReport *report = FindReport(isFullStats, StatsReport::kStatsReportTypeVideoSend, pStatsProxy->channelId());
		if (!report)
		{
			return;
		}		
		
		VideoSenderInfo_AddEncoderSetting(info, report);
		VideoSenderInfo_AddCpuMetrics(info, report);
		VideoSenderInfo_AddRtpStats(info, report);
		VideoSenderInfo_AddRtcpStats(info, report);
		const UInt8ForAdd uint8[] = {
			{ StatsReport::kStatsValueNameTargetEncFrameRate, info.target_enc_framerate },
			{ StatsReport::kStatsValueNameActualEncFrameRate, info.actual_enc_framerate },
		};
        
		VideoSenderInfo_AddQMSetting(info, report);
		VideoSenderInfo_AddBweStats(info, report);
		VideoSenderInfo_AddNetworkStats(info, report);
		Report_AddCommonFiled(report, timestamp);
	}
}
void StatsCollector::ExtractVideoReceiverInfo(bool isFullStats)
{
#ifdef VIDEO_ENABLED
	for (auto it : video_receive_stats_proxies_)
	{
		int64_t timestamp;
		ReceiveStatisticsProxy* pStatsProxy = it;
		VideoReceiveStream::Stats info = pStatsProxy->GetStats(timestamp);
		StatsReport *report = FindReport(isFullStats, StatsReport::kStatsReportTypeVideoRecv, pStatsProxy->channelId());
		if (!report)
		{
			return;
		}		
		
		VideoReciverInfo_AddReceiveBasic(info, report);
		VideoReciverInfo_AddRenderStats(info, report);
		VideoReciverInfo_AddRtpStats(info, report);
		VideoReciverInfo_AddRtcpStats(info, report);
		VideoReciverInfo_AddReceiveStats(info, report);
		VideoReciverInfo_AddDecoderStats(info, report);
		VideoReciverInfo_AddLossModeStats(info, report);
		Report_AddCommonFiled(report, timestamp);
	}
#endif
}
#endif

void StatsCollector::ExtractAudioSenderInfo(bool isFullStats)
{
	for (auto it : audio_send_stats_proxies_)
	{
		int64_t timestamp;
		AudioSendStream *pStatsPorxy = it;
		AudioSendStream::Stats info = pStatsPorxy->GetStats(timestamp);
		StatsReport *report = FindReport(isFullStats, StatsReport::kStatsReportTypeAudioSend, pStatsPorxy->channelId());
		if (!report)
		{
			return;
		}		
		AudioSenderInfo_AddBasic(info, report);
		AudioSenderInfo_AddNetworkStats(info, report);
		AudioSenderInfo_AddEchoStats(info, report);
		Report_AddCommonFiled(report, timestamp);		
	}
}

void StatsCollector::LoadAudioSenderReportToPbBuffer(StatsContentType type,
													StatsReport report,
													AudioSenderStatisticsInner *statsData)
{
	if (report.values().size() == 0)
		return;
	const StatsReport::Value *value = nullptr;
	value = report.FindValue(StatsReport::kStatsValueNameReportType);
	if (value)
		statsData->set_kstatsvaluenamereporttype(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameChannelId);
	if (value)
		statsData->set_kstatsvaluenamechannelid(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameTimestamp);
	if (value)
		statsData->set_kstatsvaluenametimestamp(value->uint64_val());
	value = report.FindValue(StatsReport::kStatsValueNameCodecImplementationName);
	if (value)
		statsData->set_kstatsvaluenamecodecimplementationname(value->string_val());
	value = report.FindValue(StatsReport::kStatsValueNameAudioInputLevel);
	if (value)
		statsData->set_kstatsvaluenameaudioinputlevel(value->int32_val());

	value = report.FindValue(StatsReport::kStatsValueNameRttInMs);
	if (value)
		statsData->set_kstatsvaluenamerttinms(value->int32_val());

	value = report.FindValue(StatsReport::kStatsValueNameEchoDelayMedian);
	if (value)
		statsData->set_kstatsvaluenameechodelaymedian(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameEchoDelayStdDev);
	if (value)
		statsData->set_kstatsvaluenameechodelaystddev(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameEchoReturnLoss);
	if (value)
		statsData->set_kstatsvaluenameechoreturnloss(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameEchoReturnLossEnhancement);
	if (value)
		statsData->set_kstatsvaluenameechoreturnlossenhancement(value->int32_val());
}

void StatsCollector::LoadAudioReceiverReportToPbBuffer(StatsContentType type,
														StatsReport report,
														AudioReceiverStatisticsInner *statsData)
{
	if (report.values().size() == 0)
		return;
	const StatsReport::Value *value = nullptr;
	value = report.FindValue(StatsReport::kStatsValueNameReportType);
	if (value)
		statsData->set_kstatsvaluenamereporttype(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameChannelId);
	if (value)
		statsData->set_kstatsvaluenamechannelid(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameTimestamp);
	if (value)
		statsData->set_kstatsvaluenametimestamp(value->uint64_val());
	value = report.FindValue(StatsReport::kStatsValueNameCodecImplementationName);
	if (value)
		statsData->set_kstatsvaluenamecodecimplementationname(value->string_val());
	value = report.FindValue(StatsReport::kStatsValueNameAudioOutputLevel);
	if (value)
		statsData->set_kstatsvaluenameaudiooutputlevel(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameCurrentDelayMs);
	if (value)
		statsData->set_kstatsvaluenamecurrentdelayms(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameJitterBufferMs);
	if (value)
		statsData->set_kstatsvaluenamejitterbufferms(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameAccelerateRate);
	if (value)
		statsData->set_kstatsvaluenameacceleraterate(value->float_val());
	value = report.FindValue(StatsReport::kStatsValueNameExpandRate);
	if (value)
		statsData->set_kstatsvaluenameexpandrate(value->float_val());
	value = report.FindValue(StatsReport::kStatsValueNamePreemptiveExpandRate);
	if (value)
		statsData->set_kstatsvaluenamepreemptiveexpandrate(value->float_val());
	value = report.FindValue(StatsReport::kStatsValueNameDecodingNormal);
	if (value)
		statsData->set_kstatsvaluenamedecodingnormal(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameDecodingPLC);
	if (value)
		statsData->set_kstatsvaluenamedecodingplc(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameDecodingCNG);
	if (value)
		statsData->set_kstatsvaluenamedecodingcng(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameDecodingPLCCNG);
	if (value)
		statsData->set_kstatsvaluenamedecodingplccng(value->int32_val());

	value = report.FindValue(StatsReport::kStatsValueNameLossFractionInPercent);
	if (value)
		statsData->set_kstatsvaluenamelossfractioninpercent(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNamePacketsLost);
	if (value)
		statsData->set_kstatsvaluenamepacketslost(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameJitterReceived);
	if (value)
		statsData->set_kstatsvaluenamejitterreceived(value->int32_val());
}

#ifdef VIDEO_ENABLED
void StatsCollector::LoadVideoSenderReportToPbBuffer(StatsContentType type,
													StatsReport report,
													VideoSenderStatisticsInner *statsData)
{
	if (report.values().size() == 0)
		return;
	const StatsReport::Value *value = nullptr;
	value = report.FindValue(StatsReport::kStatsValueNameReportType);
	if (value)
		statsData->set_kstatsvaluenamereporttype(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameChannelId);
	if (value)
		statsData->set_kstatsvaluenamechannelid(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameTimestamp);
	if (value)
		statsData->set_kstatsvaluenametimestamp(value->uint64_val());
	value = report.FindValue(StatsReport::kStatsValueNameCodecImplementationName);
	if (value)
		statsData->set_kstatsvaluenamecodecimplementationname(value->string_val());
	value = report.FindValue(StatsReport::kStatsValueNameTargetEncBitrate);
	if (value)
		statsData->set_kstatsvaluenametargetencbitrate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameActualEncBitrate);
	if (value)
		statsData->set_kstatsvaluenameactualencbitrate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameAvailableSendBandwidth);
	if (value)
		statsData->set_kstatsvaluenameavailablesendbandwidth(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameAvailableReceiveBandwidth);
	if (value)
		statsData->set_kstatsvaluenameavailablereceivebandwidth(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameRttInMs);
	if (value)
		statsData->set_kstatsvaluenamerttinms(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameBucketDelayInMs);
	if (value)
		statsData->set_kstatsvaluenamebucketdelayinms(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameTransmitBitrate);
	if (value)
		statsData->set_kstatsvaluenametransmitbitrate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameRetransmitBitrate);
	if (value)
		statsData->set_kstatsvaluenameretransmitbitrate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameFecBitrate);
	if (value)
		statsData->set_kstatsvaluenamefecbitrate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameTransmitPacketsRate);
	if (value)
		statsData->set_kstatsvaluenametransmitpacketsrate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameRetransmitPacketsRate);
	if (value)
		statsData->set_kstatsvaluenameretransmitpacketsrate(value->int32_val());

	value = report.FindValue(StatsReport::kStatsValueNameQMFrameWidth);
	if (value)
		statsData->set_kstatsvaluenameqmframewidth(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameQMFrameHeight);
	if (value)
		statsData->set_kstatsvaluenameqmframeheight(value->int32_val());
    
	value = report.FindValue(StatsReport::kStatsValueNameQMFrameRate);
	if (value)
		statsData->set_kstatsvaluenameqmframerate(value->int32_val());
    
    //zhangn added 20181104
    value = report.FindValue(StatsReport::kStatsValueNameLossFractionInPercent);
    if (value)
        statsData->set_kstatsvaluenamelossfractioninpercent(value->int32_val());
    value = report.FindValue(StatsReport::kStatsValueNameTargetEncFrameRate);
    if (value)
        statsData->set_kstatsvaluenametargetencframerate(value->int32_val());
    value = report.FindValue(StatsReport::kStatsValueNameActualEncFrameRate);
    if (value)
        statsData->set_kstatsvaluenameactualencframerate(value->int32_val());

	if (type == kStatsContentFull)
	{
		value = report.FindValue(StatsReport::kStatsValueNameSsrc);
		if (value)
			statsData->set_kstatsvaluenamessrc(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameFirsReceived);
		if (value)
			statsData->set_kstatsvaluenamefirsreceived(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameNacksReceived);
		if (value)
			statsData->set_kstatsvaluenamenacksreceived(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameNacksRequestsReceived);
		if (value)
			statsData->set_kstatsvaluenamenacksrequestsreceived(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameNacksUniqueRequestsReceived);
		if (value)
			statsData->set_kstatsvaluenamenacksuniquerequestsreceived(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameAvgEncodeMs);
		if (value)
			statsData->set_kstatsvaluenameavgencodems(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameEncodeUsagePercent);
		if (value)
			statsData->set_kstatsvaluenameencodeusagepercent(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCapturedFrameWidth);
		if (value)
			statsData->set_kstatsvaluenamecapturedframewidth(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCapturedFrameHeight);
		if (value)
			statsData->set_kstatsvaluenamecapturedframeheight(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCapturedFrameRate);
		if (value)
			statsData->set_kstatsvaluenamecapturedframerate(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingFrameWidth);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingframewidth(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingFrameHeight);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingframeheight(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingFrameRate);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingframerate(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingSimulcastNum);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingsimulcastnum(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingStartBitrate);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingstartbitrate(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingMinBitrate);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingminbitrate(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingMaxBitrate);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingmaxbitrate(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameCodecSettingTargetBitrate);
		if (value)
			statsData->set_kstatsvaluenamecodecsettingtargetbitrate(value->int32_val());
	}	
}
void StatsCollector::LoadVideoReceiverReportToPbBuffer(StatsContentType type,
													StatsReport report,
													VideoReceiverStatisticsInner *statsData)
{
	if (report.values().size() == 0)
		return;
	const StatsReport::Value *value = nullptr;
	value = report.FindValue(StatsReport::kStatsValueNameReportType);
	if (value)
		statsData->set_kstatsvaluenamereporttype(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameChannelId);
	if (value)
		statsData->set_kstatsvaluenamechannelid(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameTimestamp);
	if (value)
		statsData->set_kstatsvaluenametimestamp(value->uint64_val());
	value = report.FindValue(StatsReport::kStatsValueNameCodecImplementationName);
	if (value)
		statsData->set_kstatsvaluenamecodecimplementationname(value->string_val());

	value = report.FindValue(StatsReport::kStatsValueNameFrameWidthReceived);
	if (value)
		statsData->set_kstatsvaluenameframewidthreceived(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameFrameHeightReceived);
	if (value)
		statsData->set_kstatsvaluenameframeheightreceived(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameReceivedFrameRate);
	if (value)
		statsData->set_kstatsvaluenamereceivedframerate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameReceivedTotalBitrate);
	if (value)
		statsData->set_kstatsvaluenamereceivedtotalbitrate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameJitterBufferMs);
	if (value)
		statsData->set_kstatsvaluenamejitterbufferms(value->int32_val());

	value = report.FindValue(StatsReport::kStatsValueNameDecoderFrameRate);
	if (value)
		statsData->set_kstatsvaluenamedecoderframerate(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameDecodeMs);
	if (value)
		statsData->set_kstatsvaluenamedecodems(value->int32_val());
	if (type == kStatsContentFull)
	{
		value = report.FindValue(StatsReport::kStatsValueNameMaxDecodeMs);
		if (value)
			statsData->set_kstatsvaluenamemaxdecodems(value->int32_val());
	}
			
	value = report.FindValue(StatsReport::kStatsValueNameFrameRateRender);
	if (value)
		statsData->set_kstatsvaluenameframeraterender(value->uint32_val());
	if (type == kStatsContentFull)
	{
		value = report.FindValue(StatsReport::kStatsValueNameCurrentDelayMs);
		if (value)
			statsData->set_kstatsvaluenamecurrentdelayms(value->int32_val());
	}
	value = report.FindValue(StatsReport::kStatsValueNameRenderDelayMs);
	if (value)
		statsData->set_kstatsvaluenamerenderdelayms(value->int32_val());

	if (type == kStatsContentFull)
	{
		value = report.FindValue(StatsReport::kStatsValueNameSsrc);
		if (value)
			statsData->set_kstatsvaluenamessrc(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameBytesReceived);
		if (value)
			statsData->set_kstatsvaluenamebytesreceived(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNamePacketsReceived);
		if (value)
			statsData->set_kstatsvaluenamepacketsreceived(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameNacksRequestsSent);
		if (value)
			statsData->set_kstatsvaluenamenacksrequestssent(value->int32_val());
		value = report.FindValue(StatsReport::kStatsValueNameNacksUniqueRequestsSent);
		if (value)
			statsData->set_kstatsvaluenamenacksuniquerequestssent(value->int32_val());
	}
	value = report.FindValue(StatsReport::kStatsValueNameFirsSent);
	if (value)
		statsData->set_kstatsvaluenamefirssent(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameNacksSent);
	if (value)
		statsData->set_kstatsvaluenamenackssent(value->int32_val());
	
	value = report.FindValue(StatsReport::kStatsValueNameLossModePart1);
	if (value)
		statsData->set_kstatsvaluenamelossmodepart1(value->int64_val());
	value = report.FindValue(StatsReport::kStatsValueNameLossModePart2);
	if (value)
		statsData->set_kstatsvaluenamelossmodepart2(value->int64_val());
	value = report.FindValue(StatsReport::kStatsValueNameLossModePart3);
	if (value)
		statsData->set_kstatsvaluenamelossmodepart3(value->int64_val());
	value = report.FindValue(StatsReport::kStatsValueNameLossModePart4);
	if (value)
		statsData->set_kstatsvaluenamelossmodepart4(value->int64_val());

	value = report.FindValue(StatsReport::kStatsValueNameLossFractionInPercent);
	if (value)
    {
//        printf("seansean fraction lost:%d\n", value->int32_val());
		statsData->set_kstatsvaluenamelossfractioninpercent(value->int32_val());
    }
	value = report.FindValue(StatsReport::kStatsValueNamePacketsLost);
	if (value)
		statsData->set_kstatsvaluenamepacketslost(value->int32_val());
	value = report.FindValue(StatsReport::kStatsValueNameJitterReceived);
	if (value)
		statsData->set_kstatsvaluenamejitterreceived(value->int32_val());	
}
#endif

void StatsCollector::ExtractAudioReceiverInfo(bool isFullStats)
{
	for (auto it : audio_receive_stats_proxies_)
	{
		int64_t timestamp;
		AudioReceiveStream *pStatsPorxy = it;
		AudioReceiveStream::Stats info = pStatsPorxy->GetStats(timestamp);
		StatsReport *report = FindReport(isFullStats, StatsReport::kStatsReportTypeAudioRecv, pStatsPorxy->channelId());
		if (!report)
		{
			return;
		}
		
		AudioReceiverInfo_AddBasic(info, report);
		AudioReceiverInfo_AddDecoderStats(info, report);
		AudioReceiverInfo_AddNetworkStats(info, report);
		Report_AddCommonFiled(report, timestamp);
	}

}

#ifdef VIDEO_ENABLED
void StatsCollector::VideoReciverInfo_AddReceiveBasic(const VideoReceiveStream::Stats info,
														StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameFrameWidthReceived, info.last_width_);
	report->AddInt32(StatsReport::kStatsValueNameFrameHeightReceived, info.last_height_);
	report->AddString(StatsReport::kStatsValueNameCodecImplementationName, info.decoder_implementation_name);
}

void StatsCollector::VideoReciverInfo_AddReceiveStats(const VideoReceiveStream::Stats info,
													StatsReport *report)
{
//    printf("seansean info.received_framerate:%d, info.total_bitrate_bps:%d, info.jitter_buffer_ms:%d\n", info.received_framerate, info.total_bitrate_bps, info.jitter_buffer_ms);
	report->AddInt32(StatsReport::kStatsValueNameReceivedFrameRate, info.received_framerate);
	report->AddInt32(StatsReport::kStatsValueNameReceivedTotalBitrate, info.total_bitrate_bps);
	report->AddInt32(StatsReport::kStatsValueNameJitterBufferMs, info.jitter_buffer_ms);
}

void StatsCollector::VideoReciverInfo_AddDecoderStats(const VideoReceiveStream::Stats info, StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameDecoderFrameRate, info.decoded_framerate);
	report->AddInt32(StatsReport::kStatsValueNameDecodeMs, info.decode_ms);
	report->AddInt32(StatsReport::kStatsValueNameMaxDecodeMs, info.max_decode_ms);
}

void StatsCollector::VideoReciverInfo_AddRenderStats(const VideoReceiveStream::Stats info, StatsReport *report)
{
	report->AddUInt32(StatsReport::kStatsValueNameFrameRateRender, info.rendered_framerate);
	report->AddInt32(StatsReport::kStatsValueNameCurrentDelayMs, info.current_delay_ms);
	report->AddInt32(StatsReport::kStatsValueNameRenderDelayMs, info.render_delay_ms);
}

void StatsCollector::VideoReciverInfo_AddRtpStats(const VideoReceiveStream::Stats info, StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameSsrc, info.ssrc);
	report->AddInt32(StatsReport::kStatsValueNameBytesReceived, info.rtp_stats.transmitted.payload_bytes);
	report->AddInt32(StatsReport::kStatsValueNamePacketsReceived, info.rtp_stats.transmitted.packets);
}

void StatsCollector::VideoReciverInfo_AddRtcpStats(const VideoReceiveStream::Stats info, StatsReport *report)
{
//    printf("seansean nack_packets:%u, nack_requests:%u, unique_nack_requests:%u, fir_packets:%u, fraction_lost:%u, cumulative_lost:%u, jitter:%u\n", info.rtcp_packet_type_counts.nack_packets,info.rtcp_packet_type_counts.nack_requests, info.rtcp_packet_type_counts.unique_nack_requests, info.rtcp_packet_type_counts.fir_packets, info.rtcp_stats.fraction_lost, info.rtcp_stats.cumulative_lost, info.rtcp_stats.jitter);
	const Int32ForAdd uint32[] = {
		{ StatsReport::kStatsValueNameNacksSent, info.rtcp_packet_type_counts.nack_packets },
		{ StatsReport::kStatsValueNameNacksRequestsSent, info.rtcp_packet_type_counts.nack_requests },
		{ StatsReport::kStatsValueNameNacksUniqueRequestsSent, info.rtcp_packet_type_counts.unique_nack_requests },
		{ StatsReport::kStatsValueNameFirsSent, info.rtcp_packet_type_counts.fir_packets },
		//{ StatsReport::kStatsValueNameLossFractionInPercent, info.rtcp_stats.fraction_lost*100/255},
    { StatsReport::kStatsValueNameLossFractionInPercent, info.rtcp_stats.real_fraction_lost*100/255},
    { StatsReport::kStatsValueNamePacketsLost, info.rtcp_stats.cumulative_lost },
		{ StatsReport::kStatsValueNameJitterReceived, info.rtcp_stats.jitter },
	};
	for (const auto& i : uint32)
		report->AddInt32(i.name, i.value);
}

void StatsCollector::VideoReciverInfo_AddLossModeStats(const VideoReceiveStream::Stats info, StatsReport *report)
{
	const Int64ForAdd int64[] = {
		{ StatsReport::kStatsValueNameLossModePart1, 0 },
		{ StatsReport::kStatsValueNameLossModePart2, 0 },
		{ StatsReport::kStatsValueNameLossModePart3, 0 },
		{ StatsReport::kStatsValueNameLossModePart4, 0 },
	};
	for (const auto& i : int64)
		report->AddInt64(i.name, i.value);
}
#endif

void StatsCollector::AudioSenderInfo_AddBasic(const AudioSendStream::Stats info,
											StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameAudioInputLevel, info.audio_level);
	report->AddUInt32(StatsReport::kStatsValueNameSsrc, info.local_ssrc);
	report->AddString(StatsReport::kStatsValueNameCodecImplementationName, info.codec_name);
}

void StatsCollector::AudioSenderInfo_AddNetworkStats(const AudioSendStream::Stats info,
													StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameJitterReceived, info.jitter_ms);
	report->AddInt32(StatsReport::kStatsValueNameRttInMs, static_cast<int32_t>(info.rtt_ms));
	report->AddInt32(StatsReport::kStatsValueNameLossFractionInPercent, static_cast<int32_t>(info.fraction_lost));
}

void StatsCollector::AudioSenderInfo_AddEchoStats(const AudioSendStream::Stats info,
													StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameEchoDelayMedian, info.echo_delay_median_ms);
	report->AddInt32(StatsReport::kStatsValueNameEchoDelayStdDev, info.echo_delay_std_ms);
	report->AddInt32(StatsReport::kStatsValueNameEchoReturnLoss, info.echo_return_loss);
	report->AddInt32(StatsReport::kStatsValueNameEchoReturnLossEnhancement, info.echo_return_loss_enhancement);
}

void StatsCollector::AudioReceiverInfo_AddBasic(const AudioReceiveStream::Stats info,
												StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameAudioOutputLevel, info.audio_level);
	report->AddInt32(StatsReport::kStatsValueNameSsrc, info.remote_ssrc);
	report->AddString(StatsReport::kStatsValueNameCodecImplementationName, info.codec_name);
}

void StatsCollector::AudioReceiverInfo_AddDecoderStats(const AudioReceiveStream::Stats info, StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameDecodingNormal, info.decoding_normal);
	report->AddInt32(StatsReport::kStatsValueNameDecodingPLC, info.decoding_plc);
	report->AddInt32(StatsReport::kStatsValueNameDecodingCNG, info.decoding_cng);
	report->AddInt32(StatsReport::kStatsValueNameDecodingPLCCNG, info.decoding_plc_cng);
}

void StatsCollector::AudioReceiverInfo_AddNetworkStats(const AudioReceiveStream::Stats info, StatsReport *report)
{
	report->AddInt32(StatsReport::kStatsValueNameCurrentDelayMs, info.delay_estimate_ms);
	report->AddInt32(StatsReport::kStatsValueNameJitterBufferMs, info.jitter_buffer_ms);
	report->AddFloat(StatsReport::kStatsValueNameAccelerateRate, info.accelerate_rate);
	report->AddFloat(StatsReport::kStatsValueNameExpandRate, info.expand_rate);
	report->AddFloat(StatsReport::kStatsValueNamePreemptiveExpandRate, info.preemptive_expand_rate);

	report->AddInt32(StatsReport::kStatsValueNameLossFractionInPercent, info.fraction_lost);
	report->AddInt32(StatsReport::kStatsValueNamePacketsLost, info.packets_lost);
	report->AddInt32(StatsReport::kStatsValueNameJitterReceived, info.jitter_ms);
}

StatsReport* StatsCollector::FindReport(bool isFullStats, int reportType, int channel_id)
{
	int64_t id = (static_cast<int64_t>(reportType) << 32 | channel_id);
	if (isFullStats)
	{
		StatsReport *report = reports_full_.Find(id);
		return report;
	}
	else {
		StatsReport *report = reports_simplifed_.Find(id);
		return report;
	}
}
