/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/client/media_client_old.h"

#include "examples/peerconnection/client/defaults.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/net_helpers.h"

// Names used for media stream ids.
const char ksAudioLabel[] = "audio_label";
const char ksVideoLabel[] = "video_label";
const char ksStreamId[] = "stream_id";

#ifdef WIN32
#include "rtc_base/win32_socket_server.h"
#endif
// static const char sdp_answer[] =
//    "{\
//\"sdp\" : \"v=0\r\no=- 101907403895895076 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0
// 0\r\na=group:BUNDLE 0 1\r\na=msid-semantic: WMS stream_id\r\nm=audio 9
// RTP/AVPF 111 103 104 9 102 0 8 106 105 13 110 112 113 126\r\nc=IN IP4
// 0.0.0.0\r\na=rtcp:9 IN IP4
// 0.0.0.0\r\na=ice-ufrag:Gy5Z\r\na=ice-pwd:pF149pzUfNk/86A8r0HsSIvJ\r\na=ice-options:trickle\r\na=mid:0\r\na=extmap:1
// urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\na=extmap:2
// http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=extmap:3
// urn:ietf:params:rtp-hdrext:sdes:mid\r\na=extmap:4
// urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\na=extmap:5
// urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id\r\na=sendrecv\r\na=msid:stream_id
// audio_label\r\na=rtcp-mux\r\na=rtpmap:111 opus/48000/2\r\na=rtcp-fb:111
// transport-cc\r\na=fmtp:111 minptime=10;useinbandfec=1\r\na=rtpmap:103
// ISAC/16000\r\na=rtpmap:104 ISAC/32000\r\na=rtpmap:9 G722/8000\r\na=rtpmap:102
// ILBC/8000\r\na=rtpmap:0 PCMU/8000\r\na=rtpmap:8 PCMA/8000\r\na=rtpmap:106
// CN/32000\r\na=rtpmap:105 CN/16000\r\na=rtpmap:13 CN/8000\r\na=rtpmap:110
// telephone-event/48000\r\na=rtpmap:112 telephone-event/32000\r\na=rtpmap:113
// telephone-event/16000\r\na=rtpmap:126
// telephone-event/8000\r\na=ssrc:1194352311 cname:PsVDQJ3VfQu5PE7D\r\nm=video 9
// RTP/AVPF 96 97 98 99 100 101 127 121 125 120 124 107 108 109 123 119
// 122\r\nc=IN IP4 0.0.0.0\r\na=rtcp:9 IN IP4
// 0.0.0.0\r\na=ice-ufrag:Gy5Z\r\na=ice-pwd:pF149pzUfNk/86A8r0HsSIvJ\r\na=ice-options:trickle\r\na=mid:1\r\na=extmap:14
// urn:ietf:params:rtp-hdrext:toffset\r\na=extmap:13
// http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\na=extmap:12
// urn:3gpp:video-orientation\r\na=extmap:2
// http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=extmap:11
// http://www.webrtc.org/experiments/rtp-hdrext/playout-delay\r\na=extmap:6
// http://www.webrtc.org/experiments/rtp-hdrext/video-content-type\r\na=extmap:7
// http://www.webrtc.org/experiments/rtp-hdrext/video-timing\r\na=extmap:8
// http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07\r\na=extmap:9
// http://www.webrtc.org/experiments/rtp-hdrext/color-space\r\na=extmap:3
// urn:ietf:params:rtp-hdrext:sdes:mid\r\na=extmap:4
// urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\na=extmap:5
// urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id\r\na=sendrecv\r\na=msid:stream_id
// video_label\r\na=rtcp-mux\r\na=rtcp-rsize\r\na=rtpmap:96
// VP8/90000\r\na=rtcp-fb:96 goog-remb\r\na=rtcp-fb:96
// transport-cc\r\na=rtcp-fb:96 ccm fir\r\na=rtcp-fb:96 nack\r\na=rtcp-fb:96
// nack pli\r\na=rtpmap:97 rtx/90000\r\na=fmtp:97 apt=96\r\na=rtpmap:98
// VP9/90000\r\na=rtcp-fb:98 goog-remb\r\na=rtcp-fb:98
// transport-cc\r\na=rtcp-fb:98 ccm fir\r\na=rtcp-fb:98 nack\r\na=rtcp-fb:98
// nack pli\r\na=fmtp:98 profile-id=0\r\na=rtpmap:99 rtx/90000\r\na=fmtp:99
// apt=98\r\na=rtpmap:100 VP9/90000\r\na=rtcp-fb:100 goog-remb\r\na=rtcp-fb:100
// transport-cc\r\na=rtcp-fb:100 ccm fir\r\na=rtcp-fb:100 nack\r\na=rtcp-fb:100
// nack pli\r\na=fmtp:100 profile-id=2\r\na=rtpmap:101 rtx/90000\r\na=fmtp:101
// apt=100\r\na=rtpmap:127 H264/90000\r\na=rtcp-fb:127
// goog-remb\r\na=rtcp-fb:127 transport-cc\r\na=rtcp-fb:127 ccm
// fir\r\na=rtcp-fb:127 nack\r\na=rtcp-fb:127 nack pli\r\na=fmtp:127
// level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f\r\na=rtpmap:121
// rtx/90000\r\na=fmtp:121 apt=127\r\na=rtpmap:125 H264/90000\r\na=rtcp-fb:125
// goog-remb\r\na=rtcp-fb:125 transport-cc\r\na=rtcp-fb:125 ccm
// fir\r\na=rtcp-fb:125 nack\r\na=rtcp-fb:125 nack pli\r\na=fmtp:125
// level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42001f\r\na=rtpmap:120
// rtx/90000\r\na=fmtp:120 apt=125\r\na=rtpmap:124 H264/90000\r\na=rtcp-fb:124
// goog-remb\r\na=rtcp-fb:124 transport-cc\r\na=rtcp-fb:124 ccm
// fir\r\na=rtcp-fb:124 nack\r\na=rtcp-fb:124 nack pli\r\na=fmtp:124
// level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\na=rtpmap:107
// rtx/90000\r\na=fmtp:107 apt=124\r\na=rtpmap:108 H264/90000\r\na=rtcp-fb:108
// goog-remb\r\na=rtcp-fb:108 transport-cc\r\na=rtcp-fb:108 ccm
// fir\r\na=rtcp-fb:108 nack\r\na=rtcp-fb:108 nack pli\r\na=fmtp:108
// level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f\r\na=rtpmap:109
// rtx/90000\r\na=fmtp:109 apt=108\r\na=rtpmap:123 red/90000\r\na=rtpmap:119
// rtx/90000\r\na=fmtp:119 apt=123\r\na=rtpmap:122
// ulpfec/90000\r\na=ssrc-group:FID 1934118080 1680690671\r\na=ssrc:1934118080
// cname:PsVDQJ3VfQu5PE7D\r\na=ssrc:1680690671 cname:PsVDQJ3VfQu5PE7D\r\n\",\
//\"type\" : \"answer\"}";
//
// static const char sdp_candidate[] =
//    "{\
//\"candidate\" : \"candidate:3350409123 1 udp 2122260223 192.168.0.105 55555
// typ host generation 0 ufrag Gy5Z network-id 3\",\
//\"sdpMLineIndex\" : 0,\"sdpMid\" : \"0\"}";

