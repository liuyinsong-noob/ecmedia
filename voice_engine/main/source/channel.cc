/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "channel.h"

#include "format_macros.h"
#include "timeutils.h"
#include "common.h"
#include "audio_device.h"
#include "audio_processing.h"
#include "module_common_types.h"
#include "receive_statistics.h"
#include "rtp_payload_registry.h"
#include "rtp_receiver.h"
#include "rtp_receiver_strategy.h"
#include "audio_frame_operations.h"
#include "process_thread.h"
#include "rtp_dump.h"
#include "critical_section_wrapper.h"
#include "logging.h"
#include "trace.h"
#include "vie_network.h"
#include "voe_base.h"
#include "voe_external_media.h"
#include "voe_rtp_rtcp.h"
#include "output_mixer.h"
#include "statistics.h"
#include "transmit_mixer.h"
#include "utility.h"

#include "rtp.h"

#if defined(_WIN32)
#include <Qos.h>
#endif

namespace cloopenwebrtc {
namespace voe {

// Extend the default RTCP statistics struct with max_jitter, defined as the
// maximum jitter value seen in an RTCP report block.
struct ChannelStatistics : public RtcpStatistics {
  ChannelStatistics() : rtcp(), max_jitter(0) {}

  RtcpStatistics rtcp;
  uint32_t max_jitter;
};

// Statistics callback, called at each generation of a new RTCP report block.
class StatisticsProxy : public RtcpStatisticsCallback {
 public:
  StatisticsProxy(uint32_t ssrc)
   : stats_lock_(CriticalSectionWrapper::CreateCriticalSection()),
     ssrc_(ssrc) {}
  virtual ~StatisticsProxy() {}

  virtual void StatisticsUpdated(const RtcpStatistics& statistics,
                                 uint32_t ssrc) OVERRIDE {
    if (ssrc != ssrc_)
      return;

    CriticalSectionScoped cs(stats_lock_.get());
    stats_.rtcp = statistics;
    if (statistics.jitter > stats_.max_jitter) {
      stats_.max_jitter = statistics.jitter;
    }
  }

  virtual void CNameChanged(const char* cname, uint32_t ssrc) OVERRIDE {}

  void ResetStatistics() {
    CriticalSectionScoped cs(stats_lock_.get());
    stats_ = ChannelStatistics();
  }

  ChannelStatistics GetStats() {
    CriticalSectionScoped cs(stats_lock_.get());
    return stats_;
  }

 private:
  // StatisticsUpdated calls are triggered from threads in the RTP module,
  // while GetStats calls can be triggered from the public voice engine API,
  // hence synchronization is needed.
  scoped_ptr<CriticalSectionWrapper> stats_lock_;
  const uint32_t ssrc_;
  ChannelStatistics stats_;
};

class VoEBitrateObserver : public BitrateObserver {
 public:
  explicit VoEBitrateObserver(Channel* owner)
      : owner_(owner) {}
  virtual ~VoEBitrateObserver() {}

  // Implements BitrateObserver.
  virtual void OnNetworkChanged(const uint32_t bitrate_bps,
                                const uint8_t fraction_lost,
                                const int64_t rtt) OVERRIDE {
    // |fraction_lost| has a scale of 0 - 255.
    owner_->OnNetworkChanged(bitrate_bps, fraction_lost, rtt);
  }

 private:
  Channel* owner_;
};

int32_t
Channel::SendData(FrameType frameType,
                  uint8_t   payloadType,
                  uint32_t  timeStamp,
                  const uint8_t*  payloadData,
                  size_t    payloadSize,
                  const RTPFragmentationHeader* fragmentation)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SendData(frameType=%u, payloadType=%u, timeStamp=%u,"
                 " payloadSize=%" PRIuS ", fragmentation=0x%x)",
                 frameType, payloadType, timeStamp,
                 payloadSize, fragmentation);

    if (_includeAudioLevelIndication)
    {
        // Store current audio level in the RTP/RTCP module.
        // The level will be used in combination with voice-activity state
        // (frameType) to add an RTP header extension
		_rtpRtcpModule->SetAudioLevel(rms_level_.RMS());
    }

	if (_pause || payloadSize <= 0) {
		return 0;
	}

    // Push data from ACM to RTP/RTCP-module to deliver audio frame for
    // packetization.
    // This call will trigger Transport::SendPacket() from the RTP/RTCP module.
    if (_rtpRtcpModule->SendOutgoingData((FrameType&)frameType,
                                        payloadType,
                                        timeStamp,
                                        // Leaving the time when this frame was
                                        // received from the capture device as
                                        // undefined for voice for now.
                                        -1,
                                        payloadData,
                                        payloadSize,
                                        fragmentation) == -1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceWarning,
            "Channel::SendData() failed to send data to RTP/RTCP module");
        return -1;
    }

    _lastLocalTimeStamp = timeStamp;
    _lastPayloadType = payloadType;

	{
		CriticalSectionScoped cs(critsect_net_statistic.get());
		if(_startNetworkTime == 0)
			_startNetworkTime = time(NULL);
		if (payloadSize>0) {
			if (_isWifi) {
				_sendDataTotalWifi += payloadSize;
				_sendDataTotalWifi += 54;//14+20+8+12	//ethernet+ip+udp+rtp header
			}
			else
			{
				_sendDataTotalSim += payloadSize;
				_sendDataTotalSim += 54;//14+20+8+12	//ethernet+ip+udp+rtp header
			}
		}
	}

    return 0;
}

int32_t
Channel::InFrameType(int16_t frameType)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::InFrameType(frameType=%d)", frameType);

    CriticalSectionScoped cs(&_callbackCritSect);
    // 1 indicates speech
    _sendFrameType = (frameType == 1) ? 1 : 0;
    return 0;
}

int32_t
Channel::OnRxVadDetected(int vadDecision)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                 "Channel::OnRxVadDetected(vadDecision=%d)", vadDecision);

    CriticalSectionScoped cs(&_callbackCritSect);
    if (_rxVadObserverPtr)
    {
        _rxVadObserverPtr->OnRxVad(_channelId, vadDecision);
    }

    return 0;
}

int
Channel::SendPacket(int channel, const void *data, size_t len, int sn)
{
    static int count = 0;
    if (_loss >=1 && count++ != 0) {
        if (count == 65535) {
            count = 0;
        }
        if (count%_loss == 0) {
//            printf("sean haha loss sn %d\n",sn);
            return 0;
        }
    }
//    printf("sean haha send sn %d\n",sn);
    channel = VoEChannelId(channel);
    assert(channel == _channelId);

    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SendPacket(channel=%d, len=%" PRIuS ")", channel,
                 len);

    CriticalSectionScoped cs(&_callbackCritSect);

    if (_transportPtr == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,_channelId),
                     "Channel::SendPacket() failed to send RTP packet due to"
                     " invalid transport object");
        return -1;
    }

    uint8_t* bufferToSendPtr = (uint8_t*)data;
    size_t bufferLength = len;

    //// Dump the RTP packet to a file (if RTP dump is enabled).
    //if (_rtpDumpOut.DumpPacket((const uint8_t*)data, len) == -1)
    //{
    //    WEBRTC_TRACE(kTraceWarning, kTraceVoice,
    //                 VoEId(_instanceId,_channelId),
    //                 "Channel::SendPacket() RTP dump to output file failed");
    //}

	// SRTP or External encryption
	if (_encrypting)
	{
		//CriticalSectionScoped cs(&_callbackCritSect);

		if (_encryptionPtr)
		{
			if (!_encryptionRTPBufferPtr)
			{
				WEBRTC_TRACE(kTraceError, kTraceVoice,
					VoEId(_instanceId, _channelId),
					"Channel::SendPacket() _encryptionRTPBufferPtr is NULL");
				return -1;
			}

			// Perform encryption (SRTP or external)
			WebRtc_Word32 encryptedBufferLength = 0;
			_encryptionPtr->encrypt(_channelId,
				bufferToSendPtr,
				_encryptionRTPBufferPtr,
				bufferLength,
				(int*)&encryptedBufferLength);
			if (encryptedBufferLength <= 0)
			{
				WEBRTC_TRACE(kTraceDebug,kTraceVoice,VoEId(_instanceId,_channelId),"Channel::SendPacket() encryption failed encryptedBufferLength <= 0, is %d\n",encryptedBufferLength);
				_engineStatisticsPtr->SetLastError(
					VE_ENCRYPTION_FAILED,
					kTraceError, "Channel::SendPacket() encryption failed");
				return -1;
			}

			//append ssrc to end, 4 bytes
			//memcpy(_encryptionRTPBufferPtr+encryptedBufferLength, _encryptionRTPBufferPtr+8, 4);
			//encryptedBufferLength += 4;

			//            printf("HERE after encrypt and append\n");
			//            for (int j=0; j<encryptedBufferLength; j++) {
			//                if (j == 12) {
			//                    printf("\t");
			//                }
			//                printf("%02X ",_encryptionRTPBufferPtr[j]);
			//            }
			//            printf("\n");
			// Replace default data buffer with encrypted buffer
			bufferToSendPtr = _encryptionRTPBufferPtr;
			bufferLength = encryptedBufferLength;
		}
	}

	int backDataLen = 0;
//	if (_serviceCoreCallBack && _processDataFlag && bufferLength > 12) {
//		if (NULL == this->_sendData) {
//			this->_sendData = (void *)malloc(733);
//
//		}
//
//		_serviceCoreCallBack->onAudioData(call_id, bufferToSendPtr+12, bufferLength-12, (WebRtc_UWord8 *)this->_sendData+12, backDataLen, true);
//		memcpy(this->_sendData, bufferToSendPtr, 12);
//		bufferToSendPtr = (WebRtc_UWord8*)this->_sendData;
//		bufferLength = backDataLen+12;
//	}
    
    if (_audio_data_cb /*&& _processDataFlag*/ && bufferLength > 12) {
        if (NULL == this->_sendData) {
            this->_sendData = (void *)malloc(733);
            
        }
//        typedef int (*onAudioData)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
        _audio_data_cb(_channelId, bufferToSendPtr+12, (int)(bufferLength-12), (WebRtc_UWord8 *)this->_sendData+12, backDataLen, true);
        memcpy(this->_sendData, bufferToSendPtr, 12);
        bufferToSendPtr = (WebRtc_UWord8*)this->_sendData;
        bufferLength = backDataLen+12;
    }
    
    int n = _transportPtr->SendPacket(channel, bufferToSendPtr,
                                      bufferLength);
    if (n < 0) {
      std::string transport_name =
          _externalTransport ? "external transport" : "WebRtc sockets";
      WEBRTC_TRACE(kTraceError, kTraceVoice,
                   VoEId(_instanceId,_channelId),
                   "Channel::SendPacket() RTP transmission using %s failed",
                   transport_name.c_str());
      return -1;
    }

	{
		//CriticalSectionScoped cs(critsect_net_statistic.get());
		if(_startNetworkTime == 0)
			_startNetworkTime = time(NULL);
		//			_sendDataTotal += bufferLength;
		//			_sendDataTotal += 8;// ip+udp header
	}
    return n;
}

int
Channel::SendRTCPPacket(int channel, const void *data, size_t len)
{
    channel = VoEChannelId(channel);
    assert(channel == _channelId);

    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SendRTCPPacket(channel=%d, len=%" PRIuS ")", channel,
                 len);

    CriticalSectionScoped cs(&_callbackCritSect);
    if (_transportPtr == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "Channel::SendRTCPPacket() failed to send RTCP packet"
                     " due to invalid transport object");
        return -1;
    }

    uint8_t* bufferToSendPtr = (uint8_t*)data;
    size_t bufferLength = len;

    //// Dump the RTCP packet to a file (if RTP dump is enabled).
    //if (_rtpDumpOut.DumpPacket((const uint8_t*)data, len) == -1)
    //{
    //    WEBRTC_TRACE(kTraceWarning, kTraceVoice,
    //                 VoEId(_instanceId,_channelId),
    //                 "Channel::SendPacket() RTCP dump to output file failed");
    //}

	// SRTP or External encryption
	//if (_encrypting)
    if (false)  //hubin 2017.2.18  we don't support rtcp srtp.
	{
		//CriticalSectionScoped cs(&_callbackCritSect);

		if (_encryptionPtr)
		{
			if (!_encryptionRTCPBufferPtr)
			{
				WEBRTC_TRACE(kTraceError, kTraceVoice,
					VoEId(_instanceId, _channelId),
					"Channel::SendPacket() _encryptionRTCPBufferPtr is NULL");
				return -1;
			}

			// Perform encryption (SRTP or external).
			WebRtc_Word32 encryptedBufferLength = 0;
			_encryptionPtr->encrypt_rtcp(_channelId,
				bufferToSendPtr,
				_encryptionRTCPBufferPtr,
				bufferLength,
				(int*)&encryptedBufferLength);
			if (encryptedBufferLength <= 0)
			{
				_engineStatisticsPtr->SetLastError(
					VE_ENCRYPTION_FAILED, kTraceError,
					"Channel::SendRTCPPacket() encryption failed");
				return -1;
			}

			// Replace default data buffer with encrypted buffer
			bufferToSendPtr = _encryptionRTCPBufferPtr;
			bufferLength = encryptedBufferLength;
		}
	}

    int n = _transportPtr->SendRTCPPacket(channel,
                                          bufferToSendPtr,
                                          bufferLength);
    if (n < 0) {
      std::string transport_name =
          _externalTransport ? "external transport" : "WebRtc sockets";
      WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                   VoEId(_instanceId,_channelId),
                   "Channel::SendRTCPPacket() transmission using %s failed",
                   transport_name.c_str());
      return -1;
    }

	{
			//CriticalSectionScoped cs(critsect_net_statistic.get());
			if(_startNetworkTime == 0)
				_startNetworkTime = time(NULL);
//			_sendDataTotal += bufferLength;
//            _sendDataTotal += 46;//20 + 14 +12;// IP+UDP header
		}
    return n;
}

void
Channel::OnPlayTelephoneEvent(int32_t id,
                              uint8_t event,
                              uint16_t lengthMs,
                              uint8_t volume)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::OnPlayTelephoneEvent(id=%d, event=%u, lengthMs=%u,"
                 " volume=%u)", id, event, lengthMs, volume);

    if (!_playOutbandDtmfEvent || (event > 15))
    {
        // Ignore callback since feedback is disabled or event is not a
        // Dtmf tone event.
        return;
    }

    assert(_outputMixerPtr != NULL);

    // Start playing out the Dtmf tone (if playout is enabled).
    // Reduce length of tone with 80ms to the reduce risk of echo.
    _outputMixerPtr->PlayDtmfTone(event, lengthMs - 80, volume);
}

void
Channel::OnIncomingSSRCChanged(int32_t id, uint32_t ssrc)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::OnIncomingSSRCChanged(id=%d, SSRC=%d)",
                 id, ssrc);

    // Update ssrc so that NTP for AV sync can be updated.
    _rtpRtcpModule->SetRemoteSSRC(ssrc);
}

void Channel::OnIncomingCSRCChanged(int32_t id,
                                    uint32_t CSRC,
                                    bool added)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::OnIncomingCSRCChanged(id=%d, CSRC=%d, added=%d)",
                 id, CSRC, added);
}

void Channel::ResetStatistics(uint32_t ssrc) {
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(ssrc);
  if (statistician) {
    statistician->ResetStatistics();
  }
  statistics_proxy_->ResetStatistics();
}

int32_t
Channel::OnInitializeDecoder(
    int32_t id,
    int8_t payloadType,
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    int frequency,
    uint8_t channels,
    uint32_t rate)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::OnInitializeDecoder(id=%d, payloadType=%d, "
                 "payloadName=%s, frequency=%u, channels=%u, rate=%u)",
                 id, payloadType, payloadName, frequency, channels, rate);

    assert(VoEChannelId(id) == _channelId);

    CodecInst receiveCodec = {0};
    CodecInst dummyCodec = {0};

    receiveCodec.pltype = payloadType;
    receiveCodec.plfreq = frequency;
    receiveCodec.channels = channels;
    receiveCodec.rate = rate;
    strncpy(receiveCodec.plname, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);

    audio_coding_->Codec(payloadName, &dummyCodec, frequency, channels);
    receiveCodec.pacsize = dummyCodec.pacsize;

    // Register the new codec to the ACM
    if (audio_coding_->RegisterReceiveCodec(receiveCodec) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                     VoEId(_instanceId, _channelId),
                     "Channel::OnInitializeDecoder() invalid codec ("
                     "pt=%d, name=%s) received - 1", payloadType, payloadName);
        _engineStatisticsPtr->SetLastError(VE_AUDIO_CODING_MODULE_ERROR);
        return -1;
    }

    return 0;
}

int32_t
Channel::OnReceivedPayloadData(const uint8_t* payloadData,
                               size_t payloadSize,
                               const WebRtcRTPHeader* rtpHeader)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::OnReceivedPayloadData(payloadSize=%" PRIuS ","
                 " payloadType=%u, audioChannel=%u)",
                 payloadSize,
                 rtpHeader->header.payloadType,
                 rtpHeader->type.Audio.channel);

    if (!channel_state_.Get().playing)
    {
        // Avoid inserting into NetEQ when we are not playing. Count the
        // packet as discarded.
        WEBRTC_TRACE(kTraceStream, kTraceVoice,
                     VoEId(_instanceId, _channelId),
                     "received packet is discarded since playing is not"
                     " activated");
        _numberOfDiscardedPackets++;
        return 0;
    }

    // Push the incoming payload (parsed and ready for decoding) into the ACM
    if (audio_coding_->IncomingPacket(payloadData,
                                      payloadSize,
                                      *rtpHeader) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CODING_MODULE_ERROR, kTraceWarning,
            "Channel::OnReceivedPayloadData() unable to push data to the ACM");
        return -1;
    }

    // Update the packet delay.
    UpdatePacketDelay(rtpHeader->header.timestamp,
                      rtpHeader->header.sequenceNumber);

    int64_t round_trip_time = 0;
    _rtpRtcpModule->RTT(rtp_receiver_->SSRC(), &round_trip_time,
                        NULL, NULL, NULL);

    std::vector<uint16_t> nack_list = audio_coding_->GetNackList(
        round_trip_time);
    if (!nack_list.empty()) {
      // Can't use nack_list.data() since it's not supported by all
      // compilers.
      ResendPackets(&(nack_list[0]), static_cast<int>(nack_list.size()));
    }else //add by ylr��������ector�ڴ�й¶
	{
		nack_list.swap(nack_list);
	}
    return 0;
}

bool Channel::OnRecoveredPacket(const uint8_t* rtp_packet,
                                size_t rtp_packet_length) {
  RTPHeader header;
  if (!rtp_header_parser_->Parse(rtp_packet, rtp_packet_length, &header)) {
    WEBRTC_TRACE(kTraceDebug, cloopenwebrtc::kTraceVoice, _channelId,
                 "IncomingPacket invalid RTP header");
    return false;
  }
  header.payload_type_frequency =
      rtp_payload_registry_->GetPayloadTypeFrequency(header.payloadType);
  if (header.payload_type_frequency < 0)
    return false;
  return ReceivePacket(rtp_packet, rtp_packet_length, header, false);
}

int32_t Channel::GetAudioFrame(int32_t id, AudioFrame& audioFrame)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetAudioFrame(id=%d)", id);

    // Get 10ms raw PCM data from the ACM (mixer limits output frequency)
    if (audio_coding_->PlayoutData10Ms(audioFrame.sample_rate_hz_,
                                       &audioFrame) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "Channel::GetAudioFrame() PlayoutData10Ms() failed!");
        // In all likelihood, the audio in this frame is garbage. We return an
        // error so that the audio mixer module doesn't add it to the mix. As
        // a result, it won't be played out and the actions skipped here are
        // irrelevant.
        return -1;
    }

    if (_RxVadDetection)
    {
        UpdateRxVadDetection(audioFrame);
    }

    // Convert module ID to internal VoE channel ID
    audioFrame.id_ = VoEChannelId(audioFrame.id_);
    // Store speech type for dead-or-alive detection
    _outputSpeechType = audioFrame.speech_type_;

    ChannelState::State state = channel_state_.Get();

    if (state.rx_apm_is_enabled) {
      int err = rx_audioproc_->ProcessStream(&audioFrame);
      if (err) {
        LOG(LS_ERROR) << "ProcessStream() error: " << err;
        assert(false);
      }
    }

    float output_gain = 1.0f;
    float left_pan =  1.0f;
    float right_pan =  1.0f;
    {
      CriticalSectionScoped cs(&volume_settings_critsect_);
      output_gain = _outputGain;
      left_pan = _panLeft;
      right_pan= _panRight;
    }

    // Output volume scaling
    if (output_gain < 0.99f || output_gain > 1.01f)
    {
        AudioFrameOperations::ScaleWithSat(output_gain, audioFrame);
    }

    // Scale left and/or right channel(s) if stereo and master balance is
    // active

    if (left_pan != 1.0f || right_pan != 1.0f)
    {
        if (audioFrame.num_channels_ == 1)
        {
            // Emulate stereo mode since panning is active.
            // The mono signal is copied to both left and right channels here.
            AudioFrameOperations::MonoToStereo(&audioFrame);
        }
        // For true stereo mode (when we are receiving a stereo signal), no
        // action is needed.

        // Do the panning operation (the audio frame contains stereo at this
        // stage)
        AudioFrameOperations::Scale(left_pan, right_pan, audioFrame);
    }

    // Mix decoded PCM output with file if file mixing is enabled
    if (state.output_file_playing)
    {
        MixAudioWithFile(audioFrame, audioFrame.sample_rate_hz_);
    }

    // External media
    if (_outputExternalMedia)
    {
        CriticalSectionScoped cs(&_callbackCritSect);
        const bool isStereo = (audioFrame.num_channels_ == 2);
        if (_outputExternalMediaCallbackPtr)
        {
            _outputExternalMediaCallbackPtr->Process(
                _channelId,
                kPlaybackPerChannel,
                (int16_t*)audioFrame.data_,
                audioFrame.samples_per_channel_,
                audioFrame.sample_rate_hz_,
                isStereo);
        }
    }

    // Record playout if enabled
    {
        CriticalSectionScoped cs(&_fileCritSect);

        if (_outputFileRecording && _outputFileRecorderPtr)
        {
            _outputFileRecorderPtr->RecordAudioToFile(audioFrame);
        }
    }

    // Measure audio level (0-9)
    _outputAudioLevel.ComputeLevel(audioFrame);

    if (capture_start_rtp_time_stamp_ < 0 && audioFrame.timestamp_ != 0) {
      // The first frame with a valid rtp timestamp.
      capture_start_rtp_time_stamp_ = audioFrame.timestamp_;
    }

    if (capture_start_rtp_time_stamp_ >= 0) {
      // audioFrame.timestamp_ should be valid from now on.

      // Compute elapsed time.
      int64_t unwrap_timestamp =
          rtp_ts_wraparound_handler_->Unwrap(audioFrame.timestamp_);
      audioFrame.elapsed_time_ms_ =
          (unwrap_timestamp - capture_start_rtp_time_stamp_) /
          (GetPlayoutFrequency() / 1000);

      {
        CriticalSectionScoped lock(ts_stats_lock_.get());
        // Compute ntp time.
        audioFrame.ntp_time_ms_ = ntp_estimator_.Estimate(
            audioFrame.timestamp_);
        // |ntp_time_ms_| won't be valid until at least 2 RTCP SRs are received.
        if (audioFrame.ntp_time_ms_ > 0) {
          // Compute |capture_start_ntp_time_ms_| so that
          // |capture_start_ntp_time_ms_| + |elapsed_time_ms_| == |ntp_time_ms_|
          capture_start_ntp_time_ms_ =
              audioFrame.ntp_time_ms_ - audioFrame.elapsed_time_ms_;
        }
      }
    }

    return 0;
}

