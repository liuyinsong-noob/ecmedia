/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "voe_network_impl.h"

#include "../system_wrappers/include/format_macros.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "logging.h"
#include "../system_wrappers/include/trace.h"
#include "channel.h"
#include "voe_errors.h"
#include "voice_engine_impl.h"
#include "audio_coding_module_impl.h"

namespace yuntongxunwebrtc
{

VoENetwork* VoENetwork::GetInterface(VoiceEngine* voiceEngine)
{
    if (NULL == voiceEngine)
    {
        return NULL;
    }
    VoiceEngineImpl* s = static_cast<VoiceEngineImpl*>(voiceEngine);
    s->AddRef();
    return s;
}

VoENetworkImpl::VoENetworkImpl(voe::SharedData* shared) : _shared(shared)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "VoENetworkImpl() - ctor");
}

VoENetworkImpl::~VoENetworkImpl()
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "~VoENetworkImpl() - dtor");
}

int VoENetworkImpl::RegisterExternalTransport(int channel,
                                              Transport& transport)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetExternalTransport(channel=%d, transport=0x%x)",
                 channel, &transport);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "SetExternalTransport() failed to locate channel");
        return -1;
    }
    //add by xzq for live stream
    transport.SetRtpData(channel,channelPtr,0);
    //end 
    return channelPtr->RegisterExternalTransport(transport);
}

int VoENetworkImpl::DeRegisterExternalTransport(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "DeRegisterExternalTransport(channel=%d)", channel);
    if (!_shared->statistics().Initialized())
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice,
                     VoEId(_shared->instance_id(), -1),
                     "DeRegisterExternalTransport() - invalid state");
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "DeRegisterExternalTransport() failed to locate channel");
        return -1;
    }
    return channelPtr->DeRegisterExternalTransport();
}

int VoENetworkImpl::RegisterExternalPacketization(int channel,
	AudioPacketizationCallback* transport)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
		"SetExternalPacketization(channel=%d, transport=0x%x)",
		channel, &transport);
	if (!_shared->statistics().Initialized())
	{
		_shared->SetLastError(VE_NOT_INITED, kTraceError);
		return -1;
	}
	voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
	voe::Channel* channelPtr = ch.channel();
	if (channelPtr == NULL)
	{
		_shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
			"SetExternalTransport() failed to locate channel");
		return -1;
	}
	return channelPtr->RegisterExternalPacketization(transport);
}

int VoENetworkImpl::DeRegisterExternalPacketization(int channel)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
		"DeRegisterExternalTransport(channel=%d)", channel);
	if (!_shared->statistics().Initialized())
	{
		WEBRTC_TRACE(kTraceError, kTraceVoice,
			VoEId(_shared->instance_id(), -1),
			"DeRegisterExternalTransport() - invalid state");
	}
	voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
	voe::Channel* channelPtr = ch.channel();
	if (channelPtr == NULL)
	{
		_shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
			"DeRegisterExternalTransport() failed to locate channel");
		return -1;
	}
	return channelPtr->DeRegisterExternalPacketization();
}

int VoENetworkImpl::ReceivedRTPPacket(int channel,
                                      const void* data,
                                      size_t length) {
  return ReceivedRTPPacket(channel, data, length, yuntongxunwebrtc::PacketTime());
}

int VoENetworkImpl::ReceivedRTPPacket(int channel,
                                      const void* data,
                                      size_t length,
                                      const PacketTime& packet_time)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "ReceivedRTPPacket(channel=%d, length=%" PRIuS ")", channel,
                 length);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    // L16 at 32 kHz, stereo, 10 ms frames (+12 byte RTP header) -> 1292 bytes
    if ((length < 12) || (length > 1292))
    {
        _shared->SetLastError(VE_INVALID_PACKET);
//        LOG(LS_ERROR) << "Invalid packet length: " << length;
        return -1;
    }
    if (NULL == data)
    {
        _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
            "ReceivedRTPPacket() invalid data vector");
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "ReceivedRTPPacket() failed to locate channel");
        return -1;
    }

    if (!channelPtr->ExternalTransport())
    {
        _shared->SetLastError(VE_INVALID_OPERATION, kTraceError,
            "ReceivedRTPPacket() external transport is not enabled");
        return -1;
    }
    return channelPtr->ReceivedRTPPacket((const int8_t*) data, length,
                                         packet_time);
}

int VoENetworkImpl::ReceivedRTCPPacket(int channel, const void* data,
                                       size_t length)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "ReceivedRTCPPacket(channel=%d, length=%" PRIuS ")", channel,
                 length);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    if (length < 4)
    {
        _shared->SetLastError(VE_INVALID_PACKET, kTraceError,
            "ReceivedRTCPPacket() invalid packet length");
        return -1;
    }
    if (NULL == data)
    {
        _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
            "ReceivedRTCPPacket() invalid data vector");
        return -1;
    }
    voe::ChannelOwner ch = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = ch.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
            "ReceivedRTCPPacket() failed to locate channel");
        return -1;
    }
    if (!channelPtr->ExternalTransport())
    {
        _shared->SetLastError(VE_INVALID_OPERATION, kTraceError,
            "ReceivedRTCPPacket() external transport is not enabled");
        return -1;
    }
    return channelPtr->ReceivedRTCPPacket((const int8_t*) data, length);
}