/////F:\working\coding\modify_webrtc\win_webrtc\src\api\create_peerconnection_factory.cc
#include "../../api/audio/audio_mixer.h"
#include "../../api/audio_codecs/audio_decoder_factory.h"
#include "../../api/audio_codecs/audio_encoder_factory.h"
#include "../../api/call/call_factory_interface.h"
#include "../../api/fec_controller.h"
#include "../../api/peer_connection_interface.h"
#include "../../api/scoped_refptr.h"
#include "../../api/transport/network_control.h"
#include "../../api/video_codecs/video_decoder_factory.h"
#include "../../api/video_codecs/video_encoder_factory.h"
#include "../../logging/rtc_event_log/rtc_event_log_factory.h"
#include "../../logging/rtc_event_log/rtc_event_log_factory_interface.h"
#include "../../media/base/media_engine.h"
#include "../../media/engine/webrtc_media_engine.h"
#include "../../modules/audio_device/include/audio_device.h"
#include "../../modules/audio_processing/include/audio_processing.h"
#include "../../rtc_base/thread.h"
//////////////////////////////////////////

/////F:\working\coding\modify_webrtc\win_webrtc\src\pc\peer_connection_factory.cc
#include "../../api/fec_controller.h"
#include "../../api/media_stream_proxy.h"
#include "../../api/media_stream_track_proxy.h"
#include "../../api/media_transport_interface.h"
#include "../../api/network_state_predictor.h"
#include "../../api/peer_connection_factory_proxy.h"
#include "../../api/peer_connection_proxy.h"
#include "../../api/turn_customizer.h"
#include "../../api/video_track_source_proxy.h"
#include "../../logging/rtc_event_log/rtc_event_log.h"
#include "../../media/base/rtp_data_engine.h"
#include "../../media/sctp/sctp_transport.h"
#include "../../p2p/base/basic_packet_socket_factory.h"
#include "../../p2p/client/basic_port_allocator.h"
#include "../../pc/audio_track.h"
#include "../../pc/local_audio_source.h"
#include "../../pc/media_stream.h"
#include "../../pc/peer_connection.h"
#include "../../pc/rtp_parameters_conversion.h"
#include "../../pc/video_track.h"
#include "../../rtc_base/bind.h"
#include "../../rtc_base/checks.h"
#include "../../system_wrappers/include/field_trial.h"
#include "../../third_party/abseil-cpp/absl/memory/memory.h"
/////////////////////////////////////////////