int32_t
Channel::NeededFrequency(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::NeededFrequency(id=%d)", id);

    int highestNeeded = 0;

    // Determine highest needed receive frequency
    int32_t receiveFrequency = audio_coding_->ReceiveFrequency();

    // Return the bigger of playout and receive frequency in the ACM.
    if (audio_coding_->PlayoutFrequency() > receiveFrequency)
    {
        highestNeeded = audio_coding_->PlayoutFrequency();
    }
    else
    {
        highestNeeded = receiveFrequency;
    }

    // Special case, if we're playing a file on the playout side
    // we take that frequency into consideration as well
    // This is not needed on sending side, since the codec will
    // limit the spectrum anyway.
    if (channel_state_.Get().output_file_playing)
    {
        CriticalSectionScoped cs(&_fileCritSect);
        if (_outputFilePlayerPtr)
        {
            if(_outputFilePlayerPtr->Frequency()>highestNeeded)
            {
                highestNeeded=_outputFilePlayerPtr->Frequency();
            }
        }
    }

    return(highestNeeded);
}

int32_t
Channel::CreateChannel(Channel*& channel,
                       int32_t channelId,
                       uint32_t instanceId,
                       const Config& config)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(instanceId,channelId),
                 "Channel::CreateChannel(channelId=%d, instanceId=%d)",
        channelId, instanceId);

    channel = new Channel(channelId, instanceId, config);
    if (channel == NULL)
    {
        WEBRTC_TRACE(kTraceMemory, kTraceVoice,
                     VoEId(instanceId,channelId),
                     "Channel::CreateChannel() unable to allocate memory for"
                     " channel");
        return -1;
    }
    return 0;
}

void
Channel::PlayNotification(int32_t id, uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::PlayNotification(id=%d, durationMs=%d)",
                 id, durationMs);

    // Not implement yet
}

void
Channel::RecordNotification(int32_t id, uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::RecordNotification(id=%d, durationMs=%d)",
                 id, durationMs);

    // Not implement yet
}

void
Channel::PlayFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::PlayFileEnded(id=%d)", id);

    if (id == _inputFilePlayerId)
    {
        channel_state_.SetInputFilePlaying(false);
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "Channel::PlayFileEnded() => input file player module is"
                     " shutdown");
    }
    else if (id == _outputFilePlayerId)
    {
        channel_state_.SetOutputFilePlaying(false);
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "Channel::PlayFileEnded() => output file player module is"
                     " shutdown");
    }
}

void
Channel::RecordFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::RecordFileEnded(id=%d)", id);

    assert(id == _outputFileRecorderId);

    CriticalSectionScoped cs(&_fileCritSect);

    _outputFileRecording = false;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                 VoEId(_instanceId,_channelId),
                 "Channel::RecordFileEnded() => output file recorder module is"
                 " shutdown");
}

Channel::Channel(int32_t channelId,
                 uint32_t instanceId,
                 const Config& config) :
    _fileCritSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _callbackCritSect(*CriticalSectionWrapper::CreateCriticalSection()),
    volume_settings_critsect_(*CriticalSectionWrapper::CreateCriticalSection()),
    _instanceId(instanceId),
    _channelId(channelId),
    rtp_header_parser_(RtpHeaderParser::Create()),
    rtp_payload_registry_(
        new RTPPayloadRegistry(RTPPayloadStrategy::CreateStrategy(true))),
    rtp_receive_statistics_(ReceiveStatistics::Create(
        Clock::GetRealTimeClock())),
    rtp_receiver_(RtpReceiver::CreateAudioReceiver(
        VoEModuleId(instanceId, channelId), Clock::GetRealTimeClock(), this,
        this, this, rtp_payload_registry_.get())),
    telephone_event_handler_(rtp_receiver_->GetTelephoneEventHandler()),
    audio_coding_(AudioCodingModule::Create(
        VoEModuleId(instanceId, channelId))),
    _rtpDumpIn(*RtpDump::CreateRtpDump()),
    _rtpDumpOut(*RtpDump::CreateRtpDump()),
    _outputAudioLevel(),
    _externalTransport(false),
    _inputFilePlayerPtr(NULL),
    _outputFilePlayerPtr(NULL),
    _outputFileRecorderPtr(NULL),
    // Avoid conflict with other channels by adding 1024 - 1026,
    // won't use as much as 1024 channels.
    _inputFilePlayerId(VoEModuleId(instanceId, channelId) + 1024),
    _outputFilePlayerId(VoEModuleId(instanceId, channelId) + 1025),
    _outputFileRecorderId(VoEModuleId(instanceId, channelId) + 1026),
    _outputFileRecording(false),
    _inbandDtmfQueue(VoEModuleId(instanceId, channelId)),
    _inbandDtmfGenerator(VoEModuleId(instanceId, channelId)),
    _outputExternalMedia(false),
    _inputExternalMediaCallbackPtr(NULL),
    _outputExternalMediaCallbackPtr(NULL),
    _timeStamp(0), // This is just an offset, RTP module will add it's own random offset
    _sendTelephoneEventPayloadType(106),
	_recvTelephoneEventPayloadType(106),
    ntp_estimator_(Clock::GetRealTimeClock()),
    jitter_buffer_playout_timestamp_(0),
    playout_timestamp_rtp_(0),
    playout_timestamp_rtcp_(0),
    playout_delay_ms_(0),
    _numberOfDiscardedPackets(0),
    send_sequence_number_(0),
    ts_stats_lock_(CriticalSectionWrapper::CreateCriticalSection()),
    rtp_ts_wraparound_handler_(new TimestampWrapAroundHandler()),
    capture_start_rtp_time_stamp_(-1),
    capture_start_ntp_time_ms_(-1),
    _engineStatisticsPtr(NULL),
    _outputMixerPtr(NULL),
    _transmitMixerPtr(NULL),
    _moduleProcessThreadPtr(NULL),
    _audioDeviceModulePtr(NULL),
    _voiceEngineObserverPtr(NULL),
    _callbackCritSectPtr(NULL),
    _transportPtr(NULL),
    _rxVadObserverPtr(NULL),
    _oldVadDecision(-1),
    _sendFrameType(0),
    _externalMixing(false),
    _mixFileWithMicrophone(false),
    _mute(false),
    _panLeft(1.0f),
    _panRight(1.0f),
    _outputGain(1.0f),
    _playOutbandDtmfEvent(false),
    _playInbandDtmfEvent(false),
    _lastLocalTimeStamp(0),
    _lastPayloadType(0),
    _includeAudioLevelIndication(false),
    _outputSpeechType(AudioFrame::kNormalSpeech),
    vie_network_(NULL),
    video_channel_(-1),
    _average_jitter_buffer_delay_us(0),
    least_required_delay_ms_(0),
    _previousTimestamp(0),
    _recPacketDelayMs(20),
    _RxVadDetection(false),
    _rxAgcIsEnabled(false),
    _rxNsIsEnabled(false),
    restored_packet_in_use_(false),
    bitrate_controller_(
        BitrateController::CreateBitrateController(Clock::GetRealTimeClock(),
                                                   true)),
    rtcp_bandwidth_observer_(
        bitrate_controller_->CreateRtcpBandwidthObserver()),
    send_bitrate_observer_(new VoEBitrateObserver(this)),
    network_predictor_(new NetworkPredictor(Clock::GetRealTimeClock())),
	_encrypting(false),
	_decrypting(false),
	_encryptionPtr(NULL),
	_encryptionRTPBufferPtr(NULL),
	_decryptionRTPBufferPtr(NULL),
	_encryptionRTCPBufferPtr(NULL),
	_decryptionRTCPBufferPtr(NULL),
#ifdef WEBRTC_SRTP
	_srtpModule(*SrtpModule::CreateSrtpModule(VoEModuleId(instanceId, channelId))),
#endif
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	_numSocketThreads(KNumSocketThreads),
	_socketTransportModule(*UdpTransport::Create(VoEModuleId(instanceId, channelId), _numSocketThreads)),
#endif
	critsect_net_statistic(CriticalSectionWrapper::CreateCriticalSection()),
	_startNetworkTime(0),
	_sendDataTotalWifi(0),
	_recvDataTotalWifi(0),
	_sendDataTotalSim(0),
	_recvDataTotalSim(0),
	_isWifi(false),
	_pause(false),
    _loss(0),
    _ondtmf(false),
    _dtmf_cb(NULL),
    _media_timeout_cb(NULL),
    _stun_cb(NULL),
    _audio_data_cb(NULL),
    _rtpPacketTimedOut(false),
    _rtpPacketTimeOutIsEnabled(false),
    _rtpTimeOutSeconds(0),
	_processDataFlag(false),
    _sendData(NULL),
    _receiveData(NULL)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::Channel() - ctor");
    _inbandDtmfQueue.ResetDtmf();
    _inbandDtmfGenerator.Init();
    _outputAudioLevel.Clear();

    RtpRtcp::Configuration configuration;
    configuration.id = VoEModuleId(instanceId, channelId);
    configuration.audio = true;
    configuration.outgoing_transport = this;
    configuration.audio_messages = this;
    configuration.receive_statistics = rtp_receive_statistics_.get();
    configuration.bandwidth_callback = rtcp_bandwidth_observer_.get();

    _rtpRtcpModule.reset(RtpRtcp::CreateRtpRtcp(configuration));

    statistics_proxy_.reset(new StatisticsProxy(_rtpRtcpModule->SSRC()));
    rtp_receive_statistics_->RegisterRtcpStatisticsCallback(
        statistics_proxy_.get());

    Config audioproc_config;
    audioproc_config.Set<ExperimentalAgc>(new ExperimentalAgc(false));
	audioproc_config.Set<ExperimentalNs>(new ExperimentalNs(false));
	audioproc_config.Set<Beamforming>(new Beamforming());
	audioproc_config.Set<DelayCorrection>(new DelayCorrection());
	audioproc_config.Set<ReportedDelay>(new ReportedDelay());
    rx_audioproc_.reset(AudioProcessing::Create(audioproc_config));
}

Channel::~Channel()
{
    rtp_receive_statistics_->RegisterRtcpStatisticsCallback(NULL);
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::~Channel() - dtor");
    _rtpRtcpModule->DeRegisterRtpReceiver();
    if (_outputExternalMedia)
    {
        DeRegisterExternalMediaProcessing(kPlaybackPerChannel);
    }
    if (channel_state_.Get().input_external_media)
    {
        DeRegisterExternalMediaProcessing(kRecordingPerChannel);
    }
    StopSend();
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	StopReceiving();
	// De-register packet callback to ensure we're not in a callback when
	// deleting channel state, avoids race condition and deadlock.
	if (_socketTransportModule.InitializeReceiveSockets(NULL, 0, NULL, NULL, 0)
		!= 0)
	{
		WEBRTC_TRACE(kTraceWarning, kTraceVoice,
			VoEId(_instanceId, _channelId),
			"~Channel() failed to de-register receive callback");
	}
#endif
    StopPlayout();

    {
        CriticalSectionScoped cs(&_fileCritSect);
        if (_inputFilePlayerPtr)
        {
            _inputFilePlayerPtr->RegisterModuleFileCallback(NULL);
            _inputFilePlayerPtr->StopPlayingFile();
            FilePlayer::DestroyFilePlayer(_inputFilePlayerPtr);
            _inputFilePlayerPtr = NULL;
        }
        if (_outputFilePlayerPtr)
        {
            _outputFilePlayerPtr->RegisterModuleFileCallback(NULL);
            _outputFilePlayerPtr->StopPlayingFile();
            FilePlayer::DestroyFilePlayer(_outputFilePlayerPtr);
            _outputFilePlayerPtr = NULL;
        }
        if (_outputFileRecorderPtr)
        {
            _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
            _outputFileRecorderPtr->StopRecording();
            FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
            _outputFileRecorderPtr = NULL;
        }
    }

    // The order to safely shutdown modules in a channel is:
    // 1. De-register callbacks in modules
    // 2. De-register modules in process thread
    // 3. Destroy modules
    if (audio_coding_->RegisterTransportCallback(NULL) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "~Channel() failed to de-register transport callback"
                     " (Audio coding module)");
    }
    if (audio_coding_->RegisterVADCallback(NULL) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "~Channel() failed to de-register VAD callback"
                     " (Audio coding module)");
    }
    // De-register modules in process thread
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	if (_moduleProcessThreadPtr->DeRegisterModule(&_socketTransportModule)
		== -1)
	{
		WEBRTC_TRACE(kTraceInfo, kTraceVoice,
			VoEId(_instanceId,_channelId),
			"~Channel() failed to deregister socket module");
	}
#endif
    if (_moduleProcessThreadPtr->DeRegisterModule(_rtpRtcpModule.get()) == -1)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "~Channel() failed to deregister RTP/RTCP module");
    }

	if (_moduleProcessThreadPtr->DeRegisterModule(audio_coding_.get()) == -1)
	{
		WEBRTC_TRACE(kTraceInfo, kTraceVoice,
			VoEId(_instanceId,_channelId),
			"~Channel() failed to deregister RTP/RTCP module");
	}
    // End of modules shutdown

    // Delete other objects

#ifndef WEBRTC_EXTERNAL_TRANSPORT
	UdpTransport::Destroy(
		&_socketTransportModule);
#endif
#ifdef WEBRTC_SRTP
	SrtpModule::DestroySrtpModule(&_srtpModule);
#endif
    if (vie_network_) {
      vie_network_->Release();
      vie_network_ = NULL;
    }
    RtpDump::DestroyRtpDump(&_rtpDumpIn);
    RtpDump::DestroyRtpDump(&_rtpDumpOut);
    delete &_callbackCritSect;
    delete &_fileCritSect;
    delete &volume_settings_critsect_;
    
    if (this->_sendData) {
        free(_sendData);
        _sendData = NULL;
    }
    if (this->_receiveData) {
        free(_receiveData);
        _receiveData = NULL;
    }
}

int32_t
Channel::Init()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::Init()");

    channel_state_.Reset();

    // --- Initial sanity

    if ((_engineStatisticsPtr == NULL) ||
        (_moduleProcessThreadPtr == NULL))
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice,
                     VoEId(_instanceId,_channelId),
                     "Channel::Init() must call SetEngineInformation() first");
        return -1;
    }



    if ((audio_coding_->InitializeReceiver() == -1) ||
#ifdef WEBRTC_CODEC_AVT
        // out-of-band Dtmf tones are played out by default
        (audio_coding_->SetDtmfPlayoutStatus(true) == -1) ||
#endif
        (audio_coding_->InitializeSender() == -1))
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
            "Channel::Init() unable to initialize the ACM - 1");
        return -1;
    }

    // --- RTP/RTCP module initialization

    // Ensure that RTCP is enabled by default for the created channel.
    // Note that, the module will keep generating RTCP until it is explicitly
    // disabled by the user.
    // After StopListen (when no sockets exists), RTCP packets will no longer
    // be transmitted since the Transport object will then be invalid.
    telephone_event_handler_->SetTelephoneEventForwardToDecoder(true);
    // RTCP is enabled by default.
    _rtpRtcpModule->SetRTCPStatus(kRtcpCompound);
    // --- Register all permanent callbacks
    const bool fail =
        (audio_coding_->RegisterTransportCallback(this) == -1) ||
        (audio_coding_->RegisterVADCallback(this) == -1);

    if (fail)
    {
        _engineStatisticsPtr->SetLastError(
            VE_CANNOT_INIT_CHANNEL, kTraceError,
            "Channel::Init() callbacks not registered");
        return -1;
    }

    // --- Register all supported codecs to the receiving side of the
    // RTP/RTCP module

    CodecInst codec;
    const uint8_t nSupportedCodecs = AudioCodingModule::NumberOfCodecs();

    for (int idx = 0; idx < nSupportedCodecs; idx++)
    {
        // Open up the RTP/RTCP receiver for all supported codecs
        if ((audio_coding_->Codec(idx, &codec) == -1) ||
            (rtp_receiver_->RegisterReceivePayload(
                codec.plname,
                codec.pltype,
                codec.plfreq,
                codec.channels,
                (codec.rate < 0) ? 0 : codec.rate) == -1))
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_instanceId,_channelId),
                         "Channel::Init() unable to register %s (%d/%d/%d/%d) "
                         "to RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }
        else
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                         VoEId(_instanceId,_channelId),
                         "Channel::Init() %s (%d/%d/%d/%d) has been added to "
                         "the RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }

        // Ensure that PCMU is used as default codec on the sending side
        if (!STR_CASE_CMP(codec.plname, "PCMU") && (codec.channels == 1))
        {
            SetSendCodec(codec);
        }

        // Register default PT for outband 'telephone-event'
        if (!STR_CASE_CMP(codec.plname, "telephone-event"))
        {
            if ((_rtpRtcpModule->RegisterSendPayload(codec) == -1) ||
                (audio_coding_->RegisterReceiveCodec(codec) == -1))
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                             VoEId(_instanceId,_channelId),
                             "Channel::Init() failed to register outband "
                             "'telephone-event' (%d/%d) correctly",
                             codec.pltype, codec.plfreq);
            }
        }

        if (!STR_CASE_CMP(codec.plname, "CN"))
        {
            if ((audio_coding_->RegisterSendCodec(codec) == -1) ||
                (audio_coding_->RegisterReceiveCodec(codec) == -1) ||
                (_rtpRtcpModule->RegisterSendPayload(codec) == -1))
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                             VoEId(_instanceId,_channelId),
                             "Channel::Init() failed to register CN (%d/%d) "
                             "correctly - 1",
                             codec.pltype, codec.plfreq);
            }
        }
#ifdef WEBRTC_CODEC_RED
        // Register RED to the receiving side of the ACM.
        // We will not receive an OnInitializeDecoder() callback for RED.
        if (!STR_CASE_CMP(codec.plname, "RED"))
        {
            if (audio_coding_->RegisterReceiveCodec(codec) == -1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                             VoEId(_instanceId,_channelId),
                             "Channel::Init() failed to register RED (%d/%d) "
                             "correctly",
                             codec.pltype, codec.plfreq);
            }
        }
#endif
    }

#ifndef WEBRTC_EXTERNAL_TRANSPORT
	// Ensure that the WebRtcSocketTransport implementation is used as
	// Transport on the sending side
	{
		// A lock is needed here since users can call
		// RegisterExternalTransport() at the same time.
		CriticalSectionScoped cs(&_callbackCritSect);
		_transportPtr = &_socketTransportModule;
	}
#endif

	// --- Add modules to process thread (for periodic schedulation)
		const bool processThreadFail =
		((_moduleProcessThreadPtr->RegisterModule(_rtpRtcpModule.get()) != 0) ||
		(_moduleProcessThreadPtr->RegisterModule(audio_coding_.get()) != 0) ||
#ifndef WEBRTC_EXTERNAL_TRANSPORT
		(_moduleProcessThreadPtr->RegisterModule(
			&_socketTransportModule) != 0));
#else
		false);
#endif

	if (processThreadFail)
	{
		_engineStatisticsPtr->SetLastError(
			VE_CANNOT_INIT_CHANNEL, kTraceError,
			"Channel::Init() modules not registered");
		return -1;
	}
	// --- ACM initialization

    if (rx_audioproc_->noise_suppression()->set_level(kDefaultNsMode) != 0) {
      LOG_FERR1(LS_ERROR, noise_suppression()->set_level, kDefaultNsMode);
      return -1;
    }
    if (rx_audioproc_->gain_control()->set_mode(kDefaultRxAgcMode) != 0) {
      LOG_FERR1(LS_ERROR, gain_control()->set_mode, kDefaultRxAgcMode);
      return -1;
    }

    if (rtp_receiver_) {
        _rtpRtcpModule->RegisterRtpReceiver(rtp_receiver_.get());
    }
    return 0;
}

int32_t
Channel::SetEngineInformation(Statistics& engineStatistics,
                              OutputMixer& outputMixer,
                              voe::TransmitMixer& transmitMixer,
                              ProcessThread& moduleProcessThread,
                              AudioDeviceModule& audioDeviceModule,
                              VoiceEngineObserver* voiceEngineObserver,
                              CriticalSectionWrapper* callbackCritSect)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetEngineInformation()");
    _engineStatisticsPtr = &engineStatistics;
    _outputMixerPtr = &outputMixer;
    _transmitMixerPtr = &transmitMixer,
    _moduleProcessThreadPtr = &moduleProcessThread;
    _audioDeviceModulePtr = &audioDeviceModule;
    _voiceEngineObserverPtr = voiceEngineObserver;
    _callbackCritSectPtr = callbackCritSect;
    return 0;
}

int32_t
Channel::UpdateLocalTimeStamp()
{

    _timeStamp += _audioFrame.samples_per_channel_;
    return 0;
}

int32_t
Channel::StartPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartPlayout()");
    if (channel_state_.Get().playing)
    {
        return 0;
    }

    if (!_externalMixing) {
        // Add participant as candidates for mixing.
        if (_outputMixerPtr->SetMixabilityStatus(*this, true) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_AUDIO_CONF_MIX_MODULE_ERROR, kTraceError,
                "StartPlayout() failed to add participant to mixer");
            return -1;
        }
    }

    channel_state_.SetPlaying(true);
    if (RegisterFilePlayingToMixer() != 0)
        return -1;

    return 0;
}

int32_t
Channel::StopPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StopPlayout()");
    if (!channel_state_.Get().playing)
    {
        return 0;
    }

    if (!_externalMixing) {
        // Remove participant as candidates for mixing
        if (_outputMixerPtr->SetMixabilityStatus(*this, false) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_AUDIO_CONF_MIX_MODULE_ERROR, kTraceError,
                "StopPlayout() failed to remove participant from mixer");
            return -1;
        }
    }

    channel_state_.SetPlaying(false);
    _outputAudioLevel.Clear();

    return 0;
}

int32_t
Channel::StartSend()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartSend()");
    // Resume the previous sequence number which was reset by StopSend().
    // This needs to be done before |sending| is set to true.
    if (send_sequence_number_)
      SetInitSequenceNumber(send_sequence_number_);

    if (channel_state_.Get().sending)
    {
      return 0;
    }
    channel_state_.SetSending(true);

    if (_rtpRtcpModule->SetSendingStatus(true) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceError,
            "StartSend() RTP/RTCP failed to start sending");
        CriticalSectionScoped cs(&_callbackCritSect);
        channel_state_.SetSending(false);
        return -1;
    }

    return 0;
}

int32_t
Channel::StopSend()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StopSend()");
    if (!channel_state_.Get().sending)
    {
      return 0;
    }
    channel_state_.SetSending(false);

    // Store the sequence number to be able to pick up the same sequence for
    // the next StartSend(). This is needed for restarting device, otherwise
    // it might cause libSRTP to complain about packets being replayed.
    // TODO(xians): Remove this workaround after RtpRtcpModule's refactoring
    // CL is landed. See issue
    // https://code.google.com/p/webrtc/issues/detail?id=2111 .
    send_sequence_number_ = _rtpRtcpModule->SequenceNumber();

    // Reset sending SSRC and sequence number and triggers direct transmission
    // of RTCP BYE
    if (_rtpRtcpModule->SetSendingStatus(false) == -1 ||
        _rtpRtcpModule->ResetSendDataCountersRTP() == -1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceWarning,
            "StartSend() RTP/RTCP failed to stop sending");
    }

    return 0;
}