int VoENetworkImpl::getNetworkStatistic(int channel,  time_t &startTime, long long &sendLengthSim, long long &recvLengthSim, long long &sendLengthWifi, long long &recvLengthWifi)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
		"getNetworkStatistics channel=%d ", channel);
	if (!_shared->statistics().Initialized())
	{
		_shared->SetLastError(VE_NOT_INITED, kTraceError);
		return -1;
	}
	voe::ChannelOwner sc = _shared->channel_manager().GetChannel(channel);
	voe::Channel* channelPtr = sc.channel();
	if (channelPtr == NULL)
	{
		_shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
			"getNetworkStatistic() failed to locate channel");
		return -1;
	}
	channelPtr->getNetworkStatistic(startTime, sendLengthSim, recvLengthSim, sendLengthWifi, recvLengthWifi);
	return 0;
}

int VoENetworkImpl::SendUDPPacket(int channel,
	const void* data,
	unsigned int length,
	int& transmittedBytes,
	bool useRtcpSocket)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
		"SendUDPPacket(channel=%d, data=0x%x, length=%u, useRTCP=%d)",
		channel, data, length, useRtcpSocket);
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	if (!_shared->statistics().Initialized())
	{
		_shared->SetLastError(VE_NOT_INITED, kTraceError);
		return -1;
	}
	if (NULL == data)
	{
		_shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
			"SendUDPPacket() invalid data buffer");
		return -1;
	}
	if (0 == length)
	{
		_shared->SetLastError(VE_INVALID_PACKET, kTraceError,
			"SendUDPPacket() invalid packet size");
		return -1;
	}
	voe::ChannelOwner sc = _shared->channel_manager().GetChannel(channel);
	voe::Channel* channelPtr = sc.channel();
	if (channelPtr == NULL)
	{
		_shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
			"SendUDPPacket() failed to locate channel");
		return -1;
	}
	return channelPtr->SendUDPPacket(data,
		length,
		transmittedBytes,
		useRtcpSocket);
#else
	_shared->SetLastError(VE_EXTERNAL_TRANSPORT_ENABLED, kTraceWarning,
		"SendUDPPacket() VoE is built for external transport");
	return -1;
#endif
}
    
int VoENetworkImpl::SetPacketTimeoutNotification(int channel,
                                                 bool enable,
                                                 int timeoutSeconds)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "SetPacketTimeoutNotification(channel=%d, enable=%d, "
                 "timeoutSeconds=%d)",
                 channel, (int) enable, timeoutSeconds);
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    if (enable &&
        ((timeoutSeconds < kVoiceEngineMinPacketTimeoutSec) ||
         (timeoutSeconds > kVoiceEngineMaxPacketTimeoutSec)))
    {
        _shared->SetLastError(VE_INVALID_ARGUMENT, kTraceError,
                              "SetPacketTimeoutNotification() invalid timeout size");
        return -1;
    }
    voe::ChannelOwner sc = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = sc.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
                              "SetPacketTimeoutNotification() failed to locate channel");
        return -1;
    }
    return channelPtr->SetPacketTimeoutNotification(enable, timeoutSeconds);
}
    
int VoENetworkImpl::EnableIPv6(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "EnableIPv6(channel=%d)", channel);
    
#ifndef WEBRTC_EXTERNAL_TRANSPORT
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return -1;
    }
    voe::ChannelOwner sc = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = sc.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
                              "EnableIPv6() failed to locate channel");
        return -1;
    }
    if (channelPtr->ExternalTransport())
    {
        _shared->SetLastError(VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
                              "EnableIPv6() external transport is enabled");
        return -1;
    }
    return channelPtr->EnableIPv6();
#else
    _shared->SetLastError(VE_EXTERNAL_TRANSPORT_ENABLED, kTraceWarning,
                          "EnableIPv6() VoE is built for external transport");
    return -1;
#endif
}

bool VoENetworkImpl::IPv6IsEnabled(int channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, VoEId(_shared->instance_id(), -1),
                 "IPv6IsEnabled(channel=%d)", channel);
#ifndef WEBRTC_EXTERNAL_TRANSPORT
    if (!_shared->statistics().Initialized())
    {
        _shared->SetLastError(VE_NOT_INITED, kTraceError);
        return false;
    }
    voe::ChannelOwner sc = _shared->channel_manager().GetChannel(channel);
    voe::Channel* channelPtr = sc.channel();
    if (channelPtr == NULL)
    {
        _shared->SetLastError(VE_CHANNEL_NOT_VALID, kTraceError,
                              "IPv6IsEnabled() failed to locate channel");
        return false;
    }
    if (channelPtr->ExternalTransport())
    {
        _shared->SetLastError(VE_EXTERNAL_TRANSPORT_ENABLED, kTraceError,
                              "IPv6IsEnabled() external transport is enabled");
        return false;
    }
    return channelPtr->IPv6IsEnabled();
#else
    _shared->SetLastError(VE_EXTERNAL_TRANSPORT_ENABLED, kTraceWarning,
                          "IPv6IsEnabled() VoE is built for external transport");
    return false;
#endif
}
}  // namespace yuntongxunwebrtc
