#ifndef EC_BASE_MANAGER_H
#define EC_BASE_MANAGER_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "api/media_transport_interface.h"
#include "api/peer_connection_interface.h"
#include "api/turn_customizer.h"
#include "pc/ice_server_parsing.h"
#include "pc/jsep_transport_controller.h"
#include "pc/peer_connection_factory.h"
#include "pc/peer_connection_internal.h"
#include "pc/rtc_stats_collector.h"
#include "pc/rtp_sender.h"
#include "pc/rtp_transceiver.h"
#include "pc/sctp_transport.h"
#include "pc/stats_collector.h"
#include "pc/stream_collection.h"
#include "pc/webrtc_session_description_factory.h"
#include "rtc_base/race_checker.h"
#include "rtc_base/unique_id_generator.h"

#include "ec_peer_config.h"
#include "video_capturer/video_capturer.h"
#include "video_render/video_renderer.h"
#include "ec_sdp_helper.h"

namespace webrtc {
class ECPeerManager : public webrtc::PeerConnectionObserver,
                      public webrtc::CreateSessionDescriptionObserver
{
 public:
	 // generate both local_description & remote_description
  ECPeerManager(ECPeerConfig* config);
  ~ECPeerManager();
  bool Initialize();
  int AddRemoteRender(void* video_window);
  int StopRemoteRender();

  void AddAudioTrack();
  rtc::scoped_refptr<webrtc::VideoTrackInterface> AddVideoTrack(
      VideoTrackSourceInterface* video_device);

  const SessionDescriptionInterface* local_description() { return local_description_.get(); }

  bool SetLocalAndRemoteDescription();
  bool GenerateRemoteDescription();

  sigslot::signal1<webrtc::VideoTrackInterface*> SignalAddVideoReceiveTrack;

  void SetVideoLocalSsrc(int peer_id, unsigned int ssrc);
  void SetVideoRemoteSsrc(int peer_id, unsigned int ssrc);
  void SetAudioLocalSsrc(int peer_id, unsigned int ssrc);
  void SetAudioRemoteSsrc(int peer_id, unsigned int ssrc);

  void GetAudioCodecs(
      cricket::AudioCodecs* audio_codecs);
  void GetVideoCodecs(
      cricket::VideoCodecs* video_codecs);

  void SetSendCodecVideo(cricket::VideoCodec* video_codec);
  void SetReceiveCodecVideo(cricket::VideoCodec* video_codec);
  void SetSendCodecAudio(cricket::AudioCodec* audio_codec);
  void SetReceiveCodecAudio(cricket::AudioCodec* audio_codec);

  void SetSendDestination(const char* rtp_addr, int rtp_port);
  void AddIceCandidate();

 protected:
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();


  //
  // PeerConnectionObserver implementation.
  //

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnAddTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
          streams) override;
  void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

  void SetLocalDescription();
  void SetRemoteDescription();

  rtc::Thread* signaling_thread() { return signaling_thread_; }


 private:
  void OnReceiveVideoRemoteTrack(
      webrtc::VideoTrackInterface* remote_video_track);

  ECSDPHelper sdp_helper_;
  ECPeerConfig* peer_config_;


  rtc::Thread* signaling_thread_;
  rtc::Thread* worker_thread_;
  rtc::Thread* network_thread_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<PeerConnectionFactoryInterface> peer_connection_factory_;

  bool sendrcv_audio_;  // true: sendrcv; false: rcv only
  bool sendrcv_video_;	// true: sendrcv; false: rcv only
  std::unique_ptr<SessionDescriptionInterface> local_description_;
  std::unique_ptr<SessionDescriptionInterface> remote_description_;

  void* remote_window_;                             // TODO: modify to vector
  std::unique_ptr<VideoRenderer> remote_renderer_;  // TODO: modify to vector
  rtc::scoped_refptr<webrtc::VideoTrackInterface> remote_video_track_;

  std::unique_ptr<webrtc::IceCandidateInterface> candidate_;
  
  // have not been used
  std::vector<
      rtc::scoped_refptr<RtpTransceiverProxyWithInternal<RtpTransceiver>>>
      transceivers_;  // TODO(bugs.webrtc.org/9987): Accessed on both signaling
                      // and network thread.
};

}  // namespace webrtc

#endif //
