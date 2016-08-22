#include "statsCollector.h"
#ifdef VIDEO_ENABLED
#include "vie_render.h"
#endif

#include "trace.h"

StatsCollector::StatsCollector(char* file_name, int UpdateIntervalMs)
	  : thread_(ThreadWrapper::CreateThread(StatsCollector::StatsCollectorThreadRun, this ,kNormalPriority, "StatsCollectorThread")),
	  	file_name_(file_name),	
		trace_file_(cloopenwebrtc::FileWrapper::Create()),
		clock_(Clock::GetRealTimeClock()),
		updateIntervalMs_(UpdateIntervalMs),
		last_process_time_(clock_->TimeInMilliseconds()),
		pVideoSendStats_(NULL),
		pVideoRecvStats_(NULL),
		pAudioSendStats_(NULL),
		pAudioRecvStats_(NULL),
#ifdef VIDEO_ENABLED
		m_vie(NULL),
#endif
		m_voe(NULL)

{
	unsigned int id;
	updateEvent_ = EventWrapper::Create();
	trace_file_->OpenFile(file_name_, false);
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

	if (pAudioSendStats_)
	{
		delete pAudioSendStats_;
	}
	if (pAudioRecvStats_)
	{
		delete pAudioRecvStats_;
	}
}



bool StatsCollector::SetVideoEngin(VideoEngine *vie)
{
#ifdef VIDEO_ENABLED
	m_vie = vie;
	return true;
#endif 
	return false;
}

bool StatsCollector::SetVoiceEngin(VoiceEngine* voe)
{
	m_voe = voe;
	return true;
}



bool StatsCollector::SetVideoSendStatisticsProxy(int channelid, int capDevId)
{
#ifdef VIDEO_ENABLED
	if (!m_vie)
	{
		return false;
	}

	ViEBase*	vie_base = ViEBase::GetInterface(m_vie);
	ViECodec*	vie_codec = ViECodec::GetInterface(m_vie);
	ViERTP_RTCP *vie_rtprtcp = ViERTP_RTCP::GetInterface(m_vie);
	ViECapture *vie_capture = ViECapture::GetInterface(m_vie);

	if (vie_base)
	{
		pVideoSendStats_ = vie_base->GetSendStatisticsProxy(channelid);
		if (!pVideoSendStats_)
		{
			return false;
		}
		vie_base->RegisterSendStatisticsProxy(channelid, pVideoSendStats_);
		vie_base->Release();
		if (vie_codec)
		{
			vie_codec->RegisterEncoderObserver(channelid, *pVideoSendStats_);
			vie_codec->Release();
		}
		if (vie_rtprtcp)
		{
			vie_rtprtcp->RegisterSendChannelRtcpStatisticsCallback(channelid, pVideoSendStats_);
			vie_rtprtcp->RegisterSendChannelRtpStatisticsCallback(channelid, pVideoSendStats_);
			vie_rtprtcp->RegisterSendBitrateObserver(channelid, pVideoSendStats_);	
			vie_rtprtcp->Release();
		}

		if (vie_capture)
		{
			vie_capture->SetSendStatisticsProxy(capDevId, pVideoSendStats_);
			vie_capture->Release();
		}
		return true;
	}
	return true;
#endif
}

bool StatsCollector::SetVideoRecvStatisticsProxy(int channelid)
{
#ifdef VIDEO_ENABLED
	if (!m_vie)
	{
		return NULL;
	}
	ViEBase*	vie_base = ViEBase::GetInterface(m_vie);
	ViECodec*	vie_codec = ViECodec::GetInterface(m_vie);
	ViERTP_RTCP* vie_rtprtcp = ViERTP_RTCP::GetInterface(m_vie);
	ViERender* vie_render = ViERender::GetInterface(m_vie);

	if (vie_base)
	{
		pVideoRecvStats_ = vie_base->GetReceiveStatisticsProxy(channelid);
		if (!pVideoRecvStats_)
		{
			return false;
		}
		vie_base->RegisterReceiveStatisticsProxy(channelid, pVideoRecvStats_);
		vie_base->Release();
		if (vie_codec)
		{
			vie_codec->RegisterDecoderObserver(channelid, *pVideoRecvStats_);
			vie_codec->Release();
		}
		if (vie_rtprtcp)
		{
			vie_rtprtcp->RegisterReceiveChannelRtcpStatisticsCallback(channelid, pVideoRecvStats_);
			vie_rtprtcp->RegisterReceiveChannelRtpStatisticsCallback(channelid, pVideoRecvStats_);
			vie_rtprtcp->Release();
		}
		if (vie_render)
		{
			vie_render->AddRenderCallback(channelid, pVideoRecvStats_);
			vie_render->Release();
		}
		return true;
	}
	return false;
#endif
}

bool StatsCollector::SetAudioSendStatisticsProxy(int channelid)
{
	if (!m_voe)
	{
		return false;
	}

	if (pAudioSendStats_!=NULL)
	{
		return false;
	}

	pAudioSendStats_ = new cloopenwebrtc::AudioSendStream(m_voe, channelid);
	return true;
}
bool StatsCollector::SetAudioRecvStatisticsProxy(int channelid)
{
	if (!m_voe)
	{
		return false;
	}

	if (pAudioRecvStats_!=NULL)
	{
		return false;
	}

	pAudioRecvStats_ = new cloopenwebrtc::AudioReceiveStream(m_voe, channelid);
	return true;
}

bool StatsCollector::StatsCollectorThreadRun(void* obj)
{
	return static_cast<StatsCollector*>(obj)->ProcessStatsCollector();
}

bool StatsCollector::ProcessStatsCollector()
{
	updateEvent_->Wait(100);
	const int64_t now = clock_->TimeInMilliseconds();
	const int64_t kUpdateIntervalMs = updateIntervalMs_; //定时器
	if (now >= last_process_time_ + kUpdateIntervalMs) {
		//统计各种发送信息
		last_process_time_ = now;
#ifndef STATS_TO_STRING
#ifdef VIDEO_ENABLED
		if(pVideoSendStats_){
			pVideoSendStats_->ToFile(trace_file_);
		}
		if(pVideoRecvStats_){
			pVideoRecvStats_->ToFile(trace_file_);
		}
#endif
		if (pAudioSendStats_)
		{
			pAudioSendStats_->ToFile(trace_file_);
		}
		if (pAudioRecvStats_)
		{
			pAudioRecvStats_->ToFile(trace_file_);
		}
#else
// 		if (pVideoSendStats_) {
// 			pVideoSendStats_->ToString(trace_file_);
// 	}
// 		if (pVideoRecvStats_) {
// 			pVideoRecvStats_->ToString(trace_file_);
// 		}
// 
// 		if (pAudioSendStats_)
// 		{
// 			pAudioSendStats_->ToString(trace_file_);
// 		}
// 		if (pAudioRecvStats_)
// 		{
// 			pAudioRecvStats_->ToString(trace_file_);
// 		}

#endif
	}


	return true;
}
