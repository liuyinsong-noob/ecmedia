

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/jsep.h"
#include "api/jsep_session_description.h"
#include "api/rtp_sender_interface.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "examples/peerconnection/client/defaults.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "p2p/base/port_allocator.h"
#include "pc/session_description.h"
#include "pc/video_track_source.h"
#include "pc/webrtc_sdp.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/strings/json.h"

#include "ec_peer_manager.h"
#include "video_capturer/capturer_track_source.h"

namespace webrtc {
const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";

std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value) {
  std::string value;
  const char* env_var = getenv(env_var_name);
  if (env_var)
    value = env_var;

  if (value.empty())
    value = default_value;

  return value;
}

std::string GetPeerConnectionString() {
  return GetEnvVarOrDefault("WEBRTC_CONNECT", "stun:stun.l.google.com:19302");
}

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();
  }
};

ECPeerManager::ECPeerManager(ECPeerConfig* config)
    : sendrcv_audio_(false), sendrcv_video_(false) {
  RTC_DCHECK(config);
  peer_config_ = config;
  signaling_thread_ = config->signaling_thread;
  worker_thread_ = config->worker_thread;
  network_thread_ = config->network_thread;
}

ECPeerManager::~ECPeerManager() {
  peer_connection_.release();
  peer_connection_factory_.release();
  remote_renderer_.reset();
}

bool ECPeerManager::Initialize() {
  RTC_DCHECK(!peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);
  #if defined(WEBRTC_WIN)

  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
      network_thread_ /* network_thread */, worker_thread_ /* worker_thread */,
      signaling_thread_ /* signaling_thread */, nullptr /* default_adm */,
      CreateBuiltinAudioEncoderFactory(), CreateBuiltinAudioDecoderFactory(),
      CreateBuiltinVideoEncoderFactory(), CreateBuiltinVideoDecoderFactory(),
      nullptr /* audio_mixer */, nullptr /* audio_processing */);

  if (!peer_connection_factory_) {
    DeletePeerConnection();
    return false;
  }

  if (!CreatePeerConnection(/*dtls=*/false)) {
    DeletePeerConnection();
  }
  #endif
  return peer_connection_ != nullptr;
}

void ECPeerManager::AddAudioTrack() {
  sendrcv_audio_ = true;
  
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      peer_connection_factory_->CreateAudioTrack(
          kAudioLabel, peer_connection_factory_->CreateAudioSource(
                           cricket::AudioOptions())));
  auto result_or_error = peer_connection_->AddTrack(audio_track, {kStreamId});
  if (!result_or_error.ok()) {
    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                      << result_or_error.error().message();
  }
}

rtc::scoped_refptr<webrtc::VideoTrackInterface> ECPeerManager::AddVideoTrack(
    VideoTrackSourceInterface* video_device) {
  sendrcv_video_ = true;


  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;
  if (video_device) {
    video_track =
        peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_device);
    auto result_or_error = peer_connection_->AddTrack(video_track, {kStreamId});
    if (!result_or_error.ok()) {
      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                        << result_or_error.error().message();
    }
  }

  if (peer_connection_) {
    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }

  return video_track;
}

bool ECPeerManager::CreatePeerConnection(bool dtls) {
  RTC_DCHECK(peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = dtls;
  webrtc::PeerConnectionInterface::IceServer server;
  server.uri = GetPeerConnectionString();
  config.servers.push_back(server);

  peer_connection_ = peer_connection_factory_->CreatePeerConnection(
      config, nullptr, nullptr, this);

  //TODO: disable.
  //AddAudioTrack();

  return peer_connection_ != nullptr;
}

void ECPeerManager::DeletePeerConnection() {
  /*main_wnd_->StopLocalRenderer();
  main_wnd_->StopRemoteRenderer();*/
  peer_connection_ = nullptr;
  peer_connection_factory_ = nullptr;
  // peer_id_ = -1;
  // loopback_ = false;

}

bool ECPeerManager::SetLocalAndRemoteDescription() {
  SetLocalDescription();
  SetRemoteDescription();
  AddIceCandidate();
  return true;
}

bool ECPeerManager::GenerateRemoteDescription() {
  return true;
}

void ECPeerManager::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
        streams) {
  auto track = receiver->track();
  if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track.get());
    // TODO: start remote renderer
    OnReceiveVideoRemoteTrack(video_track);
  }
}

void ECPeerManager::OnReceiveVideoRemoteTrack(
    webrtc::VideoTrackInterface* remote_video_track) {
    #if defined(WEBRTC_WIN)
  remote_video_track_ = remote_video_track;

  if (remote_window_ && remote_video_track_) {
    remote_renderer_.reset(VideoRenderer::CreateVideoRenderer(
        remote_window_, false, remote_video_track_, worker_thread_));

    remote_renderer_->StartRender(0);
  }
#endif
}

int ECPeerManager::AddRemoteRender(void* video_window) {
  #if defined(WEBRTC_WIN)
  remote_window_ = video_window;

  if (remote_window_ && remote_video_track_) {
    remote_renderer_.reset(VideoRenderer::CreateVideoRenderer(
        remote_window_, false, remote_video_track_, worker_thread_));

    return remote_renderer_->StartRender(0);
  }
#endif
  return 0;
}

int ECPeerManager::StopRemoteRender() {
  remote_renderer_->StopRender(0);

  return 0;
}

void ECPeerManager::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  // Remote peer stopped sending a track.
  auto track = receiver->track();
  track->Release();
}