int32_t
Channel::StartReceiving()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartReceiving()");
    if (channel_state_.Get().receiving)
    {
        return 0;
    }
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	if (!_externalTransport)
	{
		if (!_socketTransportModule.ReceiveSocketsInitialized())
		{
			_engineStatisticsPtr->SetLastError(
				VE_SOCKETS_NOT_INITED, kTraceError,
				"StartReceive() must set local receiver first");
			return -1;
		}
		if (_socketTransportModule.StartReceiving(KNumberOfSocketBuffers) != 0)
		{
			_engineStatisticsPtr->SetLastError(
				VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceError,
				"StartReceiving() failed to start receiving");
			return -1;
		}
	}
#endif
    channel_state_.SetReceiving(true);
    _numberOfDiscardedPackets = 0;
    return 0;
}

int32_t
Channel::StopReceiving()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StopReceiving()");
    if (!channel_state_.Get().receiving)
    {
        return 0;
    }
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	if (!_externalTransport &&
		_socketTransportModule.ReceiveSocketsInitialized())
	{
		if (_socketTransportModule.StopReceiving() != 0)
		{
			_engineStatisticsPtr->SetLastError(
				VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceError,
				"StopReceiving() failed to stop receiving.");
			return -1;
		}
	}
#endif
    channel_state_.SetReceiving(false);
    return 0;
}

int32_t
Channel::RegisterVoiceEngineObserver(VoiceEngineObserver& observer)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::RegisterVoiceEngineObserver()");
    CriticalSectionScoped cs(&_callbackCritSect);

    if (_voiceEngineObserverPtr)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceError,
            "RegisterVoiceEngineObserver() observer already enabled");
        return -1;
    }
    _voiceEngineObserverPtr = &observer;
    return 0;
}

int32_t
Channel::DeRegisterVoiceEngineObserver()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::DeRegisterVoiceEngineObserver()");
    CriticalSectionScoped cs(&_callbackCritSect);

    if (!_voiceEngineObserverPtr)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceWarning,
            "DeRegisterVoiceEngineObserver() observer already disabled");
        return 0;
    }
    _voiceEngineObserverPtr = NULL;
    return 0;
}

int32_t
Channel::GetSendCodec(CodecInst& codec)
{
    return (audio_coding_->SendCodec(&codec));
}

int32_t
Channel::GetRecCodec(CodecInst& codec)
{
    return (audio_coding_->ReceiveCodec(&codec));
}

int32_t
Channel::SetSendCodec(const CodecInst& codec)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetSendCodec()");

    if (audio_coding_->RegisterSendCodec(codec) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,_channelId),
                     "SetSendCodec() failed to register codec to ACM");
        return -1;
    }

    if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
    {
        _rtpRtcpModule->DeRegisterSendPayload(codec.pltype);
        if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
        {
            WEBRTC_TRACE(
                    kTraceError, kTraceVoice, VoEId(_instanceId,_channelId),
                    "SetSendCodec() failed to register codec to"
                    " RTP/RTCP module");
            return -1;
        }
    }

    if (_rtpRtcpModule->SetAudioPacketSize(codec.pacsize) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,_channelId),
                     "SetSendCodec() failed to set audio packet size");
        return -1;
    }

    bitrate_controller_->SetBitrateObserver(send_bitrate_observer_.get(),
                                            codec.rate, 0, 0);

    return 0;
}

void
Channel::OnNetworkChanged(const uint32_t bitrate_bps,
                          const uint8_t fraction_lost,  // 0 - 255.
                          const int64_t rtt) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
      "Channel::OnNetworkChanged(bitrate_bps=%d, fration_lost=%d, rtt=%" PRId64
      ")", bitrate_bps, fraction_lost, rtt);
  // |fraction_lost| from BitrateObserver is short time observation of packet
  // loss rate from past. We use network predictor to make a more reasonable
  // loss rate estimation.
  network_predictor_->UpdatePacketLossRate(fraction_lost);
  uint8_t loss_rate = network_predictor_->GetLossRate();
  // Normalizes rate to 0 - 100.
  if (audio_coding_->SetPacketLossRate(100 * loss_rate / 255) != 0) {
    _engineStatisticsPtr->SetLastError(VE_AUDIO_CODING_MODULE_ERROR,
        kTraceError, "OnNetworkChanged() failed to set packet loss rate");
    assert(false);  // This should not happen.
  }
}

int32_t
Channel::SetVADStatus(bool enableVAD, ACMVADMode mode, bool disableDTX)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetVADStatus(mode=%d)", mode);
    // To disable VAD, DTX must be disabled too
    disableDTX = ((enableVAD == false) ? true : disableDTX);
    if (audio_coding_->SetVAD(!disableDTX, enableVAD, mode) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
            "SetVADStatus() failed to set VAD");
        return -1;
    }
    return 0;
}

int32_t
Channel::GetVADStatus(bool& enabledVAD, ACMVADMode& mode, bool& disabledDTX)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetVADStatus");
    if (audio_coding_->VAD(&disabledDTX, &enabledVAD, &mode) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
            "GetVADStatus() failed to get VAD status");
        return -1;
    }
    disabledDTX = !disabledDTX;
    return 0;
}

int32_t
Channel::SetRecPayloadType(const CodecInst& codec)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetRecPayloadType()");

    if (channel_state_.Get().playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_PLAYING, kTraceError,
            "SetRecPayloadType() unable to set PT while playing");
        return -1;
    }
    if (channel_state_.Get().receiving)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_LISTENING, kTraceError,
            "SetRecPayloadType() unable to set PT while listening");
        return -1;
    }

    if (codec.pltype == -1)
    {
        // De-register the selected codec (RTP/RTCP module and ACM)

        int8_t pltype(-1);
        CodecInst rxCodec = codec;

        // Get payload type for the given codec
        rtp_payload_registry_->ReceivePayloadType(
            rxCodec.plname,
            rxCodec.plfreq,
            rxCodec.channels,
            (rxCodec.rate < 0) ? 0 : rxCodec.rate,
            &pltype);
        rxCodec.pltype = pltype;

        if (rtp_receiver_->DeRegisterReceivePayload(pltype) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                    VE_RTP_RTCP_MODULE_ERROR,
                    kTraceError,
                    "SetRecPayloadType() RTP/RTCP-module deregistration "
                    "failed");
            return -1;
        }
        if (audio_coding_->UnregisterReceiveCodec(rxCodec.pltype) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
                "SetRecPayloadType() ACM deregistration failed - 1");
            return -1;
        }
        return 0;
    }

    if (rtp_receiver_->RegisterReceivePayload(
        codec.plname,
        codec.pltype,
        codec.plfreq,
        codec.channels,
        (codec.rate < 0) ? 0 : codec.rate) != 0)
    {
        // First attempt to register failed => de-register and try again
        rtp_receiver_->DeRegisterReceivePayload(codec.pltype);
        if (rtp_receiver_->RegisterReceivePayload(
            codec.plname,
            codec.pltype,
            codec.plfreq,
            codec.channels,
            (codec.rate < 0) ? 0 : codec.rate) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_RTP_RTCP_MODULE_ERROR, kTraceError,
                "SetRecPayloadType() RTP/RTCP-module registration failed");
            return -1;
        }
    }
    if (audio_coding_->RegisterReceiveCodec(codec) != 0)
    {
        audio_coding_->UnregisterReceiveCodec(codec.pltype);
        if (audio_coding_->RegisterReceiveCodec(codec) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
                "SetRecPayloadType() ACM registration failed - 1");
            return -1;
        }
    }
    return 0;
}

int32_t
Channel::GetRecPayloadType(CodecInst& codec)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetRecPayloadType()");
    int8_t payloadType(-1);
    if (rtp_payload_registry_->ReceivePayloadType(
        codec.plname,
        codec.plfreq,
        codec.channels,
        (codec.rate < 0) ? 0 : codec.rate,
        &payloadType) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceWarning,
            "GetRecPayloadType() failed to retrieve RX payload type");
        return -1;
    }
    codec.pltype = payloadType;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetRecPayloadType() => pltype=%u", codec.pltype);
    return 0;
}

int32_t
Channel::SetSendCNPayloadType(int type, PayloadFrequencies frequency)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetSendCNPayloadType()");

    CodecInst codec;
    int32_t samplingFreqHz(-1);
    const int kMono = 1;
    if (frequency == kFreq32000Hz)
        samplingFreqHz = 32000;
    else if (frequency == kFreq16000Hz)
        samplingFreqHz = 16000;

    if (audio_coding_->Codec("CN", &codec, samplingFreqHz, kMono) == -1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
            "SetSendCNPayloadType() failed to retrieve default CN codec "
            "settings");
        return -1;
    }

    // Modify the payload type (must be set to dynamic range)
    codec.pltype = type;

    if (audio_coding_->RegisterSendCodec(codec) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
            "SetSendCNPayloadType() failed to register CN to ACM");
        return -1;
    }

    if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
    {
        _rtpRtcpModule->DeRegisterSendPayload(codec.pltype);
        if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_RTP_RTCP_MODULE_ERROR, kTraceError,
                "SetSendCNPayloadType() failed to register CN to RTP/RTCP "
                "module");
            return -1;
        }
    }
    return 0;
}

int Channel::SetOpusMaxPlaybackRate(int frequency_hz) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SetOpusMaxPlaybackRate()");

  if (audio_coding_->SetOpusMaxPlaybackRate(frequency_hz) != 0) {
    _engineStatisticsPtr->SetLastError(
        VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
        "SetOpusMaxPlaybackRate() failed to set maximum playback rate");
    return -1;
  }
  return 0;
}

int32_t Channel::RegisterExternalTransport(Transport& transport)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::RegisterExternalTransport()");

    CriticalSectionScoped cs(&_callbackCritSect);
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	// Sanity checks for default (non external transport) to avoid conflict with
	// WebRtc sockets.
	if (_socketTransportModule.SendSocketsInitialized())
	{
		_engineStatisticsPtr->SetLastError(VE_SEND_SOCKETS_CONFLICT,
			kTraceError,
			"RegisterExternalTransport() send sockets already initialized");
		return -1;
	}
	if (_socketTransportModule.ReceiveSocketsInitialized())
	{
		_engineStatisticsPtr->SetLastError(VE_RECEIVE_SOCKETS_CONFLICT,
			kTraceError,
			"RegisterExternalTransport() receive sockets already initialized");
		return -1;
	}
#endif
    if (_externalTransport)
    {
        _engineStatisticsPtr->SetLastError(VE_INVALID_OPERATION,
                                           kTraceError,
              "RegisterExternalTransport() external transport already enabled");
       return -1;
    }
    _externalTransport = true;
    _transportPtr = &transport;
    return 0;
}

int32_t
Channel::DeRegisterExternalTransport()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::DeRegisterExternalTransport()");

    CriticalSectionScoped cs(&_callbackCritSect);

    if (!_transportPtr)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceWarning,
            "DeRegisterExternalTransport() external transport already "
            "disabled");
        return 0;
    }
    _externalTransport = false;
#ifdef WEBRTC_EXTERNAL_TRANSPORT
	_transportPtr = NULL;
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"DeRegisterExternalTransport() all transport is disabled");
#else
	_transportPtr = &_socketTransportModule;
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"DeRegisterExternalTransport() internal Transport is enabled");
#endif
    //_transportPtr = NULL;
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "DeRegisterExternalTransport() all transport is disabled");
    return 0;
}

int32_t Channel::RegisterExternalPacketization(AudioPacketizationCallback* transport)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
		"Channel::RegisterExternalPacketization()");

	audio_coding_->RegisterTransportCallback(transport);
	return 0;
}

int32_t Channel::DeRegisterExternalPacketization()
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
		"Channel::RegisterExternalPacketization()");

	audio_coding_->RegisterTransportCallback(NULL);
	return 0;
}
int32_t Channel::ReceivedRTPPacket(const int8_t* data, size_t length,
                                   const PacketTime& packet_time) {
  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::ReceivedRTPPacket()");

  // Store playout timestamp for the received RTP packet
  UpdatePlayoutTimestamp(false);

  //// Dump the RTP packet to a file (if RTP dump is enabled).
  if (_rtpDumpIn.DumpPacket((const uint8_t*)data,
                            (uint16_t)length) == -1) {
    WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                 VoEId(_instanceId,_channelId),
                 "Channel::SendPacket() RTP dump to input file failed");
  }
  const uint8_t* received_packet = reinterpret_cast<const uint8_t*>(data);
  RTPHeader header;
  if (!rtp_header_parser_->Parse(received_packet, length, &header)) {
    WEBRTC_TRACE(cloopenwebrtc::kTraceDebug, cloopenwebrtc::kTraceVoice, _channelId,
                 "Incoming packet: invalid RTP header");
    return -1;
  }
  header.payload_type_frequency =
      rtp_payload_registry_->GetPayloadTypeFrequency(header.payloadType);
  if (header.payload_type_frequency < 0)
    return -1;
  bool in_order = IsPacketInOrder(header);
  rtp_receive_statistics_->IncomingPacket(header, length,
      IsPacketRetransmitted(header, in_order));
  rtp_payload_registry_->SetIncomingPayloadType(header);

  // Forward any packets to ViE bandwidth estimator, if enabled.
  {
    CriticalSectionScoped cs(&_callbackCritSect);
    if (vie_network_) {
      int64_t arrival_time_ms;
      if (packet_time.timestamp != -1) {
        arrival_time_ms = (packet_time.timestamp + 500) / 1000;
      } else {
        arrival_time_ms = TickTime::MillisecondTimestamp();
      }
      size_t payload_length = length - header.headerLength;
      vie_network_->ReceivedBWEPacket(video_channel_, arrival_time_ms,
                                      payload_length, header);
    }
  }

  return ReceivePacket(received_packet, length, header, in_order) ? 0 : -1;
}

bool Channel::ReceivePacket(const uint8_t* packet,
                            size_t packet_length,
                            const RTPHeader& header,
                            bool in_order) {
  if (rtp_payload_registry_->IsEncapsulated(header)) {
    return HandleEncapsulation(packet, packet_length, header);
  }
  const uint8_t* payload = packet + header.headerLength;
  assert(packet_length >= header.headerLength);
  size_t payload_length = packet_length - header.headerLength;
  PayloadUnion payload_specific;
  if (!rtp_payload_registry_->GetPayloadSpecifics(header.payloadType,
                                                  &payload_specific)) {
    return false;
  }
  return rtp_receiver_->IncomingRtpPacket(header, payload, payload_length,
                                          payload_specific, in_order);
}

bool Channel::HandleEncapsulation(const uint8_t* packet,
                                  size_t packet_length,
                                  const RTPHeader& header) {
  if (!rtp_payload_registry_->IsRtx(header))
    return false;

  // Remove the RTX header and parse the original RTP header.
  if (packet_length < header.headerLength)
    return false;
  if (packet_length > kVoiceEngineMaxIpPacketSizeBytes)
    return false;
  if (restored_packet_in_use_) {
    WEBRTC_TRACE(cloopenwebrtc::kTraceDebug, cloopenwebrtc::kTraceVoice, _channelId,
                 "Multiple RTX headers detected, dropping packet");
    return false;
  }
  uint8_t* restored_packet_ptr = restored_packet_;
  if (!rtp_payload_registry_->RestoreOriginalPacket(
      &restored_packet_ptr, packet, &packet_length, rtp_receiver_->SSRC(),
      header)) {
    WEBRTC_TRACE(cloopenwebrtc::kTraceDebug, cloopenwebrtc::kTraceVoice, _channelId,
                 "Incoming RTX packet: invalid RTP header");
    return false;
  }
  restored_packet_in_use_ = true;
  bool ret = OnRecoveredPacket(restored_packet_ptr, packet_length);
  restored_packet_in_use_ = false;
  return ret;
}

bool Channel::IsPacketInOrder(const RTPHeader& header) const {
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  return statistician->IsPacketInOrder(header.sequenceNumber);
}

bool Channel::IsPacketRetransmitted(const RTPHeader& header,
                                    bool in_order) const {
  // Retransmissions are handled separately if RTX is enabled.
  if (rtp_payload_registry_->RtxEnabled())
    return false;
  StreamStatistician* statistician =
      rtp_receive_statistics_->GetStatistician(header.ssrc);
  if (!statistician)
    return false;
  // Check if this is a retransmission.
  int64_t min_rtt = 0;
  _rtpRtcpModule->RTT(rtp_receiver_->SSRC(), NULL, NULL, &min_rtt, NULL);
  return !in_order &&
      statistician->IsRetransmitOfOldPacket(header, min_rtt);
}

int32_t Channel::ReceivedRTCPPacket(const int8_t* data, size_t length) {
  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::ReceivedRTCPPacket()");
  // Store playout timestamp for the received RTCP packet
  UpdatePlayoutTimestamp(true);

  //// Dump the RTCP packet to a file (if RTP dump is enabled).
  if (_rtpDumpIn.DumpPacket((const uint8_t*)data, length) == -1) {
    WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                 VoEId(_instanceId,_channelId),
                 "Channel::SendPacket() RTCP dump to input file failed");
  }

  // Deliver RTCP packet to RTP/RTCP module for parsing
  if (_rtpRtcpModule->IncomingRtcpPacket((const uint8_t*)data, length) == -1) {
    _engineStatisticsPtr->SetLastError(
        VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceWarning,
        "Channel::IncomingRTPPacket() RTCP packet is invalid");
  }

  {
    CriticalSectionScoped lock(ts_stats_lock_.get());
    int64_t rtt = GetRTT();
    if (rtt == 0) {
      // Waiting for valid RTT.
      return 0;
    }
    uint32_t ntp_secs = 0;
    uint32_t ntp_frac = 0;
    uint32_t rtp_timestamp = 0;
    if (0 != _rtpRtcpModule->RemoteNTP(&ntp_secs, &ntp_frac, NULL, NULL,
                                       &rtp_timestamp)) {
      // Waiting for RTCP.
      return 0;
    }
    ntp_estimator_.UpdateRtcpTimestamp(rtt, ntp_secs, ntp_frac, rtp_timestamp);
  }
  return 0;
}

int Channel::StartPlayingFileLocally(const char* fileName,
                                     bool loop,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartPlayingFileLocally(fileNameUTF8[]=%s, loop=%d,"
                 " format=%d, volumeScaling=%5.3f, startPosition=%d, "
                 "stopPosition=%d)", fileName, loop, format, volumeScaling,
                 startPosition, stopPosition);

    if (channel_state_.Get().output_file_playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_PLAYING, kTraceError,
            "StartPlayingFileLocally() is already playing");
        return -1;
    }

    {
        CriticalSectionScoped cs(&_fileCritSect);

        if (_outputFilePlayerPtr)
        {
            _outputFilePlayerPtr->RegisterModuleFileCallback(NULL);
            FilePlayer::DestroyFilePlayer(_outputFilePlayerPtr);
            _outputFilePlayerPtr = NULL;
        }

        _outputFilePlayerPtr = FilePlayer::CreateFilePlayer(
            _outputFilePlayerId, (const FileFormats)format);

        if (_outputFilePlayerPtr == NULL)
        {
            _engineStatisticsPtr->SetLastError(
                VE_INVALID_ARGUMENT, kTraceError,
                "StartPlayingFileLocally() filePlayer format is not correct");
            return -1;
        }

        const uint32_t notificationTime(0);

        if (_outputFilePlayerPtr->StartPlayingFile(
                fileName,
                loop,
                startPosition,
                volumeScaling,
                notificationTime,
                stopPosition,
                (const CodecInst*)codecInst) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_BAD_FILE, kTraceError,
                "StartPlayingFile() failed to start file playout");
            _outputFilePlayerPtr->StopPlayingFile();
            FilePlayer::DestroyFilePlayer(_outputFilePlayerPtr);
            _outputFilePlayerPtr = NULL;
            return -1;
        }
        _outputFilePlayerPtr->RegisterModuleFileCallback(this);
        channel_state_.SetOutputFilePlaying(true);
    }

    if (RegisterFilePlayingToMixer() != 0)
        return -1;

    return 0;
}

int Channel::StartPlayingFileLocally(InStream* stream,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartPlayingFileLocally(format=%d,"
                 " volumeScaling=%5.3f, startPosition=%d, stopPosition=%d)",
                 format, volumeScaling, startPosition, stopPosition);

    if(stream == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartPlayingFileLocally() NULL as input stream");
        return -1;
    }


    if (channel_state_.Get().output_file_playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_PLAYING, kTraceError,
            "StartPlayingFileLocally() is already playing");
        return -1;
    }

    {
      CriticalSectionScoped cs(&_fileCritSect);

      // Destroy the old instance
      if (_outputFilePlayerPtr)
      {
          _outputFilePlayerPtr->RegisterModuleFileCallback(NULL);
          FilePlayer::DestroyFilePlayer(_outputFilePlayerPtr);
          _outputFilePlayerPtr = NULL;
      }

      // Create the instance
      _outputFilePlayerPtr = FilePlayer::CreateFilePlayer(
          _outputFilePlayerId,
          (const FileFormats)format);

      if (_outputFilePlayerPtr == NULL)
      {
          _engineStatisticsPtr->SetLastError(
              VE_INVALID_ARGUMENT, kTraceError,
              "StartPlayingFileLocally() filePlayer format isnot correct");
          return -1;
      }

      const uint32_t notificationTime(0);

      if (_outputFilePlayerPtr->StartPlayingFile(*stream, startPosition,
                                                 volumeScaling,
                                                 notificationTime,
                                                 stopPosition, codecInst) != 0)
      {
          _engineStatisticsPtr->SetLastError(VE_BAD_FILE, kTraceError,
                                             "StartPlayingFile() failed to "
                                             "start file playout");
          _outputFilePlayerPtr->StopPlayingFile();
          FilePlayer::DestroyFilePlayer(_outputFilePlayerPtr);
          _outputFilePlayerPtr = NULL;
          return -1;
      }
      _outputFilePlayerPtr->RegisterModuleFileCallback(this);
      channel_state_.SetOutputFilePlaying(true);
    }

    if (RegisterFilePlayingToMixer() != 0)
        return -1;

    return 0;
}

int Channel::StopPlayingFileLocally()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StopPlayingFileLocally()");

    if (!channel_state_.Get().output_file_playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceWarning,
            "StopPlayingFileLocally() isnot playing");
        return 0;
    }

    {
        CriticalSectionScoped cs(&_fileCritSect);

        if (_outputFilePlayerPtr->StopPlayingFile() != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_STOP_RECORDING_FAILED, kTraceError,
                "StopPlayingFile() could not stop playing");
            return -1;
        }
        _outputFilePlayerPtr->RegisterModuleFileCallback(NULL);
        FilePlayer::DestroyFilePlayer(_outputFilePlayerPtr);
        _outputFilePlayerPtr = NULL;
        channel_state_.SetOutputFilePlaying(false);
    }
    // _fileCritSect cannot be taken while calling
    // SetAnonymousMixibilityStatus. Refer to comments in
    // StartPlayingFileLocally(const char* ...) for more details.
    if (_outputMixerPtr->SetAnonymousMixabilityStatus(*this, false) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CONF_MIX_MODULE_ERROR, kTraceError,
            "StopPlayingFile() failed to stop participant from playing as"
            "file in the mixer");
        return -1;
    }

    return 0;
}

int Channel::IsPlayingFileLocally() const
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::IsPlayingFileLocally()");

    return channel_state_.Get().output_file_playing;
}

