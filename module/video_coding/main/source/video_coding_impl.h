/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_VIDEO_CODING_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CODING_VIDEO_CODING_IMPL_H_

#include "video_coding.h"

#include <vector>

#include "../base/thread_annotations.h"
#include "codec_database.h"
#include "frame_buffer.h"
#include "generic_decoder.h"
#include "generic_encoder.h"
#include "jitter_buffer.h"
#include "media_optimization.h"
#include "receiver.h"
#include "timing.h"
#include "../system_wrappers/include/clock.h"
#include "../system_wrappers/include/critical_section_wrapper.h"

//#define DEBUG_DECODER_BIT_STREAM

namespace cloopenwebrtc {

class EncodedFrameObserver;
class VideoEncoderRateObserver;

namespace vcm {

class DebugRecorder;

class VCMProcessTimer {
 public:
  VCMProcessTimer(int64_t periodMs, Clock* clock)
      : _clock(clock),
        _periodMs(periodMs),
        _latestMs(_clock->TimeInMilliseconds()) {}
  int64_t Period() const;
  int64_t TimeUntilProcess() const;
  void Processed();

 private:
  Clock* _clock;
  int64_t _periodMs;
  int64_t _latestMs;
};

class VideoSender {
 public:
  typedef VideoCodingModule::SenderNackMode SenderNackMode;

  VideoSender(Clock* clock, EncodedImageCallback* post_encode_callback);

  ~VideoSender();

  int32_t InitializeSender();

  // Register the send codec to be used.
  int32_t RegisterSendCodec(const VideoCodec* sendCodec,
                            uint32_t numberOfCores,
                            uint32_t maxPayloadSize);

  int32_t SendCodec(VideoCodec* currentSendCodec) const;
  VideoCodecType SendCodec() const;
  int32_t RegisterExternalEncoder(VideoEncoder* externalEncoder,
                                  uint8_t payloadType,
                                  bool internalSource);

  int32_t CodecConfigParameters(uint8_t* buffer, int32_t size) const;
  int32_t SentFrameCount(VCMFrameCount* frameCount);
  int Bitrate(unsigned int* bitrate) const;
  int FrameRate(unsigned int* framerate) const;

  int32_t SetChannelParameters(uint32_t target_bitrate,  // bits/s.
                               uint8_t lossRate,
                               int64_t rtt);

  int32_t RegisterTransportCallback(VCMPacketizationCallback* transport);
  int32_t RegisterSendStatisticsCallback(VCMSendStatisticsCallback* sendStats);
  int32_t RegisterVideoQMCallback(VCMQMSettingsCallback* videoQMSettings);
  int32_t RegisterProtectionCallback(VCMProtectionCallback* protection);
  int32_t SetVideoProtection(VCMVideoProtection videoProtection, bool enable);

  int32_t AddVideoFrame(const I420VideoFrame& videoFrame,
                        const VideoContentMetrics* _contentMetrics,
                        const CodecSpecificInfo* codecSpecificInfo);

  int32_t IntraFrameRequest(int stream_index);
  int32_t EnableFrameDropper(bool enable);

  int SetSenderNackMode(SenderNackMode mode);
  int SetSenderReferenceSelection(bool enable);
  int SetSenderFEC(bool enable);
  int SetSenderKeyFramePeriod(int periodMs);

  int StartDebugRecording(const char* file_name_utf8);
  void StopDebugRecording();

  void SuspendBelowMinBitrate();
  bool VideoSuspended() const;

  int64_t TimeUntilNextProcess();
  int32_t Process();
  //---begin
public:
	int32_t RegisterEncoderRateObserver(VideoEncoderRateObserver *observer);
  //---end

 private:
  Clock* clock_;

  scoped_ptr<DebugRecorder> recorder_;

  scoped_ptr<CriticalSectionWrapper> process_crit_sect_;
  CriticalSectionWrapper* _sendCritSect;
  VCMGenericEncoder* _encoder;
  VCMEncodedFrameCallback _encodedFrameCallback;
  std::vector<FrameType> _nextFrameTypes;
  media_optimization::MediaOptimization _mediaOpt;
  VCMSendStatisticsCallback* _sendStatsCallback;
  VCMCodecDataBase _codecDataBase;
  bool frame_dropper_enabled_;
  VCMProcessTimer _sendStatsTimer;

  VCMQMSettingsCallback* qm_settings_callback_;
  VCMProtectionCallback* protection_callback_;
};

class VideoReceiver {
 public:
  typedef VideoCodingModule::ReceiverRobustness ReceiverRobustness;

  VideoReceiver(Clock* clock, EventFactory* event_factory);
  ~VideoReceiver();

  int32_t InitializeReceiver();
  int32_t RegisterReceiveCodec(const VideoCodec* receiveCodec,
                               int32_t numberOfCores,
                               bool requireKeyFrame);

