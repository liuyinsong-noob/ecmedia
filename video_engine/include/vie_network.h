/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_NETWORK_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_NETWORK_H_

// This sub-API supports the following functionalities:
//  - Configuring send and receive addresses.
//  - External transport support.
//  - Port and address filters.
//  - Windows GQoS functions and ToS functions.
//  - Packet timeout notification.
//  - Dead‐or‐Alive connection observations.

#include "common_types.h"
//#include "StunMessageCallBack.h"

namespace cloopenwebrtc {

typedef int (*onVideoConference)(int channelid, int status, int payload);
typedef int (*onStunPacket)(int channelid, void *data, int len, const char *fromIP, int fromPort, bool isRTCP, bool isVideo);
typedef int (*onEcMediaVideoData)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
class Transport;
class VideoEngine;
class VCMPacketizationCallback;

// This enumerator describes VideoEngine packet timeout states.
enum ViEPacketTimeout {
  NoPacket = 0,
  PacketReceived = 1
};

// This class declares an abstract interface for a user defined observer. It is
// up to the VideoEngine user to implement a derived class which implements the
// observer class. The observer is registered using RegisterObserver() and
// deregistered using DeregisterObserver().
class WEBRTC_DLLEXPORT ViENetworkObserver {
public:
    // This method will be called periodically delivering a dead‐or‐alive
    // decision for a specified channel.
    virtual void OnPeriodicDeadOrAlive(const int video_channel,
                                       const bool alive) = 0;
    
    // This method is called once if a packet timeout occurred.
    virtual void PacketTimeout(const int video_channel,
                               const ViEPacketTimeout timeout) = 0;
protected:
    virtual ~ViENetworkObserver() {}
};
    
class WEBRTC_DLLEXPORT ViENetwork {
 public:
  // Default values.
  enum { KDefaultSampleTimeSeconds = 2 };

  // Factory for the ViENetwork sub‐API and increases an internal reference
  // counter if successful. Returns NULL if the API is not supported or if
  // construction fails.
  static ViENetwork* GetInterface(VideoEngine* video_engine);

  // Releases the ViENetwork sub-API and decreases an internal reference
  // counter.Returns the new reference count. This value should be zero
  // for all sub-API:s before the VideoEngine object can be safely deleted.
  virtual int Release() = 0;

  // Inform the engine about if the network adapter is currently transmitting
  // packets or not.
  virtual void SetNetworkTransmissionState(const int video_channel,
                                           const bool is_transmitting) = 0;

  // This function registers a user implementation of Transport to use for
  // sending RTP and RTCP packets on this channel.
  virtual int RegisterSendTransport(const int video_channel,
                                    Transport& transport) = 0;

  // This function deregisters a used Transport for a specified channel.
  virtual int DeregisterSendTransport(const int video_channel) = 0;

  // When using external transport for a channel, received RTP packets should
  // be passed to VideoEngine using this function. The input should contain
  // the RTP header and payload.
  virtual int ReceivedRTPPacket(const int video_channel,
                                const void* data,
                                const size_t length,
                                const PacketTime& packet_time) = 0;

  // When using external transport for a channel, received RTCP packets should
  // be passed to VideoEngine using this function.
  virtual int ReceivedRTCPPacket(const int video_channel,
                                 const void* data,
                                 const size_t length) = 0;

  // This function sets the Maximum Transition Unit (MTU) for a channel. The
  // RTP packet will be packetized based on this MTU to optimize performance
  // over the network.
  virtual int SetMTU(int video_channel, unsigned int mtu) = 0;

  // Forward (audio) packet to bandwidth estimator for the given video channel,
  // for aggregated audio+video BWE.
  virtual int ReceivedBWEPacket(const int video_channel,
      int64_t arrival_time_ms, size_t payload_size, const RTPHeader& header) {
    return 0;
  }

  // TODO(holmer): Remove the default implementation when this has been fixed
  // in fakewebrtcvideoengine.cc.
  virtual bool SetBandwidthEstimationConfig(int video_channel,
                                            const cloopenwebrtc::Config& config) {
    return false;
  }

  //---begin
//  virtual int RegisterServiceCoreCallBack(int channel, ServiceCoreCallBack *messageCallBack, const char* call_id, int firewall_policy) = 0;

  //    sean add begin 0915
  virtual int setShieldMosaic(int channel,bool flag) = 0;
  //    sean add end 0915
  virtual int setProcessData(int channel, bool flag) = 0;
  virtual int getNetworkStatistic(int channel, time_t &startTime, long long &sendLengthSim, long long &recvLengthSim, long long &sendLengthWifi, long long &recvLengthWifi) = 0;

  virtual int setVideoConferenceFlag(int channel,const char *selfSipNo ,const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip) = 0;

  virtual int setNetworkType(int channel, bool isWifi) = 0;

  // Specifies the ports to receive RTP packets on. It is also possible to set
  // port for RTCP and local IP address.
  virtual int SetLocalReceiver(const int video_channel,
	  const unsigned short rtp_port,
	  const unsigned short rtcp_port = 0,
	  const char* ip_address = NULL) = 0;

  // Gets the local receiver ports and address for a specified channel.
  virtual int GetLocalReceiver(const int video_channel,
	  unsigned short& rtp_port,
	  unsigned short& rtcp_port, char* ip_address) = 0;

  // Specifies the destination port and IP address for a specified channel.
  virtual int SetSendDestination(const int video_channel, const char *ip_address, const unsigned short rtp_port, const char *rtcp_ip_address, const unsigned short rtcp_port, const unsigned short source_rtp_port, const unsigned short source_rtcp_port) = 0;

  // Get the destination port and address for a specified channel.
  virtual int GetSendDestination(const int video_channel,
	  char* ip_address,
	  unsigned short& rtp_port,
	  unsigned short& rtcp_port,
	  unsigned short& source_rtp_port,
	  unsigned short& source_rtcp_port) = 0;

  virtual int SendUDPPacket(const int video_channel,
	  const void* data,
	  const unsigned int length,
	  int& transmitted_bytes,
	  bool use_rtcp_socket = false,
	  WebRtc_UWord16 portnr = 0,
	  const char* ip = NULL) = 0;
  //---end

    virtual int setVideoConfCb(int channel, onVideoConference video_conf_cb) = 0;
    virtual int setVideoDataCb(int channel, onEcMediaVideoData video_data_cb) = 0;
    virtual int setStunCb(int channel, onStunPacket stun_cb) = 0;
//    virtual int setSendFlag(int channel, bool flag) = 0;
//    virtual bool getSendFlag(int channel) = 0;
    // Enables IPv6, instead of IPv4, for a specified channel.
    virtual int EnableIPv6(int video_channel) = 0;
    // The function returns true if IPv6 is enabled, false otherwise.
    virtual bool IsIPv6Enabled(int video_channel) = 0;
	virtual int RegisterExternalPacketization(const int video_channel, VCMPacketizationCallback * transport) = 0;
	virtual int DeRegisterExternalPacketization(const int video_channel) = 0;

	virtual int RegisterEncoderDataObserver(const int video_channel, VCMPacketizationCallback* observer) = 0;
	virtual int DeRegisterEncoderDataObserver(const int video_channel) = 0;

 protected:
  ViENetwork() {}
  virtual ~ViENetwork() {}
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_NETWORK_H_