int Channel::RegisterFilePlayingToMixer()
{
    // Return success for not registering for file playing to mixer if:
    // 1. playing file before playout is started on that channel.
    // 2. starting playout without file playing on that channel.
    if (!channel_state_.Get().playing ||
        !channel_state_.Get().output_file_playing)
    {
        return 0;
    }

    // |_fileCritSect| cannot be taken while calling
    // SetAnonymousMixabilityStatus() since as soon as the participant is added
    // frames can be pulled by the mixer. Since the frames are generated from
    // the file, _fileCritSect will be taken. This would result in a deadlock.
    if (_outputMixerPtr->SetAnonymousMixabilityStatus(*this, true) != 0)
    {
        channel_state_.SetOutputFilePlaying(false);
        CriticalSectionScoped cs(&_fileCritSect);
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CONF_MIX_MODULE_ERROR, kTraceError,
            "StartPlayingFile() failed to add participant as file to mixer");
        _outputFilePlayerPtr->StopPlayingFile();
        FilePlayer::DestroyFilePlayer(_outputFilePlayerPtr);
        _outputFilePlayerPtr = NULL;
        return -1;
    }

    return 0;
}

int Channel::StartPlayingFileAsMicrophone(const char* fileName,
                                          bool loop,
                                          FileFormats format,
                                          int startPosition,
                                          float volumeScaling,
                                          int stopPosition,
                                          const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartPlayingFileAsMicrophone(fileNameUTF8[]=%s, "
                 "loop=%d, format=%d, volumeScaling=%5.3f, startPosition=%d, "
                 "stopPosition=%d)", fileName, loop, format, volumeScaling,
                 startPosition, stopPosition);

    CriticalSectionScoped cs(&_fileCritSect);

    if (channel_state_.Get().input_file_playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_PLAYING, kTraceWarning,
            "StartPlayingFileAsMicrophone() filePlayer is playing");
        return 0;
    }

    // Destroy the old instance
    if (_inputFilePlayerPtr)
    {
        _inputFilePlayerPtr->RegisterModuleFileCallback(NULL);
        FilePlayer::DestroyFilePlayer(_inputFilePlayerPtr);
        _inputFilePlayerPtr = NULL;
    }

    // Create the instance
    _inputFilePlayerPtr = FilePlayer::CreateFilePlayer(
        _inputFilePlayerId, (const FileFormats)format);

    if (_inputFilePlayerPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartPlayingFileAsMicrophone() filePlayer format isnot correct");
        return -1;
    }

    const uint32_t notificationTime(0);

    if (_inputFilePlayerPtr->StartPlayingFile(
        fileName,
        loop,
        startPosition,
        volumeScaling,
        notificationTime,
        stopPosition,
        (const CodecInst*)codecInst) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartPlayingFile() failed to start file playout");
        _inputFilePlayerPtr->StopPlayingFile();
        FilePlayer::DestroyFilePlayer(_inputFilePlayerPtr);
        _inputFilePlayerPtr = NULL;
        return -1;
    }
    _inputFilePlayerPtr->RegisterModuleFileCallback(this);
    channel_state_.SetInputFilePlaying(true);

    return 0;
}

int Channel::StartPlayingFileAsMicrophone(InStream* stream,
                                          FileFormats format,
                                          int startPosition,
                                          float volumeScaling,
                                          int stopPosition,
                                          const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartPlayingFileAsMicrophone(format=%d, "
                 "volumeScaling=%5.3f, startPosition=%d, stopPosition=%d)",
                 format, volumeScaling, startPosition, stopPosition);

    if(stream == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartPlayingFileAsMicrophone NULL as input stream");
        return -1;
    }

    CriticalSectionScoped cs(&_fileCritSect);

    if (channel_state_.Get().input_file_playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_PLAYING, kTraceWarning,
            "StartPlayingFileAsMicrophone() is playing");
        return 0;
    }

    // Destroy the old instance
    if (_inputFilePlayerPtr)
    {
        _inputFilePlayerPtr->RegisterModuleFileCallback(NULL);
        FilePlayer::DestroyFilePlayer(_inputFilePlayerPtr);
        _inputFilePlayerPtr = NULL;
    }

    // Create the instance
    _inputFilePlayerPtr = FilePlayer::CreateFilePlayer(
        _inputFilePlayerId, (const FileFormats)format);

    if (_inputFilePlayerPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartPlayingInputFile() filePlayer format isnot correct");
        return -1;
    }

    const uint32_t notificationTime(0);

    if (_inputFilePlayerPtr->StartPlayingFile(*stream, startPosition,
                                              volumeScaling, notificationTime,
                                              stopPosition, codecInst) != 0)
    {
        _engineStatisticsPtr->SetLastError(VE_BAD_FILE, kTraceError,
                                           "StartPlayingFile() failed to start "
                                           "file playout");
        _inputFilePlayerPtr->StopPlayingFile();
        FilePlayer::DestroyFilePlayer(_inputFilePlayerPtr);
        _inputFilePlayerPtr = NULL;
        return -1;
    }

    _inputFilePlayerPtr->RegisterModuleFileCallback(this);
    channel_state_.SetInputFilePlaying(true);

    return 0;
}

int Channel::StopPlayingFileAsMicrophone()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StopPlayingFileAsMicrophone()");

    CriticalSectionScoped cs(&_fileCritSect);

    if (!channel_state_.Get().input_file_playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceWarning,
            "StopPlayingFileAsMicrophone() isnot playing");
        return 0;
    }

    if (_inputFilePlayerPtr->StopPlayingFile() != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_STOP_RECORDING_FAILED, kTraceError,
            "StopPlayingFile() could not stop playing");
        return -1;
    }
    _inputFilePlayerPtr->RegisterModuleFileCallback(NULL);
    FilePlayer::DestroyFilePlayer(_inputFilePlayerPtr);
    _inputFilePlayerPtr = NULL;
    channel_state_.SetInputFilePlaying(false);

    return 0;
}

int Channel::IsPlayingFileAsMicrophone() const
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::IsPlayingFileAsMicrophone()");
    return channel_state_.Get().input_file_playing;
}

int Channel::StartRecordingPlayout(const char* fileName,
                                   const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartRecordingPlayout(fileName=%s)", fileName);

    if (_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,-1),
                     "StartRecordingPlayout() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0); // Not supported in VoE
    CodecInst dummyCodec={100,"L16",16000,320,1,320000};

    if ((codecInst != NULL) &&
      ((codecInst->channels < 1) || (codecInst->channels > 2)))
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingPlayout() invalid compression");
        return(-1);
    }
    if(codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst=&dummyCodec;
    }
    else if((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    }
    else
    {
        format = kFileFormatCompressedFile;
    }

    CriticalSectionScoped cs(&_fileCritSect);

    // Destroy the old instance
    if (_outputFileRecorderPtr)
    {
        _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
    }

    _outputFileRecorderPtr = FileRecorder::CreateFileRecorder(
        _outputFileRecorderId, (const FileFormats)format);
    if (_outputFileRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingPlayout() fileRecorder format isnot correct");
        return -1;
    }

    if (_outputFileRecorderPtr->StartRecordingAudioFile(
        fileName, (const CodecInst&)*codecInst, notificationTime) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartRecordingAudioFile() failed to start file recording");
        _outputFileRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
        return -1;
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(this);
    _outputFileRecording = true;

    return 0;
}

int Channel::StartRecordingPlayout(OutStream* stream,
                                   const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::StartRecordingPlayout()");

    if (_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,-1),
                     "StartRecordingPlayout() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0); // Not supported in VoE
    CodecInst dummyCodec={100,"L16",16000,320,1,320000};

    if (codecInst != NULL && codecInst->channels != 1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingPlayout() invalid compression");
        return(-1);
    }
    if(codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst=&dummyCodec;
    }
    else if((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    }
    else
    {
        format = kFileFormatCompressedFile;
    }

    CriticalSectionScoped cs(&_fileCritSect);

    // Destroy the old instance
    if (_outputFileRecorderPtr)
    {
        _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
    }

    _outputFileRecorderPtr = FileRecorder::CreateFileRecorder(
        _outputFileRecorderId, (const FileFormats)format);
    if (_outputFileRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingPlayout() fileRecorder format isnot correct");
        return -1;
    }

    if (_outputFileRecorderPtr->StartRecordingAudioFile(*stream, *codecInst,
                                                        notificationTime) != 0)
    {
        _engineStatisticsPtr->SetLastError(VE_BAD_FILE, kTraceError,
                                           "StartRecordingPlayout() failed to "
                                           "start file recording");
        _outputFileRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
        return -1;
    }

    _outputFileRecorderPtr->RegisterModuleFileCallback(this);
    _outputFileRecording = true;

    return 0;
}

int Channel::StopRecordingPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "Channel::StopRecordingPlayout()");

    if (!_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,-1),
                     "StopRecordingPlayout() isnot recording");
        return -1;
    }


    CriticalSectionScoped cs(&_fileCritSect);

    if (_outputFileRecorderPtr->StopRecording() != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_STOP_RECORDING_FAILED, kTraceError,
            "StopRecording() could not stop recording");
        return(-1);
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
    FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
    _outputFileRecorderPtr = NULL;
    _outputFileRecording = false;

    return 0;
}

void
Channel::SetMixWithMicStatus(bool mix)
{
    CriticalSectionScoped cs(&_fileCritSect);
    _mixFileWithMicrophone=mix;
}

int
Channel::GetSpeechOutputLevel(uint32_t& level) const
{
    int8_t currentLevel = _outputAudioLevel.Level();
    level = static_cast<int32_t> (currentLevel);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId),
               "GetSpeechOutputLevel() => level=%u", level);
    return 0;
}

int
Channel::GetSpeechOutputLevelFullRange(uint32_t& level) const
{
    int16_t currentLevel = _outputAudioLevel.LevelFullRange();
    level = static_cast<int32_t> (currentLevel);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId),
               "GetSpeechOutputLevelFullRange() => level=%u", level);
    return 0;
}

int
Channel::SetMute(bool enable)
{
    CriticalSectionScoped cs(&volume_settings_critsect_);
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::SetMute(enable=%d)", enable);
    _mute = enable;
    return 0;
}

bool
Channel::Mute() const
{
    CriticalSectionScoped cs(&volume_settings_critsect_);
    return _mute;
}

int
Channel::SetOutputVolumePan(float left, float right)
{
    CriticalSectionScoped cs(&volume_settings_critsect_);
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::SetOutputVolumePan()");
    _panLeft = left;
    _panRight = right;
    return 0;
}

int
Channel::GetOutputVolumePan(float& left, float& right) const
{
    CriticalSectionScoped cs(&volume_settings_critsect_);
    left = _panLeft;
    right = _panRight;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId),
               "GetOutputVolumePan() => left=%3.2f, right=%3.2f", left, right);
    return 0;
}

int
Channel::SetChannelOutputVolumeScaling(float scaling)
{
    CriticalSectionScoped cs(&volume_settings_critsect_);
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::SetChannelOutputVolumeScaling()");
    _outputGain = scaling;
    return 0;
}

int
Channel::GetChannelOutputVolumeScaling(float& scaling) const
{
    CriticalSectionScoped cs(&volume_settings_critsect_);
    scaling = _outputGain;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId),
               "GetChannelOutputVolumeScaling() => scaling=%3.2f", scaling);
    return 0;
}

int Channel::SendTelephoneEventOutband(unsigned char eventCode,
                                       int lengthMs, int attenuationDb,
                                       bool playDtmfEvent)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SendTelephoneEventOutband(..., playDtmfEvent=%d)",
               playDtmfEvent);

    _playOutbandDtmfEvent = playDtmfEvent;

    if (_rtpRtcpModule->SendTelephoneEventOutband(eventCode, lengthMs,
                                                 attenuationDb) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_SEND_DTMF_FAILED,
            kTraceWarning,
            "SendTelephoneEventOutband() failed to send event");
        return -1;
    }
    return 0;
}

int Channel::SendTelephoneEventInband(unsigned char eventCode,
                                         int lengthMs,
                                         int attenuationDb,
                                         bool playDtmfEvent)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SendTelephoneEventInband(..., playDtmfEvent=%d)",
               playDtmfEvent);

    _playInbandDtmfEvent = playDtmfEvent;
    _inbandDtmfQueue.AddDtmf(eventCode, lengthMs, attenuationDb);

    return 0;
}

int
Channel::SetSendTelephoneEventPayloadType(unsigned char type)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::SetSendTelephoneEventPayloadType()");
    if (type > 127)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "SetSendTelephoneEventPayloadType() invalid type");
        return -1;
    }
    CodecInst codec = {};
    codec.plfreq = 8000;
    codec.pltype = type;
    memcpy(codec.plname, "telephone-event", 16);
    if (_rtpRtcpModule->RegisterSendPayload(codec) != 0)
    {
        _rtpRtcpModule->DeRegisterSendPayload(codec.pltype);
        if (_rtpRtcpModule->RegisterSendPayload(codec) != 0) {
            _engineStatisticsPtr->SetLastError(
                VE_RTP_RTCP_MODULE_ERROR, kTraceError,
                "SetSendTelephoneEventPayloadType() failed to register send"
                "payload type");
            return -1;
        }
    }
    _sendTelephoneEventPayloadType = type;
    return 0;
}

int
Channel::GetSendTelephoneEventPayloadType(unsigned char& type)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetSendTelephoneEventPayloadType()");
    type = _sendTelephoneEventPayloadType;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId),
               "GetSendTelephoneEventPayloadType() => type=%u", type);
    return 0;
}
int
Channel::SetRecvTelephoneEventPayloadType(unsigned char type)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
		"Channel::SetRecvTelephoneEventPayloadType() type:%d", type);
	if (type > 127)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_ARGUMENT, kTraceError,
			"SetSendTelephoneEventPayloadType() invalid type");
		return -1;
	}
	_recvTelephoneEventPayloadType = type;
	return 0;
}

int
Channel::GetRecvTelephoneEventPayloadType(unsigned char& type)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
		"Channel::GetRecvTelephoneEventPayloadType()");
	type = _recvTelephoneEventPayloadType;
	WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
		VoEId(_instanceId, _channelId),
		"GetRecvTelephoneEventPayloadType() => type=%u", type);
	return 0;
}
int
Channel::UpdateRxVadDetection(AudioFrame& audioFrame)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::UpdateRxVadDetection()");

    int vadDecision = 1;

    vadDecision = (audioFrame.vad_activity_ == AudioFrame::kVadActive)? 1 : 0;

    if ((vadDecision != _oldVadDecision) && _rxVadObserverPtr)
    {
        OnRxVadDetected(vadDecision);
        _oldVadDecision = vadDecision;
    }

    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::UpdateRxVadDetection() => vadDecision=%d",
                 vadDecision);
    return 0;
}

int
Channel::RegisterRxVadObserver(VoERxVadCallback &observer)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::RegisterRxVadObserver()");
    CriticalSectionScoped cs(&_callbackCritSect);

    if (_rxVadObserverPtr)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceError,
            "RegisterRxVadObserver() observer already enabled");
        return -1;
    }
    _rxVadObserverPtr = &observer;
    _RxVadDetection = true;
    return 0;
}

int
Channel::DeRegisterRxVadObserver()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::DeRegisterRxVadObserver()");
    CriticalSectionScoped cs(&_callbackCritSect);

    if (!_rxVadObserverPtr)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceWarning,
            "DeRegisterRxVadObserver() observer already disabled");
        return 0;
    }
    _rxVadObserverPtr = NULL;
    _RxVadDetection = false;
    return 0;
}

int
Channel::VoiceActivityIndicator(int &activity)
{
    activity = _sendFrameType;

    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::VoiceActivityIndicator(indicator=%d)", activity);
    return 0;
}

#ifdef WEBRTC_VOICE_ENGINE_AGC

int
Channel::SetRxAgcStatus(bool enable, AgcModes mode)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetRxAgcStatus(enable=%d, mode=%d)",
                 (int)enable, (int)mode);

    GainControl::Mode agcMode = kDefaultRxAgcMode;
    switch (mode)
    {
        case kAgcDefault:
            break;
        case kAgcUnchanged:
            agcMode = rx_audioproc_->gain_control()->mode();
            break;
        case kAgcFixedDigital:
            agcMode = GainControl::kFixedDigital;
            break;
        case kAgcAdaptiveDigital:
            agcMode =GainControl::kAdaptiveDigital;
            break;
        default:
            _engineStatisticsPtr->SetLastError(
                VE_INVALID_ARGUMENT, kTraceError,
                "SetRxAgcStatus() invalid Agc mode");
            return -1;
    }

    if (rx_audioproc_->gain_control()->set_mode(agcMode) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_APM_ERROR, kTraceError,
            "SetRxAgcStatus() failed to set Agc mode");
        return -1;
    }
    if (rx_audioproc_->gain_control()->Enable(enable) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_APM_ERROR, kTraceError,
            "SetRxAgcStatus() failed to set Agc state");
        return -1;
    }

    _rxAgcIsEnabled = enable;
    channel_state_.SetRxApmIsEnabled(_rxAgcIsEnabled || _rxNsIsEnabled);

    return 0;
}

int
Channel::GetRxAgcStatus(bool& enabled, AgcModes& mode)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                     "Channel::GetRxAgcStatus(enable=?, mode=?)");

    bool enable = rx_audioproc_->gain_control()->is_enabled();
    GainControl::Mode agcMode =
        rx_audioproc_->gain_control()->mode();

    enabled = enable;

    switch (agcMode)
    {
        case GainControl::kFixedDigital:
            mode = kAgcFixedDigital;
            break;
        case GainControl::kAdaptiveDigital:
            mode = kAgcAdaptiveDigital;
            break;
        default:
            _engineStatisticsPtr->SetLastError(
                VE_APM_ERROR, kTraceError,
                "GetRxAgcStatus() invalid Agc mode");
            return -1;
    }

    return 0;
}

int
Channel::SetRxAgcConfig(AgcConfig config)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetRxAgcConfig()");

    if (rx_audioproc_->gain_control()->set_target_level_dbfs(
        config.targetLeveldBOv) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_APM_ERROR, kTraceError,
            "SetRxAgcConfig() failed to set target peak |level|"
            "(or envelope) of the Agc");
        return -1;
    }
    if (rx_audioproc_->gain_control()->set_compression_gain_db(
        config.digitalCompressionGaindB) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_APM_ERROR, kTraceError,
            "SetRxAgcConfig() failed to set the range in |gain| the"
            " digital compression stage may apply");
        return -1;
    }
    if (rx_audioproc_->gain_control()->enable_limiter(
        config.limiterEnable) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_APM_ERROR, kTraceError,
            "SetRxAgcConfig() failed to set hard limiter to the signal");
        return -1;
    }

    return 0;
}

int
Channel::GetRxAgcConfig(AgcConfig& config)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetRxAgcConfig(config=%?)");

    config.targetLeveldBOv =
        rx_audioproc_->gain_control()->target_level_dbfs();
    config.digitalCompressionGaindB =
        rx_audioproc_->gain_control()->compression_gain_db();
    config.limiterEnable =
        rx_audioproc_->gain_control()->is_limiter_enabled();

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId), "GetRxAgcConfig() => "
                   "targetLeveldBOv=%u, digitalCompressionGaindB=%u,"
                   " limiterEnable=%d",
                   config.targetLeveldBOv,
                   config.digitalCompressionGaindB,
                   config.limiterEnable);

    return 0;
}

#endif // #ifdef WEBRTC_VOICE_ENGINE_AGC

#ifdef WEBRTC_VOICE_ENGINE_NR

int
Channel::SetRxNsStatus(bool enable, NsModes mode)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetRxNsStatus(enable=%d, mode=%d)",
                 (int)enable, (int)mode);

    NoiseSuppression::Level nsLevel = kDefaultNsMode;
    switch (mode)
    {

        case kNsDefault:
            break;
        case kNsUnchanged:
            nsLevel = rx_audioproc_->noise_suppression()->level();
            break;
        case kNsConference:
            nsLevel = NoiseSuppression::kHigh;
            break;
        case kNsLowSuppression:
            nsLevel = NoiseSuppression::kLow;
            break;
        case kNsModerateSuppression:
            nsLevel = NoiseSuppression::kModerate;
            break;
        case kNsHighSuppression:
            nsLevel = NoiseSuppression::kHigh;
            break;
        case kNsVeryHighSuppression:
            nsLevel = NoiseSuppression::kVeryHigh;
            break;
    }

    if (rx_audioproc_->noise_suppression()->set_level(nsLevel)
        != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_APM_ERROR, kTraceError,
            "SetRxNsStatus() failed to set NS level");
        return -1;
    }
    if (rx_audioproc_->noise_suppression()->Enable(enable) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_APM_ERROR, kTraceError,
            "SetRxNsStatus() failed to set NS state");
        return -1;
    }

    _rxNsIsEnabled = enable;
    channel_state_.SetRxApmIsEnabled(_rxAgcIsEnabled || _rxNsIsEnabled);

    return 0;
}

int
Channel::GetRxNsStatus(bool& enabled, NsModes& mode)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetRxNsStatus(enable=?, mode=?)");

    bool enable =
        rx_audioproc_->noise_suppression()->is_enabled();
    NoiseSuppression::Level ncLevel =
        rx_audioproc_->noise_suppression()->level();

    enabled = enable;

    switch (ncLevel)
    {
        case NoiseSuppression::kLow:
            mode = kNsLowSuppression;
            break;
        case NoiseSuppression::kModerate:
            mode = kNsModerateSuppression;
            break;
        case NoiseSuppression::kHigh:
            mode = kNsHighSuppression;
            break;
        case NoiseSuppression::kVeryHigh:
            mode = kNsVeryHighSuppression;
            break;
    }

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId),
               "GetRxNsStatus() => enabled=%d, mode=%d", enabled, mode);
    return 0;
}

#endif // #ifdef WEBRTC_VOICE_ENGINE_NR

int
Channel::SetLocalSSRC(unsigned int ssrc)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                 "Channel::SetLocalSSRC()");
    if (channel_state_.Get().sending)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_SENDING, kTraceError,
            "SetLocalSSRC() already sending");
        return -1;
    }
    _rtpRtcpModule->SetSSRC(ssrc);
    return 0;
}

int
Channel::GetLocalSSRC(unsigned int& ssrc)
{
    ssrc = _rtpRtcpModule->SSRC();
    WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                 VoEId(_instanceId,_channelId),
                 "GetLocalSSRC() => ssrc=%lu", ssrc);
    return 0;
}

int
Channel::GetRemoteSSRC(unsigned int& ssrc)
{
    ssrc = rtp_receiver_->SSRC();
    WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                 VoEId(_instanceId,_channelId),
                 "GetRemoteSSRC() => ssrc=%lu", ssrc);
    return 0;
}

int Channel::SetSendAudioLevelIndicationStatus(bool enable, unsigned char id) {
  _includeAudioLevelIndication = enable;
  return SetSendRtpHeaderExtension(enable, kRtpExtensionAudioLevel, id);
}

int Channel::SetReceiveAudioLevelIndicationStatus(bool enable,
                                                  unsigned char id) {
  rtp_header_parser_->DeregisterRtpHeaderExtension(
      kRtpExtensionAudioLevel);
  if (enable && !rtp_header_parser_->RegisterRtpHeaderExtension(
          kRtpExtensionAudioLevel, id)) {
    return -1;
  }
  return 0;
}

int Channel::SetSendAbsoluteSenderTimeStatus(bool enable, unsigned char id) {
  return SetSendRtpHeaderExtension(enable, kRtpExtensionAbsoluteSendTime, id);
}

int Channel::SetReceiveAbsoluteSenderTimeStatus(bool enable, unsigned char id) {
  rtp_header_parser_->DeregisterRtpHeaderExtension(
      kRtpExtensionAbsoluteSendTime);
  if (enable && !rtp_header_parser_->RegisterRtpHeaderExtension(
      kRtpExtensionAbsoluteSendTime, id)) {
    return -1;
  }
  return 0;
}

void Channel::SetRTCPStatus(bool enable) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SetRTCPStatus()");
  _rtpRtcpModule->SetRTCPStatus(enable ? kRtcpNonCompound : kRtcpOff);
}

