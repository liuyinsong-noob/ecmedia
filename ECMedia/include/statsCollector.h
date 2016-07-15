#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H
#include "send_statistics_proxy.h"
#include "receive_statistics_proxy.h"
#include "audio_send_stream.h"
#include "audio_receive_stream.h"

#include "file_wrapper.h"
#include "clock.h"

#include "vie_base.h"
#include "voe_base.h"

using namespace cloopenwebrtc;
class StatsCollector{
public:
	StatsCollector(char* file_name, int UpdateIntervalMs);
	~StatsCollector();
	static bool StatsCollectorThreadRun(void* obj);
	bool ProcessStatsCollector();
	bool SetVideoSendStatisticsProxy(int channelid, int capDevId);
	bool SetVideoRecvStatisticsProxy(int channelid);
	bool SetAudioSendStatisticsProxy(int channelid);
	bool SetAudioRecvStatisticsProxy(int channelid);

	bool SetVideoEngin(VideoEngine* vie);
	bool SetVoiceEngin(VoiceEngine* voe);


private:
	ThreadWrapper *thread_;
	char*		file_name_;
	FileWrapper *trace_file_;

	cloopenwebrtc::Clock*		clock_;
	uint32_t	updateIntervalMs_;
	int64_t     last_process_time_;

	SendStatisticsProxy		*pVideoSendStats_;
	ReceiveStatisticsProxy	*pVideoRecvStats_;
	AudioSendStream	*pAudioSendStats_;
	AudioReceiveStream	*pAudioRecvStats_;
public:
	cloopenwebrtc::VideoEngine *m_vie;
	cloopenwebrtc::VoiceEngine *m_voe;
};

#endif