/////

/////////////////////////////////////////////

/////F:\working\coding\modify_webrtc\win_webrtc\src\examples\peerconnection\client\conductor.cc
#include "../../api/audio/audio_mixer.h"
#include "../../api/audio_codecs/audio_decoder_factory.h"
#include "../../api/audio_codecs/audio_encoder_factory.h"
#include "../../api/audio_codecs/builtin_audio_decoder_factory.h"
#include "../../api/audio_codecs/builtin_audio_encoder_factory.h"
#include "../../api/audio_options.h"
#include "../../api/create_peerconnection_factory.h"
#include "../../api/rtp_sender_interface.h"
#include "../../api/video_codecs/builtin_video_decoder_factory.h"
#include "../../api/video_codecs/builtin_video_encoder_factory.h"
#include "../../api/video_codecs/video_decoder_factory.h"
#include "../../api/video_codecs/video_encoder_factory.h"
#include "../../examples/peerconnection/client/defaults.h"
#include "../../modules/audio_device/include/audio_device.h"
#include "../../modules/audio_processing/include/audio_processing.h"
#include "../../modules/video_capture/video_capture.h"
#include "../../modules/video_capture/video_capture_factory.h"
#include "../../p2p/base/port_allocator.h"
#include "../../pc/video_track_source.h"
#include "../../rtc_base/checks.h"
#include "../../rtc_base/logging.h"
#include "../../rtc_base/ref_counted_object.h"
#include "../../rtc_base/rtc_certificate_generator.h"
#include "../../rtc_base/strings/json.h"
#include "../../test/vcm_capturer.h"
#include "../../third_party/abseil-cpp/absl/memory/memory.h"
#include "../../third_party/abseil-cpp/absl/types/optional.h"
/////////////////////////////////////////////

/////F:\working\coding\modify_webrtc\win_webrtc\src\pc\peer_connection.cc

/////////////////////////////////////////////

namespace {
////// Names used for a IceCandidate JSON object.
// static const char kCandidateSdpMidName[] = "sdpMid";
// static const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
// static const char kCandidateSdpName[] = "candidate";
////
////// Names used for a SessionDescription JSON object.
//// static const char kSessionDescriptionTypeName[] = "type";
// static const char kSessionDescriptionSdpName[] = "sdp";

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

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create() {
    const size_t kWidth = 640;
    const size_t kHeight = 480;
    const size_t kFps = 30;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = absl::WrapUnique(
          webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return new rtc::RefCountedObject<CapturerTrackSource>(
            std::move(capturer));
      }
    }

    return nullptr;
  }

 protected:
  explicit CapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};

}  // namespace

/////////////////////////////////////////////

using namespace webrtc;

MediaClient::MediaClient(MainWindow* main_wnd)
    : main_wnd_(main_wnd)
//,call_(nullptr)
{}

MediaClient::~MediaClient() {
  default_socket_factory_ = nullptr;
  default_network_manager_ = nullptr;
}

bool MediaClient::InitializePeerConnection() {
  bool bOk = true;
  if (!m_bInitialized) {
    bOk &= CreateThreads();
    bOk &= CreateRtcEventLog();
    bOk &= CreateChannelManager();
    bOk &= CreateTransportController();

    bOk &= worker_thread_->Invoke<bool>(
        RTC_FROM_HERE, [&] { return CreateCall(event_log_.get()); });

    bOk &= AddTracks();

    m_bInitialized = bOk;
    if (!m_bInitialized) {
      main_wnd_->MessageBox("Error",
                            "Failed to initialize PeerConnectionFactory", true);
    }
    main_wnd_->SwitchToStreamingUI();
  }
  return bOk;
}

