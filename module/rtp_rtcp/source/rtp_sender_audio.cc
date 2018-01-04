/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../module/rtp_rtcp/source/rtp_sender_audio.h"

#include <string.h>

#include <memory>
#include <utility>

#include "../system_wrappers/include/logging.h"
#include "../base/timeutils.h"
#include "../base/trace_event.h"
#include "../module/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "../module/rtp_rtcp/source/byte_io.h"
#include "../module/rtp_rtcp/source/rtp_header_extensions.h"
#include "../module/rtp_rtcp/source/rtp_packet_to_send.h"

namespace cloopenwebrtc {

RTPSenderAudio::RTPSenderAudio(const int32_t id, Clock* clock, RTPSender* rtp_sender)
    : _id(id),
      clock_(clock),
      rtp_sender_(rtp_sender) {}

RTPSenderAudio::~RTPSenderAudio() {}

int32_t RTPSenderAudio::RegisterAudioPayload(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const int8_t payload_type,
    const uint32_t frequency,
    const size_t channels,
    const uint32_t rate,
    RtpUtility::Payload** payload) {
  if (RtpUtility::StringCompare(payloadName, "cn", 2)) {
    cloopenwebrtc::CritScope cs(&send_audio_critsect_);
    //  we can have multiple CNG payload types
    switch (frequency) {
      case 8000:
        cngnb_payload_type_ = payload_type;
        break;
      case 16000:
        cngwb_payload_type_ = payload_type;
        break;
      case 32000:
        cngswb_payload_type_ = payload_type;
        break;
      case 48000:
        cngfb_payload_type_ = payload_type;
        break;
      default:
        return -1;
    }
  } else if (RtpUtility::StringCompare(payloadName, "telephone-event", 15)) {
    cloopenwebrtc::CritScope cs(&send_audio_critsect_);
    // Don't add it to the list
    // we dont want to allow send with a DTMF payloadtype
    dtmf_payload_type_ = payload_type;
    dtmf_payload_freq_ = frequency;
    return 0;
  }
  *payload = new RtpUtility::Payload;
  (*payload)->typeSpecific.Audio.frequency = frequency;
  (*payload)->typeSpecific.Audio.channels = channels;
  (*payload)->typeSpecific.Audio.rate = rate;
  (*payload)->audio = true;
  (*payload)->name[RTP_PAYLOAD_NAME_SIZE - 1] = '\0';
  strncpy((*payload)->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
  return 0;
}

bool RTPSenderAudio::MarkerBit(FrameType frame_type, int8_t payload_type) {
  cloopenwebrtc::CritScope cs(&send_audio_critsect_);
  // for audio true for first packet in a speech burst
  bool marker_bit = false;
  if (last_payload_type_ != payload_type) {
    if (payload_type != -1 && (cngnb_payload_type_ == payload_type ||
                               cngwb_payload_type_ == payload_type ||
                               cngswb_payload_type_ == payload_type ||
                               cngfb_payload_type_ == payload_type)) {
      // Only set a marker bit when we change payload type to a non CNG
      return false;
    }

    // payload_type differ
    if (last_payload_type_ == -1) {
      if (frame_type != kAudioFrameCN) {
        // first packet and NOT CNG
        return true;
      } else {
        // first packet and CNG
        inband_vad_active_ = true;
        return false;
      }
    }

    // not first packet AND
    // not CNG AND
    // payload_type changed

    // set a marker bit when we change payload type
    marker_bit = true;
  }

  // For G.723 G.729, AMR etc we can have inband VAD
  if (frame_type == kAudioFrameCN) {
    inband_vad_active_ = true;
  } else if (inband_vad_active_) {
    inband_vad_active_ = false;
    marker_bit = true;
  }
  return marker_bit;
}

bool RTPSenderAudio::SendAudio(FrameType frame_type,
                               int8_t payload_type,
                               uint32_t rtp_timestamp,
                               const uint8_t* payload_data,
                               size_t payload_size,
                               const RTPFragmentationHeader* fragmentation) {
  // From RFC 4733:
  // A source has wide latitude as to how often it sends event updates. A
  // natural interval is the spacing between non-event audio packets. [...]
  // Alternatively, a source MAY decide to use a different spacing for event
  // updates, with a value of 50 ms RECOMMENDED.
  constexpr int kDtmfIntervalTimeMs = 50;
  uint8_t audio_level_dbov = 0;
  uint32_t dtmf_payload_freq = 0;
  {
    cloopenwebrtc::CritScope cs(&send_audio_critsect_);
    audio_level_dbov = audio_level_dbov_;
    dtmf_payload_freq = dtmf_payload_freq_;
  }

  // Check if we have pending DTMFs to send
  if (!dtmf_event_is_on_ && dtmf_queue_.PendingDtmf()) {
    if ((clock_->TimeInMilliseconds() - dtmf_time_last_sent_) >
        kDtmfIntervalTimeMs) {
      // New tone to play
      dtmf_timestamp_ = rtp_timestamp;
      if (dtmf_queue_.NextDtmf(&dtmf_current_event_)) {
        dtmf_event_first_packet_sent_ = false;
        dtmf_length_samples_ =
            dtmf_current_event_.duration_ms * (dtmf_payload_freq / 1000);
        dtmf_event_is_on_ = true;
      }
    }
  }

  // A source MAY send events and coded audio packets for the same time
  // but we don't support it
  if (dtmf_event_is_on_) {
    if (frame_type == kEmptyFrame) {
      // kEmptyFrame is used to drive the DTMF when in CN mode
      // it can be triggered more frequently than we want to send the
      // DTMF packets.
      const unsigned int dtmf_interval_time_rtp =
          dtmf_payload_freq * kDtmfIntervalTimeMs / 1000;
      if ((rtp_timestamp - dtmf_timestamp_last_sent_) <
          dtmf_interval_time_rtp) {
        // not time to send yet
        return true;
      }
    }
    dtmf_timestamp_last_sent_ = rtp_timestamp;
    uint32_t dtmf_duration_samples = rtp_timestamp - dtmf_timestamp_;
    bool ended = false;
    bool send = true;

    if (dtmf_length_samples_ > dtmf_duration_samples) {
      if (dtmf_duration_samples <= 0) {
        // Skip send packet at start, since we shouldn't use duration 0
        send = false;
      }
    } else {
      ended = true;
      dtmf_event_is_on_ = false;
      dtmf_time_last_sent_ = clock_->TimeInMilliseconds();
    }
    if (send) {
      if (dtmf_duration_samples > 0xffff) {
        // RFC 4733 2.5.2.3 Long-Duration Events
        SendTelephoneEventPacket(ended, dtmf_timestamp_,
                                 static_cast<uint16_t>(0xffff), false);

        // set new timestap for this segment
        dtmf_timestamp_ = rtp_timestamp;
        dtmf_duration_samples -= 0xffff;
        dtmf_length_samples_ -= 0xffff;

        return SendTelephoneEventPacket(ended, dtmf_timestamp_,
            static_cast<uint16_t>(dtmf_duration_samples), false);
      } else {
        if (!SendTelephoneEventPacket(ended, dtmf_timestamp_,
                                      dtmf_duration_samples,
                                      !dtmf_event_first_packet_sent_)) {
          return false;
        }
        dtmf_event_first_packet_sent_ = true;
        return true;
      }
    }
    return true;
  }
  if (payload_size == 0 || payload_data == NULL) {
    if (frame_type == kEmptyFrame) {
      // we don't send empty audio RTP packets
      // no error since we use it to drive DTMF when we use VAD
      return true;
    }
    return false;
  }

  std::unique_ptr<RtpPacketToSend> packet = rtp_sender_->AllocatePacket();
  packet->SetMarker(MarkerBit(frame_type, payload_type));
  packet->SetPayloadType(payload_type);
  packet->SetTimestamp(rtp_timestamp);
  packet->set_capture_time_ms(clock_->TimeInMilliseconds());
  // Update audio level extension, if included.
  packet->SetExtension<AudioLevel>(frame_type == kAudioFrameSpeech,
                                   audio_level_dbov);
  packet->SetExtension<LossRate>(loss_rate_hd_ext_version_, loss_rate_);
		if (fragmentation) {
			int rtpHeaderLength = 0;
			uint16_t allocLen = 0;
			for (int counter = 0; counter < fragmentation->fragmentationVectorSize; counter++) {
				allocLen += fragmentation->fragmentationLength[counter];
			}
			uint8_t* dataBuffer =
				packet->AllocatePayload(1 + allocLen);
			if (!dataBuffer)  // Too large payload buffer.
				return false;
			if (fragmentation->fragmentationVectorSize > 1) {
				// Use the fragment info if we have one.

				//    payload[0] = fragmentation->fragmentationPlType[0];
				//    memcpy(payload + 1, payload_data + fragmentation->fragmentationOffset[0],
				//           fragmentation->fragmentationLength[0]);


				uint16_t timestampOffset = 0;
				int maxOffset = 0x3fff;
				if (fragmentation->fragmentationVectorSize == 3) {
					maxOffset = fragmentation->fragmentationTimeDiff[2];
				}
				else if (fragmentation->fragmentationVectorSize == 2)
				{
					maxOffset = fragmentation->fragmentationTimeDiff[1];
				}
				if (maxOffset <= 0x3fff) {
					//        if(fragmentation->fragmentationVectorSize != 2) {   //sean opus red test
					//          // we only support 2 codecs when using RED
					//          return -1;
					//        }
					// only 0x80 if we have multiple blocks
					size_t blockLength = 0;
					size_t REDheader = 0;
					if (fragmentation->fragmentationVectorSize == 3) {
						dataBuffer[rtpHeaderLength++] = 0x80 +
							fragmentation->fragmentationPlType[2];
						blockLength = fragmentation->fragmentationLength[2];

						// sanity blockLength
						if (blockLength > 0x3ff) {  // block length 10 bits 1023 bytes
							return false;
						}
						timestampOffset = fragmentation->fragmentationTimeDiff[2];
						REDheader = (timestampOffset << 10) + blockLength;
						RtpUtility::AssignUWord24ToBuffer(dataBuffer + rtpHeaderLength,REDheader);
						rtpHeaderLength += 3;
					}


					dataBuffer[rtpHeaderLength++] = 0x80 + fragmentation->fragmentationPlType[1];
					blockLength = fragmentation->fragmentationLength[1];
					if (blockLength > 0x3ff) {
						return false;
					}
					timestampOffset = fragmentation->fragmentationTimeDiff[1];
					REDheader = (timestampOffset << 10) + blockLength;
					RtpUtility::AssignUWord24ToBuffer(dataBuffer + rtpHeaderLength, REDheader);
					rtpHeaderLength += 3;


					dataBuffer[rtpHeaderLength++] = fragmentation->fragmentationPlType[0];
					// copy the RED data 2

					size_t fragmentationLength2 = 0;
					if (fragmentation->fragmentationVectorSize == 3) {
						memcpy(dataBuffer + rtpHeaderLength, payload_data + fragmentation->fragmentationOffset[2], fragmentation->fragmentationLength[2]);
						fragmentationLength2 = fragmentation->fragmentationLength[2];

					}

					//sean check data
					//          printf("sean check data RED2\n");
					//          for (int ii=0; ii<fragmentation->fragmentationLength[2]; ii++) {
					//              printf("%02X ", *(payloadData+fragmentation->fragmentationOffset[2]+ii));
					//          }
					//          printf("\nsean check data RED2 end\n");



					// copy the RED data 1
					memcpy(dataBuffer + rtpHeaderLength + fragmentationLength2,
						payload_data + fragmentation->fragmentationOffset[1],
						fragmentation->fragmentationLength[1]);
					//          printf("sean check data RED1\n");
					//          for (int ii=0; ii<fragmentation->fragmentationLength[1]; ii++) {
					//              printf("%02X ", *(payloadData+fragmentation->fragmentationOffset[1]+ii));
					//          }
					//          printf("\nsean check data RED1 end\n");



					// copy the normal data
					memcpy(dataBuffer + rtpHeaderLength + fragmentationLength2 +
						fragmentation->fragmentationLength[1],
						payload_data + fragmentation->fragmentationOffset[0],
						fragmentation->fragmentationLength[0]);

					//          payloadSize = fragmentation->fragmentationLength[0] +
					//          fragmentation->fragmentationLength[1] + fragmentationLength2;
				}
				else {
					// silence for too long send only new data
					dataBuffer[rtpHeaderLength++] = fragmentation->fragmentationPlType[0];
					memcpy(dataBuffer + rtpHeaderLength,
						payload_data + fragmentation->fragmentationOffset[0],
						fragmentation->fragmentationLength[0]);

					//          payloadSize = fragmentation->fragmentationLength[0];
				}
			}
			else {
				if (fragmentation->fragmentationVectorSize > 0) {
					// use the fragment info if we have one
					dataBuffer[rtpHeaderLength++] = fragmentation->fragmentationPlType[0];
					memcpy(dataBuffer + rtpHeaderLength,
						payload_data + fragmentation->fragmentationOffset[0],
						fragmentation->fragmentationLength[0]);

					//                payloadSize = fragmentation->fragmentationLength[0];
				}
			}
			//        _lastPayloadType = payloadType;
		}
		else {
			uint8_t* payload = packet->AllocatePayload(payload_size);
			if (!payload)  // Too large payload buffer.
				return false;
			memcpy(payload, payload_data, payload_size);
		}

  if (!rtp_sender_->AssignSequenceNumber(packet.get()))
    return false;


  {
    cloopenwebrtc::CritScope cs(&send_audio_critsect_);
    last_payload_type_ = payload_type;
  }
  TRACE_EVENT_ASYNC_END2("webrtc", "Audio", rtp_timestamp, "timestamp",
                         packet->Timestamp(), "seqnum",
                         packet->SequenceNumber());

    static time_t last = 0;
    int logInterval = 5;
	if( time(NULL) > last + logInterval ) {
		 LOG(LS_WARNING) << "Period log per " << logInterval << " seconds: Audio SendAudio(payloadSize=" 
			 << payload_size << ")";
        last = time(NULL);
	}

  bool send_result = rtp_sender_->SendToNetwork(
      std::move(packet), kAllowRetransmission, RtpPacketSender::kHighPriority);
  if (first_packet_sent_()) {
    LOG(LS_INFO) << "First audio RTP packet sent to pacer";
  }
  return send_result;
}


// Audio level magnitude and voice activity flag are set for each RTP packet
int32_t RTPSenderAudio::SetAudioLevel(uint8_t level_dbov) {
  if (level_dbov > 127) {
    return -1;
  }
  cloopenwebrtc::CritScope cs(&send_audio_critsect_);
  audio_level_dbov_ = level_dbov;
  return 0;
}

int32_t RTPSenderAudio::SetLossRate(uint32_t loss_rate, uint8_t loss_rate_hd_ext_version)
{
	if (loss_rate > 0x14) {
		return -1;
	}
	else if (loss_rate > 0x0f) {  // current only support max loss rate 15*5
		loss_rate = 0x0f;
	}

	cloopenwebrtc::CritScope cs(&send_audio_critsect_);
	loss_rate_hd_ext_version_ = loss_rate_hd_ext_version;
	loss_rate_ = loss_rate;
	return 0;
}

    // Set payload type for Redundant Audio Data RFC 2198
int32_t
RTPSenderAudio::SetRED(const int8_t payloadType)
{
    if(payloadType < -1 )
    {
        return -1;
    }
    _REDPayloadType = payloadType;
    return 0;
}

    // Get payload type for Redundant Audio Data RFC 2198
int32_t
RTPSenderAudio::RED(int8_t& payloadType) const
{
    if(_REDPayloadType == -1)
    {
        // not configured
        return -1;
    }
    payloadType = _REDPayloadType;
    return 0;
}

// Send a TelephoneEvent tone using RFC 2833 (4733)
int32_t RTPSenderAudio::SendTelephoneEvent(uint8_t key,
                                           uint16_t time_ms,
                                           uint8_t level) {
  DtmfQueue::Event event;
  {
    cloopenwebrtc::CritScope lock(&send_audio_critsect_);
    if (dtmf_payload_type_ < 0) {
      // TelephoneEvent payloadtype not configured
      return -1;
    }
    event.payload_type = dtmf_payload_type_;
  }
  event.key = key;
  event.duration_ms = time_ms;
  event.level = level;
  return dtmf_queue_.AddDtmf(event) ? 0 : -1;
}

bool RTPSenderAudio::SendTelephoneEventPacket(bool ended,
                                              uint32_t dtmf_timestamp,
                                              uint16_t duration,
                                              bool marker_bit) {
  uint8_t send_count = 1;
  bool result = true;

  if (ended) {
    // resend last packet in an event 3 times
    send_count = 3;
  }
  do {
    // Send DTMF data.
    constexpr RtpPacketToSend::ExtensionManager* kNoExtensions = nullptr;
    constexpr size_t kDtmfSize = 4;
    std::unique_ptr<RtpPacketToSend> packet(
        new RtpPacketToSend(kNoExtensions, kRtpHeaderSize + kDtmfSize));
    packet->SetPayloadType(dtmf_current_event_.payload_type);
    packet->SetMarker(marker_bit);
    packet->SetSsrc(rtp_sender_->SSRC());
    packet->SetTimestamp(dtmf_timestamp);
    packet->set_capture_time_ms(clock_->TimeInMilliseconds());
    if (!rtp_sender_->AssignSequenceNumber(packet.get()))
      return false;

    // Create DTMF data.
    uint8_t* dtmfbuffer = packet->AllocatePayload(kDtmfSize);
    DCHECK(dtmfbuffer);
    /*    From RFC 2833:
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |     event     |E|R| volume    |          duration             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    // R bit always cleared
    uint8_t R = 0x00;
    uint8_t volume = dtmf_current_event_.level;

    // First packet un-ended
    uint8_t E = ended ? 0x80 : 0x00;

    // First byte is Event number, equals key number
    dtmfbuffer[0] = dtmf_current_event_.key;
    dtmfbuffer[1] = E | R | volume;
    ByteWriter<uint16_t>::WriteBigEndian(dtmfbuffer + 2, duration);

    TRACE_EVENT_INSTANT2(
        TRACE_DISABLED_BY_DEFAULT("webrtc_rtp"), "Audio::SendTelephoneEvent",
        "timestamp", packet->Timestamp(), "seqnum", packet->SequenceNumber());
    result = rtp_sender_->SendToNetwork(std::move(packet), kAllowRetransmission,
                                        RtpPacketSender::kHighPriority);
    send_count--;
  } while (send_count > 0 && result);

  return result;
}
}  // namespace cloopenwebrtc