int
Channel::GetRTCPStatus(bool& enabled)
{
    RTCPMethod method = _rtpRtcpModule->RTCP();
    enabled = (method != kRtcpOff);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                 VoEId(_instanceId,_channelId),
                 "GetRTCPStatus() => enabled=%d", enabled);
    return 0;
}

int
Channel::SetRTCP_CNAME(const char cName[256])
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                 "Channel::SetRTCP_CNAME()");
    if (_rtpRtcpModule->SetCNAME(cName) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceError,
            "SetRTCP_CNAME() failed to set RTCP CNAME");
        return -1;
    }
    return 0;
}

int
Channel::GetRemoteRTCP_CNAME(char cName[256])
{
    if (cName == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "GetRemoteRTCP_CNAME() invalid CNAME input buffer");
        return -1;
    }
    char cname[RTCP_CNAME_SIZE];
    const uint32_t remoteSSRC = rtp_receiver_->SSRC();
    if (_rtpRtcpModule->RemoteCNAME(remoteSSRC, cname) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_CANNOT_RETRIEVE_CNAME, kTraceError,
            "GetRemoteRTCP_CNAME() failed to retrieve remote RTCP CNAME");
        return -1;
    }
    strcpy(cName, cname);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                 VoEId(_instanceId, _channelId),
                 "GetRemoteRTCP_CNAME() => cName=%s", cName);
    return 0;
}

int
Channel::GetRemoteRTCPData(
    unsigned int& NTPHigh,
    unsigned int& NTPLow,
    unsigned int& timestamp,
    unsigned int& playoutTimestamp,
    unsigned int* jitter,
    unsigned short* fractionLost)
{
    // --- Information from sender info in received Sender Reports

    RTCPSenderInfo senderInfo;
    if (_rtpRtcpModule->RemoteRTCPStat(&senderInfo) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_RTP_RTCP_MODULE_ERROR, kTraceError,
            "GetRemoteRTCPData() failed to retrieve sender info for remote "
            "side");
        return -1;
    }

    // We only utilize 12 out of 20 bytes in the sender info (ignores packet
    // and octet count)
    NTPHigh = senderInfo.NTPseconds;
    NTPLow = senderInfo.NTPfraction;
    timestamp = senderInfo.RTPtimeStamp;

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                 VoEId(_instanceId, _channelId),
                 "GetRemoteRTCPData() => NTPHigh=%lu, NTPLow=%lu, "
                 "timestamp=%lu",
                 NTPHigh, NTPLow, timestamp);

    // --- Locally derived information

    // This value is updated on each incoming RTCP packet (0 when no packet
    // has been received)
    playoutTimestamp = playout_timestamp_rtcp_;

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                 VoEId(_instanceId, _channelId),
                 "GetRemoteRTCPData() => playoutTimestamp=%lu",
                 playout_timestamp_rtcp_);

    if (NULL != jitter || NULL != fractionLost)
    {
        // Get all RTCP receiver report blocks that have been received on this
        // channel. If we receive RTP packets from a remote source we know the
        // remote SSRC and use the report block from him.
        // Otherwise use the first report block.
        std::vector<RTCPReportBlock> remote_stats;
        if (_rtpRtcpModule->RemoteRTCPStat(&remote_stats) != 0 ||
            remote_stats.empty()) {
          WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                       VoEId(_instanceId, _channelId),
                       "GetRemoteRTCPData() failed to measure statistics due"
                       " to lack of received RTP and/or RTCP packets");
          return -1;
        }

        uint32_t remoteSSRC = rtp_receiver_->SSRC();
        std::vector<RTCPReportBlock>::const_iterator it = remote_stats.begin();
        for (; it != remote_stats.end(); ++it) {
          if (it->remoteSSRC == remoteSSRC)
            break;
        }

        if (it == remote_stats.end()) {
          // If we have not received any RTCP packets from this SSRC it probably
          // means that we have not received any RTP packets.
          // Use the first received report block instead.
          it = remote_stats.begin();
          remoteSSRC = it->remoteSSRC;
        }

        if (jitter) {
          *jitter = it->jitter;
          WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                       VoEId(_instanceId, _channelId),
                       "GetRemoteRTCPData() => jitter = %lu", *jitter);
        }

        if (fractionLost) {
          *fractionLost = it->fractionLost;
          WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                       VoEId(_instanceId, _channelId),
                       "GetRemoteRTCPData() => fractionLost = %lu",
                       *fractionLost);
        }
    }
    return 0;
}

int
Channel::SendApplicationDefinedRTCPPacket(unsigned char subType,
                                             unsigned int name,
                                             const char* data,
                                             unsigned short dataLengthInBytes)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                 "Channel::SendApplicationDefinedRTCPPacket()");
    if (!channel_state_.Get().sending)
    {
        _engineStatisticsPtr->SetLastError(
            VE_NOT_SENDING, kTraceError,
            "SendApplicationDefinedRTCPPacket() not sending");
        return -1;
    }
    if (NULL == data)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "SendApplicationDefinedRTCPPacket() invalid data value");
        return -1;
    }
    if (dataLengthInBytes % 4 != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "SendApplicationDefinedRTCPPacket() invalid length value");
        return -1;
    }
    RTCPMethod status = _rtpRtcpModule->RTCP();
    if (status == kRtcpOff)
    {
        _engineStatisticsPtr->SetLastError(
            VE_RTCP_ERROR, kTraceError,
            "SendApplicationDefinedRTCPPacket() RTCP is disabled");
        return -1;
    }

    // Create and schedule the RTCP APP packet for transmission
    if (_rtpRtcpModule->SetRTCPApplicationSpecificData(
        subType,
        name,
        (const unsigned char*) data,
        dataLengthInBytes) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_SEND_ERROR, kTraceError,
            "SendApplicationDefinedRTCPPacket() failed to send RTCP packet");
        return -1;
    }
    return 0;
}

int
Channel::GetRTPStatistics(
        unsigned int& averageJitterMs,
        unsigned int& maxJitterMs,
        unsigned int& discardedPackets)
{
    // The jitter statistics is updated for each received RTP packet and is
    // based on received packets.
    if (_rtpRtcpModule->RTCP() == kRtcpOff) {
      // If RTCP is off, there is no timed thread in the RTCP module regularly
      // generating new stats, trigger the update manually here instead.
      StreamStatistician* statistician =
          rtp_receive_statistics_->GetStatistician(rtp_receiver_->SSRC());
      if (statistician) {
        // Don't use returned statistics, use data from proxy instead so that
        // max jitter can be fetched atomically.
        RtcpStatistics s;
        statistician->GetStatistics(&s, true);
      }
    }

    ChannelStatistics stats = statistics_proxy_->GetStats();
    const int32_t playoutFrequency = audio_coding_->PlayoutFrequency();
    if (playoutFrequency > 0) {
      // Scale RTP statistics given the current playout frequency
      maxJitterMs = stats.max_jitter / (playoutFrequency / 1000);
      averageJitterMs = stats.rtcp.jitter / (playoutFrequency / 1000);
    }

    discardedPackets = _numberOfDiscardedPackets;

    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId, _channelId),
               "GetRTPStatistics() => averageJitterMs = %lu, maxJitterMs = %lu,"
               " discardedPackets = %lu)",
               averageJitterMs, maxJitterMs, discardedPackets);
    return 0;
}

int Channel::GetRemoteRTCPReportBlocks(
    std::vector<ReportBlock>* report_blocks) {
  if (report_blocks == NULL) {
    _engineStatisticsPtr->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
      "GetRemoteRTCPReportBlock()s invalid report_blocks.");
    return -1;
  }

  // Get the report blocks from the latest received RTCP Sender or Receiver
  // Report. Each element in the vector contains the sender's SSRC and a
  // report block according to RFC 3550.
  std::vector<RTCPReportBlock> rtcp_report_blocks;
  if (_rtpRtcpModule->RemoteRTCPStat(&rtcp_report_blocks) != 0) {
    _engineStatisticsPtr->SetLastError(VE_RTP_RTCP_MODULE_ERROR, kTraceError,
        "GetRemoteRTCPReportBlocks() failed to read RTCP SR/RR report block.");
    return -1;
  }

  if (rtcp_report_blocks.empty())
    return 0;

  std::vector<RTCPReportBlock>::const_iterator it = rtcp_report_blocks.begin();
  for (; it != rtcp_report_blocks.end(); ++it) {
    ReportBlock report_block;
    report_block.sender_SSRC = it->remoteSSRC;
    report_block.source_SSRC = it->sourceSSRC;
    report_block.fraction_lost = it->fractionLost;
    report_block.cumulative_num_packets_lost = it->cumulativeLost;
    report_block.extended_highest_sequence_number = it->extendedHighSeqNum;
    report_block.interarrival_jitter = it->jitter;
    report_block.last_SR_timestamp = it->lastSR;
    report_block.delay_since_last_SR = it->delaySinceLastSR;
    report_blocks->push_back(report_block);
  }
  return 0;
}

int
Channel::GetRTPStatistics(CallStatistics& stats)
{
    // --- RtcpStatistics

    // The jitter statistics is updated for each received RTP packet and is
    // based on received packets.
    RtcpStatistics statistics;
    StreamStatistician* statistician =
        rtp_receive_statistics_->GetStatistician(rtp_receiver_->SSRC());
    if (!statistician || !statistician->GetStatistics(
        &statistics, _rtpRtcpModule->RTCP() == kRtcpOff)) {
      _engineStatisticsPtr->SetLastError(
          VE_CANNOT_RETRIEVE_RTP_STAT, kTraceWarning,
          "GetRTPStatistics() failed to read RTP statistics from the "
          "RTP/RTCP module");
    }

    stats.fractionLost = statistics.fraction_lost;
    stats.cumulativeLost = statistics.cumulative_lost;
    stats.extendedMax = statistics.extended_max_sequence_number;
    stats.jitterSamples = statistics.jitter;

    WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                 VoEId(_instanceId, _channelId),
                 "GetRTPStatistics() => fractionLost=%lu, cumulativeLost=%lu,"
                 " extendedMax=%lu, jitterSamples=%li)",
                 stats.fractionLost, stats.cumulativeLost, stats.extendedMax,
                 stats.jitterSamples);

    // --- RTT
    stats.rttMs = GetRTT();
    if (stats.rttMs == 0) {
      WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                   "GetRTPStatistics() failed to get RTT");
    } else {
      WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                   "GetRTPStatistics() => rttMs=%" PRId64, stats.rttMs);
    }

    // --- Data counters

    size_t bytesSent(0);
    uint32_t packetsSent(0);
    size_t bytesReceived(0);
    uint32_t packetsReceived(0);

    if (statistician) {
      statistician->GetDataCounters(&bytesReceived, &packetsReceived);
    }

    if (_rtpRtcpModule->DataCountersRTP(&bytesSent,
                                        &packetsSent) != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                     VoEId(_instanceId, _channelId),
                     "GetRTPStatistics() failed to retrieve RTP datacounters =>"
                     " output will not be complete");
    }

    stats.bytesSent = bytesSent;
    stats.packetsSent = packetsSent;
    stats.bytesReceived = bytesReceived;
    stats.packetsReceived = packetsReceived;

    WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                 VoEId(_instanceId, _channelId),
                 "GetRTPStatistics() => bytesSent=%" PRIuS ", packetsSent=%d,"
                 " bytesReceived=%" PRIuS ", packetsReceived=%d)",
                 stats.bytesSent, stats.packetsSent, stats.bytesReceived,
                 stats.packetsReceived);

    // --- Timestamps
    {
      CriticalSectionScoped lock(ts_stats_lock_.get());
      stats.capture_start_ntp_time_ms_ = capture_start_ntp_time_ms_;
    }
    return 0;
}

int Channel::SetREDStatus(bool enable, int redPayloadtype) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SetREDStatus()");

  if (enable) {
    if (redPayloadtype < 0 || redPayloadtype > 127) {
      _engineStatisticsPtr->SetLastError(
          VE_PLTYPE_ERROR, kTraceError,
          "SetREDStatus() invalid RED payload type");
      return -1;
    }

    if (SetRedPayloadType(redPayloadtype) < 0) {
      _engineStatisticsPtr->SetLastError(
          VE_CODEC_ERROR, kTraceError,
          "SetSecondarySendCodec() Failed to register RED ACM");
      return -1;
    }
  }

  if (audio_coding_->SetREDStatus(enable) != 0) {
    _engineStatisticsPtr->SetLastError(
        VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
        "SetREDStatus() failed to set RED state in the ACM");
    return -1;
  }
  return 0;
}

int
Channel::GetREDStatus(bool& enabled, int& redPayloadtype)
{
    enabled = audio_coding_->REDStatus();
    if (enabled)
    {
        int8_t payloadType(0);
        if (_rtpRtcpModule->SendREDPayloadType(payloadType) != 0)
        {
            _engineStatisticsPtr->SetLastError(
                VE_RTP_RTCP_MODULE_ERROR, kTraceError,
                "GetREDStatus() failed to retrieve RED PT from RTP/RTCP "
                "module");
            return -1;
        }
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                   VoEId(_instanceId, _channelId),
                   "GetREDStatus() => enabled=%d, redPayloadtype=%d",
                   enabled, redPayloadtype);
        return 0;
    }
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                 VoEId(_instanceId, _channelId),
                 "GetREDStatus() => enabled=%d", enabled);
    return 0;
}

int Channel::SetCodecFECStatus(bool enable) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SetCodecFECStatus()");

  if (audio_coding_->SetCodecFEC(enable) != 0) {
    _engineStatisticsPtr->SetLastError(
        VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
        "SetCodecFECStatus() failed to set FEC state");
    return -1;
  }
  return 0;
}

int Channel::SetLoss(int loss) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                 "Channel::SetCodecFECStatus()");
    if (loss == 0) {
        _loss = 0;
    }
    else
        _loss = 100/loss;
    return 0;
}

bool Channel::GetCodecFECStatus() {
  bool enabled = audio_coding_->CodecFEC();
  WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId, _channelId),
               "GetCodecFECStatus() => enabled=%d", enabled);
  return enabled;
}

void Channel::SetNACKStatus(bool enable, int maxNumberOfPackets) {
  // None of these functions can fail.
  _rtpRtcpModule->SetStorePacketsStatus(enable, maxNumberOfPackets);
  rtp_receive_statistics_->SetMaxReorderingThreshold(maxNumberOfPackets);
  rtp_receiver_->SetNACKStatus(enable ? kNackRtcp : kNackOff);
  if (enable)
    audio_coding_->EnableNack(maxNumberOfPackets);
  else
    audio_coding_->DisableNack();
}

// Called when we are missing one or more packets.
int Channel::ResendPackets(const uint16_t* sequence_numbers, int length) {
  return _rtpRtcpModule->SendNACK(sequence_numbers, length);
}

int
Channel::StartRTPDump(const char fileNameUTF8[1024],
                      RTPDirections direction)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                 "Channel::StartRTPDump()");
    if ((direction != kRtpIncoming) && (direction != kRtpOutgoing))
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRTPDump() invalid RTP direction");
        return -1;
    }
    RtpDump* rtpDumpPtr = (direction == kRtpIncoming) ?
        &_rtpDumpIn : &_rtpDumpOut;
    if (rtpDumpPtr == NULL)
    {
        assert(false);
        return -1;
    }
    if (rtpDumpPtr->IsActive())
    {
        rtpDumpPtr->Stop();
    }
    if (rtpDumpPtr->Start(fileNameUTF8) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartRTPDump() failed to create file");
        return -1;
    }
    return 0;
}

int
Channel::StopRTPDump(RTPDirections direction)
{
	return -1;
   WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
                 "Channel::StopRTPDump()");
    if ((direction != kRtpIncoming) && (direction != kRtpOutgoing))
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StopRTPDump() invalid RTP direction");
        return -1;
    }
    RtpDump* rtpDumpPtr = (direction == kRtpIncoming) ?
        &_rtpDumpIn : &_rtpDumpOut;
    if (rtpDumpPtr == NULL)
    {
        assert(false);
        return -1;
    }
    if (!rtpDumpPtr->IsActive())
    {
        return 0;
    }
    return rtpDumpPtr->Stop();
}

bool
Channel::RTPDumpIsActive(RTPDirections direction)
{
	return -1;
    if ((direction != kRtpIncoming) &&
        (direction != kRtpOutgoing))
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "RTPDumpIsActive() invalid RTP direction");
        return false;
    }
    RtpDump* rtpDumpPtr = (direction == kRtpIncoming) ?
        &_rtpDumpIn : &_rtpDumpOut;
    return rtpDumpPtr->IsActive();
}

void Channel::SetVideoEngineBWETarget(ViENetwork* vie_network,
                                      int video_channel) {
  CriticalSectionScoped cs(&_callbackCritSect);
  if (vie_network_) {
    vie_network_->Release();
    vie_network_ = NULL;
  }
  video_channel_ = -1;

  if (vie_network != NULL && video_channel != -1) {
    vie_network_ = vie_network;
    video_channel_ = video_channel;
  }
}


WebRtc_UWord32
Channel::Demultiplex(const AudioFrame& audioFrame, const AudioFrame& audioFrame2Up)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::Demultiplex()");
    _audioFrame.CopyFrom(audioFrame);

	//    sean add begin 20140708 original audio sample
	_audioFrame2Up.CopyFrom(audioFrame2Up);
//	if (_processOriginalDataFlag && _serviceCoreCallBack) {
//		if (NULL == this->_sendOriginalData) {
//			this->_sendOriginalData = (void *)malloc(733);
//
//		}
//
//		_serviceCoreCallBack->onOriginalAudioData(call_id, _audioFrame2Up.data_, _audioFrame2Up.samples_per_channel_*_audioFrame2Up.num_channels_,_audioFrame2Up.sample_rate_hz_,_audioFrame2Up.num_channels_,true);
//	}
	//    sean add end 20140708 original audio sample

    _audioFrame.id_ = _channelId;
    return 0;
}

void Channel::Demultiplex(const int16_t* audio_data,
                          int sample_rate,
                          int number_of_frames,
                          int number_of_channels) {
  CodecInst codec;
  GetSendCodec(codec);

  if (!mono_recording_audio_.get()) {
    // Temporary space for DownConvertToCodecFormat.
    mono_recording_audio_.reset(new int16_t[kMaxMonoDataSizeSamples]);
  }
  DownConvertToCodecFormat(audio_data,
                           number_of_frames,
                           number_of_channels,
                           sample_rate,
                           codec.channels,
                           codec.plfreq,
                           mono_recording_audio_.get(),
                           &input_resampler_,
                           &_audioFrame);
}

uint32_t
Channel::PrepareEncodeAndSend(int mixingFrequency)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::PrepareEncodeAndSend()");

    if (_audioFrame.samples_per_channel_ == 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,_channelId),
                     "Channel::PrepareEncodeAndSend() invalid audio frame");
        return 0xFFFFFFFF;
    }

    if (channel_state_.Get().input_file_playing)
    {
        MixOrReplaceAudioWithFile(mixingFrequency);
    }

    bool is_muted = Mute();  // Cache locally as Mute() takes a lock.
    if (is_muted) {
      AudioFrameOperations::Mute(_audioFrame);
    }

    if (channel_state_.Get().input_external_media)
    {
        CriticalSectionScoped cs(&_callbackCritSect);
        const bool isStereo = (_audioFrame.num_channels_ == 2);
        if (_inputExternalMediaCallbackPtr)
        {
            _inputExternalMediaCallbackPtr->Process(
                _channelId,
                kRecordingPerChannel,
               (int16_t*)_audioFrame.data_,
                _audioFrame.samples_per_channel_,
                _audioFrame.sample_rate_hz_,
                isStereo);
        }
    }

    InsertInbandDtmfTone();

    if (_includeAudioLevelIndication) {
      int length = _audioFrame.samples_per_channel_ * _audioFrame.num_channels_;
      if (is_muted) {
        rms_level_.ProcessMuted(length);
      } else {
        rms_level_.Process(_audioFrame.data_, length);
      }
    }

    return 0;
}

uint32_t
Channel::EncodeAndSend()
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::EncodeAndSend()");

    assert(_audioFrame.num_channels_ <= 2);
    if (_audioFrame.samples_per_channel_ == 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,_channelId),
                     "Channel::EncodeAndSend() invalid audio frame");
        return 0xFFFFFFFF;
    }

    _audioFrame.id_ = _channelId;

    // --- Add 10ms of raw (PCM) audio data to the encoder @ 32kHz.

    // The ACM resamples internally.
    _audioFrame.timestamp_ = _timeStamp;
    if (audio_coding_->Add10MsData((AudioFrame&)_audioFrame) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,_channelId),
                     "Channel::EncodeAndSend() ACM encoding failed");
        return 0xFFFFFFFF;
    }

    _timeStamp += _audioFrame.samples_per_channel_;

    // --- Encode if complete frame is ready

    // This call will trigger AudioPacketizationCallback::SendData if encoding
    // is done and payload is ready for packetization and transmission.
    return audio_coding_->Process();
}

int Channel::RegisterExternalMediaProcessing(
    ProcessingTypes type,
    VoEMediaProcess& processObject)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::RegisterExternalMediaProcessing()");

    CriticalSectionScoped cs(&_callbackCritSect);

    if (kPlaybackPerChannel == type)
    {
        if (_outputExternalMediaCallbackPtr)
        {
            _engineStatisticsPtr->SetLastError(
                VE_INVALID_OPERATION, kTraceError,
                "Channel::RegisterExternalMediaProcessing() "
                "output external media already enabled");
            return -1;
        }
        _outputExternalMediaCallbackPtr = &processObject;
        _outputExternalMedia = true;
    }
    else if (kRecordingPerChannel == type)
    {
        if (_inputExternalMediaCallbackPtr)
        {
            _engineStatisticsPtr->SetLastError(
                VE_INVALID_OPERATION, kTraceError,
                "Channel::RegisterExternalMediaProcessing() "
                "output external media already enabled");
            return -1;
        }
        _inputExternalMediaCallbackPtr = &processObject;
        channel_state_.SetInputExternalMedia(true);
    }
    return 0;
}

int Channel::DeRegisterExternalMediaProcessing(ProcessingTypes type)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::DeRegisterExternalMediaProcessing()");

    CriticalSectionScoped cs(&_callbackCritSect);

    if (kPlaybackPerChannel == type)
    {
        if (!_outputExternalMediaCallbackPtr)
        {
            _engineStatisticsPtr->SetLastError(
                VE_INVALID_OPERATION, kTraceWarning,
                "Channel::DeRegisterExternalMediaProcessing() "
                "output external media already disabled");
            return 0;
        }
        _outputExternalMedia = false;
        _outputExternalMediaCallbackPtr = NULL;
    }
    else if (kRecordingPerChannel == type)
    {
        if (!_inputExternalMediaCallbackPtr)
        {
            _engineStatisticsPtr->SetLastError(
                VE_INVALID_OPERATION, kTraceWarning,
                "Channel::DeRegisterExternalMediaProcessing() "
                "input external media already disabled");
            return 0;
        }
        channel_state_.SetInputExternalMedia(false);
        _inputExternalMediaCallbackPtr = NULL;
    }

    return 0;
}

int Channel::SetExternalMixing(bool enabled) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetExternalMixing(enabled=%d)", enabled);

    if (channel_state_.Get().playing)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceError,
            "Channel::SetExternalMixing() "
            "external mixing cannot be changed while playing.");
        return -1;
    }

    _externalMixing = enabled;

    return 0;
}

int
Channel::GetNetworkStatistics(NetworkStatistics& stats)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetNetworkStatistics()");
    ACMNetworkStatistics acm_stats;
    int return_value = audio_coding_->NetworkStatistics(&acm_stats);
    if (return_value >= 0) {
      memcpy(&stats, &acm_stats, sizeof(NetworkStatistics));
    }
    return return_value;
}