  int32_t RegisterExternalDecoder(VideoDecoder* externalDecoder,
                                  uint8_t payloadType,
                                  bool internalRenderTiming);
  int32_t RegisterReceiveCallback(VCMReceiveCallback* receiveCallback);
  int32_t RegisterReceiveStatisticsCallback(
      VCMReceiveStatisticsCallback* receiveStats);
  int32_t RegisterDecoderTimingCallback(
      VCMDecoderTimingCallback* decoderTiming);
  int32_t RegisterFrameTypeCallback(VCMFrameTypeCallback* frameTypeCallback);
  int32_t RegisterPacketRequestCallback(VCMPacketRequestCallback* callback);
  int RegisterRenderBufferSizeCallback(VCMRenderBufferSizeCallback* callback);

  int32_t Decode(uint16_t maxWaitTimeMs, bool shieldMosaic = false);
  int32_t ResetDecoder();

  int32_t ReceiveCodec(VideoCodec* currentReceiveCodec) const;
  VideoCodecType ReceiveCodec() const;

  int32_t IncomingPacket(const uint8_t* incomingPayload,
                         size_t payloadLength,
                         const WebRtcRTPHeader& rtpInfo);
  int32_t SetMinimumPlayoutDelay(uint32_t minPlayoutDelayMs);
  int32_t SetRenderDelay(uint32_t timeMS);
  int32_t Delay() const;
  uint32_t DiscardedPackets() const;

  int SetReceiverRobustnessMode(ReceiverRobustness robustnessMode,
                                VCMDecodeErrorMode errorMode);
  void SetNackSettings(size_t max_nack_list_size,
                       int max_packet_age_to_nack,
                       int max_incomplete_time_ms);

  void SetDecodeErrorMode(VCMDecodeErrorMode decode_error_mode);
  int SetMinReceiverDelay(int desired_delay_ms);

  int32_t SetReceiveChannelParameters(int64_t rtt);
  int32_t SetVideoProtection(VCMVideoProtection videoProtection, bool enable);

  int64_t TimeUntilNextProcess();
  int32_t Process();

  void RegisterPreDecodeImageCallback(EncodedImageCallback* observer);

 protected:
  int32_t Decode(const cloopenwebrtc::VCMEncodedFrame& frame)
      EXCLUSIVE_LOCKS_REQUIRED(_receiveCritSect);
  int32_t RequestKeyFrame();
  int32_t RequestSliceLossIndication(const uint64_t pictureID) const;
  int32_t NackList(uint16_t* nackList, uint16_t* size);

 private:
  enum VCMKeyRequestMode {
    kKeyOnError,    // Normal mode, request key frames on decoder error
    kKeyOnKeyLoss,  // Request key frames on decoder error and on packet loss
                    // in key frames.
    kKeyOnLoss,     // Request key frames on decoder error and on packet loss
                    // in any frame
  };

  Clock* const clock_;
  scoped_ptr<CriticalSectionWrapper> process_crit_sect_;
  CriticalSectionWrapper* _receiveCritSect;
  bool _receiverInited GUARDED_BY(_receiveCritSect);
  VCMTiming _timing;
  VCMReceiver _receiver;
  VCMDecodedFrameCallback _decodedFrameCallback;
  VCMFrameTypeCallback* _frameTypeCallback GUARDED_BY(process_crit_sect_);
  VCMReceiveStatisticsCallback* _receiveStatsCallback
      GUARDED_BY(process_crit_sect_);
  VCMDecoderTimingCallback* _decoderTimingCallback
      GUARDED_BY(process_crit_sect_);
  VCMPacketRequestCallback* _packetRequestCallback
      GUARDED_BY(process_crit_sect_);
  VCMRenderBufferSizeCallback* render_buffer_callback_
      GUARDED_BY(process_crit_sect_);
  VCMGenericDecoder* _decoder;
#ifdef DEBUG_DECODER_BIT_STREAM
  FILE* _bitStreamBeforeDecoder;
#endif
  VCMFrameBuffer _frameFromFile;
  VCMKeyRequestMode _keyRequestMode;
  bool _scheduleKeyRequest GUARDED_BY(process_crit_sect_);
  size_t max_nack_list_size_ GUARDED_BY(process_crit_sect_);
  EncodedImageCallback* pre_decode_image_callback_ GUARDED_BY(_receiveCritSect);

  VCMCodecDataBase _codecDataBase GUARDED_BY(_receiveCritSect);
  VCMProcessTimer _receiveStatsTimer;
  VCMProcessTimer _retransmissionTimer;
  VCMProcessTimer _keyRequestTimer;

  bool _waitForKeyFrame;
  uint16_t _lastDecodeSeqNum;
  int _forTestCount;
  //---begin
private:
	VCMFrameStorageCallback*   _frameStorageCallback GUARDED_BY(_receiveCritSect);
public:
 	WebRtc_Word32 RegisterFrameStorageCallback(
		VCMFrameStorageCallback* frameStorageCallback);
  //---end
};

}  // namespace vcm
}  // namespace webrtc
#endif  // WEBRTC_MODULES_VIDEO_CODING_VIDEO_CODING_IMPL_H_