bool MediaClient::CreateThreads() {
  if (!network_thread_) {
    owned_network_thread_ = rtc::Thread::CreateWithSocketServer();
    owned_network_thread_->SetName("pc_network_thread", nullptr);
    owned_network_thread_->Start();
    network_thread_ = owned_network_thread_.get();
  }
  EC_CHECK_VALUE(network_thread_, false);
  if (!worker_thread_) {
    owned_worker_thread_ = rtc::Thread::Create();
    owned_worker_thread_->SetName("pc_worker_thread", nullptr);
    owned_worker_thread_->Start();
    worker_thread_ = owned_worker_thread_.get();
  }
  EC_CHECK_VALUE(worker_thread_, false);

  if (!signaling_thread_) {
    signaling_thread_ = rtc::Thread::Current();
    if (!signaling_thread_) {
      // If this thread isn't already wrapped by an rtc::Thread, create a
      // wrapper and own it in this class.
      signaling_thread_ = rtc::ThreadManager::Instance()->WrapCurrentThread();
      wraps_current_thread_ = true;
    }
  }
  EC_CHECK_VALUE(signaling_thread_, false);

  return true;
}

bool MediaClient::CreateRtcEventLog() {
  // RTC_DCHECK_RUN_ON(worker_thread_);

  std::unique_ptr<RtcEventLogFactoryInterface> event_log_factory =
      CreateRtcEventLogFactory();
  EC_CHECK_VALUE(event_log_factory, false);

  auto encoding_type = RtcEventLog::EncodingType::Legacy;
  if (field_trial::IsEnabled("WebRTC-RtcEventLogNewFormat"))
    encoding_type = RtcEventLog::EncodingType::NewFormat;
  event_log_ = event_log_factory
                   ? event_log_factory->CreateRtcEventLog(encoding_type)
                   : absl::make_unique<RtcEventLogNullImpl>();

  EC_CHECK_VALUE(event_log_, false);
  return true;
}

bool MediaClient::CreateChannelManager() {
  rtc::scoped_refptr<AudioDeviceModule> default_adm = nullptr;
  rtc::scoped_refptr<AudioEncoderFactory> audio_encoder_factory =
      webrtc::CreateBuiltinAudioEncoderFactory();
  rtc::scoped_refptr<AudioDecoderFactory> audio_decoder_factory =
      webrtc::CreateBuiltinAudioDecoderFactory();
  std::unique_ptr<VideoEncoderFactory> video_encoder_factory =
      webrtc::CreateBuiltinVideoEncoderFactory();
  std::unique_ptr<VideoDecoderFactory> video_decoder_factory =
      webrtc::CreateBuiltinVideoDecoderFactory();

  rtc::scoped_refptr<AudioProcessing> audio_processing =
      AudioProcessingBuilder().Create();

  std::unique_ptr<cricket::MediaEngineInterface> media_engine =
      cricket::WebRtcMediaEngineFactory::Create(
          default_adm, audio_encoder_factory, audio_decoder_factory,
          std::move(video_encoder_factory), std::move(video_decoder_factory),
          nullptr /* audio_mixer */, audio_processing);

  EC_CHECK_VALUE(media_engine, false);

//  default_network_manager_.reset(new rtc::BasicNetworkManager());
//  EC_CHECK_VALUE(default_network_manager_, false);
//lzm-----------

  default_socket_factory_.reset(
      new rtc::BasicPacketSocketFactory(network_thread_));
  EC_CHECK_VALUE(default_socket_factory_, false);

  channel_manager_ = absl::make_unique<cricket::ChannelManager>(
      std::move(media_engine), absl::make_unique<cricket::RtpDataEngine>(),
      worker_thread_, network_thread_);

  EC_CHECK_VALUE(channel_manager_, false);

  bool bOk = channel_manager_->Init();
  EC_CHECK_VALUE(bOk, false);

  return bOk;
}

