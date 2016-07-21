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
#include "thread_wrapper.h"
#include "clock.h"
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "file_wrapper.h"

namespace cloopenwebrtc {

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

  struct UploadStats1
  {
	  char WhichStats[2];
	  char Version[2];
	  char CodecName[4];
	  int32_t PacketsSent;
	  int32_t JitterReceived;
	  int32_t PacketsLost;
	  int32_t EcEchoDelayMedian;
	  int32_t EcEchoDelayStdDev;
	  int32_t EcReturnLoss;
	  int32_t EcRetrunLossEnhancement;
	  int64_t BytesSent;
	  int64_t Rtt;
  };

public:
  AudioSendStream(VoiceEngine* voe, int channel);
  ~AudioSendStream();

  Stats GetStats() const;
  static bool AudioSendStatisticsThreadRun(void* obj);
  bool ProcessSendStatistics();
  std::string ToString() const;
  int ToBinary(char* buffer, int buffer_length);
  int ToFile(FileWrapper *pFile);
  int ToString(FileWrapper *pFile);
private:
	std::string GenerateFileName(int channel);
	void UpdateStats();
	void FillUploadStats();
private:
	VoiceEngine *voe_;
	int channel_;
	Stats audioSendStats_;
	UploadStats1 UploadStats;
	ThreadWrapper *thread_;
	FileWrapper&		 trace_file_;
	EventWrapper *		 updateEvent_;
	Clock*              clock_;
	int64_t             last_process_time_;
	scoped_ptr<CriticalSectionWrapper> crit_;
};
}  // namespace webrtc

#endif  // WEBRTC_AUDIO_SEND_STREAM_H_
