/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_AUDIO_SEND_STREAM_H_
#define WEBRTC_AUDIO_SEND_STREAM_H_

#include "voe_base.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include "../system_wrappers/include/clock.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/event_wrapper.h"
#include "../system_wrappers/include/file_wrapper.h"
#include "../system_wrappers/include/scoped_ptr.h"

namespace yuntongxunwebrtc {

class AudioSendStream {
 public:
  struct Stats {
	  Stats()
		  : local_ssrc(0),
			bytes_sent(0),
			packets_sent(0),
			packets_lost(-1),
			fraction_lost(-1.0f),
			codec_name("unknown"),
			ext_seqnum(-1),
			jitter_ms(-1),
			rtt_ms(-1),
			audio_level(-1),
			aec_quality_min(-1.0f),
			echo_delay_median_ms(-1),
			echo_delay_std_ms(-1),
			echo_return_loss(-100),
			echo_return_loss_enhancement(-100),
			typing_noise_detected(false){}
    // TODO(solenberg): Harmonize naming and defaults with receive stream stats.
    uint32_t local_ssrc;
    int64_t bytes_sent;
    int32_t bitrate = 0;
    int32_t packets_sent;
    int32_t packets_lost;
    float fraction_lost;
    std::string codec_name;
    int32_t ext_seqnum;
    int32_t jitter_ms;
    int64_t rtt_ms;
    int32_t audio_level;
    float aec_quality_min;
    int32_t echo_delay_median_ms;
    int32_t echo_delay_std_ms;
    int32_t echo_return_loss;
    int32_t echo_return_loss_enhancement;
    bool typing_noise_detected;
  };

  enum LINEINFO
  {
	  VIDEOSEND = 0,
	  VIDEORECV,
	  AUDIOSEND,
	  AUDIORECV
  };
  enum VERSIONINFO
  {
	  COMPLETE = 0,
	  SPECIAL
  };
  enum CODECNAME
  {
	  PCMU = 0,
	  G729,
	  OPUS,
	  UNKOWN
  };

public:
  AudioSendStream(VoiceEngine* voe, int channel);
  ~AudioSendStream();

  Stats GetStats(int64_t &timestamp) const;
  static bool AudioSendStatisticsThreadRun(void* obj);
  bool ProcessSendStatistics();
  int channelId() { return channel_id_; }
private:
	void UpdateStats();
private:
	VoiceEngine *voe_;
	int channel_id_;
	Stats audioSendStats_;
	ThreadWrapper *thread_;
	EventWrapper *		 updateEvent_;
	Clock*              clock_;
	int64_t             last_process_time_;
	scoped_ptr<CriticalSectionWrapper> crit_;
};
}  // namespace webrtc

#endif  // WEBRTC_AUDIO_SEND_STREAM_H_