void Channel::GetDecodingCallStatistics(AudioDecodingCallStats* stats) const {
  audio_coding_->GetDecodingCallStatistics(stats);
}

bool Channel::GetDelayEstimate(int* jitter_buffer_delay_ms,
                               int* playout_buffer_delay_ms) const {
  if (_average_jitter_buffer_delay_us == 0) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetDelayEstimate() no valid estimate.");
    return false;
  }
  *jitter_buffer_delay_ms = (_average_jitter_buffer_delay_us + 500) / 1000 +
      _recPacketDelayMs;
  *playout_buffer_delay_ms = playout_delay_ms_;
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::GetDelayEstimate()");
  return true;
}

int Channel::SetInitialPlayoutDelay(int delay_ms)
{
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::SetInitialPlayoutDelay()");
  if ((delay_ms < kVoiceEngineMinMinPlayoutDelayMs) ||
      (delay_ms > kVoiceEngineMaxMinPlayoutDelayMs))
  {
    _engineStatisticsPtr->SetLastError(
        VE_INVALID_ARGUMENT, kTraceError,
        "SetInitialPlayoutDelay() invalid min delay");
    return -1;
  }
  if (audio_coding_->SetInitialPlayoutDelay(delay_ms) != 0)
  {
    _engineStatisticsPtr->SetLastError(
        VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
        "SetInitialPlayoutDelay() failed to set min playout delay");
    return -1;
  }
  return 0;
}


int
Channel::SetMinimumPlayoutDelay(int delayMs)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetMinimumPlayoutDelay()");
    if ((delayMs < kVoiceEngineMinMinPlayoutDelayMs) ||
        (delayMs > kVoiceEngineMaxMinPlayoutDelayMs))
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "SetMinimumPlayoutDelay() invalid min delay");
        return -1;
    }
    if (audio_coding_->SetMinimumPlayoutDelay(delayMs) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
            "SetMinimumPlayoutDelay() failed to set min playout delay");
        return -1;
    }
    return 0;
}

void Channel::UpdatePlayoutTimestamp(bool rtcp) {
  uint32_t playout_timestamp = 0;

  if (audio_coding_->PlayoutTimestamp(&playout_timestamp) == -1)  {
    // This can happen if this channel has not been received any RTP packet. In
    // this case, NetEq is not capable of computing playout timestamp.
    return;
  }

  uint16_t delay_ms = 0;
  if (_audioDeviceModulePtr->PlayoutDelay(&delay_ms) == -1) {
    WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::UpdatePlayoutTimestamp() failed to read playout"
                 " delay from the ADM");
    _engineStatisticsPtr->SetLastError(
        VE_CANNOT_RETRIEVE_VALUE, kTraceError,
        "UpdatePlayoutTimestamp() failed to retrieve playout delay");
    return;
  }

  jitter_buffer_playout_timestamp_ = playout_timestamp;

  // Remove the playout delay.
  playout_timestamp -= (delay_ms * (GetPlayoutFrequency() / 1000));

  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::UpdatePlayoutTimestamp() => playoutTimestamp = %lu",
               playout_timestamp);

  if (rtcp) {
    playout_timestamp_rtcp_ = playout_timestamp;
  } else {
    playout_timestamp_rtp_ = playout_timestamp;
  }
  playout_delay_ms_ = delay_ms;
}

int Channel::GetPlayoutTimestamp(unsigned int& timestamp) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::GetPlayoutTimestamp()");
  if (playout_timestamp_rtp_ == 0)  {
    //_engineStatisticsPtr->SetLastError(
    //    VE_CANNOT_RETRIEVE_VALUE, kTraceError,
    //    "GetPlayoutTimestamp() failed to retrieve timestamp");
    return -1;
  }
  timestamp = playout_timestamp_rtp_;
  WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
               VoEId(_instanceId,_channelId),
               "GetPlayoutTimestamp() => timestamp=%u", timestamp);
  return 0;
}

int Channel::SetInitTimestamp(unsigned int timestamp) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SetInitTimestamp()");
  if (channel_state_.Get().sending) {
    _engineStatisticsPtr->SetLastError(VE_SENDING, kTraceError,
                                       "SetInitTimestamp() already sending");
    return -1;
  }
  _rtpRtcpModule->SetStartTimestamp(timestamp);
  return 0;
}

int Channel::SetInitSequenceNumber(short sequenceNumber) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, _channelId),
               "Channel::SetInitSequenceNumber()");
  if (channel_state_.Get().sending) {
    _engineStatisticsPtr->SetLastError(
        VE_SENDING, kTraceError, "SetInitSequenceNumber() already sending");
    return -1;
  }
  _rtpRtcpModule->SetSequenceNumber(sequenceNumber);
  return 0;
}

int
Channel::GetRtpRtcp(RtpRtcp** rtpRtcpModule, RtpReceiver** rtp_receiver) const
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetRtpRtcp()");
    *rtpRtcpModule = _rtpRtcpModule.get();
    *rtp_receiver = rtp_receiver_.get();
    return 0;
}

// TODO(andrew): refactor Mix functions here and in transmit_mixer.cc to use
// a shared helper.
int32_t
Channel::MixOrReplaceAudioWithFile(int mixingFrequency)
{
    scoped_ptr<int16_t[]> fileBuffer(new int16_t[640]);
    int fileSamples(0);

    {
        CriticalSectionScoped cs(&_fileCritSect);

        if (_inputFilePlayerPtr == NULL)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_instanceId, _channelId),
                         "Channel::MixOrReplaceAudioWithFile() fileplayer"
                             " doesnt exist");
            return -1;
        }

        if (_inputFilePlayerPtr->Get10msAudioFromFile(fileBuffer.get(),
                                                      fileSamples,
                                                      mixingFrequency) == -1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_instanceId, _channelId),
                         "Channel::MixOrReplaceAudioWithFile() file mixing "
                         "failed");
            return -1;
        }
        if (fileSamples == 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_instanceId, _channelId),
                         "Channel::MixOrReplaceAudioWithFile() file is ended");
            return 0;
        }
    }

    assert(_audioFrame.samples_per_channel_ == fileSamples);

    if (_mixFileWithMicrophone)
    {
        // Currently file stream is always mono.
        // TODO(xians): Change the code when FilePlayer supports real stereo.
        MixWithSat(_audioFrame.data_,
                   _audioFrame.num_channels_,
                   fileBuffer.get(),
                   1,
                   fileSamples);
    }
    else
    {
        // Replace ACM audio with file.
        // Currently file stream is always mono.
        // TODO(xians): Change the code when FilePlayer supports real stereo.
        _audioFrame.UpdateFrame(_channelId,
                                0xFFFFFFFF,
                                fileBuffer.get(),
                                fileSamples,
                                mixingFrequency,
                                AudioFrame::kNormalSpeech,
                                AudioFrame::kVadUnknown,
                                1);

    }
    return 0;
}

int32_t
Channel::MixAudioWithFile(AudioFrame& audioFrame,
                          int mixingFrequency)
{
    assert(mixingFrequency <= 48000);

    scoped_ptr<int16_t[]> fileBuffer(new int16_t[960]);
    int fileSamples(0);

    {
        CriticalSectionScoped cs(&_fileCritSect);

        if (_outputFilePlayerPtr == NULL)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_instanceId, _channelId),
                         "Channel::MixAudioWithFile() file mixing failed");
            return -1;
        }

        // We should get the frequency we ask for.
        if (_outputFilePlayerPtr->Get10msAudioFromFile(fileBuffer.get(),
                                                       fileSamples,
                                                       mixingFrequency) == -1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_instanceId, _channelId),
                         "Channel::MixAudioWithFile() file mixing failed");
            return -1;
        }
    }

    if (audioFrame.samples_per_channel_ == fileSamples)
    {
        // Currently file stream is always mono.
        // TODO(xians): Change the code when FilePlayer supports real stereo.
        MixWithSat(audioFrame.data_,
                   audioFrame.num_channels_,
                   fileBuffer.get(),
                   1,
                   fileSamples);
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,_channelId),
            "Channel::MixAudioWithFile() samples_per_channel_(%d) != "
            "fileSamples(%d)",
            audioFrame.samples_per_channel_, fileSamples);
        return -1;
    }

    return 0;
}

int
Channel::InsertInbandDtmfTone()
{
    // Check if we should start a new tone.
    if (_inbandDtmfQueue.PendingDtmf() &&
        !_inbandDtmfGenerator.IsAddingTone() &&
        _inbandDtmfGenerator.DelaySinceLastTone() >
        kMinTelephoneEventSeparationMs)
    {
        int8_t eventCode(0);
        uint16_t lengthMs(0);
        uint8_t attenuationDb(0);

        eventCode = _inbandDtmfQueue.NextDtmf(&lengthMs, &attenuationDb);
        _inbandDtmfGenerator.AddTone(eventCode, lengthMs, attenuationDb);
        if (_playInbandDtmfEvent)
        {
            // Add tone to output mixer using a reduced length to minimize
            // risk of echo.
            _outputMixerPtr->PlayDtmfTone(eventCode, lengthMs - 80,
                                          attenuationDb);
        }
    }

    if (_inbandDtmfGenerator.IsAddingTone())
    {
        uint16_t frequency(0);
        _inbandDtmfGenerator.GetSampleRate(frequency);

        if (frequency != _audioFrame.sample_rate_hz_)
        {
            // Update sample rate of Dtmf tone since the mixing frequency
            // has changed.
            _inbandDtmfGenerator.SetSampleRate(
                (uint16_t) (_audioFrame.sample_rate_hz_));
            // Reset the tone to be added taking the new sample rate into
            // account.
            _inbandDtmfGenerator.ResetTone();
        }

        int16_t toneBuffer[320];
        uint16_t toneSamples(0);
        // Get 10ms tone segment and set time since last tone to zero
        if (_inbandDtmfGenerator.Get10msTone(toneBuffer, toneSamples) == -1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                       VoEId(_instanceId, _channelId),
                       "Channel::EncodeAndSend() inserting Dtmf failed");
            return -1;
        }

        // Replace mixed audio with DTMF tone.
        for (int sample = 0;
            sample < _audioFrame.samples_per_channel_;
            sample++)
        {
            for (int channel = 0;
                channel < _audioFrame.num_channels_;
                channel++)
            {
                const int index = sample * _audioFrame.num_channels_ + channel;
                _audioFrame.data_[index] = toneBuffer[sample];
            }
        }

        assert(_audioFrame.samples_per_channel_ == toneSamples);
    } else
    {
        // Add 10ms to "delay-since-last-tone" counter
        _inbandDtmfGenerator.UpdateDelaySinceLastTone();
    }
    return 0;
}

int32_t
Channel::SendPacketRaw(const void *data, size_t len, bool RTCP)
{
    CriticalSectionScoped cs(&_callbackCritSect);
    if (_transportPtr == NULL)
    {
        return -1;
    }
    if (!RTCP)
    {
        return _transportPtr->SendPacket(_channelId, data, len);
    }
    else
    {
        return _transportPtr->SendRTCPPacket(_channelId, data, len);
    }
}

// Called for incoming RTP packets after successful RTP header parsing.
void Channel::UpdatePacketDelay(uint32_t rtp_timestamp,
                                uint16_t sequence_number) {
  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
               "Channel::UpdatePacketDelay(timestamp=%lu, sequenceNumber=%u)",
               rtp_timestamp, sequence_number);

  // Get frequency of last received payload
  int rtp_receive_frequency = GetPlayoutFrequency();

  // Update the least required delay.
  least_required_delay_ms_ = audio_coding_->LeastRequiredDelayMs();

  // |jitter_buffer_playout_timestamp_| updated in UpdatePlayoutTimestamp for
  // every incoming packet.
  uint32_t timestamp_diff_ms = (rtp_timestamp -
      jitter_buffer_playout_timestamp_) / (rtp_receive_frequency / 1000);
  if (!IsNewerTimestamp(rtp_timestamp, jitter_buffer_playout_timestamp_) ||
      timestamp_diff_ms > (2 * kVoiceEngineMaxMinPlayoutDelayMs)) {
    // If |jitter_buffer_playout_timestamp_| is newer than the incoming RTP
    // timestamp, the resulting difference is negative, but is set to zero.
    // This can happen when a network glitch causes a packet to arrive late,
    // and during long comfort noise periods with clock drift.
    timestamp_diff_ms = 0;
  }

  uint16_t packet_delay_ms = (rtp_timestamp - _previousTimestamp) /
      (rtp_receive_frequency / 1000);

  _previousTimestamp = rtp_timestamp;

  if (timestamp_diff_ms == 0) return;

  if (packet_delay_ms >= 10 && packet_delay_ms <= 60) {
    _recPacketDelayMs = packet_delay_ms;
  }

  if (_average_jitter_buffer_delay_us == 0) {
    _average_jitter_buffer_delay_us = timestamp_diff_ms * 1000;
    return;
  }

  // Filter average delay value using exponential filter (alpha is
  // 7/8). We derive 1000 *_average_jitter_buffer_delay_us here (reduces
  // risk of rounding error) and compensate for it in GetDelayEstimate()
  // later.
  _average_jitter_buffer_delay_us = (_average_jitter_buffer_delay_us * 7 +
      1000 * timestamp_diff_ms + 500) / 8;
}

void
Channel::RegisterReceiveCodecsToRTPModule()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::RegisterReceiveCodecsToRTPModule()");


    CodecInst codec;
    const uint8_t nSupportedCodecs = AudioCodingModule::NumberOfCodecs();

    for (int idx = 0; idx < nSupportedCodecs; idx++)
    {
        // Open up the RTP/RTCP receiver for all supported codecs
        if ((audio_coding_->Codec(idx, &codec) == -1) ||
            (rtp_receiver_->RegisterReceivePayload(
                codec.plname,
                codec.pltype,
                codec.plfreq,
                codec.channels,
                (codec.rate < 0) ? 0 : codec.rate) == -1))
        {
            WEBRTC_TRACE(
                         kTraceWarning,
                         kTraceVoice,
                         VoEId(_instanceId, _channelId),
                         "Channel::RegisterReceiveCodecsToRTPModule() unable"
                         " to register %s (%d/%d/%d/%d) to RTP/RTCP receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }
        else
        {
            WEBRTC_TRACE(
                         kTraceInfo,
                         kTraceVoice,
                         VoEId(_instanceId, _channelId),
                         "Channel::RegisterReceiveCodecsToRTPModule() %s "
                         "(%d/%d/%d/%d) has been added to the RTP/RTCP "
                         "receiver",
                         codec.plname, codec.pltype, codec.plfreq,
                         codec.channels, codec.rate);
        }
    }
}

// Assuming this method is called with valid payload type.
int Channel::SetRedPayloadType(int red_payload_type) {
  CodecInst codec;
  bool found_red = false;

  // Get default RED settings from the ACM database
  const int num_codecs = AudioCodingModule::NumberOfCodecs();
  for (int idx = 0; idx < num_codecs; idx++) {
    audio_coding_->Codec(idx, &codec);
    if (!STR_CASE_CMP(codec.plname, "RED")) {
      found_red = true;
      break;
    }
  }

  if (!found_red) {
    _engineStatisticsPtr->SetLastError(
        VE_CODEC_ERROR, kTraceError,
        "SetRedPayloadType() RED is not supported");
    return -1;
  }

  codec.pltype = red_payload_type;
  if (audio_coding_->RegisterSendCodec(codec) < 0) {
    _engineStatisticsPtr->SetLastError(
        VE_AUDIO_CODING_MODULE_ERROR, kTraceError,
        "SetRedPayloadType() RED registration in ACM module failed");
    return -1;
  }

  if (_rtpRtcpModule->SetSendREDPayloadType(red_payload_type) != 0) {
    _engineStatisticsPtr->SetLastError(
        VE_RTP_RTCP_MODULE_ERROR, kTraceError,
        "SetRedPayloadType() RED registration in RTP/RTCP module failed");
    return -1;
  }
  return 0;
}

int Channel::SetSendRtpHeaderExtension(bool enable, RTPExtensionType type,
                                       unsigned char id) {
  int error = 0;
  _rtpRtcpModule->DeregisterSendRtpHeaderExtension(type);
  if (enable) {
    error = _rtpRtcpModule->RegisterSendRtpHeaderExtension(type, id);
  }
  return error;
}

int32_t Channel::GetPlayoutFrequency() {
  int32_t playout_frequency = audio_coding_->PlayoutFrequency();
  CodecInst current_recive_codec;
  if (audio_coding_->ReceiveCodec(&current_recive_codec) == 0) {
    if (STR_CASE_CMP("G722", current_recive_codec.plname) == 0) {
      // Even though the actual sampling rate for G.722 audio is
      // 16,000 Hz, the RTP clock rate for the G722 payload format is
      // 8,000 Hz because that value was erroneously assigned in
      // RFC 1890 and must remain unchanged for backward compatibility.
      playout_frequency = 8000;
    } else if (STR_CASE_CMP("opus", current_recive_codec.plname) == 0) {
      // We are resampling Opus internally to 32,000 Hz until all our
      // DSP routines can operate at 48,000 Hz, but the RTP clock
      // rate for the Opus payload format is standardized to 48,000 Hz,
      // because that is the maximum supported decoding sampling rate.
      playout_frequency = 48000;
    }
  }
  return playout_frequency;
}

int64_t Channel::GetRTT() const {
  RTCPMethod method = _rtpRtcpModule->RTCP();
  if (method == kRtcpOff) {
    return 0;
  }
  std::vector<RTCPReportBlock> report_blocks;
  _rtpRtcpModule->RemoteRTCPStat(&report_blocks);
  if (report_blocks.empty()) {
    return 0;
  }

  uint32_t remoteSSRC = rtp_receiver_->SSRC();
  std::vector<RTCPReportBlock>::const_iterator it = report_blocks.begin();
  for (; it != report_blocks.end(); ++it) {
    if (it->remoteSSRC == remoteSSRC)
      break;
  }
  if (it == report_blocks.end()) {
    // We have not received packets with SSRC matching the report blocks.
    // To calculate RTT we try with the SSRC of the first report block.
    // This is very important for send-only channels where we don't know
    // the SSRC of the other end.
    remoteSSRC = report_blocks[0].remoteSSRC;
  }
  int64_t rtt = 0;
  int64_t avg_rtt = 0;
  int64_t max_rtt= 0;
  int64_t min_rtt = 0;
  if (_rtpRtcpModule->RTT(remoteSSRC, &rtt, &avg_rtt, &min_rtt, &max_rtt)
      != 0) {
    return 0;
  }
  return rtt;
}
#ifndef WEBRTC_EXTERNAL_TRANSPORT
WebRtc_Word32
	Channel::SetLocalReceiver(const WebRtc_UWord16 rtpPort,
	const WebRtc_UWord16 rtcpPort,
	const char ipAddr[64],
	const char multicastIpAddr[64])
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::SetLocalReceiver()");

	if (_externalTransport)
	{
		_engineStatisticsPtr->SetLastError(
			VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
			"SetLocalReceiver() conflict with external transport");
		return -1;
	}

	if (Sending())
	{
		_engineStatisticsPtr->SetLastError(
			VE_ALREADY_SENDING, kTraceError,
			"SetLocalReceiver() already sending");
		return -1;
	}
	if (Receiving())
	{
		_engineStatisticsPtr->SetLastError(
			VE_ALREADY_LISTENING, kTraceError,
			"SetLocalReceiver() already receiving");
		return -1;
	}

	if (_socketTransportModule.InitializeReceiveSockets(this,
		rtpPort,
		ipAddr,
		multicastIpAddr,
		rtcpPort) != 0)
	{
		UdpTransport::ErrorCode lastSockError(
			_socketTransportModule.LastError());
		switch (lastSockError)
		{
		case UdpTransport::kIpAddressInvalid:
			_engineStatisticsPtr->SetLastError(
				VE_INVALID_IP_ADDRESS, kTraceError,
				"SetLocalReceiver() invalid IP address");
			break;
		case UdpTransport::kSocketInvalid:
			_engineStatisticsPtr->SetLastError(
				VE_SOCKET_ERROR, kTraceError,
				"SetLocalReceiver() invalid socket");
			break;
		case UdpTransport::kPortInvalid:
			_engineStatisticsPtr->SetLastError(
				VE_INVALID_PORT_NMBR, kTraceError,
				"SetLocalReceiver() invalid port");
			break;
		case UdpTransport::kFailedToBindPort:
			_engineStatisticsPtr->SetLastError(
				VE_BINDING_SOCKET_TO_LOCAL_ADDRESS_FAILED, kTraceError,
				"SetLocalReceiver() binding failed");
			break;
		default:
			_engineStatisticsPtr->SetLastError(
				VE_SOCKET_ERROR, kTraceError,
				"SetLocalReceiver() undefined socket error");
			break;
		}
		return -1;
	}
	return 0;
}
#endif

#ifndef WEBRTC_EXTERNAL_TRANSPORT
WebRtc_Word32
	Channel::GetLocalReceiver(int& port, int& RTCPport, char ipAddr[64])
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::GetLocalReceiver()");

	if (_externalTransport)
	{
		_engineStatisticsPtr->SetLastError(
			VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
			"SetLocalReceiver() conflict with external transport");
		return -1;
	}

	char ipAddrTmp[UdpTransport::kIpAddressVersion6Length] = {0};
	WebRtc_UWord16 rtpPort(0);
	WebRtc_UWord16 rtcpPort(0);
	char multicastIpAddr[UdpTransport::kIpAddressVersion6Length] = {0};

	// Acquire socket information from the socket module
	if (_socketTransportModule.ReceiveSocketInformation(ipAddrTmp,
		rtpPort,
		rtcpPort,
		multicastIpAddr) != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_CANNOT_GET_SOCKET_INFO, kTraceError,
			"GetLocalReceiver() unable to retrieve socket information");
		return -1;
	}

	// Deliver valid results to the user
	port = static_cast<int> (rtpPort);
	RTCPport = static_cast<int> (rtcpPort);
	if (ipAddr != NULL)
	{
		strcpy(ipAddr, ipAddrTmp);
	}
	return 0;
}
#endif

#ifndef WEBRTC_EXTERNAL_TRANSPORT
WebRtc_Word32
Channel::SetSocks5SendData(unsigned char *data, int length, bool isRTCP) {
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
            "Channel::SetSocks5SendData()");

    if (_externalTransport)
    {
        _engineStatisticsPtr->SetLastError(
                VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
                "SetSocks5SendData() conflict with external transport");
        return -1;
    }
    return _socketTransportModule.SetSocks5SendData(data, length, isRTCP);
}

