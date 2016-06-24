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

#include "module.h"

#include "thread_annotations.h"
#include "common_types.h"
#include "frame_callback.h"
#include "rate_statistics.h"
#include "video_coding_defines.h"
#include "vie_codec.h"
#include "vie_rtp_rtcp.h"
#include "video_receive_stream.h"
#include "video_renderer.h"
#include "video_render_defines.h"

#include "file_wrapper.h"

namespace cloopenwebrtc {

class Clock;
class CriticalSectionWrapper;
class ViECodec;
class ViEDecoderObserver;

class ReceiveStatisticsProxy : public Module,
							   public ViEDecoderObserver,
                               public VCMReceiveStatisticsCallback,
							   public RtcpPacketTypeCounterObserver,
                               public RtcpStatisticsCallback,
                               public StreamDataCountersCallback,
							   public I420FrameCallback,
							   public VideoRenderCallback{
 public:
  ReceiveStatisticsProxy(int video_channel);
  virtual ~ReceiveStatisticsProxy();

  std::string ToString() const;
  VideoReceiveStream::Stats GetStats() const;

  void OnDecodedFrame();
  void OnRenderedFrame();

  //Implement module
  virtual int64_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;

  // Overrides VCMReceiveStatisticsCallback
  virtual void OnReceiveRatesUpdated(uint32_t bitRate,
                                     uint32_t frameRate) OVERRIDE;
  virtual void OnFrameCountsUpdated(const FrameCounts& frame_counts) OVERRIDE;
  virtual void OnDiscardedPacketsUpdated(int discarded_packets) OVERRIDE;

  // Overrides ViEDecoderObserver.
  virtual void IncomingCodecChanged(const int video_channel,
                                    const VideoCodec& video_codec) OVERRIDE; 

  virtual void IncomingRate(const int video_channel,
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

  //Overrides RtcpPacketTypeCounterObserver
  virtual void RtcpPacketTypesCounterUpdated(uint32_t ssrc,
											 const RtcpPacketTypeCounter& packet_counter) OVERRIDE;
  // Overrides RtcpStatisticsCallback.
  virtual void StatisticsUpdated(const cloopenwebrtc::RtcpStatistics& statistics,
                                 uint32_t ssrc) OVERRIDE;
  virtual void CNameChanged(const char* cname, uint32_t ssrc) OVERRIDE;



  // Overrides StreamDataCountersCallback.
  virtual void DataCountersUpdated(const cloopenwebrtc::StreamDataCounters& counters,
                                   uint32_t ssrc) OVERRIDE;

  //Overrides I420FrameCallback
  virtual void FrameCallback(I420VideoFrame* video_frame) OVERRIDE;

  //Override VideoRenderCallback
  virtual int32_t RenderFrame(const WebRtc_UWord32 streamId,
								I420VideoFrame& videoFrame) OVERRIDE;

private:
	std::string GenerateFileName(int video_channel);
 private:
  Clock* const clock_;
  scoped_ptr<CriticalSectionWrapper> crit_;
  int64_t             last_process_time_;
  VideoReceiveStream::Stats stats_ GUARDED_BY(crit_);
  FileWrapper&  trace_file_;
  RateStatistics decode_fps_estimator_ GUARDED_BY(crit_);
  RateStatistics renders_fps_estimator_ GUARDED_BY(crit_);
};

}  // namespace webrtc
#endif  // WEBRTC_VIDEO_RECEIVE_STATISTICS_PROXY_H_
