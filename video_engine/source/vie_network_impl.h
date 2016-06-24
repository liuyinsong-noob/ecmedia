/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_NETWORK_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_NETWORK_IMPL_H_

#include "typedefs.h"
#include "vie_network.h"
#include "vie_ref_count.h"

namespace cloopenwebrtc {

class ViESharedData;

class ViENetworkImpl
    : public ViENetwork,
      public ViERefCount {
 public:
  // Implements ViENetwork.
  virtual int Release() OVERRIDE;
  virtual void SetNetworkTransmissionState(const int video_channel,
                                           const bool is_transmitting) OVERRIDE;
  virtual int RegisterSendTransport(const int video_channel,
                                    Transport& transport) OVERRIDE;
  virtual int DeregisterSendTransport(const int video_channel) OVERRIDE;
  virtual int ReceivedRTPPacket(const int video_channel,
                                const void* data,
                                const size_t length,
                                const PacketTime& packet_time) OVERRIDE;
  virtual int ReceivedRTCPPacket(const int video_channel,
                                 const void* data,
                                 const size_t length) OVERRIDE;
  virtual int SetMTU(int video_channel, unsigned int mtu) OVERRIDE;

  virtual int ReceivedBWEPacket(const int video_channel,
                                int64_t arrival_time_ms,
                                size_t payload_size,
                                const RTPHeader& header) OVERRIDE;

  virtual bool SetBandwidthEstimationConfig(
      int video_channel,
      const cloopenwebrtc::Config& config) OVERRIDE;

//---begin
//  virtual int RegisterServiceCoreCallBack(int channel, ServiceCoreCallBack *messageCallBack, const char* call_id, int firewall_policy);

  //sean add begin 0915
  virtual int setShieldMosaic(int channel,bool flag);
  //sean add end 0915
  virtual int setProcessData(int channel, bool flag); 
  virtual int getNetworkStatistic(const int video_channel, time_t &startTime, long long &sendLengthSim, long long &recvLengthSim, long long &sendLengthWifi, long long &recvLengthWifi);
  virtual int setVideoConferenceFlag(int channel, const char *selfSipNo ,const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip);
  virtual int setNetworkType(int channel, bool isWifi);

  virtual int SetLocalReceiver(const int video_channel,
	  const unsigned short rtp_port,
	  const unsigned short rtcp_port,
	  const char* ip_address);
  virtual int GetLocalReceiver(const int video_channel,
	  unsigned short& rtp_port,
	  unsigned short& rtcp_port,
	  char* ip_address);
  virtual int SetSendDestination(const int video_channel,
	  const char* ip_address,
	  const unsigned short rtp_port,
	  const unsigned short rtcp_port,
	  const unsigned short source_rtp_port,
	  const unsigned short source_rtcp_port);
  virtual int GetSendDestination(const int video_channel,
	  char* ip_address,
	  unsigned short& rtp_port,
	  unsigned short& rtcp_port,
	  unsigned short& source_rtp_port,
	  unsigned short& source_rtcp_port);

  virtual int SendUDPPacket(const int video_channel,
	  const void* data,
	  const unsigned int length,
	  int& transmitted_bytes,
	  bool use_rtcp_socket = false,
	  /*add begin------------------Sean20130723----------for video ice------------*/
	  WebRtc_UWord16 portnr = 0,
	  const char* ip = NULL
	  /*add end--------------------Sean20130723----------for video ice------------*/
	  );
//---end

 protected:
  explicit ViENetworkImpl(ViESharedData* shared_data);
  virtual ~ViENetworkImpl();

 private:
  ViESharedData* shared_data_;
public:
  virtual int setVideoConfCb(int channel, onVideoConference video_conf_cb) OVERRIDE;
  virtual int setVideoDataCb(int channel, onEcMediaVideoData video_data_cb) OVERRIDE;
  virtual int setStunCb(int channel, onStunPacket stun_cb) OVERRIDE;
  // Enables IPv6, instead of IPv4, for a specified channel.
  virtual int EnableIPv6(int video_channel) OVERRIDE;
  // The function returns true if IPv6 is enabled, false otherwise.
  virtual bool IsIPv6Enabled(int video_channel) OVERRIDE;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_NETWORK_IMPL_H_