WebRtc_Word32
	Channel::SetSendDestination(const WebRtc_UWord16 rtpPort, const char rtp_ipAddr[64], const int sourcePort, const WebRtc_UWord16 rtcpPort, const char rtcp_ipAddr[64])
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::SetSendDestination()");

	if (_externalTransport)
	{
		_engineStatisticsPtr->SetLastError(
			VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
			"SetSendDestination() conflict with external transport");
		return -1;
	}

	// Initialize ports and IP address for the remote (destination) side.
	// By default, the sockets used for receiving are used for transmission as
	// well, hence the source ports for outgoing packets are the same as the
	// receiving ports specified in SetLocalReceiver.
	// If an extra send socket has been created, it will be utilized until a
	// new source port is specified or until the channel has been deleted and
	// recreated. If no socket exists, sockets will be created when the first
	// RTP and RTCP packets shall be transmitted (see e.g.
	// UdpTransportImpl::SendPacket()).
	//
	// NOTE: this function does not require that sockets exists; all it does is
	// to build send structures to be used with the sockets when they exist.
	// It is therefore possible to call this method before SetLocalReceiver.
	// However, sockets must exist if a multi-cast address is given as input.

	// Build send structures and enable QoS (if enabled and supported)
	if (_socketTransportModule.InitializeSendSockets(rtp_ipAddr, rtpPort, rtcp_ipAddr, rtcpPort) != UdpTransport::kNoSocketError)
	{
		UdpTransport::ErrorCode lastSockError(
			_socketTransportModule.LastError());
		switch (lastSockError)
		{
		case UdpTransport::kIpAddressInvalid:
			_engineStatisticsPtr->SetLastError(
				VE_INVALID_IP_ADDRESS, kTraceError,
				"SetSendDestination() invalid IP address 1");
			break;
		case UdpTransport::kSocketInvalid:
			_engineStatisticsPtr->SetLastError(
				VE_SOCKET_ERROR, kTraceError,
				"SetSendDestination() invalid socket 1");
			break;
		case UdpTransport::kQosError:
			_engineStatisticsPtr->SetLastError(
				VE_GQOS_ERROR, kTraceError,
				"SetSendDestination() failed to set QoS");
			break;
		case UdpTransport::kMulticastAddressInvalid:
			_engineStatisticsPtr->SetLastError(
				VE_INVALID_MULTICAST_ADDRESS, kTraceError,
				"SetSendDestination() invalid multicast address");
			break;
		default:
			_engineStatisticsPtr->SetLastError(
				VE_SOCKET_ERROR, kTraceError,
				"SetSendDestination() undefined socket error 1");
			break;
		}
		return -1;
	}

	// Check if the user has specified a non-default source port different from
	// the local receive port.
	// If so, an extra local socket will be created unless the source port is
	// not unique.
	if (sourcePort != kVoEDefault)
	{
		WebRtc_UWord16 receiverRtpPort(0);
		WebRtc_UWord16 rtcpNA(0);
		if (_socketTransportModule.ReceiveSocketInformation(NULL,
			receiverRtpPort,
			rtcpNA,
			NULL) != 0)
		{
			_engineStatisticsPtr->SetLastError(
				VE_CANNOT_GET_SOCKET_INFO, kTraceError,
				"SetSendDestination() failed to retrieve socket information");
			return -1;
		}

		WebRtc_UWord16 sourcePortUW16 =
			static_cast<WebRtc_UWord16> (sourcePort);

		// An extra socket will only be created if the specified source port
		// differs from the local receive port.
		if (sourcePortUW16 != receiverRtpPort)
		{
			// Initialize extra local socket to get a different source port
			// than the local
			// receiver port. Always use default source for RTCP.
			// Note that, this calls UdpTransport::CloseSendSockets().
			if (_socketTransportModule.InitializeSourcePorts(
				sourcePortUW16,
				sourcePortUW16+1) != 0)
			{
				UdpTransport::ErrorCode lastSockError(
					_socketTransportModule.LastError());
				switch (lastSockError)
				{
				case UdpTransport::kIpAddressInvalid:
					_engineStatisticsPtr->SetLastError(
						VE_INVALID_IP_ADDRESS, kTraceError,
						"SetSendDestination() invalid IP address 2");
					break;
				case UdpTransport::kSocketInvalid:
					_engineStatisticsPtr->SetLastError(
						VE_SOCKET_ERROR, kTraceError,
						"SetSendDestination() invalid socket 2");
					break;
				default:
					_engineStatisticsPtr->SetLastError(
						VE_SOCKET_ERROR, kTraceError,
						"SetSendDestination() undefined socket error 2");
					break;
				}
				return -1;
			}
			WEBRTC_TRACE(kTraceInfo, kTraceVoice,
				VoEId(_instanceId,_channelId),
				"SetSendDestination() extra local socket is created"
				" to facilitate unique source port");
		}
		else
		{
			WEBRTC_TRACE(kTraceInfo, kTraceVoice,
				VoEId(_instanceId,_channelId),
				"SetSendDestination() sourcePort equals the local"
				" receive port => no extra socket is created");
		}
	}

	return 0;
}
#endif

#ifndef WEBRTC_EXTERNAL_TRANSPORT
WebRtc_Word32
	Channel::GetSendDestination(int& port,
	char ipAddr[64],
	int& sourcePort,
	int& RTCPport)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::GetSendDestination()");

	if (_externalTransport)
	{
		_engineStatisticsPtr->SetLastError(
			VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
			"GetSendDestination() conflict with external transport");
		return -1;
	}

	char ipAddrTmp[UdpTransport::kIpAddressVersion6Length] = {0};
	WebRtc_UWord16 rtpPort(0);
	WebRtc_UWord16 rtcpPort(0);
	WebRtc_UWord16 rtpSourcePort(0);
	WebRtc_UWord16 rtcpSourcePort(0);

	// Acquire sending socket information from the socket module
	_socketTransportModule.SendSocketInformation(ipAddrTmp, rtpPort, rtcpPort);
	_socketTransportModule.SourcePorts(rtpSourcePort, rtcpSourcePort);

	// Deliver valid results to the user
	port = static_cast<int> (rtpPort);
	RTCPport = static_cast<int> (rtcpPort);
	sourcePort = static_cast<int> (rtpSourcePort);
	if (ipAddr != NULL)
	{
		strcpy(ipAddr, ipAddrTmp);
	}

	return 0;
}
#endif

void
	Channel::IncomingRTCPPacket(const WebRtc_Word8* incomingRtcpPacket,
	const WebRtc_Word32 rtcpPacketLength,
	const char* fromIP,
	const WebRtc_UWord16 fromPort)
{
	WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::IncomingRTCPPacket(rtcpPacketLength=%d, fromIP=%s,"
		" fromPort=%u)",
		rtcpPacketLength, fromIP, fromPort);

	{
		CriticalSectionScoped cs(critsect_net_statistic.get());
		if(_startNetworkTime == 0)
			_startNetworkTime = time(NULL);
		if (_isWifi) {
			_recvDataTotalWifi += rtcpPacketLength;
			_recvDataTotalWifi += 42;//14 + 20 + 8; //ethernet+ip+udp header
		}
		else
		{
			_recvDataTotalSim += rtcpPacketLength;
			_recvDataTotalSim += 42;//14 + 20 + 8; //ethernet+ip+udp header
		}
	}

	// Temporary buffer pointer and size for decryption
	WebRtc_UWord8* rtcpBufferPtr = (WebRtc_UWord8*)incomingRtcpPacket;
	WebRtc_Word32 rtcpBufferLength = rtcpPacketLength;

	// Store playout timestamp for the received RTCP packet
	// which will be read by the GetRemoteRTCPData API
	WebRtc_UWord32 playoutTimestamp(0);
	if (GetPlayoutTimestamp(playoutTimestamp) == 0)
	{
		playout_timestamp_rtcp_ = playoutTimestamp;
	}

	//    unsigned char *traceRtcpBuf = (unsigned char *)incomingRtcpPacket;
	//    printf("Here incomingRTCPPacket\n");
	//    for (int i=0; i<rtcpPacketLength; i++) {
	//        printf("%02X ",*(traceRtcpBuf+i));
	//    }
	//    printf("\n");


	rtp_header_t *rtp;
	//Sean ice for STUN Message
	if ( _stun_cb && rtcpPacketLength>=12 ) //rtp header
	{
		rtp = (rtp_header_t*)incomingRtcpPacket;
		if (rtp->version!=2)
		{
			unsigned short stunlen = *((unsigned short *)(incomingRtcpPacket + sizeof(unsigned short)));
			stunlen = ntohs(stunlen);
			if (stunlen + 20 ==rtcpPacketLength) {
				_stun_cb(_channelId, (void*)incomingRtcpPacket, rtcpPacketLength, fromIP, fromPort, true, false);
				return;

			}
		}
	}

	// SRTP or External decryption
	//if (_decrypting)
	if(false) //hubin 2017.2.18  we don't support rtcp srtp.
	{
		CriticalSectionScoped cs(&_callbackCritSect);

		if (_encryptionPtr)
		{
			if (!_decryptionRTCPBufferPtr)
			{
				WEBRTC_TRACE(kTraceError, kTraceVoice,
					VoEId(_instanceId, _channelId),
					"Channel::IncomingRTPPacket() _decryptionRTCPBufferPtr is NULL");
				return;
			}

			// Perform decryption (SRTP or external).
			WebRtc_Word32 decryptedBufferLength = rtcpBufferLength;
			_encryptionPtr->decrypt_rtcp(_channelId,
				rtcpBufferPtr,
				_decryptionRTCPBufferPtr,
				rtcpBufferLength,
				(int*)&decryptedBufferLength);
			if (decryptedBufferLength <= 0)
			{
				_engineStatisticsPtr->SetLastError(
					VE_DECRYPTION_FAILED, kTraceError,
					"Channel::IncomingRTCPPacket() decryption failed");
				return;
			}

			// Replace default data buffer with decrypted buffer
			rtcpBufferPtr = _decryptionRTCPBufferPtr;
			rtcpBufferLength = decryptedBufferLength;
		}
	}

	// Dump the RTCP packet to a file (if RTP dump is enabled).
	if (_rtpDumpIn.DumpPacket(rtcpBufferPtr,
		(WebRtc_UWord16)rtcpBufferLength) == -1)
	{
		WEBRTC_TRACE(kTraceWarning, kTraceVoice,
			VoEId(_instanceId,_channelId),
			"Channel::SendPacket() RTCP dump to input file failed");
	}

	// Deliver RTCP packet to RTP/RTCP module for parsing
	if (_rtpRtcpModule->IncomingRtcpPacket((const WebRtc_UWord8*)rtcpBufferPtr,
		(WebRtc_UWord16)rtcpBufferLength) == -1)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceWarning,
			"Channel::IncomingRTPPacket() RTCP packet is invalid");
		return;
	}
}

void
	Channel::IncomingRTPPacket(const WebRtc_Word8* incomingRtpPacket,
	const WebRtc_Word32 rtpPacketLength,
	const char* fromIP,
	const WebRtc_UWord16 fromPort)
{
	{
		CriticalSectionScoped cs(critsect_net_statistic.get());
		if(_startNetworkTime == 0)
			_startNetworkTime = time(NULL);
		if (_isWifi) {
			_recvDataTotalWifi += rtpPacketLength;
			_recvDataTotalWifi += 42;//20 + 14 +8; //ethernet+ip+udp header
		}
		else
		{
			_recvDataTotalSim += rtpPacketLength;
			_recvDataTotalSim += 42;//20 + 14 +8; //ethernet+ip+udp header
		}
	}


	bool dtmfret = handleRFC2833(incomingRtpPacket, rtpPacketLength);
	if (dtmfret) {
		return;
	}

	WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::IncomingRTPPacket(rtpPacketLength=%d,"
		" fromIP=%s, fromPort=%u)",
		rtpPacketLength, fromIP, fromPort);

	static time_t last = 0;
	if(time(NULL) > last+5 ) {
		WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_instanceId,_channelId),
			"Channel::IncomingRTPPacket(rtpPacketLength=%d,"
			" fromIP=%s, fromPort=%u)",
			rtpPacketLength, fromIP, fromPort);
	}
	last =time(NULL);

	// Store playout timestamp for the received RTP packet
	// to be used for upcoming delay estimations
	WebRtc_UWord32 playoutTimestamp(0);
	if (GetPlayoutTimestamp(playoutTimestamp) == 0)
	{
		playout_timestamp_rtp_ = playoutTimestamp;
	}

	WebRtc_UWord8* rtpBufferPtr = (WebRtc_UWord8*)incomingRtpPacket;
	WebRtc_Word32 rtpBufferLength = rtpPacketLength;
	rtp_header_t *rtp;

	//Sean ice for STUN Message
	if ( _stun_cb && rtpBufferLength>=12 ) //rtp header
	{
		rtp = (rtp_header_t*)rtpBufferPtr;
		if (rtp->version!=2)
		{
			unsigned short stunlen = *((unsigned short *)(rtpBufferPtr + sizeof(unsigned short)));
			stunlen = ntohs(stunlen);
			if (stunlen + 20 ==rtpBufferLength) {
				//lock
				_stun_cb(_channelId, (void*)incomingRtpPacket, rtpPacketLength, fromIP, fromPort, false, false);
				//unlock
				return;
			}
		}
	}

	// SRTP or External decryption

	if (_decrypting)
	{
		CriticalSectionScoped cs(&_callbackCritSect);
		if (_encryptionPtr)
		{
			if (!_decryptionRTPBufferPtr)
			{
				WEBRTC_TRACE(kTraceError, kTraceVoice,
					VoEId(_instanceId, _channelId),
					"Channel::IncomingRTPPacket() _decryptionRTPBufferPtr is NULL");
				return;
			}

			// Perform decryption (SRTP or external)
			//put the last 4 bytes to rtp header ssrc to restore ssrc that FreeSwitch has changed
			//memcpy(rtpBufferPtr+8, rtpBufferPtr+rtpBufferLength-4, 4);
			//rtpBufferLength -= 4;

			WebRtc_Word32 decryptedBufferLength = 0;
			_encryptionPtr->decrypt(_channelId,
				rtpBufferPtr,
				_decryptionRTPBufferPtr,
				rtpBufferLength,
				(int*)&decryptedBufferLength);

			if (decryptedBufferLength <= 0)
			{
				WEBRTC_TRACE(kTraceDebug, kTraceVoice, VoEId(_instanceId,_channelId),"Channel::IncomingRTPPacket() decryption failed  decryptedBufferLength = %d\n",decryptedBufferLength);
				_engineStatisticsPtr->SetLastError(
					VE_DECRYPTION_FAILED, kTraceError,
					"Channel::IncomingRTPPacket() decryption failed");
				return;
			}

			// Replace default data buffer with decrypted buffer
			rtpBufferPtr = _decryptionRTPBufferPtr;
			rtpBufferLength = decryptedBufferLength;
		}
	}

	int backDataLen = 0;
	if (_audio_data_cb /*&& _processDataFlag*/ && rtpBufferLength > 12) {
		if (NULL == this->_receiveData) {
			this->_receiveData = (void *)malloc(733);
		}
		_audio_data_cb(_channelId, rtpBufferPtr+12, rtpBufferLength-12, (WebRtc_UWord8 *)this->_receiveData+12, backDataLen, false);
		memcpy(this->_receiveData, rtpBufferPtr, 12);
		rtpBufferPtr = (WebRtc_UWord8*)this->_receiveData;
		rtpBufferLength = backDataLen+12;
	}
	
	// Dump the RTP packet to a file (if RTP dump is enabled).
	if (_rtpDumpIn.DumpPacket(rtpBufferPtr,
		(WebRtc_UWord16)rtpBufferLength) == -1)
	{
		WEBRTC_TRACE(kTraceWarning, kTraceVoice,
			VoEId(_instanceId,_channelId),
			"Channel::SendPacket() RTP dump to input file failed");
	}

	//---begin
	const uint8_t* received_packet = reinterpret_cast<const uint8_t*>(rtpBufferPtr);
	RTPHeader header;
	/*if (!rtp_header_parser_->Parse(received_packet, rtpPacketLength, &header)) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceDebug, cloopenwebrtc::kTraceVoice, _channelId,
			"Incoming packet: invalid RTP header");
		return -1;
	}*/

	rtp_header_parser_->Parse(received_packet, rtpBufferLength, &header);
	header.payload_type_frequency =
		rtp_payload_registry_->GetPayloadTypeFrequency(header.payloadType);
	if (header.payload_type_frequency < 0)
		return ;
	bool in_order = IsPacketInOrder(header);
	rtp_receive_statistics_->IncomingPacket(header, rtpBufferLength,
		IsPacketRetransmitted(header, in_order));
	rtp_payload_registry_->SetIncomingPayloadType(header);

	if(ReceivePacket(received_packet, rtpBufferLength, header, in_order) ==  false)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceWarning,
			"Channel::IncomingRTPPacket() RTP packet is invalid");
		return;
	}
}

int
	Channel::setProcessData(bool flag, bool originalFlag)
{
	WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::setProcessData(flag=%d)", flag);

	CriticalSectionScoped cs(&_fileCritSect);

	_processDataFlag = flag;
	_processOriginalDataFlag = originalFlag;
	return 1;
}

WebRtc_UWord32
	Channel::pause(bool mute)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::pause()");
	_pause = mute;
	return 0;
}

void Channel::getNetworkStatistic(time_t &startTime,  long long &sendLengthSim,  long long &recvLengthSim, long long &sendLengthWifi,  long long &recvLengthWifi)
{
	CriticalSectionScoped cs(critsect_net_statistic.get());
	startTime = _startNetworkTime;
	sendLengthSim = _sendDataTotalSim;
	recvLengthSim = _recvDataTotalSim;
	sendLengthWifi = _sendDataTotalWifi;
	recvLengthWifi = _recvDataTotalWifi;
}

//sean add begin 20141224 set network type
int Channel::setNetworkType(bool isWifi)
{
	_isWifi = isWifi;
	return 0;
}
//sean add end 20141224 set network type

//WebRtc_Word32
//	Channel::RegisterServiceCoreCallBack(ServiceCoreCallBack * serviceCoreCallBack, const char* call_id, int firewall_policy)
//{
//	this->_serviceCoreCallBack = serviceCoreCallBack;
//	sprintf(this->call_id, "%s", call_id);
//	this->_firewall_policy = firewall_policy;
//	return 0;
//}
//WebRtc_Word32
//	Channel::DeRegisterServiceCoreCallBack()
//{
//	_serviceCoreCallBack = NULL;
//	return 0;
//}

int
	Channel::RegisterExternalEncryption(Encryption& encryption)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::RegisterExternalEncryption()");

	CriticalSectionScoped cs(&_callbackCritSect);

	if (_encryptionPtr)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_OPERATION, kTraceError,
			"RegisterExternalEncryption() encryption already enabled");
		return -1;
	}

	_encryptionPtr = &encryption;

	_decrypting = true;
	_encrypting = true;

	return 0;
}

int
	Channel::DeRegisterExternalEncryption()
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::DeRegisterExternalEncryption()");

	CriticalSectionScoped cs(&_callbackCritSect);

	if (!_encryptionPtr)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_OPERATION, kTraceWarning,
			"DeRegisterExternalEncryption() encryption already disabled");
		return 0;
	}

	_decrypting = false;
	_encrypting = false;

	_encryptionPtr = NULL;

	return 0;
}

bool Channel::handleRFC2833(const WebRtc_Word8 *packet,const WebRtc_Word32 packetlen)
{
	rtp_header_t *rtpheader = (rtp_header_t *)packet;
	WebRtc_Word8 *pData = (WebRtc_Word8* )packet;
	char rfc2833Char[]="0123456789*#ABCDF";
	char rfcChar;
	bool end = false;

	if (rtpheader->paytype != _recvTelephoneEventPayloadType || packetlen < 16)
		return false;

	_dtmfTimeStampRTP = rtpheader->timestamp;
	pData += 12;
	end = pData[1]&0x80? 1 : 0;

	//    if (!end)
	//        return false;
	//    else
	//    {
	if (_dtmfTimeStampRTP == _lastdtmfTimeStampRTP)
		return false;
	else
	{
		if (pData[0]>-1 && pData[0]<sizeof(rfc2833Char))
		{
			rfcChar = rfc2833Char[pData[0]];
			//callback
//			if (_serviceCoreCallBack) {
//				_serviceCoreCallBack->onDtmf(call_id,rfcChar);
//			}
            if (_dtmf_cb) {
                _dtmf_cb(_channelId, rfcChar);
            }
			_lastdtmfTimeStampRTP = _dtmfTimeStampRTP;
		}
	}


	//    }
	return true;

}
    
int Channel::setDtmfCb(onReceivingDtmf dtmf_cb)
{
    _dtmf_cb = dtmf_cb;
    return 0;
}

int Channel::setStunCb(onStunPacket stun_cb)
{
    _stun_cb = stun_cb;
    return 0;
}

int Channel::setAudioDataCb(onAudioData audio_data_cb)
{
    _audio_data_cb = audio_data_cb;
    return 0;
}

int Channel::setMediaTimeoutCb(onMediaPacketTimeout media_timeout_cb)
{
    _media_timeout_cb = media_timeout_cb;
    return 0;
}

WebRtc_Word32
	Channel::SendUDPPacket(const void* data,
	unsigned int length,
	int& transmittedBytes,
	bool useRtcpSocket)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::SendUDPPacket()");
	if (_externalTransport)
	{
		_engineStatisticsPtr->SetLastError(
			VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
			"SendUDPPacket() external transport is enabled");
		return -1;
	}
	if (useRtcpSocket && !_rtpRtcpModule->RTCP())
	{
		_engineStatisticsPtr->SetLastError(
			VE_RTCP_ERROR, kTraceError,
			"SendUDPPacket() RTCP is disabled");
		return -1;
	}
	if (channel_state_.Get().sending == false)
	{
		_engineStatisticsPtr->SetLastError(
			VE_NOT_SENDING, kTraceError,
			"SendUDPPacket() not sending");
		return -1;
	}

	char* dataC = new char[length];
	if (NULL == dataC)
	{
		_engineStatisticsPtr->SetLastError(
			VE_NO_MEMORY, kTraceError,
			"SendUDPPacket() memory allocation failed");
		return -1;
	}
	memcpy(dataC, data, length);

	transmittedBytes = SendPacketRaw(dataC, length, useRtcpSocket);

	delete [] dataC;
	dataC = NULL;

	if (transmittedBytes <= 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SEND_ERROR, kTraceError,
			"SendUDPPacket() transmission failed");
		transmittedBytes = 0;
		return -1;
	}
	WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"SendUDPPacket() => transmittedBytes=%d", transmittedBytes);
	{
		CriticalSectionScoped cs(critsect_net_statistic.get());
		if(_startNetworkTime == 0)
			_startNetworkTime = time(NULL);
		//		_sendDataTotal += length;
		//		_sendDataTotal += 8;// ip+udp header
	}
	return 0;
}

#ifndef WEBRTC_EXTERNAL_TRANSPORT
WebRtc_Word32
	Channel::GetSourceInfo(int& rtpPort, int& rtcpPort, char ipAddr[64])
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::GetSourceInfo()");

	WebRtc_UWord16 rtpPortModule;
	WebRtc_UWord16 rtcpPortModule;
	char ipaddr[UdpTransport::kIpAddressVersion6Length] = {0};

	if (_socketTransportModule.RemoteSocketInformation(ipaddr,
		rtpPortModule,
		rtcpPortModule) != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceError,
			"GetSourceInfo() failed to retrieve remote socket information");
		return -1;
	}
	strcpy(ipAddr, ipaddr);
	rtpPort = rtpPortModule;
	rtcpPort = rtcpPortModule;

	WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"GetSourceInfo() => rtpPort=%d, rtcpPort=%d, ipAddr=%s",
		rtpPort, rtcpPort, ipAddr);
	return 0;
}

WebRtc_Word32
	Channel::EnableIPv6()
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::EnableIPv6()");
	if (_socketTransportModule.ReceiveSocketsInitialized() ||
		_socketTransportModule.SendSocketsInitialized())
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_OPERATION, kTraceError,
			"EnableIPv6() socket layer is already initialized");
		return -1;
	}
	if (_socketTransportModule.EnableIpV6() != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_ERROR, kTraceError,
			"EnableIPv6() failed to enable IPv6");
		const UdpTransport::ErrorCode lastError =
			_socketTransportModule.LastError();
		WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
			"UdpTransport::LastError() => %d", lastError);
		return -1;
	}
	return 0;
}

bool
	Channel::IPv6IsEnabled() const
{
	bool isEnabled = _socketTransportModule.IpV6Enabled();
	WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"IPv6IsEnabled() => %d", isEnabled);
	return isEnabled;
}

