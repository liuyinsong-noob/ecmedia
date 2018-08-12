/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_RECEIVE_STATISTICS_PROXY_H_
#define WEBRTC_VIDEO_RECEIVE_STATISTICS_PROXY_H_

#include <string>
#ifdef WIN32
#include "initializer_list"
#endif
#include "module.h"

#include "../base/thread_annotations.h"
#include "common_types.h"
#include "frame_callback.h"
#include "../base/rate_statistics.h"
#ifdef VIDEO_ENABLED
#include "video_coding_defines.h"
#include "vie_codec.h"
#include "vie_rtp_rtcp.h"
#include "video_receive_stream.h"
#include "video_renderer.h"
#include "video_render_defines.h"
#endif
#include "../system_wrappers/include/event_wrapper.h"
//#include "file_wrapper.h"
#include "../system_wrappers/include/stats_types.h"

namespace yuntongxunwebrtc {

class Clock;
class CriticalSectionWrapper;
class ViECodec;
class ViEDecoderObserver;

class ReceiveStatisticsProxy : public Module,
#ifdef VIDEO_ENABLED
							   public ViEDecoderObserver,
#endif
							   public RtcpPacketTypeCounterObserver,
                               public RtcpStatisticsCallback,
                               public StreamDataCountersCallback
#ifdef VIDEO_ENABLED
    ,
							   public I420FrameCallback,
							   public VideoRenderCallback
#endif
    {
 public:
  ReceiveStatisticsProxy(int channel_id);
  virtual ~ReceiveStatisticsProxy();
#ifdef VIDEO_ENABLED
  VideoReceiveStream::Stats GetStats(int64_t &timestamp) const;

  void OnDecodedFrame();
  void OnRenderedFrame();
#endif

  //Implement module
  virtual int64_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;

  void OnDiscardedPacketsUpdated(int discarded_packets);
#ifdef VIDEO_ENABLED
  // Overrides ViEDecoderObserver.
  virtual void IncomingCodecChanged(const int channel_id,
                                    const VideoCodec& video_codec) OVERRIDE; 

  virtual void IncomingRate(const int channel_id,
                            const unsigned int framerate,
                            const unsigned int bitrate_bps) OVERRIDE;
  virtual void DecoderTiming(int decode_ms,
                             int max_decode_ms,
                             int current_delay_ms,
                             int target_delay_ms,
                             int jitter_buffer_ms,
                             int min_playout_delay_ms,
                             int render_delay_ms) OVERRIDE;
  virtual void RequestNewKeyFrame(const int video_channel) OVERRIDE {}
#endif

  //Overrides RtcpPacketTypeCounterObserver
  virtual void RtcpPacketTypesCounterUpdated(uint32_t ssrc,
											 const RtcpPacketTypeCounter& packet_counter) OVERRIDE;
  // Overrides RtcpStatisticsCallback.
  virtual void StatisticsUpdated(const yuntongxunwebrtc::RtcpStatistics& statistics,
                                 uint32_t ssrc) OVERRIDE;
  virtual void CNameChanged(const char* cname, uint32_t ssrc) OVERRIDE;



  // Overrides StreamDataCountersCallback.
  virtual void DataCountersUpdated(const yuntongxunwebrtc::StreamDataCounters& counters,
                                   uint32_t ssrc) OVERRIDE;
#ifdef VIDEO_ENABLED
  //Overrides I420FrameCallback
  virtual void FrameCallback(I420VideoFrame* video_frame) OVERRIDE;

  //Override VideoRenderCallback
  virtual int32_t RenderFrame(const WebRtc_UWord32 streamId,
								I420VideoFrame& videoFrame) OVERRIDE;
#endif

  int channelId() { return channel_id_; }

private:
#ifdef WIN32
	void post_message(int reportType, std::initializer_list<StatsReport::Value> values);
#endif
 private:
  int channel_id_;
  Clock* const clock_;
  scoped_ptr<CriticalSectionWrapper> crit_;
  int64_t             last_process_time_;
#ifdef VIDEO_ENABLED
  VideoReceiveStream::Stats stats_ GUARDED_BY(crit_);
#endif
  EventWrapper* updateEvent_;
#ifdef VIDEO_ENABLED
  RateStatistics decode_fps_estimator_ GUARDED_BY(crit_);
  RateStatistics renders_fps_estimator_ GUARDED_BY(crit_);
#endif
};

}  // namespace webrtc
#endif  // WEBRTC_VIDEO_RECEIVE_STATISTICS_PROXY_H_
