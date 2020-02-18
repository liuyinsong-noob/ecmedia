/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MEDIA_SDK_H_
#define MEDIA_SDK_H_

#include <map>
#include <memory>
#include <string>

#include "rtc_base/net_helpers.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/signal_thread.h"
#include "rtc_base/third_party/sigslot/sigslot.h"

/////F:\working\coding\modify_webrtc\win_webrtc\src\examples\peerconnection\client\conductor.h
#include "../../api/media_stream_interface.h"
#include "../../api/peer_connection_interface.h"
#include "../../examples/peerconnection/client/main_wnd.h"
//#include "../../api/jsep.h"
//#include "../../examples/peerconnection/client/peer_connection_client.h"
///////////////////////////////////////////////////////////

/////F:\working\coding\modify_webrtc\win_webrtc\src\pc\peer_connection_factory.h
#include "../../api/media_stream_interface.h"
#include "../../api/media_transport_interface.h"
#include "../../api/peer_connection_interface.h"
#include "../../api/rtp_parameters.h"
#include "../../api/scoped_refptr.h"
#include "../../media/sctp/sctp_transport_internal.h"
#include "../../pc/channel_manager.h"
#include "../../rtc_base/rtc_certificate_generator.h"
#include "../../rtc_base/thread.h"

namespace rtc {
class BasicNetworkManager;
class BasicPacketSocketFactory;
}  // namespace rtc

///////////////////////////////////////////////////////////
/////F :\working\coding\modify_webrtc\win_webrtc\src\pc\rtp_transceiver.h
/////F:\working\coding\modify_webrtc\win_webrtc\src\pc\peer_connection.cc
/////D:\code\win_webrtc_modify\src\media\base\stream_params.h
#include "../../media/base/stream_params.h"
#include "../../pc/audio_rtp_receiver.h"
#include "../../pc/dtls_srtp_transport.h"
#include "../../pc/general_transport_controller.h"
#include "../../pc/media_session.h"
#include "../../pc/rtc_stats_collector.h"
#include "../../pc/rtc_stats_traversal.h"
#include "../../pc/rtp_receiver.h"
#include "../../pc/rtp_transceiver.h"
#include "../../pc/rtp_transport.h"
#include "../../pc/sdp_serializer.h"
#include "../../pc/session_description.h"
#include "../../pc/simulcast_description.h"
#include "../../pc/stats_collector.h"
#include "../../pc/stream_collection.h"
#include "../../pc/video_rtp_receiver.h"

/////////////////////////////////////////////

/////F:\working\coding\modify_webrtc\win_webrtc\src\p2p\base\basic_packet_socket_factory.h
#include "../../p2p/base/packet_socket_factory.h"
//#include "../../rtc_base/ref_counted_object.h"
///////////////////////////////////////////////////////////////////
///////F:\working\coding\modify_webrtc\win_webrtc\src\api\peer_connection_interface.h
// namespace webrtc {
// class AudioDeviceModule;
// class AudioMixer;
// class AudioProcessing;
// class DtlsTransportInterface;
// class SctpTransportInterface;
// class VideoDecoderFactory;
// class VideoEncoderFactory;
//}  // namespace webrtc
/////////////////////////////////////////////////////////////////////

/////F:\working\coding\modify_webrtc\win_webrtc\src\p2p\base\basic_packet_socket_factory.h
#include "../../logging/rtc_event_log/fake_rtc_event_log_factory.h"

#include "../../logging/rtc_event_log/fake_rtc_event_log.h"
#include "../../logging/rtc_event_log/rtc_event_log.h"

//#include "../../logging/rtc_event_log/fake_rtc_event_log.h"
#include "../../logging/rtc_event_log/rtc_event_log_factory_interface.h"
#include "../../rtc_base/thread.h"
/////////////////////////////////////////////////////////////////////

////////////////////////////GeneralPacketTransport////////////////////////////////
class GeneralPacketTransport : public rtc::PacketTransportInternal {
 public:
  GeneralPacketTransport();
  ~GeneralPacketTransport();

  // interface for PacketTransportInternal
  virtual const std::string& transport_name() const = 0;

  // The transport has been established.
  virtual bool writable() const = 0;

  // The transport has received a packet in the last X milliseconds, here X is
  // configured by each implementation.
  virtual bool receiving() const = 0;

  // Attempts to send the given packet.
  // The return value is < 0 on failure. The return value in failure case is not
  // descriptive. Depending on failure cause and implementation details
  // GetError() returns an descriptive errno.h error value.
  // This mimics posix socket send() or sendto() behavior.
  // TODO(johan): Reliable, meaningful, consistent error codes for all
  // implementations would be nice.
  // TODO(johan): Remove the default argument once channel code is updated.
  virtual int SendPacket(const char* data,
                         size_t len,
                         const rtc::PacketOptions& options,
                         int flags = 0) = 0;

  // Sets a socket option. Note that not all options are
  // supported by all transport types.
  virtual int SetOption(rtc::Socket::Option opt, int value) = 0;