bool MediaClient::CreateCall(RtcEventLog* event_log) {
  RTC_DCHECK_RUN_ON(worker_thread_);

  const int kMinBandwidthBps = 30000;
  const int kStartBandwidthBps = 300000;
  const int kMaxBandwidthBps = 2000000;

  EC_CHECK_VALUE(channel_manager_, false);
  EC_CHECK_VALUE(channel_manager_->media_engine(), false);

  webrtc::Call::Config call_config(event_log);

  std::unique_ptr<webrtc::CallFactoryInterface> call_factory =
      CreateCallFactory();
  EC_CHECK_VALUE(call_factory, false);

  call_config.audio_state =
      channel_manager_->media_engine()->voice().GetAudioState();

  call_config.bitrate_config.min_bitrate_bps = kMinBandwidthBps;
  call_config.bitrate_config.start_bitrate_bps = kStartBandwidthBps;
  call_config.bitrate_config.max_bitrate_bps = kMaxBandwidthBps;

  call_config.fec_controller_factory =
      nullptr;  // fec_controller_factory_.get();
  call_config.task_queue_factory = task_queue_factory_.get();
  call_config.network_state_predictor_factory =
      nullptr;  // network_state_predictor_factory_.get();

  if (field_trial::IsEnabled("WebRTC-Bwe-InjectedCongestionController")) {
    RTC_LOG(LS_INFO) << "Using injected network controller factory";
    call_config.network_controller_factory =
        nullptr;  // injected_network_controller_factory_.get();
  } else {
    RTC_LOG(LS_INFO) << "Using default network controller factory";
  }

  call_ = std::unique_ptr<Call>(call_factory->CreateCall(call_config));
  EC_CHECK_VALUE(call_, false);

  channel_manager_->SetVideoRtxEnabled(true);

  return true;
}

bool MediaClient::AddAudioTrack() {
  RTC_DCHECK_RUN_ON(signaling_thread_);

  bool bOk = false;
  cricket::AudioOptions options;
  rtc::scoped_refptr<webrtc::LocalAudioSource> source(
      LocalAudioSource::Create(&options));
  EC_CHECK_VALUE(source, false);

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track =
      AudioTrackProxy::Create(
          signaling_thread_,
          AudioTrack::Create("audio_label" /*kAudioLabel*/, source));
  EC_CHECK_VALUE(audio_track, false);

  auto audio_sender = AudioRtpSender::Create(
      worker_thread_, "audio_label" /*rtc::CreateRandomUuid()*/,
      nullptr /*stats_.get()*/);
  EC_CHECK_VALUE(audio_sender, false);
  bOk = CreateVoiceChannel("0");
  EC_CHECK_VALUE(bOk, false);

  rtc::scoped_refptr<RtpSenderProxyWithInternal<RtpSenderInternal>>
      senderAudio = RtpSenderProxyWithInternal<RtpSenderInternal>::Create(
          signaling_thread_, audio_sender);
  EC_CHECK_VALUE(senderAudio, false);

  bOk = senderAudio->SetTrack(audio_track);
  EC_CHECK_VALUE(bOk, false);

  senderAudio->internal()->set_stream_ids({"stream_id"});
  senderAudio->internal()->set_init_send_encodings(
      std::vector<RtpEncodingParameters>());

  rtc::scoped_refptr<RtpReceiverProxyWithInternal<RtpReceiverInternal>>
      receiverAudio = RtpReceiverProxyWithInternal<RtpReceiverInternal>::Create(
          signaling_thread_,
          new AudioRtpReceiver(worker_thread_,
                               "31d047b7-c245-4300-b696-4575a84d815e",
                               std::vector<std::string>({})));
  EC_CHECK_VALUE(receiverAudio, false);

  auto transceiverAudio =
      RtpTransceiverProxyWithInternal<RtpTransceiver>::Create(
          signaling_thread_, new RtpTransceiver(senderAudio, receiverAudio));
  transceivers_.push_back(transceiverAudio);
  EC_CHECK_VALUE(transceiverAudio, false);

  if (transceiverAudio->internal()) {
    transceiverAudio->internal()->SetChannel(voice_channel_);
    if (transceiverAudio->internal()->sender_internal()) {
      transceiverAudio->internal()->sender_internal()->SetSsrc(512);
    }
    if (transceiverAudio->internal()->receiver_internal()) {
      transceiverAudio->internal()->receiver_internal()->SetupMediaChannel(512);
    }
  }

  if (voice_channel_ && voice_channel_->media_channel()) {
    worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
      return voice_channel_->media_channel()->OnReadyToSend(true);
    });
    worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
      return voice_channel_->media_channel()->SetPlayout(true);
    });
    if (!bOk) {
      RTC_LOG(LS_ERROR) << "Failed to SetSend on audio channel";
    }
    EC_CHECK_VALUE(bOk, false);

    worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
      return voice_channel_->media_channel()->SetSend(true);
    });
  }
  return true;
}