void ECPeerManager::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate) {
  // Json::StyledWriter writer;
  // Json::Value jmessage;

  /*jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();*/
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }

  // jmessage[kCandidateSdpName] = sdp;
  // std::unique_ptr<webrtc::IceCandidateInterface> candidate(
  //    webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
  // if (!candidate.get()) {
  //  RTC_LOG(WARNING) << "Can't parse received candidate message. "
  //                   << "SdpParseError was: " << error.description;
  //  return;
  //}
  // if (!peer_connection_->AddIceCandidate(candidate.get())) {
  //  RTC_LOG(WARNING) << "Failed to apply the received candidate";
  //  return;
  //}
}

// when create offer success£¬ then set local description
void ECPeerManager::OnSuccess(webrtc::SessionDescriptionInterface* desc_ptr) {
  RTC_DCHECK_RUN_ON(signaling_thread());

  std::unique_ptr<SessionDescriptionInterface> desc(desc_ptr);
  local_description_ = std::move(desc);

  std::string sdp;
  desc_ptr->ToString(&sdp);
  // remote:answer
  // Replace message type from "offer" to "answer"
  std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
      sdp_helper_.CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
  remote_description_ = std::move(session_description);
}

void ECPeerManager::OnFailure(webrtc::RTCError error) {}

void ECPeerManager::SetLocalDescription() {
  // local:offer

  sdp_helper_.SetSDPCname("7TrBHVUBcZpP51OQ", "7TrBHVUBcZpP51OQ",
                          local_description_.get());

  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create(), local_description_.get());
}

void ECPeerManager::SetRemoteDescription() {
  // sdp_helper_.SetSDPSsrcs(1384160922, 596935862, remote_description_.get());

  sdp_helper_.SetSDPCname("7TrBHVUBcZpP51OQ", "7TrBHVUBcZpP51OQ",
                          remote_description_.get());


  std::string sdp;
  remote_description_->ToString(&sdp);

  peer_connection_->SetRemoteDescription(
      DummySetSessionDescriptionObserver::Create(), remote_description_.get());
}

void ECPeerManager::SetSendDestination(const char* rtp_addr, int rtp_port) {
  std::string sdp_mid;
  int sdp_mlineindex = 0;
  std::string sdp;

  // std::string sdp =
  //    "candidate:3031090232 1 udp 2122194687 192.168.1.20 9078 typ host
  //    generation 0  network-id 4 network-cost 10";

  // std::string sdp =
  //    "candidate:3031090232 1 udp 2122194687 192.168.1.61 9078 typ host
  //    generation 0  network-id 4 network-cost 10";

  // sdp = "candidate:3031090232 1 udp 2122194687 192.168.178.35 9078 typ host
  // generation 0  network-id 4 network-cost 10";

  sdp =
      "candidate:3031090232 1 udp 2122194687 127.0.0.1 9078 typ host "
      "generation 0  network-id 4 network-cost 10";

  webrtc::SdpParseError error;
  candidate_ = absl::WrapUnique(
      webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
  if (!candidate_.get()) {
    RTC_LOG(WARNING) << "Can't parse received candidate message. "
                     << "SdpParseError was: " << error.description;
    return;
  }

  std::string s(rtp_addr);
  rtc::SocketAddress address(s, rtp_port);
  //lzm---------
 // candidate_->candidate().set_address(address);

}

void ECPeerManager::AddIceCandidate() {
  RTC_LOG(WARNING) << "ECPeerManager: received candidate address: " << candidate_->candidate().address().ToString();
  if (!peer_connection_->AddIceCandidate(candidate_.get())) {
    RTC_LOG(WARNING) << "Failed to apply the received candidate" << "asdf";
    return;
  }
}

/******************************codec***********************************/
void ECPeerManager::GetAudioCodecs(cricket::AudioCodecs* audio_codecs) {
  sdp_helper_.GetAudioCodecs(audio_codecs, local_description_.get());
}

void ECPeerManager::GetVideoCodecs(cricket::VideoCodecs* video_codecs) {
  sdp_helper_.GetVideoCodecs(video_codecs, local_description_.get());
}

void ECPeerManager::SetSendCodecVideo(cricket::VideoCodec* video_codec) {
  if (remote_description_) {
    sdp_helper_.SetSDPAVCodec(nullptr, video_codec, remote_description_.get());
  }
}

void ECPeerManager::SetReceiveCodecVideo(cricket::VideoCodec* video_codec) {
  if (local_description_) {
    sdp_helper_.SetSDPAVCodec(nullptr, video_codec, local_description_.get());
  }
}

void ECPeerManager::SetSendCodecAudio(cricket::AudioCodec* audio_codec) {
  if (remote_description_) {
    sdp_helper_.SetSDPAVCodec(audio_codec, nullptr, remote_description_.get());
  }
}

void ECPeerManager::SetReceiveCodecAudio(cricket::AudioCodec* audio_codec) {
  if (local_description_) {
    sdp_helper_.SetSDPAVCodec(audio_codec, nullptr, local_description_.get());
  }
}

/******************************ssrc***********************************/
void ECPeerManager::SetVideoLocalSsrc(int peer_id, unsigned int ssrc) {
  if (local_description_) {
    sdp_helper_.SetSDPSsrcs(0, ssrc, local_description_.get());
  }
}

void ECPeerManager::SetVideoRemoteSsrc(int peer_id, unsigned int ssrc) {
  if (remote_description_) {
    sdp_helper_.SetSDPSsrcs(0, ssrc, remote_description_.get());
  }
}

void ECPeerManager::SetAudioLocalSsrc(int peer_id, unsigned int ssrc) {
  if (local_description_) {
    sdp_helper_.SetSDPSsrcs(ssrc, 0, local_description_.get());
  }
}

void ECPeerManager::SetAudioRemoteSsrc(int peer_id, unsigned int ssrc) {
  if (remote_description_) {
    sdp_helper_.SetSDPSsrcs(ssrc, 0, remote_description_.get());
  }
}

}  // namespace webrtc