WebRtc_Word32
	Channel::SetSourceFilter(int rtpPort, int rtcpPort, const char ipAddr[64])
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::SetSourceFilter()");
	if (_socketTransportModule.SetFilterPorts(
		static_cast<WebRtc_UWord16>(rtpPort),
		static_cast<WebRtc_UWord16>(rtcpPort)) != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceError,
			"SetSourceFilter() failed to set filter ports");
		const UdpTransport::ErrorCode lastError =
			_socketTransportModule.LastError();
		WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
			"UdpTransport::LastError() => %d",
			lastError);
		return -1;
	}
	const char* filterIpAddress = ipAddr;
	if (_socketTransportModule.SetFilterIP(filterIpAddress) != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_IP_ADDRESS, kTraceError,
			"SetSourceFilter() failed to set filter IP address");
		const UdpTransport::ErrorCode lastError =
			_socketTransportModule.LastError();
		WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
			"UdpTransport::LastError() => %d", lastError);
		return -1;
	}
	return 0;
}

WebRtc_Word32
	Channel::GetSourceFilter(int& rtpPort, int& rtcpPort, char ipAddr[64])
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::GetSourceFilter()");
	WebRtc_UWord16 rtpFilterPort(0);
	WebRtc_UWord16 rtcpFilterPort(0);
	if (_socketTransportModule.FilterPorts(rtpFilterPort, rtcpFilterPort) != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceWarning,
			"GetSourceFilter() failed to retrieve filter ports");
	}
	char ipAddrTmp[UdpTransport::kIpAddressVersion6Length] = {0};
	if (_socketTransportModule.FilterIP(ipAddrTmp) != 0)
	{
		// no filter has been configured (not seen as an error)
		memset(ipAddrTmp,
			0, UdpTransport::kIpAddressVersion6Length);
	}
	rtpPort = static_cast<int> (rtpFilterPort);
	rtcpPort = static_cast<int> (rtcpFilterPort);
	strcpy(ipAddr, ipAddrTmp);
	WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"GetSourceFilter() => rtpPort=%d, rtcpPort=%d, ipAddr=%s",
		rtpPort, rtcpPort, ipAddr);
	return 0;
}

WebRtc_Word32
	Channel::SetSendTOS(int DSCP, int priority, bool useSetSockopt)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::SetSendTOS(DSCP=%d, useSetSockopt=%d)",
		DSCP, (int)useSetSockopt);

	// Set TOS value and possibly try to force usage of setsockopt()
	if (_socketTransportModule.SetToS(DSCP, useSetSockopt) != 0)
	{
		UdpTransport::ErrorCode lastSockError(
			_socketTransportModule.LastError());
		switch (lastSockError)
		{
		case UdpTransport::kTosError:
			_engineStatisticsPtr->SetLastError(VE_TOS_ERROR, kTraceError,
				"SetSendTOS() TOS error");
			break;
		case UdpTransport::kQosError:
			_engineStatisticsPtr->SetLastError(
				VE_TOS_GQOS_CONFLICT, kTraceError,
				"SetSendTOS() GQOS error");
			break;
		case UdpTransport::kTosInvalid:
			// can't switch SetSockOpt method without disabling TOS first, or
			// SetSockopt() call failed
			_engineStatisticsPtr->SetLastError(VE_TOS_INVALID, kTraceError,
				"SetSendTOS() invalid TOS");
			break;
		case UdpTransport::kSocketInvalid:
			_engineStatisticsPtr->SetLastError(VE_SOCKET_ERROR, kTraceError,
				"SetSendTOS() invalid Socket");
			break;
		default:
			_engineStatisticsPtr->SetLastError(VE_TOS_ERROR, kTraceError,
				"SetSendTOS() TOS error");
			break;
		}
		WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,_channelId),
			"UdpTransport =>  lastError = %d",
			lastSockError);
		return -1;
	}

	// Set priority (PCP) value, -1 means don't change
	if (-1 != priority)
	{
		if (_socketTransportModule.SetPCP(priority) != 0)
		{
			UdpTransport::ErrorCode lastSockError(
				_socketTransportModule.LastError());
			switch (lastSockError)
			{
			case UdpTransport::kPcpError:
				_engineStatisticsPtr->SetLastError(VE_TOS_ERROR, kTraceError,
					"SetSendTOS() PCP error");
				break;
			case UdpTransport::kQosError:
				_engineStatisticsPtr->SetLastError(
					VE_TOS_GQOS_CONFLICT, kTraceError,
					"SetSendTOS() GQOS conflict");
				break;
			case UdpTransport::kSocketInvalid:
				_engineStatisticsPtr->SetLastError(
					VE_SOCKET_ERROR, kTraceError,
					"SetSendTOS() invalid Socket");
				break;
			default:
				_engineStatisticsPtr->SetLastError(VE_TOS_ERROR, kTraceError,
					"SetSendTOS() PCP error");
				break;
			}
			WEBRTC_TRACE(kTraceError, kTraceVoice,
				VoEId(_instanceId,_channelId),
				"UdpTransport =>  lastError = %d",
				lastSockError);
			return -1;
		}
	}

	return 0;
}

WebRtc_Word32
	Channel::GetSendTOS(int &DSCP, int& priority, bool &useSetSockopt)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::GetSendTOS(DSCP=?, useSetSockopt=?)");
	WebRtc_Word32 dscp(0), prio(0);
	bool setSockopt(false);
	if (_socketTransportModule.ToS(dscp, setSockopt) != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceError,
			"GetSendTOS() failed to get TOS info");
		return -1;
	}
	if (_socketTransportModule.PCP(prio) != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceError,
			"GetSendTOS() failed to get PCP info");
		return -1;
	}
	DSCP = static_cast<int> (dscp);
	priority = static_cast<int> (prio);
	useSetSockopt = setSockopt;
	WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
		"GetSendTOS() => DSCP=%d, priority=%d, useSetSockopt=%d",
		DSCP, priority, (int)useSetSockopt);
	return 0;
}

#if defined(_WIN32)
WebRtc_Word32
	Channel::SetSendGQoS(bool enable, int serviceType, int overrideDSCP)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::SetSendGQoS(enable=%d, serviceType=%d, "
		"overrideDSCP=%d)",
		(int)enable, serviceType, overrideDSCP);
	if(!_socketTransportModule.ReceiveSocketsInitialized())
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKETS_NOT_INITED, kTraceError,
			"SetSendGQoS() GQoS state must be set after sockets are created");
		return -1;
	}
	if(!_socketTransportModule.SendSocketsInitialized())
	{
		_engineStatisticsPtr->SetLastError(
			VE_DESTINATION_NOT_INITED, kTraceError,
			"SetSendGQoS() GQoS state must be set after sending side is "
			"initialized");
		return -1;
	}
	if (enable &&
		(serviceType != SERVICETYPE_BESTEFFORT) &&
		(serviceType != SERVICETYPE_CONTROLLEDLOAD) &&
		(serviceType != SERVICETYPE_GUARANTEED) &&
		(serviceType != SERVICETYPE_QUALITATIVE))
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_ARGUMENT, kTraceError,
			"SetSendGQoS() Invalid service type");
		return -1;
	}
	if (enable && ((overrideDSCP <  0) || (overrideDSCP > 63)))
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_ARGUMENT, kTraceError,
			"SetSendGQoS() Invalid overrideDSCP value");
		return -1;
	}

	// Avoid GQoS/ToS conflict when user wants to override the default DSCP
	// mapping
	bool QoS(false);
	WebRtc_Word32 sType(0);
	WebRtc_Word32 ovrDSCP(0);
	if (_socketTransportModule.QoS(QoS, sType, ovrDSCP))
	{
		_engineStatisticsPtr->SetLastError(
			VE_SOCKET_TRANSPORT_MODULE_ERROR, kTraceError,
			"SetSendGQoS() failed to get QOS info");
		return -1;
	}
	if (QoS && ovrDSCP == 0 && overrideDSCP != 0)
	{
		_engineStatisticsPtr->SetLastError(
			VE_TOS_GQOS_CONFLICT, kTraceError,
			"SetSendGQoS() QOS is already enabled and overrideDSCP differs,"
			" not allowed");
		return -1;
	}
	const WebRtc_Word32 maxBitrate(0);
	if (_socketTransportModule.SetQoS(enable,
		static_cast<WebRtc_Word32>(serviceType),
		maxBitrate,
		static_cast<WebRtc_Word32>(overrideDSCP),
		true))
	{
		UdpTransport::ErrorCode lastSockError(
			_socketTransportModule.LastError());
		switch (lastSockError)
		{
		case UdpTransport::kQosError:
			_engineStatisticsPtr->SetLastError(VE_GQOS_ERROR, kTraceError,
				"SetSendGQoS() QOS error");
			break;
		default:
			_engineStatisticsPtr->SetLastError(VE_SOCKET_ERROR, kTraceError,
				"SetSendGQoS() Socket error");
			break;
		}
		WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,_channelId),
			"UdpTransport() => lastError = %d",
			lastSockError);
		return -1;
	}
	return 0;
}
#endif

#if defined(_WIN32)
WebRtc_Word32
	Channel::GetSendGQoS(bool &enabled, int &serviceType, int &overrideDSCP)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::GetSendGQoS(enable=?, serviceType=?, "
		"overrideDSCP=?)");

	bool QoS(false);
	WebRtc_Word32 serviceTypeModule(0);
	WebRtc_Word32 overrideDSCPModule(0);
	_socketTransportModule.QoS(QoS, serviceTypeModule, overrideDSCPModule);

	enabled = QoS;
	serviceType = static_cast<int> (serviceTypeModule);
	overrideDSCP = static_cast<int> (overrideDSCPModule);

	WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"GetSendGQoS() => enabled=%d, serviceType=%d, overrideDSCP=%d",
		(int)enabled, serviceType, overrideDSCP);
	return 0;
}
#endif
#endif


WebRtc_Word32
Channel::SetPacketTimeoutNotification(bool enable, int timeoutSeconds)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::SetPacketTimeoutNotification()");
    if (enable)
    {
        const WebRtc_UWord32 RTPtimeoutMS = 1000*timeoutSeconds;
        const WebRtc_UWord32 RTCPtimeoutMS = 0;
        rtp_receiver_->SetPacketTimeout(RTPtimeoutMS);
        _rtpPacketTimeOutIsEnabled = true;
        _rtpTimeOutSeconds = timeoutSeconds;
    }
    else
    {
        rtp_receiver_->SetPacketTimeout(0);
        _rtpPacketTimeOutIsEnabled = false;
        _rtpTimeOutSeconds = 0;
    }
    return 0;
}

WebRtc_Word32
Channel::GetPacketTimeoutNotification(bool& enabled, int& timeoutSeconds)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::GetPacketTimeoutNotification()");
    enabled = _rtpPacketTimeOutIsEnabled;
    if (enabled)
    {
        timeoutSeconds = _rtpTimeOutSeconds;
    }
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "GetPacketTimeoutNotification() => enabled=%d,"
                 " timeoutSeconds=%d",
                 enabled, timeoutSeconds);
    return 0;
}

void
Channel::OnPacketTimeout(const WebRtc_Word32 id)
{
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::OnPacketTimeout(id=%d)", id);

    CriticalSectionScoped cs(_callbackCritSectPtr);
    if (_voiceEngineObserverPtr)
    {
        if (channel_state_.Get().receiving || _externalTransport)
        {
            WebRtc_Word32 channel = VoEChannelId(id);
            assert(channel == _channelId);
            // Ensure that next OnReceivedPacket() callback will trigger
            // a VE_PACKET_RECEIPT_RESTARTED callback.
            _rtpPacketTimedOut = true;
            // Deliver callback to the observer
            WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                         VoEId(_instanceId,_channelId),
                         "Channel::OnPacketTimeout() => "
                         "CallbackOnError(VE_RECEIVE_PACKET_TIMEOUT)");
            _voiceEngineObserverPtr->CallbackOnError(channel,
                                                     VE_RECEIVE_PACKET_TIMEOUT);
        }
    }
    if (_media_timeout_cb)
    {
        if (channel_state_.Get().receiving || _externalTransport)
        {
            WebRtc_Word32 channel = VoEChannelId(id);
            assert(channel == _channelId);
            // Ensure that next OnReceivedPacket() callback will trigger
            // a VE_PACKET_RECEIPT_RESTARTED callback.
            _rtpPacketTimedOut = true;
            // Deliver callback to the observer
            WEBRTC_TRACE(kTraceStateInfo, kTraceVoice,
                         VoEId(_instanceId,_channelId),
                         "Channel::OnPacketTimeout() => "
                         "_media_timeout_cb(%d)", channel);
            _media_timeout_cb(channel);
        }
    }
}

void
Channel::OnReceivedPacket(const WebRtc_Word32 id,
                          const RtpRtcpPacketType packetType)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
                 "Channel::OnReceivedPacket(id=%d, packetType=%d)",
                 id, packetType);
    
    assert(VoEChannelId(id) == _channelId);
    
    // Notify only for the case when we have restarted an RTP session.
    if (_rtpPacketTimedOut && (kPacketRtp == packetType))
    {
        CriticalSectionScoped cs(_callbackCritSectPtr);
        if (_voiceEngineObserverPtr)
        {
            WebRtc_Word32 channel = VoEChannelId(id);
            assert(channel == _channelId);
            // Reset timeout mechanism
            _rtpPacketTimedOut = false;
            // Deliver callback to the observer
            WEBRTC_TRACE(kTraceInfo, kTraceVoice,
                         VoEId(_instanceId,_channelId),
                         "Channel::OnPacketTimeout() =>"
                         " CallbackOnError(VE_PACKET_RECEIPT_RESTARTED)");
            _voiceEngineObserverPtr->CallbackOnError(
                                                     channel,
                                                     VE_PACKET_RECEIPT_RESTARTED);
        }
    }
}

int32_t Channel::SetKeepAliveStatus(
	const bool enable, const int8_t unknownPayloadType,
	const uint16_t deltaTransmitTimeMS)
{
	WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideo, VoEId(_instanceId, _channelId),
		"%s", __FUNCTION__);

	if (enable && _rtpRtcpModule->RTPKeepalive())
	{
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
			VoEId(_instanceId, _channelId),
			"%s: RTP keepalive already enabled", __FUNCTION__);
		return -1;
	}
	else if (!enable && !_rtpRtcpModule->RTPKeepalive())
	{
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
			VoEId(_instanceId, _channelId),
			"%s: RTP keepalive already disabled", __FUNCTION__);
		return -1;
	}

	if (_rtpRtcpModule->SetRTPKeepaliveStatus(enable, unknownPayloadType,
		deltaTransmitTimeMS) != 0)
	{
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
			VoEId(_instanceId, _channelId),
			"%s: Could not set RTP keepalive status %d", __FUNCTION__,
			enable);
		//        if (enable == false && !_rtpRtcpModule->DefaultModuleRegistered())
		//        {
		//            // Not sending media and we try to disable keep alive
		//            _rtpRtcp.ResetSendDataCountersRTP();
		//            _rtpRtcp.SetSendingStatus(false);
		//        }
		return -1;
	}

	if (enable && !_rtpRtcpModule->Sending())
	{
		// Enable sending to start sending Sender reports instead of receive
		// reports
		if (_rtpRtcpModule->SetSendingStatus(true) != 0)
		{
			_rtpRtcpModule->SetRTPKeepaliveStatus(false, 0, 0);
			WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
				VoEId(_instanceId, _channelId),
				"%s: Could not start sending", __FUNCTION__);
			return -1;
		}
	}
	else if (!enable && !_rtpRtcpModule->SendingMedia())
	{
		// Not sending media and we're disabling keep alive
		_rtpRtcpModule->ResetSendDataCountersRTP();
		if (_rtpRtcpModule->SetSendingStatus(false) != 0)
		{
			WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
				VoEId(_instanceId, _channelId),
				"%s: Could not stop sending", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------
// GetKeepAliveStatus
// ----------------------------------------------------------------------------

int32_t Channel::GetKeepAliveStatus(
	bool& enabled, int8_t& unknownPayloadType,
	uint16_t& deltaTransmitTimeMs)
{
	WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideo, VoEId(_instanceId, _channelId),
		"%s", __FUNCTION__);
	if (_rtpRtcpModule->RTPKeepaliveStatus(&enabled, &unknownPayloadType,
		&deltaTransmitTimeMs) != 0)
	{
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideo,
			VoEId(_instanceId, _channelId),
			"%s: Could not get RTP keepalive status", __FUNCTION__);
		return -1;
	}
	WEBRTC_TRACE(
		cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideo, VoEId(_instanceId, _channelId),
		"%s: enabled = %d, unknownPayloadType = %d, deltaTransmitTimeMs = %ul",
		__FUNCTION__, enabled, (int32_t)unknownPayloadType,
		deltaTransmitTimeMs);

	return 0;
}
    
#ifdef WEBRTC_SRTP
int Channel::CcpSrtpInit()
{
	int err = _srtpModule.CcpSrtpInit(_channelId);
	return err;
}

int Channel::CcpSrtpShutdown()
{
	int err = _srtpModule.CcpSrtpShutdown(_channelId);
	return err;
}

int
Channel::EnableSRTPSend(ccp_srtp_crypto_suite_t crypt_type, const char* key)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::EnableSRTPSend()");

	CriticalSectionScoped cs(&_callbackCritSect);

	if (_encrypting)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_OPERATION, kTraceWarning,
			"EnableSRTPSend() encryption already enabled");
		return -1;
	}

	if (key == NULL)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_ARGUMENT, kTraceWarning,
			"EnableSRTPSend() invalid key string");
		return -1;
	}
	unsigned int ssrc;
	GetLocalSSRC(ssrc);
	if (_srtpModule.EnableSRTPSend(_channelId, crypt_type, key, ssrc) == -1)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SRTP_ERROR, kTraceError,
			"EnableSRTPSend() failed to enable SRTP encryption");
		return -1;
	}

	_encryptionRTPBufferPtr = new WebRtc_UWord8[kVoiceEngineMaxIpPacketSizeBytes];
	_encryptionRTCPBufferPtr = new WebRtc_UWord8[kVoiceEngineMaxIpPacketSizeBytes];

	if (_encryptionPtr == NULL)
	{
		_encryptionPtr = &_srtpModule;
	}

	_encrypting = true;

	return 0;
}

int
	Channel::DisableSRTPSend()
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::DisableSRTPSend()");

	CriticalSectionScoped cs(&_callbackCritSect);

	if (!_encrypting)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_OPERATION, kTraceWarning,
			"DisableSRTPSend() SRTP encryption already disabled");
		return 0;
	}

	_encrypting = false;

	if (_encryptionRTPBufferPtr) {
		delete _encryptionRTPBufferPtr;
		_encryptionRTPBufferPtr = NULL;
	}
	if (_encryptionRTCPBufferPtr) {
		delete _encryptionRTCPBufferPtr;
		_encryptionRTCPBufferPtr = NULL;
	}

	//    if (_srtpModule.DisableSRTPEncrypt() == -1)
	if (_srtpModule.DisableSRTPSend(_channelId) == -1)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SRTP_ERROR, kTraceError,
			"DisableSRTPSend() failed to disable SRTP encryption");
		return -1;
	}

	//    if (!_srtpModule.SRTPDecrypt() && !_srtpModule.SRTPEncrypt())
	//    if (!_srtpModule.) //seantodo
	{
		// Both directions are disabled
		_encryptionPtr = NULL;
	}

	return 0;
}

int
Channel::EnableSRTPReceive(ccp_srtp_crypto_suite_t crypt_type,
	const char* key)
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::EnableSRTPReceive()");

	CriticalSectionScoped cs(&_callbackCritSect);

	if (_decrypting)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_OPERATION, kTraceWarning,
			"EnableSRTPReceive() SRTP decryption already enabled");
		return -1;
	}

	if (key == NULL)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_ARGUMENT, kTraceWarning,
			"EnableSRTPReceive() invalid key string");
		return -1;
	}

	//if ((((kEncryption == level) ||
	//	(kEncryptionAndAuthentication == level)) &&
	//	((cipherKeyLength < kMinSrtpEncryptLength) ||
	//	(cipherKeyLength > kMaxSrtpEncryptLength))) ||
	//	(((kAuthentication == level) ||
	//	(kEncryptionAndAuthentication == level)) &&
	//	(kAuthHmacSha1 == authType) &&
	//	((authKeyLength > kMaxSrtpAuthSha1Length) ||
	//	(authTagLength > kMaxSrtpAuthSha1Length))) ||
	//	(((kAuthentication == level) ||
	//	(kEncryptionAndAuthentication == level)) &&
	//	(kAuthNull == authType) &&
	//	((authKeyLength > kMaxSrtpKeyAuthNullLength) ||
	//	(authTagLength > kMaxSrtpTagAuthNullLength))))
	//{
	//	_engineStatisticsPtr->SetLastError(
	//		VE_INVALID_ARGUMENT, kTraceError,
	//		"EnableSRTPReceive() invalid key length(s)");
	//	return -1;
	//}

	//    if (_srtpModule.EnableSRTPDecrypt(
	//        !useForRTCP,
	//        (SrtpModule::CipherTypes)cipherType,
	//        cipherKeyLength,
	//        (SrtpModule::AuthenticationTypes)authType,
	//        authKeyLength,
	//        authTagLength,
	//        (SrtpModule::SecurityLevels)level,
	//        key) == -1)
	//    printf("cipherType = %d;cipherKeyLength = %d;authType = %d;authKeyLength = %d;authTagLength = %d;level = %d;key = %s\n",cipherType,cipherKeyLength,authType,authKeyLength,authTagLength,level,key);
	if (_srtpModule.EnableSRTPReceive(_channelId, crypt_type, key) == -1)
	{
		_engineStatisticsPtr->SetLastError(
			VE_SRTP_ERROR, kTraceError,
			"EnableSRTPReceive() failed to enable SRTP decryption");
		return -1;
	}

	_decryptionRTPBufferPtr = new WebRtc_UWord8[kVoiceEngineMaxIpPacketSizeBytes];
	_decryptionRTCPBufferPtr = new WebRtc_UWord8[kVoiceEngineMaxIpPacketSizeBytes];

	if (_encryptionPtr == NULL)
	{
		_encryptionPtr = &_srtpModule;
	}
	_decrypting = true;

	return 0;
}

int
	Channel::DisableSRTPReceive()
{
	WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,_channelId),
		"Channel::DisableSRTPReceive()");

	CriticalSectionScoped cs(&_callbackCritSect);

	if (!_decrypting)
	{
		_engineStatisticsPtr->SetLastError(
			VE_INVALID_OPERATION, kTraceWarning,
			"DisableSRTPReceive() SRTP decryption already disabled");
		return 0;
	}

	_decrypting = false;

	if (_decryptionRTPBufferPtr) {
		delete _decryptionRTPBufferPtr;
		_decryptionRTPBufferPtr = NULL;
	}
	if (_decryptionRTCPBufferPtr) {
		delete _decryptionRTCPBufferPtr;
		_decryptionRTCPBufferPtr = NULL;
	}

	//    if (_srtpModule.DisableSRTPDecrypt() == -1)
	if (_srtpModule.DisableSRTPReceive(_channelId))
	{
		_engineStatisticsPtr->SetLastError(
			VE_SRTP_ERROR, kTraceError,
			"DisableSRTPReceive() failed to disable SRTP decryption");
		return -1;
	}

	//    if (!_srtpModule.SRTPDecrypt() && !_srtpModule.SRTPEncrypt()) //seantodo
	{
		_encryptionPtr = NULL;
	}

	return 0;
}

#endif
}  // namespace voe
}  // namespace cloopenwebrtc