bool MediaClient::AddVideoTrack() {
  RTC_DCHECK_RUN_ON(signaling_thread_);

  bool bOk = false;
  rtc::scoped_refptr<CapturerTrackSource> video_device =
      CapturerTrackSource::Create();
  EC_CHECK_VALUE(video_device, false);

  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
      VideoTrackProxy::Create(
          signaling_thread_, worker_thread_,
          VideoTrack::Create("video_label", video_device, worker_thread_)));
  main_wnd_->StartLocalRenderer(video_track);
  EC_CHECK_VALUE(video_track, false);

  auto video_sender = VideoRtpSender::Create(
      worker_thread_, "video_label" /*rtc::CreateRandomUuid()*/);
  EC_CHECK_VALUE(video_sender, false);

  bOk = CreateVideoChannel("1");
  EC_CHECK_VALUE(bOk, false);

  rtc::scoped_refptr<RtpSenderProxyWithInternal<RtpSenderInternal>>
      senderVideo = RtpSenderProxyWithInternal<RtpSenderInternal>::Create(
          signaling_thread_, video_sender);
  EC_CHECK_VALUE(senderVideo, false);

  bOk = senderVideo->SetTrack(video_track);
  EC_CHECK_VALUE(bOk, false);

  senderVideo->internal()->set_stream_ids({"stream_id"});
  senderVideo->internal()->set_init_send_encodings(
      std::vector<RtpEncodingParameters>());

  rtc::scoped_refptr<RtpReceiverProxyWithInternal<RtpReceiverInternal>>
      receiverVideo = RtpReceiverProxyWithInternal<RtpReceiverInternal>::Create(
          signaling_thread_,
          new VideoRtpReceiver(worker_thread_,
                               "31d047b7-c245-4300-b696-4575a84d815e",
                               std::vector<std::string>({})));
  EC_CHECK_VALUE(receiverVideo, false);

  main_wnd_->StartRemoteRenderer(
      static_cast<webrtc::VideoTrackInterface*>(receiverVideo->track().get()));

  auto transceiverVideo =
      RtpTransceiverProxyWithInternal<RtpTransceiver>::Create(
          signaling_thread_, new RtpTransceiver(senderVideo, receiverVideo));
  EC_CHECK_VALUE(transceiverVideo, false);
  transceivers_.push_back(transceiverVideo);

  if (transceiverVideo->internal()) {
    transceiverVideo->internal()->SetChannel(video_channel_);
    if (transceiverVideo->internal()->sender_internal()) {
      transceiverVideo->internal()->sender_internal()->SetSsrc(128);
    }
    if (transceiverVideo->internal()->receiver_internal()) {
      transceiverVideo->internal()->receiver_internal()->SetupMediaChannel(128);
    }
  }

  if (video_channel_ && video_channel_->media_channel()) {
    worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
      return video_channel_->media_channel()->OnReadyToSend(true);
    });
    bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
      return video_channel_->media_channel()->SetSend(true);
    });
    if (!bOk) {
      RTC_LOG(LS_ERROR) << "Failed to SetSend on video channel";
    }
    EC_CHECK_VALUE(bOk, false);
  }
  return bOk;
}

uint32_t MediaClient::AddTracks() {
  RTC_DCHECK_RUN_ON(signaling_thread_);

  uint32_t type = AV_NONE;
  bool bOk = AddAudioTrack();
  type = bOk ? (type | AV_AUDIO) : type;

  bOk = AddVideoTrack();
  type = bOk ? (type | AV_VIDEO) : type;

  return type;
}

