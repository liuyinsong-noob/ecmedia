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

#include "module.h"

#include "thread_annotations.h"
#include "common_types.h"
#include "video_codec_interface.h"
#include "clock.h"
#include "scoped_ptr.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "video_send_stream.h"
#include "video_coding_defines.h"

#include "file_wrapper.h"
#include "call_stats.h"
#include "bitrate_controller.h"
#include "remote_bitrate_estimator.h"
#include "rtp_rtcp_defines.h"
#include "overuse_frame_detector.h"
#include "paced_sender.h"

namespace cloopenwebrtc {

class ViEEncoderObserver;
class VideoEncoderRateObserver;

class CriticalSectionWrapper;

class SendStatisticsProxy : public Module,
							public ViEEncoderObserver,
							public VideoEncoderRateObserver,
							public RtcpStatisticsCallback,
							public RtcpPacketTypeCounterObserver,
							public FrameCountObserver,
							public StreamDataCountersCallback,
							public BitrateStatisticsObserver,
							public SendSideDelayObserver{
 public:
  static const int kStatsTimeoutMs;

  SendStatisticsProxy(int video_channel_);
  virtual ~SendStatisticsProxy();

  //Implement module
  virtual int64_t TimeUntilNextProcess();
  virtual int32_t Process();

  VideoSendStream::Stats GetStats();

  std::string ToString() const;


  //vieEncoder::sendData()
  //send_statistics_proxy_->OnSendEncodedImage(encoded_image, rtp_video_hdr);
  virtual void OnSendEncodedImage(const EncodedImage& encoded_image,
	  const RTPVideoHeader* rtp_video_header);

 //protected:
public:
  // From ViEEncoderObserver.
  //int32_t RegisterEncoderObserver(ViEEncoderObserver* observer);
  virtual void OutgoingRate(const int video_channel,
                            const unsigned int framerate,
                            const unsigned int bitrate) OVERRIDE;

  virtual void SuspendChange(int video_channel, bool is_suspended) OVERRIDE;

  // Implements VideoEncoderRateObserver.
  //int32_t RegisterEncoderRateObserver(VideoEncoderRateObserver *observer);
  virtual void OnSetRates(uint32_t bitrate_bps, int framerate) override;

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
						uint32_t ssrc) override;

  // From StreamDataCountersCallback.
  // RegisterSendChannelRtpStatisticsCallback(int channel, RtcpStatisticsCallback* callback);
  void DataCountersUpdated(const StreamDataCounters& counters,
							uint32_t ssrc) override;

  // From BitrateStatisticsObserver.
  //RegisterSendBitrateObserver(int channel, BitrateStatisticsObserver* callback);
  void Notify(const BitrateStatistics& total_stats,
			  const BitrateStatistics& retransmit_stats,
			  uint32_t ssrc) override;

  //From SendSideDelayObserver
  void SendSideDelayUpdated(int avg_delay_ms,
							  int max_delay_ms,
							  uint32_t ssrc) override;

  void SetCallStats(CallStats *stats);
  void SetBitrateController(BitrateController *controller);
  void SetRemoteBitrateEstimator(RemoteBitrateEstimator *remote_bitrate_estimator);
  void SetOverUseDetector(OveruseFrameDetector *overuse_detector);
  void SetPacedSender(PacedSender *paced_sender);
  void UpdateInputSize(int width, int height);
private:
	std::string GenerateFileName(int video_channel);

 private: 
  scoped_ptr<CriticalSectionWrapper> crit_;
  VideoSendStream::Stats stats_ GUARDED_BY(crit_);
  Call::Stats	call_ GUARDED_BY(crit_);


private:
	 Clock*              clock_;
	 int64_t             last_process_time_;
	 FileWrapper&		 trace_file_;
	 int				 video_channel_;
	 CallStats*			 call_stats_;
	 BitrateController*			bitrate_controller_;
	 RemoteBitrateEstimator*	remote_bitrate_estimator_;
	 OveruseFrameDetector		*overuse_detector_;
	 PacedSender*				paced_sender_;
};

}  // namespace webrtc
#endif  // WEBRTC_VIDEO_SEND_STATISTICS_PROXY_H_
