/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_AUDIO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_AUDIO_H_

#include "../common_types.h"
#include "../base/constructormagic.h"
#include "../base/criticalsection.h"
#include "../base/onetimeevent.h"
#include "../module/rtp_rtcp/source/dtmf_queue.h"
#include "../module/rtp_rtcp/source/rtp_rtcp_config.h"
#include "../module/rtp_rtcp/source/rtp_sender.h"
#include "../module/rtp_rtcp/source/rtp_utility.h"
#include "../typedefs.h"

namespace cloopenwebrtc {

class RTPSenderAudio {
 public:
  RTPSenderAudio(const int32_t id, Clock* clock, RTPSender* rtp_sender);
  ~RTPSenderAudio();

  int32_t RegisterAudioPayload(const char payloadName[RTP_PAYLOAD_NAME_SIZE],
                               int8_t payload_type,
                               uint32_t frequency,
                               size_t channels,
                               uint32_t rate,
                               RtpUtility::Payload** payload);

  bool SendAudio(FrameType frame_type,
                 int8_t payload_type,
                 uint32_t capture_timestamp,
                 const uint8_t* payload_data,
                 size_t payload_size,
                 const RTPFragmentationHeader* fragmentation);

  // Store the audio level in dBov for
  // header-extension-for-audio-level-indication.
  // Valid range is [0,100]. Actual value is negative.
  int32_t SetAudioLevel(uint8_t level_dbov);

  int32_t SetLossRate(uint32_t loss_rate, uint8_t loss_rate_hd_ext_version);

  // Send a DTMF tone using RFC 2833 (4733)
  int32_t SendTelephoneEvent(uint8_t key, uint16_t time_ms, uint8_t level);

    // Set payload type for Redundant Audio Data RFC 2198
    int32_t SetRED(const int8_t payloadType);

    // Get payload type for Redundant Audio Data RFC 2198
    int32_t RED(int8_t& payloadType) const;
 protected:
  bool SendTelephoneEventPacket(
      bool ended,
      uint32_t dtmf_timestamp,
      uint16_t duration,
      bool marker_bit);  // set on first packet in talk burst

  bool MarkerBit(FrameType frame_type, int8_t payload_type);

 private:
  int32_t             _id;
  Clock* const clock_ = nullptr;
  RTPSender* const rtp_sender_ = nullptr;

  cloopenwebrtc::CriticalSection send_audio_critsect_;

  // DTMF.
  bool dtmf_event_is_on_ = false;
  bool dtmf_event_first_packet_sent_ = false;
  int8_t dtmf_payload_type_ GUARDED_BY(send_audio_critsect_) = -1;
  uint32_t dtmf_payload_freq_ GUARDED_BY(send_audio_critsect_) = 8000;
  uint32_t dtmf_timestamp_ = 0;
  uint32_t dtmf_length_samples_ = 0;
  int64_t dtmf_time_last_sent_ = 0;
  uint32_t dtmf_timestamp_last_sent_ = 0;
  DtmfQueue::Event dtmf_current_event_;
  DtmfQueue dtmf_queue_;
  int8_t _REDPayloadType;

  // VAD detection, used for marker bit.
  bool inband_vad_active_ GUARDED_BY(send_audio_critsect_) = false;
  int8_t cngnb_payload_type_ GUARDED_BY(send_audio_critsect_) = -1;
  int8_t cngwb_payload_type_ GUARDED_BY(send_audio_critsect_) = -1;
  int8_t cngswb_payload_type_ GUARDED_BY(send_audio_critsect_) = -1;
  int8_t cngfb_payload_type_ GUARDED_BY(send_audio_critsect_) = -1;
  int8_t last_payload_type_ GUARDED_BY(send_audio_critsect_) = -1;

  // Audio level indication.
  // (https://datatracker.ietf.org/doc/draft-lennox-avt-rtp-audio-level-exthdr/)
  uint8_t audio_level_dbov_ GUARDED_BY(send_audio_critsect_) = 0;
  uint8_t loss_rate_hd_ext_version_ GUARDED_BY(send_audio_critsect_) = 0;
  uint8_t loss_rate_ GUARDED_BY(send_audio_critsect_) = 0;
  OneTimeEvent first_packet_sent_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(RTPSenderAudio);
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_SENDER_AUDIO_H_