  // TODO(pthatcher): Once Chrome's MockPacketTransportInterface implements
  // this, remove the default implementation.
  virtual bool GetOption(rtc::Socket::Option opt, int* value) { return false; }

  // Returns the most recent error that occurred on this channel.
  virtual int GetError() = 0;

 protected:
  // interface for PacketTransportInterface
  // Only for internal use. Returns a pointer to an internal interface, for use
  // by the implementation.
  virtual rtc::PacketTransportInternal* GetInternal() = 0;

 private:
};

/////////////////////////////////////////////////////////////////////

////////////////////////////GeneralRtpTransport////////////////////////////////
class GeneralRtpTransport : public webrtc::RtpTransportInternal {
 public:
  GeneralRtpTransport();
  ~GeneralRtpTransport();

  // interface for RtpTransportInternal
  virtual void SetRtcpMuxEnabled(bool enable) = 0;

  // TODO(zstein): Remove PacketTransport setters. Clients should pass these
  // in to constructors instead and construct a new RtpTransportInternal instead
  // of updating them.
  virtual bool rtcp_mux_enabled() const = 0;

  virtual rtc::PacketTransportInternal* rtp_packet_transport() const = 0;
  virtual void SetRtpPacketTransport(rtc::PacketTransportInternal* rtp) = 0;

  virtual rtc::PacketTransportInternal* rtcp_packet_transport() const = 0;
  virtual void SetRtcpPacketTransport(rtc::PacketTransportInternal* rtcp) = 0;

  virtual bool IsReadyToSend() const = 0;

  //// Called whenever a transport's ready-to-send state changes. The argument
  //// is true if all used transports are ready to send. This is more specific
  //// than just "writable"; it means the last send didn't return ENOTCONN.
  // sigslot::signal1<bool> SignalReadyToSend;

  //// Called whenever an RTCP packet is received. There is no equivalent signal
  //// for RTP packets because they would be forwarded to the BaseChannel
  /// through / the RtpDemuxer callback.
  // sigslot::signal2<rtc::CopyOnWriteBuffer*, int64_t>
  // SignalRtcpPacketReceived;

  //// Called whenever the network route of the P2P layer transport changes.
  //// The argument is an optional network route.
  // sigslot::signal1<absl::optional<rtc::NetworkRoute>>
  // SignalNetworkRouteChanged;

  //// Called whenever a transport's writable state might change. The argument
  /// is / true if the transport is writable, otherwise it is false.
  // sigslot::signal1<bool> SignalWritableState;

  // sigslot::signal1<const rtc::SentPacket&> SignalSentPacket;

  virtual bool IsWritable(bool rtcp) const = 0;

  // TODO(zhihuang): Pass the |packet| by copy so that the original data
  // wouldn't be modified.
  virtual bool SendRtpPacket(rtc::CopyOnWriteBuffer* packet,
                             const rtc::PacketOptions& options,
                             int flags) = 0;

  virtual bool SendRtcpPacket(rtc::CopyOnWriteBuffer* packet,
                              const rtc::PacketOptions& options,
                              int flags) = 0;

  // This method updates the RTP header extension map so that the RTP transport
  // can parse the received packets and identify the MID. This is called by the
  // BaseChannel when setting the content description.
  //
  // TODO(zhihuang): Merging and replacing following methods handling header
  // extensions with SetParameters:
  //   UpdateRtpHeaderExtensionMap,
  //   UpdateSendEncryptedHeaderExtensionIds,
  //   UpdateRecvEncryptedHeaderExtensionIds,
  //   CacheRtpAbsSendTimeHeaderExtension,
  virtual void UpdateRtpHeaderExtensionMap(
      const cricket::RtpHeaderExtensions& header_extensions) = 0;

  virtual bool IsSrtpActive() const = 0;

  virtual bool RegisterRtpDemuxerSink(
      const webrtc::RtpDemuxerCriteria& criteria,
      webrtc::RtpPacketSinkInterface* sink) = 0;

  virtual bool UnregisterRtpDemuxerSink(
      webrtc::RtpPacketSinkInterface* sink) = 0;

  // interface for SrtpTransportInterface
  // There are some limitations of the current implementation:
  //  1. Send and receive keys must use the same crypto suite.
  //  2. The keys can't be changed after initially set.
  //  3. The keys must be set before creating a sender/receiver using the SRTP
  //     transport.
  // Set the SRTP keying material for sending RTP and RTCP.
  virtual webrtc::RTCError SetSrtpSendKey(
      const cricket::CryptoParams& params) = 0;

  // Set the SRTP keying material for receiving RTP and RTCP.
  virtual webrtc::RTCError SetSrtpReceiveKey(
      const cricket::CryptoParams& params) = 0;

 private:
};

/////////////////////////////////////////////////////////////////////

////////////////////////////GeneralTransportController////////////////////////////////
// class GeneralTransportController : public sigslot::has_slots<> {
// public:
//  GeneralTransportController();
//  ~GeneralTransportController();
//
// private:
//
//};

