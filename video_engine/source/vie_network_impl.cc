/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_network_impl.h"

#include <stdio.h>
#if (defined(WIN32_) || defined(WIN64_))
#include <qos.h>
#endif

#include "engine_configurations.h"
#include "logging.h"
#include "vie_errors.h"
#include "vie_channel.h"
#include "vie_channel_manager.h"
#include "vie_defines.h"
#include "vie_encoder.h"
#include "vie_impl.h"
#include "vie_shared_data.h"

#include "trace.h"

namespace cloopenwebrtc {

ViENetwork* ViENetwork::GetInterface(VideoEngine* video_engine) {
  if (!video_engine) {
    return NULL;
  }
  VideoEngineImpl* vie_impl = static_cast<VideoEngineImpl*>(video_engine);
  ViENetworkImpl* vie_networkImpl = vie_impl;
  // Increase ref count.
  (*vie_networkImpl)++;
  return vie_networkImpl;
}

int ViENetworkImpl::Release() {
  // Decrease ref count.
  (*this)--;

  int32_t ref_count = GetCount();
  if (ref_count < 0) {
    LOG(LS_ERROR) << "ViENetwork release too many times";
    shared_data_->SetLastError(kViEAPIDoesNotExist);
    return -1;
  }
  return ref_count;
}

void ViENetworkImpl::SetNetworkTransmissionState(const int video_channel,
                                                 const bool is_transmitting) {
  LOG_F(LS_INFO) << "channel: " << video_channel
                 << " transmitting: " << (is_transmitting ? "yes" : "no");
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEEncoder* vie_encoder = cs.Encoder(video_channel);
  if (!vie_encoder) {
    shared_data_->SetLastError(kViENetworkInvalidChannelId);
    return;
  }
  vie_encoder->SetNetworkTransmissionState(is_transmitting);
}

ViENetworkImpl::ViENetworkImpl(ViESharedData* shared_data)
    : shared_data_(shared_data) {}

ViENetworkImpl::~ViENetworkImpl() {}

int ViENetworkImpl::RegisterSendTransport(const int video_channel,
                                          Transport& transport) {
  LOG_F(LS_INFO) << "channel: " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViENetworkInvalidChannelId);
    return -1;
  }
  if (vie_channel->Sending()) {
    LOG_F(LS_ERROR) << "Already sending on channel: " << video_channel;
    shared_data_->SetLastError(kViENetworkAlreadySending);
    return -1;
  }
  if (vie_channel->RegisterSendTransport(&transport) != 0) {
    shared_data_->SetLastError(kViENetworkUnknownError);
    return -1;
  }
  transport.SetRtpData(video_channel,vie_channel->GetReceiver(),1);
  return 0;
}

int ViENetworkImpl::DeregisterSendTransport(const int video_channel) {
  LOG_F(LS_INFO) << "channel: " << video_channel;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViENetworkInvalidChannelId);
    return -1;
  }
  if (vie_channel->Sending()) {
    LOG_F(LS_ERROR) << "Actively sending on channel: " << video_channel;
    shared_data_->SetLastError(kViENetworkAlreadySending);
    return -1;
  }
  if (vie_channel->DeregisterSendTransport() != 0) {
    shared_data_->SetLastError(kViENetworkUnknownError);
    return -1;
  }
  return 0;
}


int ViENetworkImpl::RegisterExternalPacketization(const int video_channel,
	VCMPacketizationCallback* transport) {
	LOG_F(LS_INFO) << "channel: " << video_channel;
	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(video_channel);
	if (!vie_channel) {
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}
	ViEEncoder* vie_encoder = cs.Encoder(video_channel);
	assert(vie_encoder);
	vie_encoder->RegisterExternalPacketization(transport);
	return 0;
}

int ViENetworkImpl::DeRegisterExternalPacketization(const int video_channel) {
	LOG_F(LS_INFO) << "channel: " << video_channel;
	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(video_channel);
	if (!vie_channel) {
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}
	ViEEncoder* vie_encoder = cs.Encoder(video_channel);
	assert(vie_encoder);
	vie_encoder->RegisterExternalPacketization(NULL);
	
	return 0;
}


