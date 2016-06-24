/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// ViESender is responsible for sending packets to network.

#ifndef WEBRTC_VIDEO_ENGINE_VIE_SENDER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_SENDER_H_

#include "common_types.h"
#include "engine_configurations.h"
#include "scoped_ptr.h"
#include "typedefs.h"
#include "vie_defines.h"

namespace cloopenwebrtc {
typedef int (*onEcMediaVideoData)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
class CriticalSectionWrapper;
class RtpDump;
class Transport;
class VideoCodingModule;

class ViESender: public Transport {
 public:
  explicit ViESender(const int32_t channel_id);
  ~ViESender();

  // Registers transport to use for sending RTP and RTCP.
  int RegisterSendTransport(Transport* transport);
  int DeregisterSendTransport();

  // Stores all incoming packets to file.
  int StartRTPDump(const char file_nameUTF8[1024]);
  int StopRTPDump();

  // Implements Transport.
  virtual int SendPacket(int vie_id, const void* data, size_t len, int sn = 0) OVERRIDE;
  virtual int SendRTCPPacket(int vie_id, const void* data, size_t len) OVERRIDE;

 private:
  const int32_t channel_id_;

  scoped_ptr<CriticalSectionWrapper> critsect_;

  Transport* transport_;
  RtpDump* rtp_dump_;
  //---begin
public:
	// Registers an encryption class to use before sending packets.
	int RegisterExternalEncryption(Encryption* encryption);
	int DeregisterExternalEncryption();

	void getVieSenderStatistic(long long &startTime, long long &sendDataTotalSim, long long &sendDataTotalWifi);
	void setNetworkStatus(bool isWifi);
private:
	Encryption* external_encryption_;
	WebRtc_UWord8* encryption_buffer_;

	time_t  _startNetworkTime;
	long long _sendDataTotalSim;
	long long _sendDataTotalWifi;
	bool _isWifi;
	scoped_ptr<CriticalSectionWrapper> critsect_net_statistic;
  //---end
public:
    int SetVideoDataCb(onEcMediaVideoData video_data_cb);
private:
    onEcMediaVideoData video_data_cb_;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_SENDER_H_
