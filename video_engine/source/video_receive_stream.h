/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_RECEIVE_STREAM_H_
#define WEBRTC_VIDEO_RECEIVE_STREAM_H_

#include <string>
#include "common_types.h"
#include "../config.h"

namespace yuntongxunwebrtc {

class VideoReceiveStream {
 public:
  struct Stats : public SsrcStats {
    Stats()
        : received_framerate(0),
          decoded_framerate(0),
          rendered_framerate(0),
		  decoder_implementation_name ("unknown"),
		  decode_ms(0),
		  max_decode_ms(0),
		  current_delay_ms(0),
		  target_delay_ms(0),
		  jitter_buffer_ms(0),
		  min_playout_delay_ms(0),
		  render_delay_ms(10),
		  current_payload_type(-1),
		  total_bitrate_bps(0),
		  discarded_packets(0),
	//	  sync_offset_ms(std::numeric_limits<int>::max()),
		  last_height_(0),
		  last_width_(0),
		  ssrc(0),
		  avg_delay_ms(0) {}

    int received_framerate;
    uint32_t decoded_framerate;
    uint32_t rendered_framerate;
	int received_bitrate;
	int decoder_bitrate;

	// Decoder stats.
	std::string decoder_implementation_name;
	FrameCounts frame_counts;
	int decode_ms;
	int max_decode_ms;
	int current_delay_ms;
	int target_delay_ms;
	int jitter_buffer_ms;
	int min_playout_delay_ms;
	int render_delay_ms;

	int current_payload_type;

	int total_bitrate_bps;
	int discarded_packets;

	int sync_offset_ms;

	int last_height_;
	int last_width_;

	uint32_t ssrc;
	std::string c_name;
	StreamDataCounters rtp_stats;
	RtcpPacketTypeCounter rtcp_packet_type_counts;
	RtcpStatistics rtcp_stats;
    int avg_delay_ms;
  };
  
  virtual void Start() = 0;
  virtual void Stop() = 0;

  // TODO(pbos): Add info on currently-received codec to Stats.
  virtual Stats GetStats() const = 0;

 protected:
  virtual ~VideoReceiveStream() {}
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_RECEIVE_STREAM_H_