int ViENetworkImpl::RegisterEncoderDataObserver(const int video_channel,
	VCMPacketizationCallback* observer) {
	LOG_F(LS_INFO) << "channel: " << video_channel;
	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(video_channel);
	if (!vie_channel) {
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}
	ViEEncoder* vie_encoder = cs.Encoder(video_channel);
	assert(vie_encoder);
	vie_encoder->RegisterEncoderDataObserver(observer);
	return 0;
}

int ViENetworkImpl::DeRegisterEncoderDataObserver(const int video_channel) {
	LOG_F(LS_INFO) << "channel: " << video_channel;
	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(video_channel);
	if (!vie_channel) {
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}
	ViEEncoder* vie_encoder = cs.Encoder(video_channel);
	assert(vie_encoder);
	vie_encoder->DeRegisterEncoderDataObserver();

	return 0;
}

int ViENetworkImpl::ReceivedRTPPacket(const int video_channel, const void* data,
                                      const size_t length,
                                      const PacketTime& packet_time) {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViENetworkInvalidChannelId);
    return -1;
  }
  return vie_channel->ReceivedRTPPacket(data, length, packet_time);
}

int ViENetworkImpl::ReceivedRTCPPacket(const int video_channel,
                                       const void* data, const size_t length) {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViENetworkInvalidChannelId);
    return -1;
  }
  return vie_channel->ReceivedRTCPPacket(data, length);
}

int ViENetworkImpl::SetMTU(int video_channel, unsigned int mtu) {
  LOG_F(LS_INFO) << "channel: " << video_channel << " mtu: " << mtu;
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViENetworkInvalidChannelId);
    return -1;
  }
  if (vie_channel->SetMTU(mtu) != 0) {
    shared_data_->SetLastError(kViENetworkUnknownError);
    return -1;
  }
  return 0;
}

int ViENetworkImpl::ReceivedBWEPacket(const int video_channel,
                                      int64_t arrival_time_ms,
                                      size_t payload_size,
                                      const RTPHeader& header) {
  ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
  ViEChannel* vie_channel = cs.Channel(video_channel);
  if (!vie_channel) {
    shared_data_->SetLastError(kViENetworkInvalidChannelId);
    return -1;
  }

  vie_channel->ReceivedBWEPacket(arrival_time_ms, payload_size, header);
  return 0;
}

bool ViENetworkImpl::SetBandwidthEstimationConfig(
    int video_channel, const cloopenwebrtc::Config& config) {
  LOG_F(LS_INFO) << "channel: " << video_channel;
  return shared_data_->channel_manager()->SetBandwidthEstimationConfig(
      video_channel, config);
}

//int ViENetworkImpl::RegisterServiceCoreCallBack(int channel, ServiceCoreCallBack *messageCallBack, const char* call_id, int firewall_policy)
//{
//
//	LOG_F(LS_INFO) << "channel: " << channel;
//	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
//	ViEChannel* vie_channel = cs.Channel(channel);
//	if (!vie_channel) {
//		// The channel doesn't exists.
//		LOG_F(LS_ERROR) << "Channel doesn't exist";
//		shared_data_->SetLastError(kViENetworkInvalidChannelId);
//		return -1;
//	}
//
//	return vie_channel->RegisterServiceCoreCallBack(messageCallBack, call_id,firewall_policy);
//
//}

//sean add begin 0915
int ViENetworkImpl::setShieldMosaic(int channel,bool flag)
{
	LOG_F(LS_INFO) << __FUNCTION__ << "channel:" <<channel;
	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(channel);
	if (!vie_channel) {
		// The channel doesn't exists.
		LOG_F(LS_ERROR) << "Channel doesn't exist";
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}

	return vie_channel->SetShieldMosaic(flag);
}
//sean add end 0915