bool MediaClient::CreateTransport() {
  EC_CHECK_VALUE(transport_controller_, false);

  cricket::SessionDescription sdAudio, sdVideo;
  cricket::AudioContentDescription* audio =
      new cricket::AudioContentDescription();
  cricket::VideoContentDescription* video =
      new cricket::VideoContentDescription();
  EC_CHECK_VALUE(audio, false);
  EC_CHECK_VALUE(video, false);

  audio->set_direction(webrtc::RtpTransceiverDirection::kSendRecv);
  audio->set_rtcp_mux(true);
  video->set_direction(webrtc::RtpTransceiverDirection::kSendRecv);
  video->set_rtcp_mux(true);

  cricket::StreamParams stAudio, stVideo;
  stAudio.set_stream_ids({ksStreamId});
  stAudio.id = ksAudioLabel;
  stVideo.set_stream_ids({ksStreamId});
  stVideo.id = ksVideoLabel;
  audio->AddStream(stAudio);
  video->AddStream(stVideo);

  sdAudio.AddContent("0", cricket::MediaProtocolType::kRtp, false, audio);
  sdVideo.AddContent("1", cricket::MediaProtocolType::kRtp, false, video);

  RTCError error;
  error = network_thread_->Invoke<RTCError>(RTC_FROM_HERE, [=, &sdAudio] {
    return transport_controller_->ApplyDescription_n(true, &sdAudio);
  });
  EC_CHECK_VALUE(error.ok(), false);

  error = network_thread_->Invoke<RTCError>(RTC_FROM_HERE, [=, &sdVideo] {
    return transport_controller_->ApplyDescription_n(false, &sdVideo);
  });
  EC_CHECK_VALUE(error.ok(), false);

  return true;
}

bool MediaClient::CreateTransportController() {
  GeneralTransportController::Config config;
  config.transport_observer = this;
  config.event_log = event_log_.get();
  config.disable_encryption = true;

  transport_controller_.reset(new GeneralTransportController(
      signaling_thread_, network_thread_, nullptr /*port_allocator_.get()*/,
      nullptr /*async_resolver_factory_.get()*/, config));
  EC_CHECK_VALUE(transport_controller_, false);

  bool bOk = CreateTransport();
  EC_CHECK_VALUE(bOk, false);

  network_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
    transport_controller_->setUdpConnection("0", default_socket_factory_.get(),
                                            audioAddressLocal_,
                                            audioAddressRemote_);
  });

  network_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
    transport_controller_->setUdpConnection("1", default_socket_factory_.get(),
                                            videoAddressLocal_,
                                            videoAddressRemote_);
  });

  return bOk;
}

bool MediaClient::CreateVoiceChannel(const std::string& mid) {
  EC_CHECK_VALUE(channel_manager_, false);
  EC_CHECK_VALUE(transport_controller_, false);

  CryptoOptions option;
  bool bOk = false;
  bool bSrtpRequired = false;
  webrtc::RtpTransportInternal* rtp_transport = nullptr;
  webrtc::MediaTransportInterface* media_transport = nullptr;

  rtp_transport = transport_controller_->GetRtpTransport("0");
  EC_CHECK_VALUE(rtp_transport, false);

  voice_channel_ = channel_manager_->CreateVoiceChannel(
      call_.get(), media_config_, rtp_transport, media_transport,
      signaling_thread_, mid, bSrtpRequired, option, &ssrc_generator_,
      audio_options_);
  EC_CHECK_VALUE(voice_channel_, false);

  //  voice_channel->SignalDtlsSrtpSetupFailure.connect(
  //    this, &PeerConnection::OnDtlsSrtpSetupFailure);
  voice_channel_->SignalSentPacket.connect(this, &MediaClient::OnSentPacket_w);


  voice_channel_->SetRtpTransport(rtp_transport);

  network_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
    return rtp_transport->SignalWritableState(rtp_transport);
  });

  EC_CHECK_VALUE(voice_channel_->media_channel(), false);

  cricket::AudioSendParameters sendParams;
  cricket::AudioRecvParameters recvParams;
  sendParams.mid = "0";

  channel_manager_->GetSupportedAudioSendCodecs(&sendParams.codecs);
  channel_manager_->GetSupportedAudioRtpHeaderExtensions(
      &sendParams.extensions);

  channel_manager_->GetSupportedAudioReceiveCodecs(&recvParams.codecs);
  channel_manager_->GetSupportedAudioRtpHeaderExtensions(
      &recvParams.extensions);

  cricket::StreamParams audio_stream;
  audio_stream.cname = "DcRqgGg4U0HjSqLy";
  audio_stream.id = ksAudioLabel;
  audio_stream.add_ssrc(512);

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->SetSendParameters(sendParams);
  });
  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->SetRecvParameters(recvParams);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->AddSendStream(audio_stream);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->AddRecvStream(audio_stream);
  });

  webrtc::RtpDemuxerCriteria demuxer_criteria;
  demuxer_criteria.mid = "0";
  demuxer_criteria.ssrcs.insert(512);
  transport_controller_->RegisterRtpDemuxerSink(demuxer_criteria,
                                                voice_channel_);

  return bOk;
}

