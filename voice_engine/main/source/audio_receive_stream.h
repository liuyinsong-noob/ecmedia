/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_AUDIO_RECEIVE_STREAM_H_
#define WEBRTC_AUDIO_RECEIVE_STREAM_H_

#include "voe_base.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include "../system_wrappers/include/event_wrapper.h"
#include "../system_wrappers/include/file_wrapper.h"
#include "../system_wrappers/include/clock.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/scoped_ptr.h"

namespace cloopenwebrtc {

class AudioReceiveStream{
 public:
  struct Stats {
	  Stats()
		  : remote_ssrc(0),
			bytes_rcvd(0),
			packets_rcvd(0),
			packets_lost(0),
			fraction_lost(0.0f),
			codec_name("unkonwn"),
			ext_seqnum(0),
			jitter_ms(0),
			jitter_buffer_ms(0),
			jitter_buffer_preferred_ms(0),
			delay_estimate_ms(0),
			audio_level(-1),
			expand_rate(0.0f),
			speech_expand_rate(0.0f),
			secondary_decoded_rate(0.0f),
			accelerate_rate(0.0f),
			preemptive_expand_rate(0.0f),
			decoding_calls_to_silence_generator(0),
			decoding_calls_to_neteq(0),
			decoding_normal(0),
			decoding_plc(0),
			decoding_cng(0),
			decoding_plc_cng(0),
			capture_start_ntp_time_ms(0){

	  }

    uint32_t remote_ssrc;
    int64_t bytes_rcvd;
    uint32_t packets_rcvd;
    uint32_t packets_lost;
    float fraction_lost;
    std::string codec_name;
    uint32_t ext_seqnum;
    uint32_t jitter_ms;
    uint32_t jitter_buffer_ms;
    uint32_t jitter_buffer_preferred_ms;
    uint32_t delay_estimate_ms;
    int32_t audio_level;
    float expand_rate;
    float speech_expand_rate;
    float secondary_decoded_rate;
    float accelerate_rate;
    float preemptive_expand_rate;
    int32_t decoding_calls_to_silence_generator;
    int32_t decoding_calls_to_neteq;
    int32_t decoding_normal;
    int32_t decoding_plc;
    int32_t decoding_cng;
    int32_t decoding_plc_cng;
    int64_t capture_start_ntp_time_ms;
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
	AudioReceiveStream(VoiceEngine* voe, int channel);
	~AudioReceiveStream();
	Stats GetStats(int64_t &timestamp) const;
	static bool AudioRecvStatisticsThreadRun(void* obj);
	bool ProcessRecvStatistics();
	int channelId() { return channel_id_; }
private:
	void UpdateStats();
private:
	VoiceEngine *voe_;
	int channel_id_;
	Stats audioRecvStats_;
	ThreadWrapper *thread_;
	EventWrapper*	updateEvent_;
	Clock*              clock_;
	int64_t             last_process_time_;
	scoped_ptr<CriticalSectionWrapper> crit_;
};
}  // namespace webrtc

#endif  // WEBRTC_AUDIO_RECEIVE_STREAM_H_