/////////////////////////////////////////////////////////////////////
#define EC_CHECK_VALUE(v, r) \
	if (!v) return r;
// using namespace rtc;
// using namespace webrtc;
class MainWindow;

class MediaClient : public webrtc::GeneralTransportController::Observer,
                    public sigslot::has_slots<> {
 public:
  enum AV_TYPE {
    AV_NONE = 0,
    AV_AUDIO = 1 << 0,
    AV_VIDEO = 1 << 1,
    AV_AUDIO_VIDEO = AV_AUDIO | AV_VIDEO,
    AV_MAX
  };
  MediaClient(MainWindow* main_wnd = nullptr);
  ~MediaClient();

  bool InitializePeerConnection();

  void SetSocketAddress(const rtc::SocketAddress& local,
                        const rtc::SocketAddress& remote,
                        int audioPort,
                        int videoPort);

  void OnSentPacket_w(const rtc::SentPacket& sent_packet);

 private:
  bool CreateThreads();
  bool CreateRtcEventLog();
  bool CreateChannelManager();
  bool CreateCall(webrtc::RtcEventLog* event_log);

  uint32_t AddTracks();
  bool AddAudioTrack();
  bool AddVideoTrack();
  bool CreateTransport();
  bool CreateTransportController();

  bool CreateVoiceChannel(const std::string& mid) RTC_RUN_ON(signaling_thread_);
  bool CreateVideoChannel(const std::string& mid) RTC_RUN_ON(signaling_thread_);

  virtual bool OnTransportChanged(
      const std::string& mid,
      webrtc::RtpTransportInternal* rtp_transport,
      rtc::scoped_refptr<webrtc::DtlsTransport> dtls_transport,
      webrtc::MediaTransportInterface* media_transport);

 private:
  // The EventLog needs to outlive |call_| (and any other object that uses
  // it).
  std::unique_ptr<webrtc::RtcEventLog>
      event_log_;  // RTC_GUARDED_BY(worker_thread());

  // The unique_ptr belongs to the worker thread, but the Call object manages
  // its own thread safety.
  std::unique_ptr<webrtc::Call> call_;  // RTC_GUARDED_BY(worker_thread());

  // Points to the same thing as `call_`. Since it's const, we may read the
  // pointer from any thread.
  // webrtc::Call* const call_ptr_ = nullptr;

  std::unique_ptr<rtc::BasicNetworkManager> default_network_manager_;
  std::unique_ptr<rtc::BasicPacketSocketFactory> default_socket_factory_;
  std::unique_ptr<cricket::ChannelManager> channel_manager_;
  // rtc::scoped_refptr<webrtc::AudioState> audio_state_;

  bool wraps_current_thread_;
  rtc::Thread* network_thread_ = nullptr;
  rtc::Thread* worker_thread_ = nullptr;
  rtc::Thread* signaling_thread_ = nullptr;
  std::unique_ptr<rtc::Thread> owned_network_thread_;
  std::unique_ptr<rtc::Thread> owned_worker_thread_;
  const std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;

  cricket::VoiceChannel* voice_channel_ = nullptr;
  cricket::VideoChannel* video_channel_ = nullptr;

  std::vector<rtc::scoped_refptr<
      webrtc::RtpTransceiverProxyWithInternal<webrtc::RtpTransceiver>>>
      transceivers_;  // TODO(bugs.webrtc.org/9987): Accessed on both
                      // std::vector<webrtc::RtpTransceiver> transceivers_;  //
  // TODO(bugs.webrtc.org/9987): Accessed on both
  // signaling and network thread.

  ///////////////<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<//////////////
  // the paramations for creating audio/video channel
  // This object should be used to generate any SSRC that is not explicitly
  // specified by the user (or by the remote party).
  // The generator is not used directly, instead it is passed on to the
  // channel manager and the session description factory.
  rtc::UniqueRandomIdGenerator ssrc_generator_
      RTC_GUARDED_BY(signaling_thread_);

  struct cricket::MediaConfig media_config_;
  // Member variables for caching global options.
  cricket::AudioOptions audio_options_ RTC_GUARDED_BY(signaling_thread_);
  cricket::VideoOptions video_options_ RTC_GUARDED_BY(signaling_thread_);
  /////////////////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>////////////////////////

  std::unique_ptr<webrtc::GeneralTransportController>
      transport_controller_;  // TODO(bugs.webrtc.org/9987): Accessed on both

  //std::unique_ptr<cricket::PortAllocator>
  //    port_allocator_;  // TODO(bugs.webrtc.org/9987): Accessed on both
  //                      // signaling and network thread.

  MainWindow* main_wnd_ = nullptr;
  bool m_bInitialized = false;

  rtc::SocketAddress audioAddressLocal_;
  rtc::SocketAddress audioAddressRemote_;
  rtc::SocketAddress videoAddressLocal_;
  rtc::SocketAddress videoAddressRemote_;
};

#endif  // MEDIA_SDK_H_