bool MediaClient::CreateVideoChannel(const std::string& mid) {
  EC_CHECK_VALUE(channel_manager_, false);

  CryptoOptions option;
  bool bSrtpRequired = false;
  webrtc::RtpTransportInternal* rtp_transport = nullptr;
  webrtc::MediaTransportInterface* media_transport = nullptr;

  EC_CHECK_VALUE(transport_controller_, false);

  rtp_transport = transport_controller_->GetRtpTransport("1");

  EC_CHECK_VALUE(rtp_transport, false);

  video_channel_ = channel_manager_->CreateVideoChannel(
      call_.get(), media_config_, rtp_transport, media_transport,
      signaling_thread_, mid, bSrtpRequired, option, &ssrc_generator_,
      video_options_);

  EC_CHECK_VALUE(video_channel_, false);

  bool bOk = false;
  video_channel_->SetRtpTransport(rtp_transport);

  //  video_channel->SignalDtlsSrtpSetupFailure.connect(
  //    this, &PeerConnection::OnDtlsSrtpSetupFailure);
  video_channel_->SignalSentPacket.connect(this, &MediaClient::OnSentPacket_w);


  network_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
    return rtp_transport->SignalWritableState(rtp_transport);
  });

  EC_CHECK_VALUE(video_channel_->media_channel(), false);

  cricket::VideoSendParameters sendParams;
  cricket::VideoRecvParameters recvParams;
  sendParams.mid = "1";

  channel_manager_->GetSupportedVideoCodecs(&sendParams.codecs);
  channel_manager_->GetSupportedVideoCodecs(&recvParams.codecs);
  channel_manager_->GetSupportedVideoRtpHeaderExtensions(
      &sendParams.extensions);
  channel_manager_->GetSupportedVideoRtpHeaderExtensions(
      &recvParams.extensions);

  cricket::StreamParams video_stream;
  video_stream.cname = "DcRqgGg4U0HjSqLy";
  video_stream.id = ksVideoLabel;
  video_stream.add_ssrc(128);

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->SetSendParameters(sendParams);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->SetRecvParameters(recvParams);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->AddSendStream(video_stream);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->AddRecvStream(video_stream);
  });

  webrtc::RtpDemuxerCriteria demuxer_criteria;
  demuxer_criteria.mid = "1";
  demuxer_criteria.ssrcs.insert(128);
  transport_controller_->RegisterRtpDemuxerSink(demuxer_criteria,
                                                video_channel_);

  return bOk;
}

bool MediaClient::OnTransportChanged(
    const std::string& mid,
    webrtc::RtpTransportInternal* rtp_transport,
    rtc::scoped_refptr<webrtc::DtlsTransport> dtls_transport,
    webrtc::MediaTransportInterface* media_transport) {
  return false;
}

void MediaClient::SetSocketAddress(const rtc::SocketAddress& local,
                                   const rtc::SocketAddress& remote,
                                   int audioPort,
                                   int videoPort) {
  audioAddressLocal_ = local;
  audioAddressLocal_.SetPort(audioPort);

  audioAddressRemote_ = remote;
  audioAddressRemote_.SetPort(audioPort);

  videoAddressLocal_ = local;
  videoAddressLocal_.SetPort(videoPort);

  videoAddressRemote_ = remote;
  videoAddressRemote_.SetPort(videoPort);
}

void MediaClient::OnSentPacket_w(const rtc::SentPacket& sent_packet) {
  RTC_DCHECK_RUN_ON(worker_thread_);
  EC_CHECK_VALUE(call_,void());
  call_->OnSentPacket(sent_packet);
}


//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////GeneralPacketTransport////////////////////////////////
GeneralPacketTransport::GeneralPacketTransport() {}

GeneralPacketTransport::~GeneralPacketTransport() {}

/////////////////////////////////////////////////////////////////////

////////////////////////////GeneralRtpTransport////////////////////////////////
GeneralRtpTransport::GeneralRtpTransport() {}

GeneralRtpTransport::~GeneralRtpTransport() {}

/////////////////////////////////////////////////////////////////////

//////////////////////////////GeneralTransportController////////////////////////////////
// GeneralTransportController::GeneralTransportController() {}
//
// GeneralTransportController::~GeneralTransportController() {}
//
/////////////////////////////////////////////////////////////////////
