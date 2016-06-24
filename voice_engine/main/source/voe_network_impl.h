/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VOICE_ENGINE_VOE_NETWORK_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_NETWORK_IMPL_H

#include "voe_network.h"

#include "shared_data.h"


namespace cloopenwebrtc
{

class VoENetworkImpl: public VoENetwork
{
public:
    virtual int RegisterExternalTransport(int channel,
                                          Transport& transport) OVERRIDE;

    virtual int DeRegisterExternalTransport(int channel) OVERRIDE;

    virtual int ReceivedRTPPacket(int channel,
                                  const void* data,
                                  size_t length) OVERRIDE;
    virtual int ReceivedRTPPacket(int channel,
                                  const void* data,
                                  size_t length,
                                  const PacketTime& packet_time) OVERRIDE;

    virtual int ReceivedRTCPPacket(int channel,
                                   const void* data,
                                   size_t length) OVERRIDE;

	//---begin
	virtual int getNetworkStatistic(int channel, 
		time_t &startTime, 
		long long &sendLengthSim,
		long long &recvLengthSim,
		long long &sendLengthWifi,
		long long &recvLengthWifi);

	virtual int SendUDPPacket(int channel,
		const void* data,
		unsigned int length,
		int& transmittedBytes,
		bool useRtcpSocket = false);
	//---end
    // Enables or disables warnings that report if packets have not been
    // received in |timeoutSeconds| seconds for a specific |channel|.
    virtual int SetPacketTimeoutNotification(
                                             int channel, bool enable, int timeoutSeconds = 2);
    // Enables IPv6 for a specified |channel|.
    virtual int EnableIPv6(int channel) OVERRIDE;
    // Gets the current IPv6 staus for a specified |channel|.
    virtual bool IPv6IsEnabled(int channel) OVERRIDE;

protected:
    VoENetworkImpl(voe::SharedData* shared);
    virtual ~VoENetworkImpl();
private:
    voe::SharedData* _shared;
};

}  // namespace webrtc

#endif  // WEBRTC_VOICE_ENGINE_VOE_NETWORK_IMPL_H