int ViENetworkImpl::setVideoConferenceFlag(int channel, const char *selfSipNo , const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip)
{
	LOG_F(LS_INFO) << __FUNCTION__ << "channel:" <<channel;	

	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(channel);
	if (!vie_channel) {
		// The channel doesn't exists.
		LOG_F(LS_ERROR) << "Channel doesn't exist";
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}

	return vie_channel->SetVideoConferenceFlag(selfSipNo, sipNo, conferenceNo, confPasswd, port, ip);
}

int ViENetworkImpl::setProcessData(int channel, bool flag)
{
	//Don't implement at the moment.
	return 0;
}


int ViENetworkImpl::getNetworkStatistic(const int channel, time_t &startTime, long long &sendLengthSim, long long &recvLengthSim, long long &sendLengthWifi, long long &recvLengthWifi)
{
	LOG_F(LS_INFO) << __FUNCTION__ << "channel:" <<channel;
	
	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(channel);
	if (!vie_channel) {
		LOG_F(LS_ERROR) << "Channel doesn't exist";
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}
	vie_channel->getNetworkStatistic(startTime, sendLengthSim, recvLengthSim, sendLengthWifi, recvLengthWifi);
	return 0;
}

int ViENetworkImpl::setNetworkType(int channel, bool isWifi)
{
	LOG_F(LS_INFO) << __FUNCTION__ << "channel:" <<channel;
	ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
	ViEChannel* vie_channel = cs.Channel(channel);
	if (!vie_channel) {
		LOG_F(LS_ERROR) << "Channel doesn't exist";
		shared_data_->SetLastError(kViENetworkInvalidChannelId);
		return -1;
	}
	vie_channel->setNetworkType(isWifi);
	return 0;
}

int ViENetworkImpl::SetSendDestination(const int video_channel,
		const char *rtp_ip_address,
		const unsigned short rtp_port,
		const char *rtcp_ip_address,
		const unsigned short rtcp_port,
		const unsigned short source_rtp_port,
		const unsigned short source_rtcp_port)
	{
		WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
			ViEId(shared_data_->instance_id(), video_channel),
			"%s(channel: %d, rtp_ip_address: %s, rtp_port: %u, rtcp_ip_address: %s, rtcp_port: %u, "
			"sourceRtpPort: %u, source_rtcp_port: %u)",
			__FUNCTION__, video_channel, rtp_ip_address, rtp_port, rtcp_ip_address, rtcp_port,
			source_rtp_port, source_rtcp_port);
		/*if (!shared_data_->Initialized()) {
			shared_data_->SetLastError(kViENotInitialized);
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
				"%s - ViE instance %d not initialized", __FUNCTION__,
				shared_data_->instance_id());
			return -1;
		}*/

		ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
		ViEChannel* vie_channel = cs.Channel(video_channel);
		if (!vie_channel) {
			WEBRTC_TRACE(kTraceError, kTraceVideo,
				ViEId(shared_data_->instance_id(), video_channel),
				"%s Channel doesn't exist", __FUNCTION__);
			shared_data_->SetLastError(kViENetworkInvalidChannelId);
			return -1;
		}
		/*delete begin------------------Sean20130724----------for video ice------------*/
		//    currently it works ok
		//  if (vie_channel->Sending()) {
		//    WEBRTC_TRACE(kTraceError, kTraceVideo,
		//                 ViEId(shared_data_->instance_id(), video_channel),
		//                 "%s Channel already sending.", __FUNCTION__);
		//    shared_data_->SetLastError(kViENetworkAlreadySending);
		//    return -1;
		//  }
		/*delete end--------------------Sean20130724----------for video ice------------*/
		if (vie_channel->SetSendDestination(rtp_ip_address, rtp_port, rtcp_ip_address, rtcp_port, source_rtp_port, source_rtcp_port) != 0) {
				shared_data_->SetLastError(kViENetworkUnknownError);
				return -1;
		}
		return 0;
}

	int ViENetworkImpl::SetSocks5SendData(int channel_id, unsigned char *data, int length, bool isRTCP) {
		ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
		ViEChannel* vie_channel = cs.Channel(channel_id);
		if (!vie_channel) {
			WEBRTC_TRACE(kTraceError, kTraceVideo,
					ViEId(shared_data_->instance_id(), channel_id),
					"%s Channel doesn't exist", __FUNCTION__);
			shared_data_->SetLastError(kViENetworkInvalidChannelId);
			return -1;
		}

		if (vie_channel->SetSocks5SendData(data, length, isRTCP) != 0) {
			shared_data_->SetLastError(kViENetworkUnknownError);
			return -1;
		}
		return 0;
	}


