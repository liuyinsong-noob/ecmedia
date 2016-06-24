/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_RTP_RTCP_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_RTP_RTCP_IMPL_H_

#include "rtp_rtcp_defines.h"
#include "typedefs.h"
#include "vie_rtp_rtcp.h"
#include "vie_ref_count.h"

namespace cloopenwebrtc {

class ViESharedData;

class ViERTP_RTCPImpl
    : public ViERTP_RTCP,
      public ViERefCount {
 public:
  // Implements ViERTP_RTCP.
  virtual int Release() OVERRIDE;
  virtual int SetLocalSSRC(const int video_channel,
                           const unsigned int SSRC,
                           const StreamType usage,
                           const unsigned char simulcast_idx) OVERRIDE;
  virtual int GetLocalSSRC(const int video_channel,
                           unsigned int& SSRC) const OVERRIDE;  // NOLINT
  virtual int SetRemoteSSRCType(const int video_channel,
                                const StreamType usage,
                                const unsigned int SSRC) const OVERRIDE;
  virtual int GetRemoteSSRC(const int video_channel,
                            unsigned int& SSRC) const OVERRIDE;  // NOLINT
  virtual int GetRemoteCSRCs(const int video_channel,
                             unsigned int CSRCs[kRtpCsrcSize]) const OVERRIDE;
  virtual int SetRtxSendPayloadType(const int video_channel,
                                    const uint8_t payload_type) OVERRIDE;
  virtual int SetRtxReceivePayloadType(const int video_channel,
                                       const uint8_t payload_type) OVERRIDE;
  virtual int SetStartSequenceNumber(const int video_channel,
                                     uint16_t sequence_number) OVERRIDE;
  virtual void SetRtpStateForSsrc(int video_channel,
                                  uint32_t ssrc,
                                  const RtpState& rtp_state) OVERRIDE;
  virtual RtpState GetRtpStateForSsrc(int video_channel,
                                      uint32_t ssrc) OVERRIDE;
  virtual int SetRTCPStatus(const int video_channel,
                            const ViERTCPMode rtcp_mode) OVERRIDE;
  virtual int GetRTCPStatus(const int video_channel,
                            ViERTCPMode& rtcp_mode) const OVERRIDE;
  virtual int SetRTCPCName(const int video_channel,
                           const char rtcp_cname[KMaxRTCPCNameLength]) OVERRIDE;
  virtual int GetRemoteRTCPCName(const int video_channel,
                                 char rtcp_cname[KMaxRTCPCNameLength]) const OVERRIDE;
  virtual int SendApplicationDefinedRTCPPacket(
      const int video_channel,
      const unsigned char sub_type,
      unsigned int name,
      const char* data,
      uint16_t data_length_in_bytes) OVERRIDE;
  virtual int SetNACKStatus(const int video_channel, const bool enable) OVERRIDE;
  virtual int SetFECStatus(const int video_channel, const bool enable,
                           const unsigned char payload_typeRED,
                           const unsigned char payload_typeFEC) OVERRIDE;
  virtual int SetHybridNACKFECStatus(const int video_channel, const bool enable,
                                     const unsigned char payload_typeRED,
                                     const unsigned char payload_typeFEC) OVERRIDE;
  virtual int SetSenderBufferingMode(int video_channel,
                                     int target_delay_ms) OVERRIDE;
  virtual int SetReceiverBufferingMode(int video_channel,
                                       int target_delay_ms) OVERRIDE;
  virtual int SetKeyFrameRequestMethod(const int video_channel,
                                       const ViEKeyFrameRequestMethod method) OVERRIDE;
  virtual int SetTMMBRStatus(const int video_channel, const bool enable) OVERRIDE;
  virtual int SetRembStatus(int video_channel, bool sender, bool receiver) OVERRIDE;
  virtual int SetSendTimestampOffsetStatus(int video_channel,
                                           bool enable,
                                           int id) OVERRIDE;
  virtual int SetReceiveTimestampOffsetStatus(int video_channel,
                                              bool enable,
                                              int id) OVERRIDE;
  virtual int SetSendAbsoluteSendTimeStatus(int video_channel,
                                            bool enable,
                                            int id) OVERRIDE;
  virtual int SetReceiveAbsoluteSendTimeStatus(int video_channel,
                                               bool enable,
                                               int id) OVERRIDE;
  virtual int SetRtcpXrRrtrStatus(int video_channel, bool enable) OVERRIDE;
  virtual int SetTransmissionSmoothingStatus(int video_channel, bool enable) OVERRIDE;
  virtual int SetMinTransmitBitrate(int video_channel,
                                    int min_transmit_bitrate_kbps) OVERRIDE;
  virtual int SetReservedTransmitBitrate(
      int video_channel, unsigned int reserved_transmit_bitrate_bps) OVERRIDE;
  virtual int GetReceiveChannelRtcpStatistics(const int video_channel,
                                              RtcpStatistics& basic_stats,
                                              int64_t& rtt_ms) const OVERRIDE;
  virtual int GetSendChannelRtcpStatistics(const int video_channel,
                                           RtcpStatistics& basic_stats,
                                           int64_t& rtt_ms) const OVERRIDE;
  virtual int GetRtpStatistics(const int video_channel,
                               StreamDataCounters& sent,
                               StreamDataCounters& received) const OVERRIDE;
  virtual int GetRtcpPacketTypeCounters(
      int video_channel,
      RtcpPacketTypeCounter* packets_sent,
      RtcpPacketTypeCounter* packets_received) const OVERRIDE;
  virtual int GetBandwidthUsage(const int video_channel,
                                unsigned int& total_bitrate_sent,
                                unsigned int& video_bitrate_sent,
                                unsigned int& fec_bitrate_sent,
                                unsigned int& nackBitrateSent) const OVERRIDE;
  virtual int GetEstimatedSendBandwidth(
      const int video_channel,
      unsigned int* estimated_bandwidth) const OVERRIDE;
  virtual int GetEstimatedReceiveBandwidth(
      const int video_channel,
      unsigned int* estimated_bandwidth) const OVERRIDE;
  virtual int GetReceiveBandwidthEstimatorStats(
      const int video_channel, ReceiveBandwidthEstimatorStats* output) const OVERRIDE;
  virtual int GetPacerQueuingDelayMs(const int video_channel,
                                     int64_t* delay_ms) const OVERRIDE;
  virtual int StartRTPDump(const int video_channel,
                           const char file_nameUTF8[1024],
                           RTPDirections direction) OVERRIDE;
  virtual int StopRTPDump(const int video_channel, RTPDirections direction) OVERRIDE;
  virtual int RegisterRTPObserver(const int video_channel,
                                  ViERTPObserver& observer) OVERRIDE;
  virtual int DeregisterRTPObserver(const int video_channel) OVERRIDE;

  virtual int RegisterSendChannelRtcpStatisticsCallback(
      int channel, RtcpStatisticsCallback* callback) OVERRIDE;
  virtual int DeregisterSendChannelRtcpStatisticsCallback(
      int channel, RtcpStatisticsCallback* callback) OVERRIDE;
  virtual int RegisterReceiveChannelRtcpStatisticsCallback(
        int channel, RtcpStatisticsCallback* callback) OVERRIDE;
    virtual int DeregisterReceiveChannelRtcpStatisticsCallback(
        int channel, RtcpStatisticsCallback* callback) OVERRIDE;
  virtual int RegisterSendChannelRtpStatisticsCallback(
      int channel, StreamDataCountersCallback* callback) OVERRIDE;
  virtual int DeregisterSendChannelRtpStatisticsCallback(
      int channel, StreamDataCountersCallback* callback) OVERRIDE;
  virtual int RegisterReceiveChannelRtpStatisticsCallback(
      int channel, StreamDataCountersCallback* callback) OVERRIDE;
  virtual int DeregisterReceiveChannelRtpStatisticsCallback(
      int channel, StreamDataCountersCallback* callback) OVERRIDE;
  virtual int RegisterSendBitrateObserver(
      int channel, BitrateStatisticsObserver* callback) OVERRIDE;
  virtual int DeregisterSendBitrateObserver(
      int channel, BitrateStatisticsObserver* callback) OVERRIDE;
  virtual int RegisterSendFrameCountObserver(
      int channel, FrameCountObserver* callback) OVERRIDE;
  virtual int DeregisterSendFrameCountObserver(
      int channel, FrameCountObserver* callback) OVERRIDE;

 protected:
  explicit ViERTP_RTCPImpl(ViESharedData* shared_data);
  virtual ~ViERTP_RTCPImpl();

 private:
  ViESharedData* shared_data_;

  //---begin
    virtual int RequestKeyFrame(const int video_channel) OVERRIDE;
    virtual int SetRTPKeepAliveStatus(
                                    const int videoChannel, bool enable, const char unknownPayloadType,
                                    const unsigned int deltaTransmitTimeSeconds) OVERRIDE;
    virtual int GetRTPKeepAliveStatus(const int videoChannel, bool& enabled,
                                    char& unkownPayloadType,
                                    unsigned int& deltaTransmitTimeSeconds) OVERRIDE;
  //---end
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_RTP_RTCP_IMPL_H_
