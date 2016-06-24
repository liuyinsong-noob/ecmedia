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

#include "common_types.h"
#include "config.h"
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
   struct StreamStats {
	  FrameCounts frame_counts; //通过FrameCountObserver获取
	  int width;
	  int height;
	  // TODO(holmer): Move bitrate_bps out to the webrtc::Call layer.
	  int total_bitrate_bps; //通过BitrateStatisticsObserver获取
	  int retransmit_bitrate_bps; //通过BitrateStatisticsObserver获取
	  int avg_delay_ms;
	  int max_delay_ms;
	  StreamDataCounters rtp_stats; //通过StreamDataCountersCallback获取
	  RtcpPacketTypeCounter rtcp_packet_type_counts; //通过RtcpPacketTypeCounterObserver获取
	  RtcpStatistics rtcp_stats;  //通过RtcpStatisticsCallback获取
  };
  struct Stats{
	  Stats()
		  :encoder_implementation_name("unknown"),
		  input_frame_rate(0),
		  target_media_bitrate_bps(0),
		  avg_encode_time_ms(0),
	      encode_usage_percent(0),
		  encode_frame_rate(0),
		  media_bitrate_bps(0),
		  input_width(0),
		  input_height(0),
		  sent_width(0),
		  sent_height(0),
		  suspended(false){}
	  std::string encoder_implementation_name;
	  int input_frame_rate;
	  int target_media_bitrate_bps;
	  int avg_encode_time_ms;
	  int encode_usage_percent;
	  int encode_frame_rate; //发送端实际的编码帧率
	  int media_bitrate_bps; //发送端实际的发送速率
	  int input_width;
	  int input_height;
	  int sent_width; //发送图像宽
	  int sent_height;//发送图像高
	  bool suspended;//视频是否挂起
	  StreamStats stream;
  };

  struct Config {
	  Config()
		  : encoder_settings(),
		    rtp(),
		    render_delay_ms(0),
			target_delay_ms(0),
			suspend_below_min_bitrate(false){} 
// 	  explicit Config(Transport* send_transport)
// 		  : send_transport(send_transport) {}

	  std::string ToString() const;

	  struct EncoderSettings {
		  EncoderSettings()
			  : payload_name(nullptr),
				payload_type(-1),
				internal_source(false),
				full_overuse_time(false),
				encoder(nullptr){}
			    


		  std::string ToString() const;

		  std::string payload_name;
		  int payload_type;

		  // TODO(sophiechang): Delete this field when no one is using internal
		  // sources anymore.
		  bool internal_source;

		  // Allow 100% encoder utilization. Used for HW encoders where CPU isn't
		  // expected to be the limiting factor, but a chip could be running at
		  // 30fps (for example) exactly.
		  bool full_overuse_time;

		  // Uninitialized VideoEncoder instance to be used for encoding. Will be
		  // initialized from inside the VideoSendStream.
		  VideoEncoder* encoder;
	  } encoder_settings;

	  static const size_t kDefaultMaxPacketSize = 1500 - 40;  // TCP over IPv4.
	  struct Rtp {
		  Rtp()
			  :	rtcp_mode(RtcpMode::kCompound),
				max_packet_size(kDefaultMaxPacketSize){}
				
		  std::string ToString() const;

		  std::vector<uint32_t> ssrcs;

		  // See RtcpMode for description.
		  RtcpMode rtcp_mode;

		  // Max RTP packet size delivered to send transport from VideoEngine.
		  size_t max_packet_size;

		  // RTP header extensions to use for this send stream.
		  std::vector<RtpExtension> extensions;

		  // See NackConfig for description.
		  NackConfig nack;

		  // See FecConfig for description.
		  FecConfig fec;

		  // Settings for RTP retransmission payload format, see RFC 4588 for
		  // details.
		  struct Rtx {
			  std::string ToString() const;
			  // SSRCs to use for the RTX streams.
			  std::vector<uint32_t> ssrcs;

			  // Payload type to use for the RTX stream.
			  int payload_type;
		  } rtx;

		  // RTCP CNAME, see RFC 3550.
		  std::string c_name;
	  } rtp;

// 	  // Transport for outgoing packets.
// 	  Transport* send_transport = nullptr;
// 
// 	  // Callback for overuse and normal usage based on the jitter of incoming
// 	  // captured frames. 'nullptr' disables the callback.
// 	  LoadObserver* overuse_callback = nullptr;
// 
// 	  // Called for each I420 frame before encoding the frame. Can be used for
// 	  // effects, snapshots etc. 'nullptr' disables the callback.
// 	  rtc::VideoSinkInterface<VideoFrame>* pre_encode_callback = nullptr;
// 
// 	  // Called for each encoded frame, e.g. used for file storage. 'nullptr'
// 	  // disables the callback. Also measures timing and passes the time
// 	  // spent on encoding. This timing will not fire if encoding takes longer
// 	  // than the measuring window, since the sample data will have been dropped.
// 	  EncodedFrameObserver* post_encode_callback = nullptr;
// 
// 	  // Renderer for local preview. The local renderer will be called even if
// 	  // sending hasn't started. 'nullptr' disables local rendering.
// 	  rtc::VideoSinkInterface<VideoFrame>* local_renderer = nullptr;

	  // Expected delay needed by the renderer, i.e. the frame will be delivered
	  // this many milliseconds, if possible, earlier than expected render time.
	  // Only valid if |local_renderer| is set.
	  int render_delay_ms;

	  // Target delay in milliseconds. A positive value indicates this stream is
	  // used for streaming instead of a real-time call.
	  int target_delay_ms;

	  // True if the stream should be suspended when the available bitrate fall
	  // below the minimum configured bitrate. If this variable is false, the
	  // stream may send at a rate higher than the estimated available bitrate.
	  bool suspend_below_min_bitrate;
  };

 protected:
  virtual ~VideoSendStream() {}
};

class Call{
public:
struct Stats{
	Stats() :
		send_bandwidth_bps(0),
		recv_bandwidth_bps(0),
		pacer_delay_ms(0),
		rtt_ms(-1){}
	uint32_t send_bandwidth_bps;
	unsigned int recv_bandwidth_bps;
	int64_t pacer_delay_ms;
	int64_t rtt_ms;
};
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_SEND_STREAM_H_