int ViENetworkImpl::GetSendDestination(const int video_channel,
	char* ip_address,
	unsigned short& rtp_port,
	unsigned short& rtcp_port,
	unsigned short& source_rtp_port,
	unsigned short& source_rtcp_port) {
		WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
			ViEId(shared_data_->instance_id(), video_channel),
			"%s(channel: %d)", __FUNCTION__, video_channel);
		ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
		ViEChannel* vie_channel = cs.Channel(video_channel);
		if (!vie_channel) {
			WEBRTC_TRACE(kTraceError, kTraceVideo,
				ViEId(shared_data_->instance_id(), video_channel),
				"Channel doesn't exist");
			shared_data_->SetLastError(kViENetworkInvalidChannelId);
			return -1;
		}
		if (vie_channel->GetSendDestination(ip_address, rtp_port, rtcp_port,
			source_rtp_port,
			source_rtcp_port) != 0) {
				shared_data_->SetLastError(kViENetworkDestinationNotSet);
				return -1;
		}
		return 0;
}
int ViENetworkImpl::SetLocalReceiver(const int video_channel,
	const unsigned short rtp_port,
	const unsigned short rtcp_port,
	const char* ip_address) {
		WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
			ViEId(shared_data_->instance_id(), video_channel),
			"%s(channel: %d, rtp_port: %u, rtcp_port: %u, ip_address: %s)",
			__FUNCTION__, video_channel, rtp_port, rtcp_port, ip_address);
		/*if (!shared_data_->Initialized()) {
			shared_data_->SetLastError(kViENotInitialized);
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
				"%s - ViE instance %d not initialized", __FUNCTION__,
				shared_data_->instance_id());
			return -1;
		}*/

		ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
		ViEChannel* vie_channel = cs.Channel(video_channel);
		if (!vie_channel) {
			// The channel doesn't exists.
			WEBRTC_TRACE(kTraceError, kTraceVideo,
				ViEId(shared_data_->instance_id(), video_channel),
				"Channel doesn't exist");
			shared_data_->SetLastError(kViENetworkInvalidChannelId);
			return -1;
		}

		if (vie_channel->Receiving()) {
			shared_data_->SetLastError(kViENetworkAlreadyReceiving);
			return -1;
		}
		if (vie_channel->SetLocalReceiver(rtp_port, rtcp_port, ip_address) != 0) {
			shared_data_->SetLastError(kViENetworkUnknownError);
			return -1;
		}
		return 0;
}

int ViENetworkImpl::GetLocalReceiver(const int video_channel,
	unsigned short& rtp_port,
	unsigned short& rtcp_port,
	char* ip_address) {
		WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
			ViEId(shared_data_->instance_id(), video_channel),
			"%s(channel: %d)", __FUNCTION__, video_channel);

		ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
		ViEChannel* vie_channel = cs.Channel(video_channel);
		if (!vie_channel) {
			WEBRTC_TRACE(kTraceError, kTraceVideo,
				ViEId(shared_data_->instance_id(), video_channel),
				"Channel doesn't exist");
			shared_data_->SetLastError(kViENetworkInvalidChannelId);
			return -1;
		}
		if (vie_channel->GetLocalReceiver(rtp_port,rtcp_port, ip_address) != 0) {
			shared_data_->SetLastError(kViENetworkLocalReceiverNotSet);
			return -1;
		}
		return 0;
}

