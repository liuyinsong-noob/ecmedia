/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_SEND_STREAM_H_
#define WEBRTC_VIDEO_SEND_STREAM_H_

#include <map>
#include <string>
#include <list>

#include "common_types.h"
#include "../config.h"
#include "frame_callback.h"
#include "video_renderer.h"

namespace cloopenwebrtc {

class VideoEncoder;

// Class to deliver captured frame to the video send stream.
class VideoSendStreamInput {
 public:
  // These methods do not lock internally and must be called sequentially.
  // If your application switches input sources synchronization must be done
  // externally to make sure that any old frames are not delivered concurrently.
  virtual void SwapFrame(I420VideoFrame* video_frame) = 0;

 protected:
  virtual ~VideoSendStreamInput() {}
};

class VideoSendStream {
 public:
	 struct CallStats {
		 CallStats() :
			 sendside_bwe_bps(0),
			 recv_bandwidth_bps(0),
			 pacer_delay_ms(0),
			 rtt_ms(-1) {}
		 uint32_t sendside_bwe_bps;
		 uint32_t recv_bandwidth_bps;
		 int64_t pacer_delay_ms;
		 int64_t rtt_ms;
	 };
   struct StreamStats {
	  FrameCounts frame_counts; //通过FrameCountObserver获取
	  int width;
	  int height;
	  BitrateStatistics total_stats;
	  BitrateStatistics retransmit_stats;
	  int avg_delay_ms;
	  int max_delay_ms;
	  StreamDataCounters rtp_stats; //通过StreamDataCountersCallback获取
	  RtcpPacketTypeCounter rtcp_packet_type_counts; //通过RtcpPacketTypeCounterObserver获取
	  RtcpStatistics rtcp_stats;  //通过RtcpStatisticsCallback获取
  };
   struct Config
   {
	   struct EncoderSettings {
		   std::string	ToString();
		   std::string	payload_name = "unknown";
		   int	payload_type;
		   int	width;
		   int  height;
		   int  startBitrate;  // kilobits/sec.
		   int  maxBitrate;  // kilobits/sec.
		   int  minBitrate;  // kilobits/sec.
		   int  targetBitrate;  // kilobits/sec.
		   int  maxFramerate;
		   VideoCodecUnion     codecSpecific; //not used for now
		   unsigned int        qpMax;
		   uint8_t      numberOfSimulcastStreams;
		   SimulcastStream     simulcastStream[kMaxSimulcastStreams];
	   } encoder_settings;

	   std::string ToString();
   };
  struct Stats{
	  Stats()
		  :encoder_implementation_name("unknown"),
		  target_enc_framerate(0),
		  target_enc_bitrate_bps(0),
		  avg_encode_time_ms(0),
	      encode_usage_percent(0),
		  actual_enc_framerate(0),
		  actual_enc_bitrate_bps(0),
		  captured_width(0),
		  captured_height(0),
		  captured_framerate(0),
		  qm_width(0),
		  qm_height(0),
		  qm_framerate(0),
		  suspended(false){}
	  std::string encoder_implementation_name;
	  int target_enc_framerate;
	  int actual_enc_framerate;
	  int target_enc_bitrate_bps;
	  int actual_enc_bitrate_bps;
	  int avg_encode_time_ms;
	  int encode_usage_percent;
	  int captured_width;
	  int captured_height;
	  int captured_framerate;
	  int qm_width;
	  int qm_height;
	  int qm_framerate;
	  bool suspended;//视频是否挂起
	  std::list<unsigned int> ssrc_streams;
	  std::map<uint32_t, StreamStats> substreams;
	  StreamStats stream;
	  CallStats	call;
	  Config	config;
  };
  

 protected:
  virtual ~VideoSendStream() {}
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_SEND_STREAM_H_
