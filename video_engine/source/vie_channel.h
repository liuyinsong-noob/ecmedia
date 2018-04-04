/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_H_

#include <list>

#include "remote_bitrate_estimator.h"
#include "rtp_rtcp.h"
#include "rtp_rtcp_defines.h"
#include "video_coding_defines.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/scoped_ptr.h"
#include "../system_wrappers/include/tick_util.h"
#include "typedefs.h"
#include "vie_network.h"
#include "vie_rtp_rtcp.h"
#include "vie_defines.h"
#include "vie_frame_provider_base.h"
#include "vie_receiver.h"
#include "vie_sender.h"
#include "vie_sync_module.h"

#include "../base/thread_annotations.h"
//#include "StunMessageCallBack.h"

#ifndef WEBRTC_EXTERNAL_TRANSPORT
#include "udp_transport.h"
#else
#include "tcp_transport.h"
#endif

#include "vie_file_recorder.h"

#ifdef WEBRTC_SRTP
#include "SrtpModule.h"
#endif

namespace cloopenwebrtc {
typedef int (*onEcMediaRequestKeyFrame)(const int channelid);
class CallStatsObserver;
class ChannelStatsObserver;
class Config;
class CriticalSectionWrapper;
class EncodedImageCallback;
class I420FrameCallback;
class PacedSender;
class ProcessThread;
class ReceiveStatisticsProxy;
class ReportBlockStats;
class RtcpRttStats;
class ThreadWrapper;
class ViEDecoderObserver;
class ViEEffectFilter;
class ViERTPObserver;
class VideoCodingModule;
class VideoDecoder;
class VideoRenderCallback;
class VoEVideoSync;
class TransportFeedbackObserver;
class RtcEventLog;
class SsrcObserver;

enum ResolutionIndex
{
	R_128_96_15 = 0,
	R_160_120_15,
	R_176_144_15,
	R_320_240_15,
	R_352_288_15,
	R_480_360_15,
	R_640_360_15,
	R_640_480_15,
	R_640_480_30,
	R_848_480_15,
	R_848_480_30,
	R_1280_720_15,
	R_1280_720_30,
	R_1920_1080_15,
	R_1920_1080_30,
	R_2048_1080_30
};


struct ResolutionInst
{
	ResolutionIndex index;
	uint16_t  width;
	uint16_t  height;
	uint32_t  targetBitrate;
};


class ViEChannel
    : public VCMFrameTypeCallback,
      public VCMReceiveCallback,
      public VCMReceiveStatisticsCallback,
      public VCMDecoderTimingCallback,
      public VCMPacketRequestCallback,
	  public VCMFrameStorageCallback,//add
      public RtpFeedback,
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	    public UdpTransportData, //add
#else
      public TcpTransportData, //add      
#endif      
      public ViEFrameProviderBase {
 public:
  friend class ChannelStatsObserver;

  ViEChannel(int32_t channel_id,
             int32_t engine_id,
             uint32_t number_of_cores,
             const Config& config,
             // ProcessThread& module_process_thread,
             RtcpIntraFrameObserver* intra_frame_observer,
             RtcpBandwidthObserver* bandwidth_observer,
             RemoteBitrateEstimator* remote_bitrate_estimator,
             RtcpRttStats* rtt_stats,
             PacedSender* paced_sender,
             RtpRtcp* default_rtp_rtcp,
             bool sender,
	         TransportFeedbackObserver* transport_feedback_observer);
  ~ViEChannel();

  int32_t Init();

#ifndef WEBRTC_EXTERNAL_TRANSPORT
  int32_t SetUdpTransport(UdpTransport *transport, int32_t rtp_port);
  UdpTransport *GetUdpTransport();
#else
   int32_t SetTcpTransport(TcpTransport *transport, int32_t rtp_port);
  TcpTransport *GetTcpTransport();
#endif

  // Sets the encoder to use for the channel. |new_stream| indicates the encoder
  // type has changed and we should start a new RTP stream.
  int32_t SetSendCodec(const VideoCodec& video_codec, bool new_stream = true);
  int32_t SetReceiveCodec(const VideoCodec& video_codec);
  int32_t GetReceiveCodec(VideoCodec* video_codec);
  int32_t RegisterCodecObserver(ViEDecoderObserver* observer);
  // Registers an external decoder. |buffered_rendering| means that the decoder
  // will render frames after decoding according to the render timestamp
  // provided by the video coding module. |render_delay| indicates the time
  // needed to decode and render a frame.
  int32_t RegisterExternalDecoder(const uint8_t pl_type,
                                  VideoDecoder* decoder,
                                  bool buffered_rendering,
                                  int32_t render_delay);
  int32_t DeRegisterExternalDecoder(const uint8_t pl_type);
  int32_t ReceiveCodecStatistics(uint32_t* num_key_frames,
                                 uint32_t* num_delta_frames);
  uint32_t DiscardedPackets() const;

  // Returns the estimated delay in milliseconds.
  int ReceiveDelay() const;

  // Only affects calls to SetReceiveCodec done after this call.
  int32_t WaitForKeyFrame(bool wait);

  // If enabled, a key frame request will be sent as soon as there are lost
  // packets. If |only_key_frames| are set, requests are only sent for loss in
  // key frames.
  int32_t SetSignalPacketLossStatus(bool enable, bool only_key_frames);

  void SetRTCPMode(const RtcpMode rtcp_mode);
  RtcpMode GetRTCPMode() const;
  int32_t SetNACKStatus(const bool enable);
  int32_t SetFECStatus(const bool enable,
                       const unsigned char payload_typeRED,
                       const unsigned char payload_typeFEC);
  int32_t SetHybridNACKFECStatus(const bool enable,
                                 const unsigned char payload_typeRED,
                                 const unsigned char payload_typeFEC);
  int SetSenderBufferingMode(int target_delay_ms);
  int SetReceiverBufferingMode(int target_delay_ms);
  int32_t SetKeyFrameRequestMethod(const KeyFrameRequestMethod method);
  void EnableRemb(bool enable);
  int SetSendTimestampOffsetStatus(bool enable, int id);
  int SetReceiveTimestampOffsetStatus(bool enable, int id);
  int SetSendAbsoluteSendTimeStatus(bool enable, int id);
  int SetReceiveAbsoluteSendTimeStatus(bool enable, int id);
  bool GetReceiveAbsoluteSendTimeStatus() const;
  void SetRtcpXrRrtrStatus(bool enable);
  void SetTransmissionSmoothingStatus(bool enable);
  void EnableTMMBR(bool enable);
  int32_t EnableKeyFrameRequestCallback(const bool enable);

  // Sets SSRC for outgoing stream.
  uint32_t GetSSRCNum() { return ssrc_all_num_; };

  int32_t GetResolution(ResolutionInst &info);

  int32_t SetLocalSendSSRC(const uint32_t SSRC, const StreamType usage);

  int32_t SetSSRC(const uint32_t SSRC,
                  const StreamType usage,
                  const unsigned char simulcast_idx);

  int32_t RequestRemoteSSRC(const uint32_t SSRC);

  int32_t CancelRemoteSSRC();

  // Gets SSRC for outgoing stream number |idx|.
  int32_t GetLocalSSRC(uint8_t idx, unsigned int* ssrc);

  // Gets SSRC for the incoming stream.
  int32_t GetRemoteSSRC(uint32_t* ssrc);

  // Gets the CSRC for the incoming stream.
  int32_t GetRemoteCSRC(uint32_t CSRCs[kRtpCsrcSize]);

  int SetRtxSendPayloadType(int payload_type);
  void SetRtxReceivePayloadType(int payload_type);

  // Sets the starting sequence number, must be called before StartSend.
  int32_t SetStartSequenceNumber(uint16_t sequence_number);

  void SetRtpStateForSsrc(uint32_t ssrc, const RtpState& rtp_state);
  RtpState GetRtpStateForSsrc(uint32_t ssrc);

  // Sets the CName for the outgoing stream on the channel.
  int32_t SetRTCPCName(const char rtcp_cname[]);

  // Gets the CName of the incoming stream.
  int32_t GetRemoteRTCPCName(char rtcp_cname[]);
  int32_t RegisterRtpObserver(ViERTPObserver* observer);
  int32_t SendApplicationDefinedRTCPPacket(
      const uint8_t sub_type,
      uint32_t name,
      const uint8_t* data,
      uint16_t data_length_in_bytes);

  // Returns statistics reported by the remote client in an RTCP packet.
  int32_t GetSendRtcpStatistics(uint16_t* fraction_lost,
                                uint32_t* cumulative_lost,
                                uint32_t* extended_max,
                                uint32_t* jitter_samples,
                                int64_t* rtt_ms);

  // Called on receipt of RTCP report block from remote side.
  void RegisterSendChannelRtcpStatisticsCallback(
      RtcpStatisticsCallback* callback);

  // Returns our localy created statistics of the received RTP stream.
  int32_t GetReceivedRtcpStatistics(uint16_t* fraction_lost,
                                    uint32_t* cumulative_lost,
                                    uint32_t* extended_max,
                                    uint32_t* jitter_samples,
                                    int64_t* rtt_ms);

  // Called on generation of RTCP stats
  void RegisterReceiveChannelRtcpStatisticsCallback(
      RtcpStatisticsCallback* callback);

  // Gets sent/received packets statistics.
  int32_t GetRtpStatistics(size_t* bytes_sent,
                           uint32_t* packets_sent,
                           size_t* bytes_received,
                           uint32_t* packets_received) const;

  // Gets send statistics for the rtp and rtx stream.
  void GetSendStreamDataCounters(StreamDataCounters* rtp_counters,
                                 StreamDataCounters* rtx_counters) const;

  // Gets received stream data counters.
  void GetReceiveStreamDataCounters(StreamDataCounters* rtp_counters,
                                    StreamDataCounters* rtx_counters) const;

  // Called on update of RTP statistics.
  void RegisterSendChannelRtpStatisticsCallback(
      StreamDataCountersCallback* callback);

  // Called on update of RTP statistics.
  void RegisterReceiveChannelRtpStatisticsCallback(
      StreamDataCountersCallback* callback);

  void GetRtcpPacketTypeCounters(RtcpPacketTypeCounter* packets_sent,
                                 RtcpPacketTypeCounter* packets_received) const;

  void GetBandwidthUsage(uint32_t* total_bitrate_sent,
                         uint32_t* video_bitrate_sent,
                         uint32_t* fec_bitrate_sent,
                         uint32_t* nackBitrateSent) const;
  // TODO(holmer): Deprecated. We should use the SendSideDelayObserver instead
  // to avoid deadlocks.
  bool GetSendSideDelay(int* avg_send_delay, int* max_send_delay) const;
  void RegisterSendSideDelayObserver(SendSideDelayObserver* observer);
  void GetReceiveBandwidthEstimatorStats(
      ReceiveBandwidthEstimatorStats* output) const;

  // Called on any new send bitrate estimate.
  void RegisterSendBitrateObserver(BitrateStatisticsObserver* observer);

  int32_t StartRTPDump(const char file_nameUTF8[1024],
                       RTPDirections direction);
  int32_t StopRTPDump(RTPDirections direction);

  // Implements RtpFeedback.
  virtual int32_t OnInitializeDecoder(
      const int32_t id,
      int8_t payloadType,
      const char payloadName[RTP_PAYLOAD_NAME_SIZE],
      int frequency,
      size_t channels,
      uint32_t rate);
  virtual void OnIncomingSSRCChanged(const int32_t id,
                                     const uint32_t ssrc);
  virtual void OnIncomingCSRCChanged(const int32_t id,
                                     const uint32_t CSRC,
                                     const bool added);
  virtual void ResetStatistics(uint32_t);

  int32_t SetLocalReceiver(const uint16_t rtp_port,
                           const uint16_t rtcp_port,
                           const char* ip_address);
  //int32_t GetLocalReceiver(uint16_t* rtp_port,
  //                         uint16_t* rtcp_port,
  //                         char* ip_address) const;
  int32_t GetLocalReceiver(uint16_t& rtp_port,
						  uint16_t& rtcp_port,
						  char* ip_address) const;
  int32_t SetSocks5SendData(unsigned char *data, int length, bool isRTCP);
  int8_t SetMixMediaStream(bool enable, char *mixture, unsigned char version);
  int32_t SetSendDestination(const char *rtp_ip_address, const uint16_t rtp_port, const char *rtcp_ip_address, const uint16_t rtcp_port, const uint16_t source_rtp_port, const uint16_t source_rtcp_port);
  /* int32_t GetSendDestination(char* ip_address,
  uint16_t* rtp_port,
  uint16_t* rtcp_port,
  uint16_t* source_rtp_port,
  uint16_t* source_rtcp_port) const;*/

  int32_t GetSendDestination(char* ip_address,
							  uint16_t& rtp_port,
							  uint16_t& rtcp_port,
							  uint16_t& source_rtp_port,
							  uint16_t& source_rtcp_port) const;
  int32_t GetSourceInfo(uint16_t* rtp_port,
                        uint16_t* rtcp_port,
                        char* ip_address,
                        uint32_t ip_address_length);

  int32_t SetRemoteSSRCType(const StreamType usage, const uint32_t SSRC);

  int32_t StartSend();
  int32_t StopSend();
  bool Sending();
  int32_t StartReceive();
  int32_t StopReceive();

  int32_t RegisterSendTransport(Transport* transport);
  int32_t DeregisterSendTransport();

  // Incoming packet from external transport.
  int32_t ReceivedRTPPacket(const void* rtp_packet,
                            const size_t rtp_packet_length,
                            const PacketTime& packet_time);

  // Incoming packet from external transport.
  int32_t ReceivedRTCPPacket(const void* rtcp_packet,
                             const size_t rtcp_packet_length);

  // Sets the maximum transfer unit size for the network link, i.e. including
  // IP, UDP and RTP headers.
  int32_t SetMTU(uint16_t mtu);

  // Returns maximum allowed payload size, i.e. the maximum allowed size of
  // encoded data in each packet.
  uint16_t MaxDataPayloadLength() const;
  int32_t SetMaxPacketBurstSize(uint16_t max_number_of_packets);
  int32_t SetPacketBurstSpreadState(bool enable, const uint16_t frame_periodMS);

  int32_t EnableColorEnhancement(bool enable);

  // Gets the modules used by the channel.
  RtpRtcp* rtp_rtcp();

  //Set simulcast send RtpRtcp modlule
  void SetDefaultSimulcatRtpRtcp(std::list<RtpRtcp*> default_simulcast_rtp_rtcp);

  CallStatsObserver* GetStatsObserver();

  // Implements VCMReceiveCallback.
  virtual int32_t FrameToRender(I420VideoFrame& video_frame);  // NOLINT

  // Implements VCMReceiveCallback.
  virtual int32_t ReceivedDecodedReferenceFrame(
      const uint64_t picture_id);

  // Implements VCMReceiveCallback.
  virtual void IncomingCodecChanged(const VideoCodec& codec);

  // Implements VCMReceiveStatisticsCallback.
  virtual void OnReceiveRatesUpdated(uint32_t bit_rate,
                                     uint32_t frame_rate) OVERRIDE;
  virtual void OnDiscardedPacketsUpdated(int discarded_packets) OVERRIDE;
  virtual void OnFrameCountsUpdated(const FrameCounts& frame_counts) OVERRIDE;

  // Implements VCMDecoderTimingCallback.
  virtual void OnDecoderTiming(int decode_ms,
                               int max_decode_ms,
                               int current_delay_ms,
                               int target_delay_ms,
                               int jitter_buffer_ms,
                               int min_playout_delay_ms,
                               int render_delay_ms);

  // Implements VideoFrameTypeCallback.
  virtual int32_t RequestKeyFrame();

  // Implements VideoFrameTypeCallback.
  virtual int32_t SliceLossIndicationRequest(
      const uint64_t picture_id);

  // Implements VideoPacketRequestCallback.
  virtual int32_t ResendPackets(const uint16_t* sequence_numbers,
                                uint16_t length);

  int32_t SetVoiceChannel(int32_t ve_channel_id,
                          VoEVideoSync* ve_sync_interface);
  int32_t VoiceChannel();

  // Implements ViEFrameProviderBase.
  virtual int FrameCallbackChanged() {return -1;}

  int32_t RegisterEffectFilter(ViEEffectFilter* effect_filter);

  // New-style callbacks, used by VideoReceiveStream.
  void RegisterPreRenderCallback(I420FrameCallback* pre_render_callback);
  void RegisterPreDecodeImageCallback(
      EncodedImageCallback* pre_decode_callback);

  void RegisterSendFrameCountObserver(FrameCountObserver* observer);
  void RegisterReceiveRtcpPacketTypeCounterObserver(RtcpPacketTypeCounterObserver* observer);
  void RegisterSendRtcpPacketTypeCountObserver(RtcpPacketTypeCounterObserver* observer);
  void ReceivedBWEPacket(int64_t arrival_time_ms, size_t payload_size,
                         const RTPHeader& header);
  ViEReceiver *GetReceiver() { return &vie_receiver_ ; }
  ViESender	*GetVieSender() { return &vie_sender_; } //add by ylr;
  RtpRtcp::Configuration CreateRtpRtcpConfiguration();
  //add by dingxf
  void AddRemoteI420FrameCallback(ECMedia_I420FrameCallBack callback);

 protected:
  static bool ChannelDecodeThreadFunction(void* obj);
  bool ChannelDecodeProcess();

  void OnRttUpdate(int64_t rtt);

 private:
  void ReserveRtpRtcpModules(size_t total_modules)
      EXCLUSIVE_LOCKS_REQUIRED(rtp_rtcp_cs_);
  RtpRtcp* GetRtpRtcpModule(size_t simulcast_idx) const
      EXCLUSIVE_LOCKS_REQUIRED(rtp_rtcp_cs_);
  RtpRtcp* CreateRtpRtcpModule();
  // Assumed to be protected.
  int32_t StartDecodeThread();
  int32_t StopDecodeThread();

  int32_t ProcessNACKRequest(const bool enable);
  int32_t ProcessFECRequest(const bool enable,
                            const unsigned char payload_typeRED,
                            const unsigned char payload_typeFEC);
  // Compute NACK list parameters for the buffering mode.
  int GetRequiredNackListSize(int target_delay_ms);
  void SetRtxSendStatus(bool enable);

  void UpdateHistograms();
  void UpdateHistogramsAtStopSend();

  // ViEChannel exposes methods that allow to modify observers and callbacks
  // to be modified. Such an API-style is cumbersome to implement and maintain
  // at all the levels when comparing to only setting them at construction. As
  // so this class instantiates its children with a wrapper that can be modified
  // at a later time.
  template <class T>
  class RegisterableCallback : public T {
   public:
    RegisterableCallback()
        : critsect_(CriticalSectionWrapper::CreateCriticalSection()),
          callback_(NULL) {}

    void Set(T* callback) {
      CriticalSectionScoped cs(critsect_.get());
      callback_ = callback;
    }

   protected:
    // Note: this should be implemented with a RW-lock to allow simultaneous
    // calls into the callback. However that doesn't seem to be needed for the
    // current type of callbacks covered by this class.
    scoped_ptr<CriticalSectionWrapper> critsect_;
    T* callback_ GUARDED_BY(critsect_);

   private:
    DISALLOW_COPY_AND_ASSIGN(RegisterableCallback);
  };

  class RegisterableBitrateStatisticsObserver:
    public RegisterableCallback<BitrateStatisticsObserver> {
    virtual void Notify(uint32_t total_stats,
                        uint32_t retransmit_stats,
                        uint32_t ssrc) {
      CriticalSectionScoped cs(critsect_.get());
      if (callback_)
        callback_->Notify(total_stats, retransmit_stats, ssrc);
    }
  }
  send_bitrate_observer_;

  class RegisterableSendFrameCountObserver
      : public RegisterableCallback<FrameCountObserver> {
   public:
    virtual void FrameCountUpdated(const FrameCounts& frame_counts,
                                   uint32_t ssrc) {
      CriticalSectionScoped cs(critsect_.get());
      if (callback_)
        callback_->FrameCountUpdated(frame_counts, ssrc);
    }

   private:
  } send_frame_count_observer_;

  class RegisterableReceiveRtcpPacketTypeCounterObserver
	  : public RegisterableCallback<RtcpPacketTypeCounterObserver> {
  public:
	   virtual void RtcpPacketTypesCounterUpdated(uint32_t ssrc, 
												const RtcpPacketTypeCounter& packet_counter) {
			  CriticalSectionScoped cs(critsect_.get());
			  if (callback_)
				  callback_->RtcpPacketTypesCounterUpdated(ssrc, packet_counter);
	  }

  private:
  } receive_rtcp_packettype_count_observer_;

  class RegisterableSendSideDelayObserver :
      public RegisterableCallback<SendSideDelayObserver> {
    virtual void SendSideDelayUpdated(int avg_delay_ms,
                                      int max_delay_ms,
                                      uint32_t ssrc) OVERRIDE {
      CriticalSectionScoped cs(critsect_.get());
      if (callback_)
        callback_->SendSideDelayUpdated(avg_delay_ms, max_delay_ms, ssrc);
    }
  } send_side_delay_observer_;

  int32_t channel_id_;
  int32_t engine_id_;
  uint32_t number_of_cores_;
  uint8_t num_socket_threads_;

  // Used for all registered callbacks except rendering.
  scoped_ptr<CriticalSectionWrapper> callback_cs_;
  scoped_ptr<CriticalSectionWrapper> rtp_rtcp_cs_;

  RtpRtcp* default_rtp_rtcp_;
  std::list<RtpRtcp*> default_simulcast_rtp_rtcp_;

  // Owned modules/classes.
  scoped_ptr<RtpRtcp> rtp_rtcp_;
  std::list<RtpRtcp*> simulcast_rtp_rtcp_;
  std::list<RtpRtcp*> removed_rtp_rtcp_;
  VideoCodingModule* const vcm_;
  ViEReceiver vie_receiver_;
  ViESender vie_sender_;
  ViESyncModule vie_sync_;

  // Helper to report call statistics.
  scoped_ptr<ChannelStatsObserver> stats_observer_;

  // Not owned.
  VCMReceiveStatisticsCallback* vcm_receive_stats_callback_
  GUARDED_BY(callback_cs_);
//   ReceiveStatisticsProxy* receive_stats_proxy_callback_
// 	  GUARDED_BY(callback_cs_);
  FrameCounts receive_frame_counts_ GUARDED_BY(callback_cs_);
  ProcessThread& module_process_thread_;
  ViEDecoderObserver* codec_observer_;
  bool do_key_frame_callbackRequest_;
  ViERTPObserver* rtp_observer_;
  RtcpIntraFrameObserver* intra_frame_observer_;
  RtcpRttStats* rtt_stats_;
  PacedSender* paced_sender_;
  TransportFeedbackObserver* transport_feedback_observer_;

  scoped_ptr<RtcpBandwidthObserver> bandwidth_observer_;
  int send_timestamp_extension_id_;
  int absolute_send_time_extension_id_;

  Transport* external_transport_;

  bool decoder_reset_;
  // Current receive codec used for codec change callback.
  VideoCodec receive_codec_;
  bool wait_for_key_frame_;
  ThreadWrapper* decode_thread_;

  ViEEffectFilter* effect_filter_;
  bool color_enhancement_;

  // User set MTU, -1 if not set.
  uint16_t mtu_;
  const bool sender_;

  int nack_history_size_sender_;
  int max_nack_reordering_threshold_;
  I420FrameCallback* pre_render_callback_;

  scoped_ptr<ReportBlockStats> report_block_stats_sender_;
  scoped_ptr<ReportBlockStats> report_block_stats_receiver_;

  RtcEventLog *event_log_;
  //add by dingxf
  ECMedia_I420FrameCallBack remote_frame_callback_;

public:
	void SetRtcEventLog(RtcEventLog *event_log);

//---begin


private:
//	ServiceCoreCallBack *_serviceCoreCallBack;
	char call_id[9];
	int _firewallPolicy;

	bool _isWifi;

	//sean add begin 0915
	bool _shield_mosaic;
	//sean add end 0915

	time_t  _startNetworkTime;
	long _recvDataTotalSim;
	long _recvDataTotalWifi;
	scoped_ptr<CriticalSectionWrapper> critsect_net_statistic;

	//sean add begin 20140705 video conference
	bool _videoConferencePacketReceived;
	bool _isVideoConference;
	char *_conferenceNo;
	char *_conferPassword;
	char *_sipNo;
	char *_selfSipNo;
	int _confPort;
	char *_confIP;

	Encryption* external_encryption_;

	bool _encrypting;
	bool _decrypting;
	//WebRtc_UWord8* _encryptionRTPBufferPtr;
	//WebRtc_UWord8* _decryptionRTPBufferPtr;
	//WebRtc_UWord8* _encryptionRTCPBufferPtr;
	//WebRtc_UWord8* _decryptionRTCPBufferPtr;
#ifdef WEBRTC_SRTP
	SrtpModule& _srtpModule;
#endif

#ifndef WEBRTC_EXTERNAL_TRANSPORT
	uint32_t ssrc_all_num_;
	uint32_t local_ssrc_main_;  //svc send channel big resolution; trunk;not scv recv channel
	uint32_t local_ssrc_slave_; //only scv send small resolution 
	uint32_t remote_ssrc_;
	UdpTransport *socket_transport_;
#else
  uint32_t ssrc_all_num_;
  uint32_t local_ssrc_main_;  //svc send channel big resolution; trunk;not scv recv channel
  uint32_t local_ssrc_slave_; //only scv send small resolution 
  uint32_t remote_ssrc_;
  TcpTransport *socket_transport_;  
#endif
	int32_t _rtp_port;
	bool isSVCChannel_;
	 ViEFileRecorder file_recorder_;
public:
//	int32_t RegisterServiceCoreCallBack(ServiceCoreCallBack *, const char* call_id,int firewall_policy);
//	int32_t DeRegisterServiceCoreCallBack();
	int32_t SetVideoConferenceFlag(const char *selfSipNo, const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip);
	//sean add end 20140705 video conference
	int32_t SetShieldMosaic(bool flag);
	int32_t setProcessData(bool flag);
	void getNetworkStatistic(time_t &startTime,  long long &sendLengthSim,  long long &recvLengthSim, long long &sendLengthWifi, long long &recvLengthWifi);
	void setNetworkType(bool isWifi);


	int32_t RegisterExternalEncryption(Encryption* encryption);
	int32_t DeRegisterExternalEncryption();
#ifdef WEBRTC_SRTP
	int CcpSrtpInit();
	int CcpSrtpShutdown();
	int EnableSRTPSend(ccp_srtp_crypto_suite_t crypt_type, const char* key);
	int DisableSRTPSend();
	int EnableSRTPReceive(ccp_srtp_crypto_suite_t crypt_type, const char* key);
	int DisableSRTPReceive();
#endif

	// Implements UdpTransportData.
	virtual void IncomingRTPPacket(const int8_t* rtp_packet,
		const int32_t rtp_packet_length,
		const char* from_ip,
		const uint16_t from_port) OVERRIDE;
	virtual void IncomingRTCPPacket(const int8_t* rtcp_packet,
		const int32_t rtcp_packet_length,
		const char* from_ip,
		const uint16_t from_port) OVERRIDE;

	bool Receiving();

	int32_t SendUDPPacket(const int8_t* data,
		const WebRtc_UWord32 length,
		int32_t& transmitted_bytes,
		bool use_rtcp_socket,
		/*add begin------------------Sean20130723----------for video ice------------*/
		uint16_t portnr = 0,
		const char* ip = NULL
		/*add end--------------------Sean20130723----------for video ice------------*/
		);

	void RegisterFrameStorageCallback(VCMFrameStorageCallback* frameStorageCallback);

	ViEFileRecorder& GetIncomingFileRecorder();
	void ReleaseIncomingFileRecorder();

	// Implements VCM.
	virtual int32_t StoreReceivedFrame(
		const EncodedVideoData& frame_to_store) OVERRIDE;

    int setVideoConfCb(onVideoConference video_conf_cb);
    int setVideoDataCb(onEcMediaVideoData video_data_cb);
    int setStunCb(onStunPacket stun_cb);
    int32_t SetRequestKeyFrameCb(bool isVideoConf,onEcMediaRequestKeyFrame cb);
    
    int32_t SetPacketTimeoutNotification(bool enable,
                                               WebRtc_UWord32 timeout_seconds);
    int32_t RegisterNetworkObserver(ViENetworkObserver* observer);
    virtual void OnPacketTimeout(const int32_t id) OVERRIDE {};
    virtual void OnReceivedPacket(const int32_t id,
                                  const RtpRtcpPacketType packet_type) OVERRIDE {};
    int32_t EnableIPv6();
    bool IsIPv6Enabled();
    
    int32_t SetKeepAliveStatus(const bool enable,
                                     const int8_t unknownPayloadType,
                                     const uint16_t deltaTransmitTimeMS);
    
    int32_t GetKeepAliveStatus(bool& enable,
                                     int8_t& unknownPayloadType,
                                     uint16_t& deltaTransmitTimeMS);

	RtpReceiver* GetRtpReceiver();
	private:
		scoped_ptr<ReceiveStatisticsProxy> receive_statistics_proxy_;
		onVideoConference _video_conf_cb;
	    onEcMediaVideoData _video_data_cb;
	    onStunPacket _stun_cb;
	    onEcMediaRequestKeyFrame _key_frame_cb;
		bool _isVideoConf;
		SsrcObserver* ssrc_observer_;
	public:
		ReceiveStatisticsProxy* GetReceiveStatisticsProxy();
		void SetSsrcObserver(SsrcObserver* ssrcObserver);

//---end
 
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_H_
