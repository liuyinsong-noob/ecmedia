/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_sender.h"

#include <assert.h>
#include "rtp_sender.h"

#include "rtp_dump.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/trace.h"

#include <time.h>

namespace cloopenwebrtc {

ViESender::ViESender(int channel_id)
    : channel_id_(channel_id),
      critsect_(CriticalSectionWrapper::CreateCriticalSection()),
      transport_(NULL),
      rtp_dump_(NULL),
	  external_encryption_(NULL),
	  encryption_buffer_(NULL),
	  callback_encryption_buffer_(NULL),
	  _startNetworkTime(NULL),
	  _sendDataTotalSim(0),
	  _sendDataTotalWifi(0),
	  _isWifi(false),
	   critsect_net_statistic(CriticalSectionWrapper::CreateCriticalSection()),
      video_data_cb_(NULL){
}

ViESender::~ViESender() {
	if (encryption_buffer_) {
		delete[] encryption_buffer_;
		encryption_buffer_ = NULL;
	}

  if (rtp_dump_) {
    rtp_dump_->Stop();
    RtpDump::DestroyRtpDump(rtp_dump_);
    rtp_dump_ = NULL;
  }
}

int ViESender::RegisterSendTransport(Transport* transport) {
  CriticalSectionScoped cs(critsect_.get());
  if (transport_) {
    return -1;
  }
  transport_ = transport;
  return 0;
}

int ViESender::DeregisterSendTransport() {
  CriticalSectionScoped cs(critsect_.get());
  if (transport_ == NULL) {
    return -1;
  }
  transport_ = NULL;
  return 0;
}

int ViESender::StartRTPDump(const char file_nameUTF8[1024]) {
  CriticalSectionScoped cs(critsect_.get());
  if (rtp_dump_) {
    // Packet dump is already started, restart it.
    rtp_dump_->Stop();
  } else {
    rtp_dump_ = RtpDump::CreateRtpDump();
    if (rtp_dump_ == NULL) {
      return -1;
    }
  }
  if (rtp_dump_->Start(file_nameUTF8) != 0) {
    RtpDump::DestroyRtpDump(rtp_dump_);
    rtp_dump_ = NULL;
    return -1;
  }
  return 0;
}

int ViESender::StopRTPDump() {
  CriticalSectionScoped cs(critsect_.get());
  if (rtp_dump_) {
    if (rtp_dump_->IsActive()) {
      rtp_dump_->Stop();
    }
    RtpDump::DestroyRtpDump(rtp_dump_);
    rtp_dump_ = NULL;
  } else {
    return -1;
  }
  return 0;
}

int ViESender::SendRtp(int vie_id, const uint8_t* packet, size_t length, const PacketOptions* options) {
  CriticalSectionScoped cs(critsect_.get());
  if (!transport_) {
    // No transport
    return -1;
  }
  //assert(ChannelId(vie_id) == channel_id_); //assert happens when open screenshare

  void* tmp_ptr = (void*)(packet);
  unsigned char* send_packet = static_cast<unsigned char*>(tmp_ptr);
  int send_packet_length = length;

  if (rtp_dump_) {
    rtp_dump_->DumpPacket(static_cast<const uint8_t*>(send_packet), send_packet_length);
  }

  if (external_encryption_) {
	  external_encryption_->encrypt(channel_id_, send_packet,
									encryption_buffer_, send_packet_length,
									static_cast<int*>(&send_packet_length));

	  //append ssrc to end, 4 bytes
	  //memcpy(encryption_buffer_ + send_packet_length, encryption_buffer_ + 8, 4);
	  //send_packet_length += 4;
	  send_packet = encryption_buffer_;

  }

    int return_len = 0;
    if (video_data_cb_) {
        video_data_cb_(channel_id_, send_packet+12, send_packet_length-12, callback_encryption_buffer_ +12, return_len, true);
        memcpy(callback_encryption_buffer_, send_packet, 12);
        send_packet = callback_encryption_buffer_;
        send_packet_length = return_len + 12;
    }
    
  int bytes_sent = transport_->SendRtp(channel_id_, send_packet, send_packet_length);

  if (bytes_sent != send_packet_length) {
	  WEBRTC_TRACE(cloopenwebrtc::kTraceWarning, cloopenwebrtc::kTraceVideo, channel_id_,
		  "ViESender::SendPacket - Transport failed to send RTP packet");
  }
  {
	  CriticalSectionScoped cs(critsect_net_statistic.get());
	  if(_startNetworkTime == 0)
		  _startNetworkTime = time(NULL);
	  if (_isWifi) {
		  _sendDataTotalWifi += bytes_sent;
		  _sendDataTotalWifi += 42;//14 + 20 + 8;//ethernet+ip+udp header
	  }
	  else
	  {
		  _sendDataTotalSim += bytes_sent;
		  _sendDataTotalSim += 42;//14 + 20 + 8;//ethernet+ip+udp header
	  }
  }

  return bytes_sent;
}

int ViESender::SendRtcp(int vie_id, const uint8_t* packet, size_t length) {
  CriticalSectionScoped cs(critsect_.get());
  if (!transport_) {
    return -1;
  }
//  assert(ChannelId(vie_id) == channel_id_);

  // Prepare for possible encryption and sending.
  // TODO(mflodman) Change decrypt to get rid of this cast.
  void* tmp_ptr = (void*)(packet);
  unsigned char* send_packet = static_cast<unsigned char*>(tmp_ptr);
  int send_packet_length = length;

  if (rtp_dump_) {
    rtp_dump_->DumpPacket(packet, length);
  }

//  if (external_encryption_) 
  if (false)  //hubin 2017.2.18  we don't support rtcp srtp.
 {
	  external_encryption_->encrypt_rtcp(
		  channel_id_, send_packet, encryption_buffer_, send_packet_length,
		  static_cast<int*>(&send_packet_length));

	  //append ssrc to end, 4 bytes
	  //memcpy(encryption_buffer_ + send_packet_length, encryption_buffer_ + 4, 4);
	  //send_packet_length += 4;
	  send_packet = encryption_buffer_;
  }

  int bytes_sent = transport_->SendRtcp(channel_id_, send_packet, send_packet_length);

  {
	  CriticalSectionScoped cs(critsect_net_statistic.get());
	  if(_startNetworkTime == 0)
		  _startNetworkTime = time(NULL);
	  if (_isWifi) {
		  _sendDataTotalWifi += bytes_sent;
		  _sendDataTotalWifi += 42;//14 + 20 + 8;//ethernet+ip+udp header
	  }
	  else
	  {
		  _sendDataTotalSim += bytes_sent;
		  _sendDataTotalSim += 42;//14 + 20 + 8;//ethernet+ip+udp header
	  }

  }

  return bytes_sent;
}

int ViESender::RegisterExternalEncryption(Encryption* encryption) {
	CriticalSectionScoped cs(critsect_.get());
	if (external_encryption_) {
		return -1;
	}
	encryption_buffer_ = new WebRtc_UWord8[kViEMaxMtu];
	if (encryption_buffer_ == NULL) {
		return -1;
	}
	external_encryption_ = encryption;
	return 0;
}

int ViESender::DeregisterExternalEncryption() {
	CriticalSectionScoped cs(critsect_.get());
	if (external_encryption_ == NULL) {
		return -1;
	}
	if (encryption_buffer_) {
		delete[] encryption_buffer_;
		encryption_buffer_ = NULL;
	}
	external_encryption_ = NULL;
	return 0;
}

void ViESender::setNetworkStatus(bool isWifi)
{
	_isWifi = isWifi;
}

void ViESender::getVieSenderStatistic(long long &startTime, long long &sendDataTotalSim, long long &sendDataTotalWifi)
{
	CriticalSectionScoped cs(critsect_net_statistic.get());
	startTime = _startNetworkTime;
	sendDataTotalSim = _sendDataTotalSim;
	sendDataTotalWifi = _sendDataTotalWifi;
}

int ViESender::SetVideoDataCb(onEcMediaVideoData video_data_cb)
{
    if (!callback_encryption_buffer_) {
		callback_encryption_buffer_ = new WebRtc_UWord8[kViEMaxMtu];
    }
    if (callback_encryption_buffer_ == NULL) {
        return -1;
    }
    video_data_cb_ = video_data_cb;
    return 0;
}
}  // namespace webrtc