int ViENetworkImpl::SendUDPPacket(const int video_channel, const void* data,
	const unsigned int length,
	int& transmitted_bytes,
	bool use_rtcp_socket,
	/*add begin------------------Sean20130723----------for video ice------------*/
	WebRtc_UWord16 portnr,
	const char* ip
	/*add end--------------------Sean20130723----------for video ice------------*/
	) {
		WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
			ViEId(shared_data_->instance_id(), video_channel),
			"%s(channel: %d, data: -, length: %d, transmitter_bytes: -, "
			"useRtcpSocket: %d)", __FUNCTION__, video_channel, length,
			use_rtcp_socket);
		/*if (!shared_data_->Initialized()) {
			shared_data_->SetLastError(kViENotInitialized);
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
				"%s - ViE instance %d not initialized", __FUNCTION__,
				shared_data_->instance_id());
			return -1;
		}*/
		ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
		ViEChannel* vie_channel = cs.Channel(video_channel);
		if (!vie_channel) {
			WEBRTC_TRACE(kTraceError, kTraceVideo,
				ViEId(shared_data_->instance_id(), video_channel),
				"Channel doesn't exist");
			shared_data_->SetLastError(kViENetworkInvalidChannelId);
			return -1;
		}
		if (vie_channel->SendUDPPacket((const WebRtc_Word8*) data, length,
			(WebRtc_Word32&) transmitted_bytes,
			use_rtcp_socket,portnr,ip) < 0) {
				shared_data_->SetLastError(kViENetworkUnknownError);
				return -1;
		}
		return 0;
}

int ViENetworkImpl::setVideoConfCb(int channel, onVideoConference video_conf_cb)
{
    
    LOG_F(LS_INFO) <<__FUNCTION__<< " channel: " << channel;
    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEChannel* vie_channel = cs.Channel(channel);
    if (!vie_channel) {
        shared_data_->SetLastError(kViENetworkInvalidChannelId);
        LOG_F(LS_ERROR)<<"Channel doesn't exist";
        return -1;
    }
    vie_channel->setVideoConfCb(video_conf_cb);
    return 0;
}

int ViENetworkImpl::setVideoDataCb(int channel, onEcMediaVideoData video_data_cb)
{
    LOG_F(LS_INFO) <<__FUNCTION__<< " channel: " << channel;
    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEChannel* vie_channel = cs.Channel(channel);
    if (!vie_channel) {
        shared_data_->SetLastError(kViENetworkInvalidChannelId);
        LOG_F(LS_ERROR)<<"Channel doesn't exist";
        return -1;
    }
    vie_channel->setVideoDataCb(video_data_cb);
    return 0;
}

int ViENetworkImpl::setStunCb(int channel, onStunPacket stun_cb)
{
    LOG_F(LS_INFO) <<__FUNCTION__<< " channel: " << channel;
    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEChannel* vie_channel = cs.Channel(channel);
    if (!vie_channel) {
        shared_data_->SetLastError(kViENetworkInvalidChannelId);
        LOG_F(LS_ERROR)<<"Channel doesn't exist";
        return -1;
    }
    vie_channel->setStunCb(stun_cb);
    return 0;
}
    
int ViENetworkImpl::EnableIPv6(int video_channel) {
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
                 ViEId(shared_data_->instance_id(), video_channel),
                 "%s(channel: %d)", __FUNCTION__, video_channel);
    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEChannel* vie_channel = cs.Channel(video_channel);
    if (!vie_channel) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
                     ViEId(shared_data_->instance_id(), video_channel),
                     "Channel doesn't exist");
        shared_data_->SetLastError(kViENetworkInvalidChannelId);
        return -1;
    }
    if (vie_channel->EnableIPv6() != 0) {
        shared_data_->SetLastError(kViENetworkUnknownError);
        return -1;
    }
    return 0;
}

bool ViENetworkImpl::IsIPv6Enabled(int video_channel) {
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
                 ViEId(shared_data_->instance_id(), video_channel),
                 "%s(channel: %d)", __FUNCTION__, video_channel);
    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEChannel* vie_channel = cs.Channel(video_channel);
    if (!vie_channel) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
                     ViEId(shared_data_->instance_id(), video_channel),
                     "Channel doesn't exist");
        shared_data_->SetLastError(kViENetworkInvalidChannelId);
        return false;
    }
    return vie_channel->IsIPv6Enabled();
}


    
}  // namespace webrtc
