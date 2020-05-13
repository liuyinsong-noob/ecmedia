
#include "media_client.h"

#if defined(WEBRTC_ANDROID)
#include <jni.h>
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "modules/utility/include/jvm_android.h"
#include "sdk/android/native_api/video/wrapper.h"
#include "sdk/android/src/jni/android_video_track_source.h"
#include "sdk/android/src/jni/pc/video.h"
#include "sdk/android/src/jni/video_sink.h"
#endif

#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/call/call_factory_interface.h"
#include "api/fec_controller.h"
#include "api/media_stream_proxy.h"
#include "api/media_stream_track_proxy.h"
#include "api/rtp_sender_interface.h"
#include "api/transport/network_control.h"
#include "api/video/i420_buffer.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_track_source_proxy.h"

#include "media/base/codec.h"
#include "media/base/media_engine.h"
#include "media/base/rtp_data_engine.h"
#include "media/engine/webrtc_media_engine.h"
#include "media/engine/webrtc_voice_engine.h"
#include "media/sctp/sctp_transport.h"

#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"

#include "rtc_base/bind.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/strings/json.h"
#include "rtc_base/system/file_wrapper.h"

#include "pc/audio_track.h"
#include "pc/local_audio_source.h"
#include "pc/media_stream.h"
#include "pc/rtp_parameters_conversion.h"
#include "pc/video_track.h"
#include "pc/video_track_source.h"

#include "third_party/abseil-cpp/absl/memory/memory.h"
#include "third_party/abseil-cpp/absl/strings/ascii.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"

#include "logging/rtc_event_log/rtc_event_log_factory.h"
#include "system_wrappers/include/field_trial.h"
#include "video_capturer/capturer_track_source.h"

#define API_LOG(sev) RTC_LOG(sev) << "[API] " << __FUNCTION__ << " "
#if defined(WEBRTC_IOS)
#include "sdk/objc/api/ecmedia/objc_client.h"
#endif

namespace ecmedia_sdk {

static const char ksAudioLabel[] = "audio_label";
static const char ksVideoLabel[] = "video_label";
static const char ksStreamId[] = "stream_id";

// add bu yukening
const int kMinBandwidthBps = 30000;
#if defined(WEBRTC_WIN)
const int kStartBandwidthBps = 800000;
const int kMaxBandwidthBps = 2000000;
#else
const int kStartBandwidthBps = 500000;
const int kMaxBandwidthBps = 1000000;
#endif

std::string getStrFromInt(int n) {
  std::string ret;
  std::ostringstream os;
  os << n;
  ret = os.str().c_str();

  return ret;
}
void ReadMediaConfig(const char* filename) {
  char buf[16 * 1024] = {0};
  webrtc::FileWrapper fr = webrtc::FileWrapper::OpenReadOnly(filename);
  if (!fr.is_open())
    return;

  fr.Read(buf, sizeof(buf) - 1);
  std::string ccmode, h264_encoder = "openh264";
  Json::Reader reader;
  Json::Value config;
  if (!reader.parse(buf, config)) {
    return;
  }

  rtc::GetStringFromJsonObject(config, "Congestion-Control-Mode", &ccmode);
  rtc::GetStringFromJsonObject(config, "H264-Encoder", &h264_encoder);

  static std::string trail_string;
  trail_string.append("EC-Congestion-Control-Mode");
  if (ccmode == "gcc") {
    trail_string.append("/gcc/");
  } else
    trail_string.append("/bbr/");

  trail_string.append("EC-H264-Encoder");
  if (h264_encoder == "openh264") {
    trail_string.append("/openh264/");
  } else
    trail_string.append("/x264/");

  webrtc::field_trial::InitFieldTrialsFromString(trail_string.c_str());
}

///////////////////////MediaClient//////////////////////
rtc::LogSink* MediaClient::ec_log_ = nullptr;
MediaClient* MediaClient::m_pInstance = NULL;
rtc::CriticalSection MediaClient::m_critical;

MediaClient* MediaClient::GetInstance() {
  rtc::CritScope lock(&m_critical);
  if (m_pInstance == NULL) {
    m_pInstance = new MediaClient();
    RTC_CHECK(m_pInstance);
  }
  return m_pInstance;
}

void MediaClient::DestroyInstance() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  rtc::CritScope lock(&m_critical);
  if (m_pInstance) {
    delete m_pInstance;
    m_pInstance = NULL;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
}

MediaClient::MediaClient() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";

  m_bInitialized = false;
  m_bControll = false;
  isCreateCall = true;
  pAudioDevice = nullptr;
  own_adm = nullptr;
#if defined(WEBRTC_WIN)
  desktop_device_ = nullptr;
  ReadMediaConfig("ecmedia.cfg");
#endif
  m_nConnected = SC_UNCONNECTED;
  mVideoChannels_.clear();
  mVoiceChannels_.clear();
  video_track_ = nullptr;
  asum_ = 0;
  // Initialize();
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
}

MediaClient::~MediaClient() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  if (signaling_thread_) {
    signaling_thread_->Invoke<void>(RTC_FROM_HERE, [this] {
      RTC_DCHECK_RUN_ON(signaling_thread_);
      channel_manager_.reset(nullptr);
      default_socket_factory_ = nullptr;
      channelGenerator_.reset(nullptr);
      std::vector<rtc::scoped_refptr<webrtc::VideoTrackInterface>>::iterator it = video_tracks_.begin();
      while (it != video_tracks_.end()) {
        (*it).get()->GetSource()->Release();
        while ((*it).release() != nullptr); 
        it++;
      }
      video_tracks_.clear();
      video_track_.release();
    });
  }
#if defined(WEBRTC_WIN)
  for (auto it = desktop_devices_.begin(); it != desktop_devices_.end(); it++) {
    if (it->second) {
      while (it->second.release() != NULL)
        ;
    }
  }
  desktop_devices_.clear();
#endif
  if (own_adm != nullptr) {
    own_adm->Release();
  }

  if (ec_log_) {
    delete ec_log_;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
}
// wwx
bool MediaClient::SetTrace(const char* path, int min_sev) {
  RTC_LOG(INFO) << __FUNCTION__ << ", path: " << path << "min_sev:" << min_sev;
  if (path) {
    rtc::LoggingSeverity ls = (rtc::LoggingSeverity)min_sev;
    if (!ec_log_) {
      ec_log_ = new ECLog(path);
      rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
      rtc::LogMessage::LogTimestamps();
      // rtc::LogMessage::LogThreads();
      rtc::LogMessage::AddLogToStream(ec_log_, ls);
    }
    return true;
  }
  return false;
  //if (!ec_log_) {
  //  ec_log_ = new ECLog(path);
  //}
  //rtc::LoggingSeverity ls = (rtc::LoggingSeverity)min_sev;
  //if (bfirst) {
  //  rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
  //  rtc::LogMessage::LogTimestamps();
  //  // rtc::LogMessage::LogThreads();
  //  rtc::LogMessage::AddLogToStream(ec_log_, ls);
  //  bfirst = false;
  //}
  //return true;
}

#if defined(WEBRTC_ANDROID)
bool MediaClient::Initialize(JNIEnv* env,
                             jobject jencoder_factory,
                             jobject jdecoder_factory) {
#else
bool MediaClient::Initialize() {
#endif
#if defined(WEBRTC_ANDROID)
  return AndroidInitialize(env, jencoder_factory, jdecoder_factory);
#else

  // Set Field Trials String
  // webrtc::field_trial::IsEnabled("WebRTC-DisableUlpFecExperiment");
  // webrtc::FLAG_force_fieldtrials ="WebRTC-FlexFEC-03-Advertised/Enabled/";
  bool bOk = true;
  if (!m_bInitialized) {
    vcm_device_info_.reset(webrtc::VideoCaptureFactory::CreateDeviceInfo());
    bOk &= CreateThreads();
    EC_CHECK_VALUE(signaling_thread_, false);
    channelGenerator_.reset(new ChannelGenerator);

    bOk &= signaling_thread_->Invoke<bool>(
        RTC_FROM_HERE, [&] { return CreateChannelManager(); });

    bOk &= worker_thread_->Invoke<bool>(RTC_FROM_HERE,
                                        [&] { return CreateRtcEventLog(); });

    m_bInitialized = bOk;

    //SetTrace("ecmediaAPI.txt", 1);
  }
  return bOk;
#endif
}  // namespace ecmedia_sdk

#if defined(WEBRTC_ANDROID)
bool MediaClient::AndroidInitialize(JNIEnv* env,
                                    jobject jencoder_factory,
                                    jobject jdecoder_factory) {
  bool bOk = true;
  InitializeJVM();
  RTC_LOG(INFO) << __FUNCTION__ << " env: " << env
                << " jencoder_factory:" << jdecoder_factory
                << " jdecoder_factory: " << jdecoder_factory;

  if (!m_bInitialized) {
    vcm_device_info_.reset(webrtc::VideoCaptureFactory::CreateDeviceInfo());
    bOk &= CreateThreads();
    EC_CHECK_VALUE(signaling_thread_, false);
    channelGenerator_.reset(new ChannelGenerator);

    std::unique_ptr<webrtc::VideoEncoderFactory> encoderFactory;
    std::unique_ptr<webrtc::VideoDecoderFactory> decoderFactory;
    if (env && jencoder_factory && jdecoder_factory) {
      encoderFactory = std::unique_ptr<webrtc::VideoEncoderFactory>(
          webrtc::jni::CreateVideoEncoderFactory(
              env, webrtc::JavaParamRef<jobject>(env, jencoder_factory)));
      decoderFactory = std::unique_ptr<webrtc::VideoDecoderFactory>(
          webrtc::jni::CreateVideoDecoderFactory(
              env, webrtc::JavaParamRef<jobject>(env, jdecoder_factory)));
    }

    bOk &= signaling_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
      RTC_LOG(INFO) << __FUNCTION__ << " env: " << env
                    << " encoderFactory:" << encoderFactory
                    << " decoderFactory: " << decoderFactory;
      bool success = CreateChannelManager(std::move(encoderFactory),
                                          std::move(decoderFactory));
      RTC_LOG(INFO) << __FUNCTION__ << " env: " << env
                    << " encoderFactory:" << encoderFactory
                    << " decoderFactory: " << decoderFactory
                    << " success: " << success;
      return success;
    });

    bOk &= worker_thread_->Invoke<bool>(RTC_FROM_HERE,
                                        [&] { return CreateRtcEventLog(); });

    m_bInitialized = bOk;

    SetTrace("ecmediaAPI.txt", 1);
  }
  return bOk;
}
#endif

void MediaClient::UnInitialize() {
  // RTC_LOG(INFO) << __FUNCTION__  << "(),"<< " begin... ";
  /* for (auto it = desktop_devices_.begin(); it != desktop_devices_.end();
   it++) { if (it->second) { while(it->second.release()!=NULL);
         }
   }
   desktop_devices_.clear();*/
  transceivers_.clear();
  mapChannelSsrcs_.clear();
  TrackChannels_.clear();
  RtpSenders_.clear();
  if (pAudioDevice != nullptr) {
    delete[] pAudioDevice;
    pAudioDevice = nullptr;
  }

  if (channelGenerator_) {
    channelGenerator_->ResetGenerator();
  }

  if (signaling_thread_) {
    signaling_thread_->Invoke<void>(RTC_FROM_HERE, [this] {
      RTC_DCHECK_RUN_ON(signaling_thread_);
      std::vector<rtc::scoped_refptr<webrtc::VideoTrackInterface>>::iterator it = video_tracks_.begin();
      while (it != video_tracks_.end()) {
        (*it).get()->GetSource()->Release();
        while ((*it).release() != nullptr);
        it++;
      }
      video_tracks_.clear();
      asum_ = 0;
    });
  }
  // RTC_LOG(INFO) << __FUNCTION__  << "(),"<< " end... ";
}

void MediaClient::DestroyTransport() {
  RTC_LOG(INFO) << __FUNCTION__;
  if (m_nConnected == SC_CONNECTED) {
    m_nConnected = SC_DISCONNECTING;
    signaling_thread_->Invoke<bool>(RTC_FROM_HERE,
                                    [&] { return DisposeConnect(); });
    m_nConnected = SC_UNCONNECTED;
  };
}

bool MediaClient::GenerateChannelId(int& channelId) {
  EC_CHECK_VALUE((channelGenerator_), false);
  CreateTransportController();
  if (channelGenerator_->GeneratorId(channelId)) {
    mapChannelSsrcs_[channelId].mid = getStrFromInt(channelId);
    return true;
  }
  return false;
}

bool MediaClient::CreateThreads() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
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
    owned_signaling_thread_ = rtc::Thread::Create();
    owned_signaling_thread_->SetName("pc_signaling_thread", nullptr);
    owned_signaling_thread_->Start();
    signaling_thread_ = owned_signaling_thread_.get();
  }
#if defined(WEBRTC_IOS)
  ObjCCallClient::GetInstance()->InitDevice(signaling_thread_, worker_thread_);
#endif
  EC_CHECK_VALUE(signaling_thread_, false);
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return true;
}

bool MediaClient::CreateRtcEventLog() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  RTC_DCHECK_RUN_ON(worker_thread_);

  std::unique_ptr<webrtc::RtcEventLogFactoryInterface> event_log_factory =
      webrtc::CreateRtcEventLogFactory();
  EC_CHECK_VALUE(event_log_factory, false);

  auto encoding_type = webrtc::RtcEventLog::EncodingType::Legacy;
  if (webrtc::field_trial::IsEnabled("WebRTC-RtcEventLogNewFormat"))
    encoding_type = webrtc::RtcEventLog::EncodingType::NewFormat;
  event_log_ = event_log_factory
                   ? event_log_factory->CreateRtcEventLog(encoding_type)
                   : absl::make_unique<webrtc::RtcEventLogNullImpl>();

  EC_CHECK_VALUE(event_log_, false);
  event_log_ptr_ = event_log_.get();
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return true;
}

#if defined(WEBRTC_ANDROID)
bool MediaClient::CreateChannelManager(
    std::unique_ptr<webrtc::VideoEncoderFactory> encoderFactory,
    std::unique_ptr<webrtc::VideoDecoderFactory> decoderFactory) {
#else
bool MediaClient::CreateChannelManager() {
#endif
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  RTC_DCHECK_RUN_ON(signaling_thread_);
  rtc::InitRandom(rtc::Time32());

  rtc::scoped_refptr<webrtc::AudioDeviceModule> default_adm = nullptr;
  rtc::scoped_refptr<webrtc::AudioEncoderFactory> audio_encoder_factory =
      webrtc::CreateBuiltinAudioEncoderFactory();
  rtc::scoped_refptr<webrtc::AudioDecoderFactory> audio_decoder_factory =
      webrtc::CreateBuiltinAudioDecoderFactory();
  std::unique_ptr<webrtc::VideoEncoderFactory> video_encoder_factory =
#if defined(WEBRTC_IOS)
      ObjCCallClient::GetInstance()->getVideoEncoderFactory();
#elif defined(WEBRTC_ANDROID)
      encoderFactory != nullptr ? std::move(encoderFactory)
                                : webrtc::CreateBuiltinVideoEncoderFactory();
#elif defined(WEBRTC_WIN)
    webrtc::CreateBuiltinVideoEncoderFactory();
#endif

  std::unique_ptr<webrtc::VideoDecoderFactory> video_decoder_factory =
#if defined(WEBRTC_IOS)
      ObjCCallClient::GetInstance()->getVideoDecoderFactory();
#elif defined(WEBRTC_ANDROID)
      decoderFactory != nullptr ? std::move(decoderFactory)
                                : webrtc::CreateBuiltinVideoDecoderFactory();
#elif defined(WEBRTC_WIN)
    webrtc::CreateBuiltinVideoDecoderFactory();
#endif

  rtc::scoped_refptr<webrtc::AudioProcessing> audio_processing =
      webrtc::AudioProcessingBuilder().Create();

  std::unique_ptr<cricket::MediaEngineInterface> media_engine =
      cricket::WebRtcMediaEngineFactory::Create(
          default_adm, audio_encoder_factory, audio_decoder_factory,
          std::move(video_encoder_factory), std::move(video_decoder_factory),
          nullptr /* audio_mixer */, audio_processing);

  EC_CHECK_VALUE(media_engine, false);

  default_socket_factory_.reset(
      new rtc::BasicPacketSocketFactory(network_thread_));
  EC_CHECK_VALUE(default_socket_factory_, false);

  channel_manager_ = absl::make_unique<cricket::ChannelManager>(
      std::move(media_engine), absl::make_unique<cricket::RtpDataEngine>(),
      worker_thread_, network_thread_);

  EC_CHECK_VALUE(channel_manager_, false);

  channel_manager_->SetVideoRtxEnabled(true);
  bool bOk = channel_manager_->Init();
  EC_CHECK_VALUE(bOk, false);
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return bOk;
}

bool MediaClient::CreateCall(webrtc::RtcEventLog* event_log) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin..."
                << ", event_log: " << event_log;
  RTC_DCHECK_RUN_ON(worker_thread_);

  EC_CHECK_VALUE(channel_manager_, false);
  EC_CHECK_VALUE(channel_manager_->media_engine(), false);

  webrtc::Call::Config call_config(event_log);

  std::unique_ptr<webrtc::CallFactoryInterface> call_factory =
      webrtc::CreateCallFactory();
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

  if (webrtc::field_trial::IsEnabled(
          "WebRTC-Bwe-InjectedCongestionController")) {
    RTC_LOG(LS_INFO) << "Using injected network controller factory";
    call_config.network_controller_factory =
        nullptr;  // injected_network_controller_factory_.get();
  } else {
    RTC_LOG(LS_INFO) << "Using default network controller factory";
  }

  call_ = std::unique_ptr<webrtc::Call>(call_factory->CreateCall(call_config));
  EC_CHECK_VALUE(call_, false);
  call_ptr_ = call_.get();
  // channel_manager_->SetVideoRtxEnabled(true);
  call_factory.release();
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return true;
}

///////////////////////////////////multiMultichannel
/////////////////////////////////////////////
bool MediaClient::CreateTransport(const char* local_addr,
                                  int local_port,
                                  const char* remote_addr,
                                  int remote_port,
                                  const std::string& tid,
                                  bool bUdp) {
  API_LOG(INFO) << "l_addr: " << local_addr << ", l_port: " << local_port
                << ", r_addr: " << remote_addr << ", r_port: " << remote_port
                << ", tid: " << tid;

  rtc::SocketAddress local, remote;
  std::ostringstream strL, strRemote;
  strL << local_addr << ":" << local_port;
  local.FromString(strL.str().c_str());
  strRemote << remote_addr << ":" << remote_port;
  remote.FromString(strRemote.str().c_str());

  cricket::SessionDescription sdVideo;
  cricket::VideoContentDescription* video =
      new cricket::VideoContentDescription();
  EC_CHECK_VALUE(video, false);

  video->set_direction(webrtc::RtpTransceiverDirection::kSendRecv);
  video->set_rtcp_mux(true);
  cricket::StreamParams stVideo;
  stVideo.set_stream_ids({ksStreamId});
  stVideo.id = ksVideoLabel;
  video->AddStream(stVideo);
  sdVideo.AddContent(tid, cricket::MediaProtocolType::kRtp, false, video);

  webrtc::RTCError error =
      network_thread_->Invoke<webrtc::RTCError>(RTC_FROM_HERE, [=, &sdVideo] {
        return transport_controller_->ApplyDescription_n(false, &sdVideo);
      });
  EC_CHECK_VALUE(error.ok(), false);

  network_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
    if (bUdp)
      transport_controller_->setUdpConnection(
          tid, default_socket_factory_.get(), local, remote);
  });
  webrtc::RtpTransportInternal *
      rtp_transport = transport_controller_->GetRtpTransport(tid);
  if (rtp_transport)
    rtp_transport->SignalSentPacket.connect(this, &MediaClient::OnSentPacket);
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return true;
}
void MediaClient::DestroyChannel(int channel_id, bool is_video) {
  API_LOG(INFO) << "channel_id: " << channel_id << ", is_video: " << is_video;
  std::map<int, cricket::VideoChannel*>::iterator it = mVideoChannels_.begin();
  std::map<int, cricket::VoiceChannel*>::iterator ait = mVoiceChannels_.begin();
  while (it != mVideoChannels_.end()) {
    if (it->second == nullptr) {
      mVideoChannels_.erase(it);
      it = mVideoChannels_.begin();
    } else {
      it++;
    }
  }
  while (ait != mVoiceChannels_.end()) {
    if (ait->second == nullptr) {
      mVoiceChannels_.erase(ait);
      ait = mVoiceChannels_.begin();
    } else {
      ait++;
    }
  }

  if (is_video) {
    for (auto t : TrackChannels_) {
      if (t.first == channel_id) {
        RtpSenders_[t.first].get()->SetTrack(nullptr);

#if defined(WEBRTC_WIN)
        renderWndsManager_->RemoveLocalRenderer(t.first);
#endif
        RtpSenders_.erase(RtpSenders_.find(channel_id));
        TrackChannels_.erase(TrackChannels_.find(channel_id));
        break;
      }
    }
    it = mVideoChannels_.begin();
    while (it != mVideoChannels_.end()) {
      if (it->first == channel_id) {
#if defined(WEBRTC_WIN)
        renderWndsManager_->RemoveRemoteRenderWnd(channel_id);
#endif
        cricket::VideoChannel* channel = it->second;
        if (channel != nullptr) {
          channel->disconnect_all();
          for (auto transceiver : transceivers_) {
            cricket::ChannelInterface* channel_1 =
                transceiver->internal()->channel();
            if (channel == channel_1) {
              if (signaling_thread_) {
                signaling_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
                  RTC_DCHECK_RUN_ON(signaling_thread_);
                  transceiver->internal()->SetChannel(nullptr);
                });
              }
              DestroyChannelInterface(channel);
            }
          }
        }

        mVideoChannels_.erase(it);
        break;
      } else {
        it++;
      }
    }
  } else {
    ait = mVoiceChannels_.begin();
    while (ait != mVoiceChannels_.end()) {
      if (ait->first == channel_id) {
        cricket::VoiceChannel* channel = ait->second;
        if (channel != nullptr) {
          channel->disconnect_all();
          for (auto transceiver : transceivers_) {
            cricket::ChannelInterface* channel_1 =
                transceiver->internal()->channel();
            if (channel == channel_1) {
              signaling_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
                transceiver->internal()->SetChannel(nullptr);
                DestroyChannelInterface(channel);
              });
            }
          }
        }
        mVoiceChannels_.erase(ait);
        break;
      } else {
        ait++;
      }
    }
  }

  if (mVideoChannels_.size() == 0 && mVoiceChannels_.size() == 0) {
    DestroyTransport();
    UnInitialize();
  }
}

bool MediaClient::CreateChannel(const std::string& settings,
                                int channel_id,
                                bool is_video) {
  bool bOk = false;
  if (signaling_thread_) {
    bOk = signaling_thread_->Invoke<bool>(RTC_FROM_HERE, [=] {
      RTC_DCHECK_RUN_ON(signaling_thread_);
      if (is_video) {
        return CreateVideoChannel(settings, channel_id);
      } else {
        return CreateVoiceChannel(settings, channel_id);
      }
    });
  }
  API_LOG(INFO) << "channel_id: " << channel_id << ", is_video: " << is_video
                << ",tid: " << settings;
  return bOk;
}

bool MediaClient::CreateVideoChannel(const std::string& settings,
                                     int channelId) {
  EC_CHECK_VALUE(channel_manager_, false);
  EC_CHECK_VALUE(transport_controller_, false);
  std::string setting = settings;
#if defined(WEBRTC_IOS)
  // setting =
  //     "{\n   \"transportId\" : \"tran_1\",\n  \"red\" : 1,  \n
  //     \"fecType\" : 1, \n  \"nack\" : 1,\n   \"codecs\" : [\n      {\n
  //     \"codecName\" : \"H264\",\n         \"payloadType\" : 98,\n
  //     \"new_payloadType\" : 104\n      },\n      {\n \"codecName\" :
  //     \"rtx\",\n         \"payloadType\" : 101,\n \"new_payloadType\" :
  //     105,\n  \"associated_payloadType\" : 104\n   },\n      {\n
  //     \"codecName\" : \"red\",\n         \"payloadType\" : 104,\n
  //     \"new_payloadType\" : 110\n  },\n      {\n         \"codecName\" :
  //     \"rtx\",\n         \"payloadType\" : 105,\n    \"new_payloadType\"
  //     : 111,\n   \"associated_payloadType\" : 110\n  },\n      {\n
  //     \"codecName\" : \"ulpfec\",\n         \"payloadType\" : 106,\n
  //     \"new_payloadType\" : 112\n  }\n   ]\n}\n";

//      "{\n   \"transportId\" : \"tran_1\",\n  \"red\" : 0,  \n  \"fecType\" :
//      " "0, \n  \"nack\" : 1,\n   \"codecs\" : [\n      {\n         "
//      "\"codecName\" : \"H264\",\n         \"payloadType\" : 98,\n         "
//      "\"new_payloadType\" : 104\n      },\n      {\n         \"codecName\" :
//      "
//      "\"rtx\",\n         \"payloadType\" : 101,\n      \"new_payloadType\" :
//      " "105,\n  \"associated_payloadType\" : 104\n   }\n    ]\n}\n";
#elif defined(WEBRTC_WIN)
  // setting =
  //        "{\n   \"transportId\" : \"tran_1\",\n  \"red\" : 1,  \n
  //        \"fecType\" : 1, \n  \"nack\" : 1,\n   \"codecs\" : [\n      {\n
  //        \"codecName\" : \"H264\",\n         \"payloadType\" : 102,\n
  //        \"new_payloadType\" : 104\n      },\n      {\n \"codecName\" :
  //        \"rtx\",\n         \"payloadType\" : 103,\n \"new_payloadType\"
  //        : 105,\n  \"associated_payloadType\" : 104\n   },\n      {\n
  //        \"codecName\" : \"red\",\n         \"payloadType\" : 110,\n
  //        \"new_payloadType\" : 110\n  },\n      {\n         \"codecName\"
  //        : \"rtx\",\n         \"payloadType\" : 111,\n
  //        \"new_payloadType\" : 111,\n   \"associated_payloadType\" :
  //        110\n  },\n      {\n         \"codecName\" : \"ulpfec\",\n
  //        \"payloadType\" : 112,\n      \"new_payloadType\" : 112\n  }\n
  //        ]\n}\n";

  /*   "{\n   \"transportId\" : \"tran_1\",\n  \"red\" : 0,  \n  \"fecType\" : "
     "0, \n  \"nack\" : 1,\n   \"codecs\" : [\n      {\n         "
     "\"codecName\" : \"H264\",\n         \"payloadType\" : 102,\n         "
     "\"new_payloadType\" : 104\n      },\n      {\n         \"codecName\" : "
     "\"rtx\",\n         \"payloadType\" : 103,\n      \"new_payloadType\" : "
     "105,\n  \"associated_payloadType\" : 104\n   }\n    ]\n}\n";*/
#endif

  bool bOk = false;
  webrtc::CryptoOptions option;
  bool bSrtpRequired = false;
  webrtc::RtpTransportInternal* rtp_transport = nullptr;
  webrtc::MediaTransportInterface* media_transport = nullptr;

  std::string mid = GetMidFromChannelId(channelId);
  VideoCodecConfig config;
  if (!ParseVideoCodecSetting(setting.c_str(), &config)) {
    RTC_LOG(LS_ERROR) << " ParseVideoCodecSetting false...";
    return false;
  }
  rtp_transport = transport_controller_->GetRtpTransport(config.transportId);
  EC_CHECK_VALUE(rtp_transport, false);
  if (isCreateCall) {
    bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
      EC_CHECK_VALUE(event_log_ptr_, false);
      return CreateCall(event_log_ptr_);
    });
    isCreateCall = false;
  }

  cricket::VideoChannel* video_channel_ = channel_manager_->CreateVideoChannel(
      call_ptr_, media_config_, rtp_transport, media_transport,
      signaling_thread_, mid, bSrtpRequired, option, &ssrc_generator_,
      video_options_);

  EC_CHECK_VALUE(video_channel_, false);

  //video_channel_->SignalSentPacket.connect(this,
  //                                         &MediaClient::OnSentPacket_Video);


  //video_channel_->SetRtpTransport(rtp_transport);

  network_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
    rtp_transport->SignalWritableState(rtp_transport);
  });

  EC_CHECK_VALUE(video_channel_->media_channel(), false);

  cricket::VideoSendParameters vidoe_send_params;
  vidoe_send_params.mid = mid;

  vidoe_send_params.max_bandwidth_bps = kMaxBandwidthBps;
  //
  // config.codecName = "h264";
  // config.payloadType = 104;
  channel_manager_->GetSupportedVideoCodecs(&vidoe_send_params.codecs);
  // FilterVideoCodec(&config.video_stream_configs, vidoe_send_params.codecs);
  FilterVideoCodec(config, vidoe_send_params.codecs);
  channel_manager_->GetSupportedVideoRtpHeaderExtensions(
      &vidoe_send_params.extensions);

  /* if (vidoe_send_params.codecs.size() > 0) {
     vidoe_send_params.codecs.at(0).params[cricket::kCodecParamMinBitrate] =
         getStrFromInt(config.minBitrateKps);
     vidoe_send_params.codecs.at(0).params[cricket::kCodecParamMaxBitrate] =
         getStrFromInt(config.maxBitrateKps);
     vidoe_send_params.codecs.at(0).params[cricket::kCodecParamStartBitrate] =
         getStrFromInt(config.startBitrateKps);
     // sendParams.codecs.at(0).params[cricket::kDefaultVideoMaxFramerate] =
     //    getStrFromInt(config.maxFramerate);
     vidoe_send_params.codecs.at(0).params[cricket::kCodecParamMaxQuantization]
   = getStrFromInt(config.maxQp);
   }*/

  cricket::StreamParams video_stream_params;
  video_stream_params.cname = "DcRqgGg4U0HjSqLy";
  video_stream_params.id = ksVideoLabel;

  std::vector<uint32_t> ssrcsRemote;
  std::vector<uint32_t> ssrcsLocal;
  GetMediaSsrc(true, channelId, ssrcsLocal);
  GetMediaSsrc(false, channelId, ssrcsRemote);
  std::vector<uint32_t>::iterator it = ssrcsLocal.begin();
  while (it != ssrcsLocal.end()) {
    video_stream_params.add_ssrc(*it);
    video_stream_params.AddFidSsrc(*it, *it | 0x40);
    it++;
  }

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->SetSendParameters(
        vidoe_send_params);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->AddSendStream(video_stream_params);
  });

  cricket::StreamParams video_stream_params_recv;
  video_stream_params_recv.cname = "DcRqgGg4U0HjSqLy";
  video_stream_params_recv.id = ksVideoLabel;

  std::vector<uint32_t>::iterator itr = ssrcsRemote.begin();
  while (itr != ssrcsRemote.end()) {
    video_stream_params_recv.add_ssrc(*itr);
    video_stream_params_recv.AddFidSsrc(*itr, *itr | 0x40);
    itr++;
  }
  cricket::VideoRecvParameters video_recv_params;
  signaling_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
    channel_manager_->GetSupportedVideoCodecs(&video_recv_params.codecs);
    //  FilterVideoCodec(&config.video_stream_configs,
    //  video_recv_params.codecs);
    FilterVideoCodec(config, video_recv_params.codecs);
    channel_manager_->GetSupportedVideoRtpHeaderExtensions(
        &video_recv_params.extensions);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->SetRecvParameters(
        video_recv_params);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return video_channel_->media_channel()->AddRecvStream(
        video_stream_params_recv);
  });
  webrtc::RtpDemuxerCriteria demuxer_criteria;
  demuxer_criteria.mid = config.transportId;
  itr = ssrcsRemote.begin();
  while (itr != ssrcsRemote.end()) {
    demuxer_criteria.ssrcs.insert(*itr);
    itr++;
  }

  if (demuxer_criteria.ssrcs.size()) {
    transport_controller_->RegisterRtpDemuxerSink(demuxer_criteria,
                                                  video_channel_);
  }
  mVideoChannels_[channelId] = video_channel_;
  /*if (config.isScreenShare) {
    SetVideoDegradationMode(channelId,webrtc::DegradationPreference::MAINTAIN_RESOLUTION);
  }*/
  return bOk;
}

bool MediaClient::RequestRemoteSsrc(int channel_id, int flag, int32_t ssrc) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "() "
                << " begin... "
                << ", channel_id: " << channel_id << ", ssrc: " << ssrc;
  cricket::WebRtcVideoChannel* internal_video_channel =
      GetInternalVideoChannel(channel_id);
  bool bOk = false;
  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return internal_video_channel->RequestRemoteSsrc(channel_id, flag, ssrc);
  });
  return bOk;
}

bool MediaClient::SetLocalMute(int channel_id, bool bMute) {
  API_LOG(INFO) << "channel_id " << channel_id << ", bMute : " << bMute;
  for (const auto& transceiver : transceivers_) {
    std::string mid = GetMidFromChannelId(channel_id);
    if (transceiver.get()->mid() == mid) {
      return transceiver.get()->sender()->track()->set_enabled(!bMute);
      break;
    }
  }
  return false;
}

bool MediaClient::SetLoudSpeakerStatus(bool enabled) {
  API_LOG(INFO) << "enabled:" << enabled;
#if defined(WEBRTC_IOS)
  return ObjCCallClient::GetInstance()->SetSpeakerStatus(enabled);
#endif
  return true;
}

bool MediaClient::GetLoudSpeakerStatus(bool& enabled) {
  API_LOG(INFO) << "enabled:" << enabled;
#if defined(WEBRTC_IOS)
  return ObjCCallClient::GetInstance()->GetSpeakerStatus(enabled);
#endif
  return true;
}

bool MediaClient::SetRemoteMute(int channel_id, bool bMute) {
  API_LOG(INFO) << "channel_id: " << channel_id << ", bMute: " << bMute;
#if defined(WEBRTC_IOS)
  return ObjCCallClient::GetInstance()->SetSpeakerStatus(!bMute);
#endif
  return false;
}

bool MediaClient::CreateVoiceChannel(const std::string& settings,
                                     int channelId) {
  EC_CHECK_VALUE(channel_manager_, false);
  EC_CHECK_VALUE(transport_controller_, false);
  SetAEC(true);
  SetNS(true);
  SetAGC(true);
  audio_options_.experimental_agc = true;
  audio_options_.experimental_ns = true;
  audio_options_.delay_agnostic_aec = true;

  webrtc::CryptoOptions option;
  bool bOk = false;
  bool bSrtpRequired = false;
  webrtc::RtpTransportInternal* rtp_transport = nullptr;
  webrtc::MediaTransportInterface* media_transport = nullptr;

  std::string mid = GetMidFromChannelId(channelId);

  AudioCodecConfig config;
  if (!ParseAudioCodecSetting(settings.c_str(), &config)) {
    return false;
  }

  rtp_transport = transport_controller_->GetRtpTransport(config.transportId);
  EC_CHECK_VALUE(rtp_transport, false);
  cricket::VoiceChannel* voice_channel_ = channel_manager_->CreateVoiceChannel(
      call_ptr_, media_config_, rtp_transport, media_transport,
      signaling_thread_, mid, bSrtpRequired, option, &ssrc_generator_,
      audio_options_);
  EC_CHECK_VALUE(voice_channel_, false);

  //voice_channel_->SignalSentPacket.connect(this,
  //                                         &MediaClient::OnSentPacket_Voice);
  //voice_channel_->SetRtpTransport(rtp_transport);

  network_thread_->Invoke<void>(RTC_FROM_HERE, [=] {
    return rtp_transport->SignalWritableState(rtp_transport);
  });

  EC_CHECK_VALUE(voice_channel_->media_channel(), false);

  cricket::AudioSendParameters sendParams;
  sendParams.mid = mid;

  signaling_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
    channel_manager_->GetSupportedAudioSendCodecs(&sendParams.codecs);
    channel_manager_->GetSupportedAudioRtpHeaderExtensions(
        &sendParams.extensions);
  });

  FilterAudioCodec(config, sendParams.codecs);

  cricket::StreamParams audio_stream_send;
  audio_stream_send.cname = "DcRqgGg4U0HjSqLy";
  audio_stream_send.id = ksAudioLabel;

  std::vector<uint32_t> ssrcsRemote;
  std::vector<uint32_t> ssrcsLocal;
  GetMediaSsrc(true, channelId, ssrcsLocal);
  GetMediaSsrc(false, channelId, ssrcsRemote);
  std::vector<uint32_t>::iterator it = ssrcsLocal.begin();
  while (it != ssrcsLocal.end()) {
    audio_stream_send.add_ssrc(*it);
    it++;
  }

  sendParams.max_bandwidth_bps = kMaxBandwidthBps;
  //

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->SetSendParameters(sendParams);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->AddSendStream(audio_stream_send);
  });

  cricket::StreamParams audio_stream_recv;
  audio_stream_recv.cname = "DcRqgGg4U0HjSqLy";
  audio_stream_recv.id = ksAudioLabel;
  std::vector<uint32_t>::iterator itr = ssrcsRemote.begin();
  while (itr != ssrcsRemote.end()) {
    audio_stream_recv.add_ssrc(*itr);
    itr++;
  }

  cricket::AudioRecvParameters recvParams;
  channel_manager_->GetSupportedAudioReceiveCodecs(&recvParams.codecs);
  channel_manager_->GetSupportedAudioRtpHeaderExtensions(
      &recvParams.extensions);

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->SetRecvParameters(recvParams);
  });

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    return voice_channel_->media_channel()->AddRecvStream(audio_stream_recv);
  });

  webrtc::RtpDemuxerCriteria demuxer_criteria;
  demuxer_criteria.mid = config.transportId;
  itr = ssrcsRemote.begin();
  while (itr != ssrcsRemote.end()) {
    demuxer_criteria.ssrcs.insert(*itr);
    itr++;
  }
  transport_controller_->RegisterRtpDemuxerSink(demuxer_criteria,
                                                voice_channel_);
  mVoiceChannels_[channelId] = voice_channel_;

  return bOk;
}

bool MediaClient::SelectVoiceSource(
    int channelId,
    const std::string& track_id,
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audiotrack) {
  API_LOG(INFO) << " channelId: " << channelId << ", track_id: " << track_id
                << ", audiotrack: " << audiotrack;
  bool bResult = false;
  bResult = signaling_thread_->Invoke<bool>(RTC_FROM_HERE, [this, channelId,
                                                            track_id,
                                                            audiotrack] {
    RTC_DCHECK_RUN_ON(signaling_thread_);
    bool bOk = false;

    auto audio_sender = webrtc::AudioRtpSender::Create(
        worker_thread_, track_id /*rtc::CreateRandomUuid()*/,
        nullptr /*stats_.get()*/);

    EC_CHECK_VALUE(audio_sender, false);

    rtc::scoped_refptr<
        webrtc::RtpSenderProxyWithInternal<webrtc::RtpSenderInternal>>
        senderAudio = webrtc::RtpSenderProxyWithInternal<
            webrtc::RtpSenderInternal>::Create(signaling_thread_, audio_sender);
    EC_CHECK_VALUE(senderAudio, false);

    bOk = senderAudio->SetTrack(audiotrack.get());
    EC_CHECK_VALUE(bOk, false);
    const std::vector<std::string>& stream_ids = {"stream_id"};
    senderAudio->internal()->set_stream_ids(stream_ids);
    senderAudio->internal()->set_init_send_encodings(
        std::vector<webrtc::RtpEncodingParameters>());

    rtc::scoped_refptr<
        webrtc::RtpReceiverProxyWithInternal<webrtc::RtpReceiverInternal>>
        receiverAudio = webrtc::
            RtpReceiverProxyWithInternal<webrtc::RtpReceiverInternal>::Create(
                signaling_thread_, new webrtc::AudioRtpReceiver(
                                       worker_thread_, rtc::CreateRandomUuid(),
                                       std::vector<std::string>({})));

    EC_CHECK_VALUE(receiverAudio, false);

    auto transceiverAudio =
        webrtc::RtpTransceiverProxyWithInternal<webrtc::RtpTransceiver>::Create(
            signaling_thread_,
            new webrtc::RtpTransceiver(senderAudio, receiverAudio));

    std::string mid = GetMidFromChannelId(channelId);

    transceiverAudio->internal()->set_mid(mid);

    EC_CHECK_VALUE(transceiverAudio, false);

    transceivers_.push_back(transceiverAudio);
    EC_CHECK_VALUE(mVoiceChannels_[channelId], false);

    std::vector<uint32_t> ssrcsRemote;
    std::vector<uint32_t> ssrcsLocal;
    GetMediaSsrc(true, channelId, ssrcsLocal);
    GetMediaSsrc(false, channelId, ssrcsRemote);
    std::vector<uint32_t>::iterator it = ssrcsLocal.begin();
    std::vector<uint32_t>::iterator itr = ssrcsRemote.begin();
    if (transceiverAudio->internal()) {
      transceiverAudio->internal()->SetChannel(mVoiceChannels_[channelId]);
      if (transceiverAudio->internal()->sender_internal()) {
        while (it != ssrcsLocal.end()) {
          transceiverAudio->internal()->sender_internal()->SetSsrc(*it);
          it++;
        }
      }
      if (transceiverAudio->internal()->receiver_internal()) {
        while (itr != ssrcsRemote.end()) {
          transceiverAudio->internal()->receiver_internal()->SetupMediaChannel(
              *itr);
          itr++;
        }
      }
    }
    return true;
  });
  return bResult;
}

bool MediaClient::SelectVideoSource(
    int channelid,
    const std::string& track_id,
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track) {
  API_LOG(INFO) << "channelid: " << channelid << ", track_id: " << track_id
                << ", video_track: " << video_track;
  bool bResult = false;
  bResult = signaling_thread_->Invoke<bool>(RTC_FROM_HERE, [this, channelid,
                                                            track_id,
                                                            video_track] {
    RTC_DCHECK_RUN_ON(signaling_thread_);
    bool bOk = false;
    if (RtpSenders_.find(channelid) != RtpSenders_.end()) {
      bOk = RtpSenders_[channelid].get()->SetTrack(video_track);

    } else {
      auto video_sender = webrtc::VideoRtpSender::Create(
          worker_thread_, track_id /*rtc::CreateRandomUuid()*/);
      EC_CHECK_VALUE(video_sender, false);

      rtc::scoped_refptr<
          webrtc::RtpSenderProxyWithInternal<webrtc::RtpSenderInternal>>
          senderVideo = webrtc::RtpSenderProxyWithInternal<
              webrtc::RtpSenderInternal>::Create(signaling_thread_,
                                                 video_sender);
      EC_CHECK_VALUE(senderVideo, false);

      bOk = senderVideo->SetTrack(video_track);
      RtpSenders_[channelid] = senderVideo;

      EC_CHECK_VALUE(bOk, false);
      const std::vector<std::string>& stream_ids = {"stream_id"};
      // senderVideo->internal()->set_stream_ids({"stream_id"});
      senderVideo->internal()->set_stream_ids(stream_ids);
      senderVideo->internal()->set_init_send_encodings(
          std::vector<webrtc::RtpEncodingParameters>());

      rtc::scoped_refptr<
          webrtc::RtpReceiverProxyWithInternal<webrtc::RtpReceiverInternal>>
          receiverVideo = webrtc::RtpReceiverProxyWithInternal<
              webrtc::RtpReceiverInternal>::Create(signaling_thread_,
                                                   new webrtc::VideoRtpReceiver(
                                                       worker_thread_,
                                                       rtc::CreateRandomUuid(),
                                                       std::vector<std::string>(
                                                           {})));
      EC_CHECK_VALUE(receiverVideo, false);

#if defined WEBRTC_WIN
      if (renderWndsManager_) {
        renderWndsManager_->StartRemoteRenderer(
            channelid, static_cast<webrtc::VideoTrackInterface*>(
                           receiverVideo->track().get()));
      }
#elif defined(WEBRTC_IOS)
      rtc::VideoSinkInterface<webrtc::VideoFrame>* sink =
      ObjCCallClient:: GetInstance()->getRemoteVideoSilkByChannelID(channelid);
      if(sink){
          static_cast<webrtc::VideoTrackInterface*>(
            receiverVideo->track().get())->AddOrUpdateSink(sink,
                                                           rtc::VideoSinkWants());
      }
#elif defined(WEBRTC_ANDROID)
	rtc::VideoSinkInterface<webrtc::VideoFrame>* sink =
		GetRemoteVideoSink(channelid);
	RTC_LOG(INFO) << "GetRemoteVideoSink() sink: " << sink;
	if (sink && receiverVideo->track()) {
		webrtc::VideoTrackInterface* track_android =
			static_cast<webrtc::VideoTrackInterface*>(
				receiverVideo->track().get());
		if (track_android) {
			track_android->AddOrUpdateSink(sink, rtc::VideoSinkWants());
			RTC_LOG(INFO) << "track_android->AddOrUpdateSink track_android: " << track_android;
		}
	}
#endif

      auto transceiverVideo = webrtc::RtpTransceiverProxyWithInternal<
          webrtc::RtpTransceiver>::Create(signaling_thread_,
                                          new webrtc::RtpTransceiver(
                                              senderVideo, receiverVideo));
      // transceiverVideo->internal()->AddSender
      EC_CHECK_VALUE(transceiverVideo, false);
      std::string mid = GetMidFromChannelId(channelid);
      transceiverVideo->internal()->set_mid(mid);
      transceivers_.push_back(transceiverVideo);
      EC_CHECK_VALUE(mVideoChannels_[channelid], false);
      std::vector<uint32_t> ssrcsRemote;
      std::vector<uint32_t> ssrcsLocal;
      GetMediaSsrc(true, channelid, ssrcsLocal);
      GetMediaSsrc(false, channelid, ssrcsRemote);
      std::vector<uint32_t>::iterator it = ssrcsLocal.begin();
      std::vector<uint32_t>::iterator itr = ssrcsRemote.begin();
      if (transceiverVideo->internal()) {
        transceiverVideo->internal()->set_created_by_addtrack(true);

        if (!video_track) {
          RTC_LOG(INFO) << __FUNCTION__ << " video_track is null kRecvOnly";

          transceiverVideo->internal()->set_direction(
              webrtc::RtpTransceiverDirection::kRecvOnly);
        } else {
          RTC_LOG(INFO) << __FUNCTION__ << " video_track kSendRecv";
          transceiverVideo->internal()->set_direction(
              webrtc::RtpTransceiverDirection::kSendOnly);
        }

        transceiverVideo->internal()->SetChannel(mVideoChannels_[channelid]);
        if (transceiverVideo->internal()->sender_internal() && video_track) {
          while (it != ssrcsLocal.end()) {
            transceiverVideo->internal()->sender_internal()->SetSsrc(*it);
            it++;
          }
        }
        if (transceiverVideo->internal()->receiver_internal()) {
          while (itr != ssrcsRemote.end()) {
            transceiverVideo->internal()
                ->receiver_internal()
                ->SetupMediaChannel(*itr);
            itr++;
          }
        }
      }
    }
    return bOk;
  });
  if (bResult) {
    TrackChannels_[channelid] = video_track;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return bResult;
}

rtc::scoped_refptr<webrtc::VideoTrackInterface>
MediaClient::SelectVideoSourceOnFlight(int channelid,
                                       int device_index,
                                       const std::string& track_params) {
#if defined WEBRTC_WIN
  bool bResult = false;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track1 =
      signaling_thread_
          ->Invoke<rtc::scoped_refptr<webrtc::VideoTrackInterface>>(
              RTC_FROM_HERE, [&] {
                rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;
                auto it = cameraId_videoTrack_pairs_.find(device_index);
                if (it != cameraId_videoTrack_pairs_.end()) {
                  video_track = it->second;
                } else {
                  video_track = CreateLocalVideoTrack(track_params);
                  // EC_CHECK_VALUE(video_track, false);
                }

                std::string mid = GetMidFromChannelId(channelid);
                // RTC_LOG(LS_INFO) << "---ylr channelid: " << channelid << "
                // mid: " << mid;
                for (auto transceiver : transceivers_) {
                  // RTC_LOG(LS_INFO) << "---ylr transceiver mid: "
                  //                  <<
                  //                  transceiver->internal()->mid().value_or("not
                  //                  set");
                  if (transceiver->internal()->mid().value_or("not set") ==
                          mid &&
                      transceiver->internal()->sender_internal()->SetTrack(
                          video_track)) {
                    // EC_CHECK_VALUE(renderWndsManager_, false);
                    renderWndsManager_->UpdateLocalVideoTrack(channelid,
                                                              video_track);
                  }
                }

                return video_track;
              });
  if (bResult)
    RTC_LOG(INFO) << "---ylr SelectVideoSourceOnFlight on flight ok.";
  else
    RTC_LOG(INFO) << "---ylr SelectVideoSourceOnFlight on flight fail!";
  return video_track1;
#endif
  return nullptr;
}

void MediaClient::DestroyLocalAudioTrack(
    rtc::scoped_refptr<webrtc::AudioTrackInterface> track) {
  API_LOG(INFO) << "track: " << track;

  cameraId_videoTrack_pairs_.clear();
  if (track) {
    for (int i = 0; i < 5; i++) {
      if (track == audio_tracks_[i]) {
        audio_tracks_[i].release();
        audio_tracks_[i] = NULL;
        break;
      } else {
        track.release();
      }
    }
  }

  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
}
rtc::scoped_refptr<webrtc::AudioTrackInterface>
MediaClient::CreateLocalVoiceTrack(const std::string& track_id) {
  API_LOG(INFO) << "track_id: " << track_id << ", voice_index: " << track_id;

  if (signaling_thread_) {
    audio_tracks_[asum_] =
        signaling_thread_
            ->Invoke<rtc::scoped_refptr<webrtc::AudioTrackInterface>>(
                RTC_FROM_HERE, [this, track_id] {
                  RTC_DCHECK_RUN_ON(signaling_thread_);
                  cricket::AudioOptions options;
                  rtc::scoped_refptr<webrtc::LocalAudioSource> source(
                      webrtc::LocalAudioSource::Create(&options));
                  RTC_DCHECK(source);
                  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track =
                      nullptr;
                  if (source) {
                    audio_track = webrtc::AudioTrackProxy::Create(
                        signaling_thread_,
                        webrtc::AudioTrack::Create(track_id /*kAudioLabel*/,
                                                   source));
                    // EC_CHECK_VALUE(audio_track, false);
                    RTC_DCHECK(audio_track);
                  }
                  return audio_track;
                });
    asum_++;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return audio_tracks_[asum_ - 1];
}

void MediaClient::DestroyLocalVideoTrack(
    rtc::scoped_refptr<webrtc::VideoTrackInterface> track) {
  API_LOG(INFO) << "track: " << track;
  if (track) {
    for (auto t : TrackChannels_) {
      if (t.second == track) {
        RtpSenders_[t.first].get()->SetTrack(nullptr);
#if defined(WEBRTC_WIN)
        renderWndsManager_->RemoveLocalRenderer(t.first);
#endif
      }
    }
    std::vector<rtc::scoped_refptr<webrtc::VideoTrackInterface>>::iterator it = video_tracks_.begin();
    while(it != video_tracks_.end()) {
      if (track == *it) {
        signaling_thread_->Invoke<void>(
            RTC_FROM_HERE, [track] { track.get()->GetSource()->Release(); });
        while (track.release() != nullptr);
        video_tracks_.erase(it);
        break;
      } else {
        it++;
	  }
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
}

int MediaClient::StartScreenShare(int type) {
#if defined(WEBRTC_WIN)
  RTC_LOG(INFO) << __FUNCTION__ << "()";
  desktop_device_ = desktop_devices_.find(type)->second;
  if (desktop_device_) {
    worker_thread_->Invoke<void>(RTC_FROM_HERE,
                                 [this] { desktop_device_->Start(); });
    return 0;
  } else {
    return -1;
  }
#endif
  return 0;
}

int MediaClient::StopScreenShare(int type) {
#if defined(WEBRTC_WIN)
  RTC_LOG(INFO) << __FUNCTION__ << "()";
  desktop_device_ = desktop_devices_.find(type)->second;
  if (desktop_device_) {
    desktop_device_->Stop();
  }
#endif
  return 0;
}
int MediaClient::GetWindowsList(int type,
                                webrtc::DesktopCapturer::SourceList& source) {
#if defined(WEBRTC_WIN)
  auto it = desktop_devices_.find(type);
  if (it != desktop_devices_.end()) {
    it->second->GetCaptureSources(source);
    return 0;
  } else {
    return -1;
  }
#endif
  return 0;
}

int MediaClient::CreateDesktopCapture(int type) {
  API_LOG(INFO) << "type: " << type;
#if defined(WEBRTC_WIN)
  auto it = desktop_devices_.find(type);
  if (it != desktop_devices_.end()) {
    return 0;
  }
  desktop_device_ = win_desk::ECDesktopCapture::Create(type);
  if (desktop_device_) {
    desktop_devices_[type] = desktop_device_;
    return 0;
  } else {
    return -1;
  }
#endif
  return 0;
}
int MediaClient::SetDesktopSourceID(int type, int id) {
#if defined(WEBRTC_WIN)
  if (id < 0 || type < 0) {
    return -1;
  }
  auto it = desktop_devices_.find(type);
  if (it != desktop_devices_.end()) {
    it->second->SetDesktopSourceID(id);
    return 0;
  } else {
    return -1;
  }
#endif
  return 0;
}

rtc::scoped_refptr<webrtc::VideoTrackInterface>
MediaClient::CreateLocalVideoTrack(const std::string& track_params) {
  API_LOG(INFO) << "track_params: " << track_params;
  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(track_params, jmessage)) {
    return nullptr;
  }

  VideoDeviceConfig config;
  ParseVideoDeviceSetting(track_params.c_str(), &config);

  int type = config.videoSourceType;
  std::string track_id = config.trackId;

  int camera_index = 0;
  camera_index = config.deviceIndex;

  // rtc::GetIntFromJsonObject(jmessage, "video_mode", &type);
  // rtc::GetIntFromJsonObject(jmessage, "camera_index", &camera_index);
  // rtc::GetStringFromJsonObject(jmessage, "track_id", &track_id);

  if (signaling_thread_) {
    video_track_ =
        signaling_thread_
            ->Invoke<rtc::scoped_refptr<webrtc::VideoTrackInterface>>(
                RTC_FROM_HERE, [this, type, track_id, config, camera_index] {
                  RTC_DCHECK_RUN_ON(signaling_thread_);
                  //  EC_CHECK_VALUE((channelId >= 0), false);
                  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track =
                      nullptr;

                  webrtc::DesktopCapturer::SourceList sources;
#ifdef WEBRTC_WIN
                  rtc::scoped_refptr<CapturerTrackSource> video_device =
                      nullptr;
#endif
                  switch (type) {
                    case VIDEO_CAMPER:
#if defined(WEBRTC_WIN)
                      video_device = CapturerTrackSource::Create(
                          config.width, config.height, config.maxFramerate,
                          camera_index);
                      if (video_device) {
                        video_track = webrtc::VideoTrackProxy::Create(
                            signaling_thread_, worker_thread_,
                            webrtc::VideoTrack::Create(track_id, video_device,
                                                       worker_thread_));
                      }

#elif defined(WEBRTC_IOS)
             ObjCCallClient::GetInstance()->SetCameraIndex(camera_index);
             video_track = webrtc::VideoTrackProxy::Create(
                  signaling_thread_, worker_thread_,
                  webrtc::VideoTrack::Create(
                      track_id,
                      ObjCCallClient::GetInstance()->getLocalVideoSource(
                          signaling_thread_, worker_thread_),
                      worker_thread_));
#else
	        //ytx_todo 
		 RTC_LOG(INFO) <<"andorid camer id"<< camera_index ;

#endif

                      return video_track;
                    case VIDEO_SCREEN:
#ifdef WEBRTC_WIN
                      desktop_device_ =
                          desktop_devices_.find(camera_index)->second;

                      video_track = webrtc::VideoTrackProxy::Create(
                          signaling_thread_, worker_thread_,
                          webrtc::VideoTrack::Create(track_id, desktop_device_,
                                                     worker_thread_));
#endif
                      return video_track;

                      break;
                    default:
                      return video_track;
                      break;
                  }
                });

    video_tracks_.push_back(video_track_);
    return video_track_;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end...";
  return NULL;
}

bool MediaClient::RequestRemoteVideo(int channel_id, int32_t remote_ssrc) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channel_id: " << channel_id << ", remote_ssrc,"
                << remote_ssrc;
  return true;
}

bool MediaClient::StartChannel(int channel_id) {
  API_LOG(INFO) << "channel_id: " << channel_id;
  bool bOk = false;
  m_nConnected = SC_CONNECTED;

  bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
    if (mVideoChannels_[channel_id] &&
        mVideoChannels_[channel_id]->media_channel()) {
      mVideoChannels_[channel_id]->media_channel()->OnReadyToSend(true);
      return mVideoChannels_[channel_id]->media_channel()->SetSend(true);
    } else if (mVoiceChannels_[channel_id] &&
               mVoiceChannels_[channel_id]->media_channel()) {
      mVoiceChannels_[channel_id]->media_channel()->OnReadyToSend(true);
      mVoiceChannels_[channel_id]->media_channel()->SetPlayout(true);
      mVoiceChannels_[channel_id]->media_channel()->SetSend(true);
      return true;
    }
    return false;
  });
  return bOk;
}

bool MediaClient::StopChannel(int channel_id) {
  API_LOG(INFO) << "channel_id: " << channel_id;
  bool bOk = false;

  if (mVideoChannels_[channel_id] &&
      mVideoChannels_[channel_id]->media_channel()) {
    bOk = worker_thread_->Invoke<bool>(RTC_FROM_HERE, [&] {
      mVideoChannels_[channel_id]->media_channel()->OnReadyToSend(false);
      return mVideoChannels_[channel_id]->media_channel()->SetSend(false);
    });

  } else if (mVoiceChannels_[channel_id] &&
             mVoiceChannels_[channel_id]->media_channel()) {
    worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
      mVoiceChannels_[channel_id]->media_channel()->OnReadyToSend(false);
      mVoiceChannels_[channel_id]->media_channel()->SetPlayout(false);
      mVoiceChannels_[channel_id]->media_channel()->SetSend(false);
    });
    return true;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return bOk;
}
/////////////////////////////////MultiChannel  end
///////////////////////////////////////////////////////////////

bool MediaClient::CreateTransportController(bool disable_encryp) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << ", disable_encryp: " << disable_encryp;
  EC_CHECK_VALUE(network_thread_, false);
  if (!m_bControll) {
    webrtc::GeneralTransportController::Config config;
    transportControllerObserve_.reset(
        new TransportControllerObserve(network_thread_, this));

    config.transport_observer = transportControllerObserve_.get();
    config.event_log = event_log_.get();
    config.disable_encryption = disable_encryp;

    transport_controller_.reset(new webrtc::GeneralTransportController(
        signaling_thread_, network_thread_, nullptr /*port_allocator_.get()*/,
        nullptr /*async_resolver_factory_.get()*/, config));
    EC_CHECK_VALUE(transport_controller_, false);
    m_bControll = true;
  }
  return true;
}

 void MediaClient::OnSentPacket(const rtc::SentPacket& sent_packet) {
    // RTC_LOG(INFO) << __FUNCTION__ << "() packet number:" <<
     // sent_packet.packet_id;
  RTC_DCHECK_RUN_ON(worker_thread_);
  EC_CHECK_VALUE(call_, void());
  call_->OnSentPacket(sent_packet);
  
}

bool MediaClient::DisposeConnect() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  RTC_DCHECK_RUN_ON(signaling_thread_);

  for (const auto& transceiver : transceivers_) {
    transceiver->Stop();
  }

  DestroyAllChannels();

  network_thread_->Invoke<void>(RTC_FROM_HERE, [this] {
    RTC_DCHECK_RUN_ON(network_thread_);
    transport_controller_->DestroyAllGeneralTransports_n();
    transport_controller_.reset(nullptr);
    transportControllerObserve_.reset(nullptr);
  });

  m_bControll = false;
#if defined WEBRTC_WIN
  renderWndsManager_.reset(nullptr);
#endif

  worker_thread_->Invoke<void>(RTC_FROM_HERE, [this] {
    RTC_DCHECK_RUN_ON(worker_thread_);
    call_.reset();
  });
  isCreateCall = true;
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return true;
}

void MediaClient::DestroyAllChannels() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  for (const auto& transceiver : transceivers_) {
    if (transceiver->media_type() == cricket::MEDIA_TYPE_VIDEO) {
      DestroyTransceiverChannel(transceiver);
    }
  }
  for (const auto& transceiver : transceivers_) {
    if (transceiver->media_type() == cricket::MEDIA_TYPE_AUDIO) {
      DestroyTransceiverChannel(transceiver);
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
}

void MediaClient::DestroyTransceiverChannel(
    rtc::scoped_refptr<
        webrtc::RtpTransceiverProxyWithInternal<webrtc::RtpTransceiver>>
        transceiver) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin..."
                << ", transceiver: " << transceiver;
  RTC_DCHECK(transceiver);

  cricket::ChannelInterface* channel = transceiver->internal()->channel();
  if (channel) {
    transceiver->internal()->SetChannel(nullptr);
    DestroyChannelInterface(channel);
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
}

void MediaClient::DestroyChannelInterface(cricket::ChannelInterface* channel) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channel: " << channel;
  EC_CHECK_VALUE(channel, void());
  EC_CHECK_VALUE(channel_manager_, void());
  switch (channel->media_type()) {
    case cricket::MEDIA_TYPE_AUDIO:
      channel_manager_->DestroyVoiceChannel(
          static_cast<cricket::VoiceChannel*>(channel));
      break;
    case cricket::MEDIA_TYPE_VIDEO:
      channel_manager_->DestroyVideoChannel(
          static_cast<cricket::VideoChannel*>(channel));
      break;
    case cricket::MEDIA_TYPE_DATA:
      channel_manager_->DestroyRtpDataChannel(
          static_cast<cricket::RtpDataChannel*>(channel));
      break;
    default:
      RTC_NOTREACHED() << "Unknown media type: " << channel->media_type();
      break;
  }
}

cricket::ChannelInterface* MediaClient::GetChannel(const std::string& mid) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", mid: " << mid;
  for (const auto& transceiver : transceivers_) {
    cricket::ChannelInterface* channel = transceiver->internal()->channel();
    if (channel && channel->content_name() == mid) {
      return channel;
    }
  }
  if (rtp_data_channel() && rtp_data_channel()->content_name() == mid) {
    return rtp_data_channel();
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return nullptr;
}

cricket::RtpDataChannel* MediaClient::rtp_data_channel() const {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", rtp_data_channel_: " << rtp_data_channel_;
  return rtp_data_channel_;
}

bool MediaClient::InitRenderWndsManager() {
#if defined WEBRTC_WIN
  if (!renderWndsManager_) {
    renderWndsManager_.reset(new win_render::RenderWndsManager());
  }
  EC_CHECK_VALUE(renderWndsManager_, false);
#endif
  return true;
}

bool MediaClient::SetLocalVideoRenderWindow(int channel_id,
                                            int render_mode,
                                            void* view) {
  API_LOG(INFO) << "channel_id: " << channel_id << ", video_window: " << view;
  EC_CHECK_VALUE(view, false);
  EC_CHECK_VALUE((channel_id >= 0), false);

#if defined WEBRTC_WIN
  if (!renderWndsManager_) {
    InitRenderWndsManager();
  }

  renderWndsManager_->SetLocalRenderWnd(channel_id, render_mode, view,
                                        worker_thread_, nullptr);
#elif defined(WEBRTC_IOS)
  ObjCCallClient::GetInstance()->SetLocalWindowView(view);
#endif
  return true;
}

bool MediaClient::PreviewTrack(int window_id, void* video_track) {
  API_LOG(INFO) << "window_id: " << window_id << ", window_id: " << window_id;
  webrtc::VideoTrackInterface* track =
      (webrtc::VideoTrackInterface*)(video_track);
#if defined WEBRTC_WIN
  EC_CHECK_VALUE(renderWndsManager_, false);
  return renderWndsManager_->StartLocalRenderer(window_id, track);
#elif defined(WEBRTC_IOS)
  ObjCCallClient::GetInstance()->PreviewTrack(window_id, video_track);
#endif
  track = NULL;
  return true;
}

bool MediaClient::SetRemoteVideoRenderWindow(int channel_Id,
                                             int render_mode,
                                             void* view) {
  API_LOG(INFO) << "channel_id: " << channel_Id << ", video_window: " << view;
  EC_CHECK_VALUE((channel_Id >= 0), false);
#if defined WEBRTC_WIN
  EC_CHECK_VALUE(view, false);

  if (!renderWndsManager_) {
    InitRenderWndsManager();
  }

  renderWndsManager_->AddRemoteRenderWnd(channel_Id, render_mode, view, worker_thread_,
                                         nullptr);
#elif defined(WEBRTC_IOS)
  ObjCCallClient::GetInstance()->SetRemoteWindowView(channel_Id, view);
#endif
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end...";
  return true;
}

bool MediaClient::ReleaseChannelId(int channelId) {
  API_LOG(INFO) << "channel_id : " << channelId;
  EC_CHECK_VALUE((channelId >= 0), false);
  std::map<int, ChannelSsrcs>::iterator it;
  it = mapChannelSsrcs_.find(channelId);
  if (it != mapChannelSsrcs_.end()) {
    if (channelGenerator_) {
      channelGenerator_->ReturnId(channelId);
    }
    it->second.ssrcLocal.clear();
    it->second.ssrcRemote.clear();
    mapChannelSsrcs_.erase(it);
    return true;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return false;
}

bool MediaClient::AddMediaSsrc(bool is_local, int channelId, uint32_t ssrc) {
  API_LOG(INFO) << "is_local: " << is_local << ", channelId: " << channelId
                << ", ssrc: " << ssrc;
  EC_CHECK_VALUE((channelId >= 0), false);

  std::map<int, ChannelSsrcs>::iterator it;

  it = mapChannelSsrcs_.find(channelId);
  if (it != mapChannelSsrcs_.end()) {
    if (is_local) {
      it->second.ssrcLocal.push_back(ssrc);

    } else {
      uint8_t buf[4] = {0};
      uint32_t temp = ssrc;
      buf[0] = temp & 0xF0;
      buf[1] = temp >> 8;
      buf[2] = temp >> 16;
      buf[3] = temp >> 24;
      uint32_t ssrcRemote =
          buf[0] + buf[1] * 256 + buf[2] * 256 * 256 + buf[3] * 256 * 256 * 256;
      it->second.ssrcRemote.push_back(ssrcRemote);
    }
    return true;
  }

  return false;
}

bool MediaClient::GetMediaSsrc(bool is_local,
                               int channelId,
                               std::vector<uint32_t>& ssrcs) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", is_local: " << is_local << ", channelId: " << channelId;
  //  EC_CHECK_VALUE(ssrc, false);
  EC_CHECK_VALUE((channelId >= 0), false);

  std::map<int, ChannelSsrcs>::iterator it;
  it = mapChannelSsrcs_.find(channelId);
  if (it != mapChannelSsrcs_.end()) {
    if (is_local) {
      ssrcs = it->second.ssrcLocal;

    } else {
      ssrcs = it->second.ssrcRemote;
    }
    return true;
  }

  return false;
}

std::string MediaClient::GetMidFromChannelId(int channelId) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channelId: " << channelId;
  std::string mid;
  EC_CHECK_VALUE((channelId >= 0), mid);

  std::map<int, ChannelSsrcs>::iterator it;
  it = mapChannelSsrcs_.find(channelId);
  if (it != mapChannelSsrcs_.end()) {
    mid = it->second.mid;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... "
                << ", return mid: " << mid;
  return mid;
}

bool MediaClient::FilterAudioCodec(const AudioCodecConfig& config,
                                   std::vector<cricket::AudioCodec>& vec) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  std::string name;
  std::string cname = config.codecName;
  absl::AsciiStrToUpper(&cname);
  std::vector<cricket::AudioCodec>::iterator it = vec.begin();
  while (it != vec.end()) {
    name = it->name;
    absl::AsciiStrToUpper(&name);
    if (name.compare(cname) == 0 && it->clockrate == config.clockRate) {
      it->id = config.payloadType;
      it++;
    } else {
      it = vec.erase(it);
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return vec.size() > 0;
}

bool isFirst = true;
bool MediaClient::FilterVideoCodec(const VideoCodecConfig& config,
                                   std::vector<cricket::VideoCodec>& vec) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  std::string name;
  std::string cname = config.codecName;
  absl::AsciiStrToLower(&cname);
  std::vector<cricket::VideoCodec>::iterator it = vec.begin();
  cricket::VideoCodec vc;
  bool find_codec = false;
  bool found_rtx = false;
  int out = -1;
  while (it != vec.end()) {
    name = it->name;
    absl::AsciiStrToLower(&name);
    if (config.red && name.compare(cricket::kRedCodecName) == 0) {
      RTC_LOG(INFO) << __FUNCTION__ << " red name: " << it->name
                    << " id: " << it->id;
      it++;
    } else if ((webrtc::FecMechanism)config.fecType ==
                   webrtc::FecMechanism::RED_AND_ULPFEC &&
               name.compare(cricket::kUlpfecCodecName) == 0) {
      RTC_LOG(INFO) << __FUNCTION__ << " fec name: " << it->name
                    << " id: " << it->id;
      it++;
    } else if (name.compare(cname) == 0 && !find_codec) {
      it->GetParam("packetization-mode", &out);
      if (out == 1) {
        RTC_LOG(INFO) << __FUNCTION__ << " found codec. name: " << it->name
                      << " id: " << it->id
                      << " payloadTYpe: " << config.payloadType;
        it->id = config.payloadType;
        find_codec = true;
        it++;
      } else {
        RTC_LOG(INFO) << __FUNCTION__ << " deleted found. name: " << it->name
                      << " id: " << it->id;
        it = vec.erase(it);
      }
    } else if (name.compare("rtx") == 0 &&
               /*it->id == config.rtxPayload*/ config.rtx == 1 && !found_rtx) {
      it->SetParam(cricket::kCodecParamAssociatedPayloadType,
                   config.payloadType);
      RTC_LOG(INFO) << __FUNCTION__ << " rtx name: " << it->name
                    << " id: " << it->id
                    << " payloadTYpe: " << config.payloadType;
      it->id = config.rtxPayload;
      found_rtx = true;
      it++;
    } else {
      RTC_LOG(INFO) << __FUNCTION__ << " delete name: " << it->name
                    << " id: " << it->id;
      it = vec.erase(it);
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << " print selected item begin ";
  it = vec.begin();
  while (it != vec.end()) {
    RTC_LOG(INFO) << __FUNCTION__ << " selected item. name: " << it->name
                  << " id: " << it->id;
    it++;
  }
  RTC_LOG(INFO) << __FUNCTION__ << " print selected item end";
  /* if (!find_codec) {
     vc.id = config.payloadType;
     vc.name = config.codecName;
     vec.insert(vec.begin(), vc);
   }*/
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return vec.size() > 0;
}

bool MediaClient::FilterVideoCodec(std::vector<VideoStreamConfig>* configs,
                                   std::vector<cricket::VideoCodec>& vec) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  if (!configs || configs->size() == 0)
    return false;
  auto it_vec = vec.begin();
  while (it_vec != vec.end()) {
    auto it = configs->begin();
    while (it != configs->end()) {
      if (it->name.compare(it_vec->name) == 0 && it->payloadType == it_vec->id)
        break;
      it++;
    }
    if (it == configs->end())
      it_vec = vec.erase(it_vec);
    else {
      it_vec->id = it->new_payloadType;
      if (it_vec->GetCodecType() == cricket::VideoCodec::CODEC_RTX) {
        it_vec->SetParam(cricket::kCodecParamAssociatedPayloadType,
                         it->associated_payloadType);
      }
      it_vec++;
    }
  }
  return vec.size() > 0;
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return true;
}

uint32_t MediaClient::GetNumberOfVideoDevices() {
  int num = 0;
#if defined(WEBRTC_IOS)
  num = ObjCCallClient::GetInstance()->GetNumberOfVideoDevices();
#endif
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (info) {
    num = info->NumberOfDevices();
  }
  API_LOG(INFO) << "camera_nums: " << num;
  return num;
}

bool MediaClient::GetVideoDevices(char* jsonDeviceInfos, int* length) {
  API_LOG(INFO) << "devices: " << jsonDeviceInfos << ", len: " << length;
  if (length) {
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
#if defined(WEBRTC_IOS)
    info.reset(ObjCCallClient::GetInstance()->getVideoCaptureDeviceInfo());
#endif
    if (info) {
      uint32_t num_devices = info->NumberOfDevices();
      const int bufLen = 300;
      char deviceId[bufLen] = {0};
      char deviceName[bufLen] = {0};
      char productId[bufLen] = {0};

      Json::Value devices(Json::objectValue);
      webrtc::VideoCaptureCapability capability;

      for (uint32_t i = 0; i < num_devices; i++) {
        memset(deviceId, 0, bufLen);
        memset(deviceName, 0, bufLen);
        memset(productId, 0, bufLen);

        info->GetDeviceName(i, deviceName, bufLen, deviceId, bufLen, productId,
                            bufLen);

        Json::Value device(Json::objectValue);
        device["deviceIndex"] = i;
        device["deviceId"] = deviceId;
        device["deviceName"] = deviceName;

        uint32_t num_capabilities = info->NumberOfCapabilities(deviceId);
        for (uint32_t j = 0; j < num_capabilities; j++) {
          info->GetCapability(deviceId, j, capability);

          Json::Value cap(Json::objectValue);
          cap["capabilityIndex"] = j;
          cap["width"] = capability.width;
          cap["height"] = capability.height;
          cap["maxFPS"] = capability.maxFPS;
          device["capabilities"].append(cap);
        }
        devices["devices"].append(device);
      }

      std::string strDevices = devices.toStyledString();
      int len = strDevices.length();
      if (len > *length) {
        *length = len;
        return false;
      } else if (jsonDeviceInfos) {
        std::memset(jsonDeviceInfos, 0, *length);
        std::memcpy(jsonDeviceInfos, strDevices.c_str(), len);
        return true;
      }
    }
  }
  return false;
}

bool MediaClient::GetVideoCodecs(char* jsonVideoCodecInfos, int* length) {
  API_LOG(INFO) << "jsonVideoCodecInfos begin...";

  if (channel_manager_ && length) {
    cricket::VideoSendParameters codecParams;
    Json::Value codecs(Json::objectValue);
    channel_manager_->GetSupportedVideoCodecs(&codecParams.codecs);

    for (auto codec : codecParams.codecs) {
      Json::Value jsonCodec(Json::objectValue);
      jsonCodec["codecName"] = codec.name;
      jsonCodec["payloadType"] = codec.id;
      codecs["codecs"].append(jsonCodec);
    }
    std::string strCodecs = codecs.toStyledString();
    int len = strCodecs.length();
    if (len > *length) {
      *length = len;
      API_LOG(INFO) << "jsonVideoCodecInfos end... len:" << jsonVideoCodecInfos
                    << " bigger than length:" << *length;
      return false;
    } else if (jsonVideoCodecInfos) {
      std::memset(jsonVideoCodecInfos, 0, *length);
      std::memcpy(jsonVideoCodecInfos, strCodecs.c_str(), len);
      API_LOG(INFO) << "jsonVideoCodecInfos end... " << jsonVideoCodecInfos
                    << ", length: " << *length;
      return true;
    }
  }

  API_LOG(INFO) << "jsonVideoCodecInfos end...";
  return false;
}

bool MediaClient::GetAudioCodecs(char* jsonAudioCodecInfos, int* length) {
  API_LOG(INFO) << "jsonAudioCodecInfos begin...";

  if (channel_manager_ && length) {
    Json::Value codecs(Json::objectValue);
    cricket::AudioSendParameters sendParams;
    signaling_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
      channel_manager_->GetSupportedAudioSendCodecs(&sendParams.codecs);
    });

    for (auto codec : sendParams.codecs) {
      Json::Value jsonCodec(Json::objectValue);
      jsonCodec["clockrate"] = codec.clockrate;
      jsonCodec["codecName"] = codec.name;
      jsonCodec["payloadType"] = codec.id;
      codecs["codecs"].append(jsonCodec);
    }

    std::string strCodecs = codecs.toStyledString();
    int len = strCodecs.length();
    if (len > *length) {
      *length = len;
      return false;
    } else if (jsonAudioCodecInfos) {
      std::memset(jsonAudioCodecInfos, 0, *length);
      std::memcpy(jsonAudioCodecInfos, strCodecs.c_str(), len);
      API_LOG(INFO) << "jsonAudioCodecInfos end.... " << jsonAudioCodecInfos
                    << ", length: " << *length;
      return true;
    }
  }
  API_LOG(INFO) << "jsonAudioCodecInfos end...";
  return false;
}

bool MediaClient::ParseVideoDeviceSetting(const char* videoDeviceSettings,
                                          VideoDeviceConfig* config) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", videoDeviceSettings: " << videoDeviceSettings
                << ", config: " << config;
  if (videoDeviceSettings && config) {
    Json::Value settings;
    Json::Reader reader;
    if (reader.parse(videoDeviceSettings, settings)) {
      if (!settings.isNull()) {
        int value = 0;
        std::string token;
        if (rtc::GetIntFromJsonObject(settings, "width", &value)) {
          config->width = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "height", &value)) {
          config->height = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "maxFramerate", &value)) {
          config->maxFramerate = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "deviceIndex", &value)) {
          config->deviceIndex = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "videoSourceType", &value)) {
          config->videoSourceType = value;
        }
        if (rtc::GetStringFromJsonObject(settings, "trackId", &token)) {
          config->trackId = token;
        }
        if (rtc::GetStringFromJsonObject(settings, "transportId", &token)) {
          config->transportId = token;
        }

        return true;
      }
    }
  }
  return false;
}

bool MediaClient::ParseAudioCodecSetting(const char* audioCodecSettings,
                                         AudioCodecConfig* config) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", audioCodecSettings: " << audioCodecSettings
                << ", config: " << config;
  if (audioCodecSettings && config) {
    Json::Value settings;
    Json::Reader reader;
    if (reader.parse(audioCodecSettings, settings)) {
      if (!settings.isNull()) {
        int value = 0;
        std::string token;
        if (rtc::GetIntFromJsonObject(settings, "channels", &value)) {
          config->channels = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "clockRate", &value)) {
          config->clockRate = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "payloadType", &value)) {
          config->payloadType = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "minBitrateKps", &value)) {
          config->minBitrateKps = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "startBitrateKps", &value)) {
          config->startBitrateKps = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "maxBitrateKps", &value)) {
          config->maxBitrateKps = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "codecType", &value)) {
          config->codecType = value;
        }
        if (rtc::GetStringFromJsonObject(settings, "codecName", &token)) {
          config->codecName = token;
        }
        if (rtc::GetStringFromJsonObject(settings, "trackId", &token)) {
          config->trackId = token;
        }
        if (rtc::GetStringFromJsonObject(settings, "transportId", &token)) {
          config->transportId = token;
        }

        return true;
      }
    }
  }
  return false;
}

bool MediaClient::ParseVideoCodecSetting(const char* videoCodecSettings,
                                         VideoCodecConfig* config) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", videoCodecSettings: " << videoCodecSettings
                << ", config: " << config;
  if (videoCodecSettings && config) {
    Json::Value settings;
    Json::Reader reader;
    if (reader.parse(videoCodecSettings, settings)) {
      if (!settings.isNull()) {
        int value = 0;
        bool enable = false;
        std::string token;
        if (rtc::GetIntFromJsonObject(settings, "fecType", &value)) {
          config->fecType = value;
        }
        if (rtc::GetBoolFromJsonObject(settings, "nack", &enable)) {
          config->nack = enable;
        }
        if (rtc::GetBoolFromJsonObject(settings, "red", &enable)) {
          config->red = enable;
        }
        if (rtc::GetBoolFromJsonObject(settings, "isScreenShare", &enable)) {
          config->isScreenShare = enable;
        }
        if (rtc::GetIntFromJsonObject(settings, "payloadType", &value)) {
          config->payloadType = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "rtxPayload", &value)) {
          config->rtxPayload = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "fecPayload", &value)) {
          config->fecPayload = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "width", &value)) {
          config->width = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "height", &value)) {
          config->height = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "minBitrateKps", &value)) {
          config->minBitrateKps = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "startBitrateKps", &value)) {
          config->startBitrateKps = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "maxBitrateKps", &value)) {
          config->maxBitrateKps = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "maxFramerate", &value)) {
          config->maxFramerate = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "codecType", &value)) {
          config->codecType = value;
        }
        if (rtc::GetIntFromJsonObject(settings, "maxQp", &value)) {
          config->maxQp = value;
        }
        if (rtc::GetStringFromJsonObject(settings, "codecName", &token)) {
          config->codecName = token;
        }
        if (rtc::GetStringFromJsonObject(settings, "trackId", &token)) {
          config->trackId = token;
        }
        if (rtc::GetStringFromJsonObject(settings, "transportId", &token)) {
          config->transportId = token;
        }

        const Json::Value arrayObj = settings["codecs"];
        for (unsigned int i = 0; i < arrayObj.size(); i++) {
          VideoStreamConfig stream_config;
          int value = 0;
          std::string name;
          if (rtc::GetStringFromJsonObject(arrayObj[i], "codecName", &name)) {
            stream_config.name = name;
          }
          if (rtc::GetIntFromJsonObject(arrayObj[i], "payloadType", &value)) {
            stream_config.payloadType = value;
          }
          if (rtc::GetIntFromJsonObject(arrayObj[i], "new_payloadType",
                                        &value)) {
            stream_config.new_payloadType = value;
          }
          if (rtc::GetIntFromJsonObject(arrayObj[i], "associated_payloadType",
                                        &value)) {
            stream_config.associated_payloadType = value;
          }
          config->video_stream_configs.push_back(stream_config);
        }

        return true;
      }
    }
  }
  return false;
}

bool MediaClient::ParseVideoCodecSetting(
    const char* videoCodecSettings,
    std::vector<VideoStreamConfig>* configs) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", videoCodecSettings: " << videoCodecSettings
                << ", config: " << configs;

  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(videoCodecSettings, root)) {
    return -1;
  }

  return true;
}

bool MediaClient::GetStringJsonString(const char* json,
                                      const std::string& key,
                                      std::string* value) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", json: " << json << ", key: " << key
                << ", value: " << value;
  if (json && !key.empty() && value) {
    Json::Reader reader;
    Json::Value jsonValue;
    if (reader.parse(json, jsonValue)) {
      if (!jsonValue.isNull()) {
        std::string token;
        if (rtc::GetStringFromJsonObject(jsonValue, key, &token)) {
          *value = token;
        }

        return true;
      }
    }
  }
  return false;
}

bool MediaClient::GetIntJsonString(const char* json,
                                   const std::string& key,
                                   int* value) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", json: " << json << ", key: " << key
                << ", value: " << value;
  if (json && !key.empty() && value) {
    Json::Reader reader;
    Json::Value jsonValue;
    if (reader.parse(json, jsonValue)) {
      if (!jsonValue.isNull()) {
        int token = 0;

        if (rtc::GetIntFromJsonObject(jsonValue, key, &token)) {
          *value = token;
        }

        return true;
      }
    }
  }
  return false;
}

cricket::WebRtcVideoChannel* MediaClient::GetInternalVideoChannel(
    const int channelId) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channelId: " << channelId;
  if (mVideoChannels_[channelId]) {
    cricket::VideoChannel* videoChannel = mVideoChannels_[channelId];
    cricket::VideoMediaChannel* media_channel = videoChannel->media_channel();
    // downcast from VideoMediaChannel to WebrtcVideoChannel
    cricket::WebRtcVideoChannel* internal_video_channel =
        static_cast<cricket::WebRtcVideoChannel*>(media_channel);
    RTC_LOG(INFO) << __FUNCTION__ << "(),"
                  << " end... "
                  << ", internal_video_channel: " << internal_video_channel;
    return internal_video_channel;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... nullptr ";
  return nullptr;
}

bool MediaClient::SetVideoNackStatus(const int channelId,
                                     const bool enable_nack) {
  API_LOG(INFO) << "channelId: " << channelId
                << ", enable_nack: " << enable_nack;
  cricket::WebRtcVideoChannel* internal_video_channel =
      GetInternalVideoChannel(channelId);

  if (internal_video_channel) {
    // TODO: thread_checker检查
    cricket::VideoSendParameters send_params =
        internal_video_channel->GetSendParameters();

    cricket::VideoCodec send_codec;
    if (!internal_video_channel->GetSendCodec(&send_codec))
      return false;

    if (enable_nack && !cricket::HasNack(send_codec))
      send_codec.feedback_params.Add(cricket::FeedbackParam(
          cricket::kRtcpFbParamNack, cricket::kParamValueEmpty));
    else if (!enable_nack && cricket::HasNack(send_codec))
      send_codec.feedback_params.Remove(cricket::FeedbackParam(
          cricket::kRtcpFbParamNack, cricket::kParamValueEmpty));
    else
      return true;

    send_params.codecs[0] = send_codec;
    return internal_video_channel->SetSendParameters(send_params);
  }

  return false;
}
bool MediaClient::SetVideoUlpFecStatus(const int channelId,
                                       const bool enable,
                                       const uint8_t payloadtype_red,
                                       const uint8_t payloadtype_fec) {
  API_LOG(INFO) << "channelId: " << channelId << ", enable: " << enable
                << ", payloadtype_red: " << payloadtype_red
                << ", payloadtype_fec: " << payloadtype_fec;
  cricket::WebRtcVideoChannel* internal_video_channel =
      GetInternalVideoChannel(channelId);
  if (internal_video_channel) {
    // TODO: thread_checker检查
    cricket::VideoSendParameters send_params =
        internal_video_channel->GetSendParameters();

    cricket::VideoCodec send_codec;
    if (!internal_video_channel->GetSendCodec(&send_codec))
      return false;

    if (enable) {
      InsertVideoCodec(send_params.codecs, cricket::kRedCodecName,
                       payloadtype_red);
      InsertVideoCodec(send_params.codecs, cricket::kUlpfecCodecName,
                       payloadtype_fec);
    } else {
      RemoveVideoCodec(send_params.codecs, cricket::kRedCodecName);
      RemoveVideoCodec(send_params.codecs, cricket::kUlpfecCodecName);
    }
    internal_video_channel->SetSendParameters(send_params);
  }
  return false;
}

bool MediaClient::SetVideoDegradationMode(
    const int channelId,
    const webrtc::DegradationPreference mode) {
  API_LOG(INFO) << "channelId: " << channelId << ", mode: " << mode;
  cricket::WebRtcVideoChannel* internal_video_channel =
      GetInternalVideoChannel(channelId);
  if (internal_video_channel) {
    std::vector<uint32_t> local_ssrcs;
    if (!GetMediaSsrc(true, channelId, local_ssrcs))
      return false;

    webrtc::RTCError err;
    for (auto ssrc : local_ssrcs) {
      webrtc::RtpParameters rtp_params =
          internal_video_channel->GetRtpSendParameters(ssrc);

      rtp_params.degradation_preference = mode;
      err = internal_video_channel->SetRtpSendParameters(ssrc, rtp_params);
      if (err.type() != webrtc::RTCErrorType::NONE) {
        return false;
      }
    }
    return err.ok();
  }
  return false;
}

bool MediaClient::SendKeyFrame(const int channelId) {
  API_LOG(INFO) << "channelId: " << channelId;
  cricket::WebRtcVideoChannel* internal_video_channel =
      GetInternalVideoChannel(channelId);
  if (internal_video_channel) {
    worker_thread_->Invoke<bool>(RTC_FROM_HERE, [=] {
      RTC_DCHECK_RUN_ON(worker_thread_);
      return internal_video_channel->SendKeyframe();
    });
  }
  return true;
}

bool MediaClient::SetKeyFrameRequestCallback(const int channelId,
                                             OnRequestKeyFrameCallback cb) {
  API_LOG(INFO) << "channelId: " << channelId << ", cb: " << (void*)cb;
  cricket::WebRtcVideoChannel* internal_video_channel =
      GetInternalVideoChannel(channelId);
  if (internal_video_channel) {
    internal_video_channel->SetRequestKeyframeCallback(channelId, cb);
  }
  return false;
}

bool MediaClient::InsertVideoCodec(cricket::VideoCodecs& input_codecs,
                                   const std::string& codec_name,
                                   uint8_t payload_type) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  auto it = input_codecs.begin();
  for (; it != input_codecs.end(); it++) {
    if ((*it).name == codec_name) {
      (*it).id = payload_type;
      return true;
    }
  }
  webrtc::SdpVideoFormat sdp(codec_name);
  cricket::VideoCodec codec(sdp);
  codec.id = payload_type;
  input_codecs.insert(input_codecs.end(), codec);
  return true;
}

bool MediaClient::RemoveVideoCodec(cricket::VideoCodecs& input_codecs,
                                   const std::string& codec_name) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  auto it = input_codecs.begin();
  for (; it != input_codecs.end();) {
    if ((*it).name == codec_name) {
      it = input_codecs.erase(it);
    } else {
      it++;
    }
  }
  return true;
}

bool MediaClient::SetAEC(bool enable) {
  API_LOG(INFO) << "enable: " << enable;
  audio_options_.echo_cancellation = enable;
  EC_CHECK_VALUE(audio_options_.echo_cancellation, true);
  /*channel_manager_->media_engine()
      ->voice()
      .GetAudioState()
      ->audio_processing()
      ->ApplyConfig(Config)*/

  return true;
}
bool MediaClient::SetAGC(bool enable) {
  API_LOG(INFO) << "enable: " << enable;
  audio_options_.auto_gain_control = enable;
  // bool enabled;
  // enabled=own_adm->BuiltInAECIsAvailable;
  EC_CHECK_VALUE(audio_options_.auto_gain_control, true);
  // return enabled;
  return true;
}
bool MediaClient::SetNS(bool enable) {
  API_LOG(INFO) << "enable: " << enable;
  audio_options_.noise_suppression = enable;
  EC_CHECK_VALUE(audio_options_.noise_suppression, true);
  return true;
}

bool MediaClient::CreateAudioDevice() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  bool bOk = true;
#if defined(WEBRTC_WIN)
  bOk = signaling_thread_->Invoke<bool>(RTC_FROM_HERE, [this] {
    if (own_adm == nullptr) {
      own_adm = webrtc::AudioDeviceModule::Create(
          webrtc::AudioDeviceModule::kPlatformDefaultAudio);
      own_adm->Init();
      return true;
    } else {
      return false;
    }
  });
#endif

  return bOk;
}

bool MediaClient::SetAudioRecordingVolume(uint32_t vol) {
  /*rtc::scoped_refptr<webrtc::AudioDeviceModule> own_adm;
  own_adm = webrtc::AudioDeviceModule::Create(
      webrtc::AudioDeviceModule::kPlatformDefaultAudio);
  assert(own_adm);
  own_adm->Init();
 */
  API_LOG(INFO) << "vol: " << vol;
  CreateAudioDevice();
  EC_CHECK_VALUE((own_adm != nullptr), false);
  bool can_vol = false;
  own_adm->MicrophoneVolumeIsAvailable(&can_vol);
  if (can_vol)
    own_adm->SetMicrophoneVolume(vol);
  else {
    return false;
  }
  own_adm->InitRecording();

  return true;
}

char* MediaClient::GetAudioDeviceList(int* length) {
  API_LOG(INFO) << "len: " << *length;
  CreateAudioDevice();
  // if (json && length) {
  EC_CHECK_VALUE((own_adm != nullptr), NULL);
  const int num = own_adm->RecordingDevices();
  int i = 0;
  if (num <= 0) {
    *length = 0;
    return NULL;
    // return false;
  }
  Json::Value devices(Json::objectValue);
  Json::Value d(Json::objectValue);
  for (i = 0; i < num; i++) {
    char name[webrtc::kAdmMaxDeviceNameSize];
    char guid[webrtc::kAdmMaxGuidSize];
    int ret = own_adm->RecordingDeviceName(i, name, guid);

    EC_CHECK_VALUE((ret != -1), NULL);
    Json::Value device(Json::objectValue);
    device["deviceIndex"] = i;
    device["deviceName"] = name;
    device["deviceGuid"] = guid;
    // d["devices"].append(device);
    d["recordings"].append(device);
  }
  devices["devices"].append(d);
  // std::string strDevice = devices.toStyledString();

  /* if (len > *length) {
     return false;
   } else {
     std::memset(json, 0, *length);
     std::memcpy(json, strDevice.c_str(), len);
     own_adm->InitRecording();
     return true;
   }*/
  // std::memcpy(pAudioDevice,strDevice.c_str(),len);
  // own_adm->InitRecording();
  //  return true;
  const int num2 = own_adm->PlayoutDevices();
  if (num2 <= 0) {
    return NULL;
  }
  Json::Value playdevices(Json::objectValue);
  for (i = 0; i < num2; i++) {
    char name[webrtc::kAdmMaxDeviceNameSize];
    char guid[webrtc::kAdmMaxGuidSize];
    int ret = own_adm->PlayoutDeviceName(i, name, guid);

    EC_CHECK_VALUE((ret != -1), NULL);
    Json::Value device(Json::objectValue);
    device["deviceIndex"] = i;
    device["deviceName"] = name;
    device["deviceGuid"] = guid;
    playdevices["playouts"].append(device);
  }
  devices["devices"].append(playdevices);
  std::string strDevice = devices.toStyledString();
  // std::string strDev = strDevice + strDevice2;
  int len = strDevice.length();
  if (pAudioDevice != nullptr) {
    delete[] pAudioDevice;
  }
  pAudioDevice = new char[len + 1];
  memset(pAudioDevice, 0, len + 1);
  std::memcpy(pAudioDevice, strDevice.c_str(), len);
  // json =pAudioDevice;
  *length = len + 1;
  RTC_LOG(INFO) << __FUNCTION__ << pAudioDevice << ", length: " << *length;
  return pAudioDevice;
  //   int len2 = strDevice2.length();

  // }
  return NULL;
}

bool MediaClient::SetAudioRecordingDevice(int index) {
  API_LOG(INFO) << "index: " << index;
  CreateAudioDevice();
  EC_CHECK_VALUE((own_adm != nullptr), false);
  int num_devices = own_adm->RecordingDevices();
  if (audio_layer_ == webrtc::AudioDeviceModule::kWindowsCoreAudio2) {
    num_devices += 2;
  }
  EC_CHECK_VALUE((num_devices != 0), false);
  // Verify that all available recording devices can be set (not enabled yet).
  for (int j = 0; j < num_devices; ++j) {
    int ret = own_adm->SetRecordingDevice(j);
    EC_CHECK_VALUE((ret != -1), false);
  }
#ifdef WEBRTC_WIN
  // On Windows, verify the alternative method where the user can select device
  // by role.
/*  EC_CHECK_VALUE(
      (own_adm->SetRecordingDevice(webrtc::AudioDeviceModule::kDefaultDevice)),
      false);
  EC_CHECK_VALUE((own_adm->SetRecordingDevice(
                     webrtc::AudioDeviceModule::kDefaultCommunicationDevice)),
                 false);*/
#endif
  if (!(own_adm->SetRecordingDevice(index))) {
    own_adm->InitRecording();
    return true;
  }

  else
    return false;
}

bool MediaClient::SetAudioRecordingDeviceOnFlight(int i) {
  rtc::scoped_refptr<webrtc::AudioState> audio_state =
      channel_manager_->media_engine()->voice().GetAudioState();
  if (audio_state->SetRecordingDevice(i) == -1)
    return false;
  return true;
}

bool MediaClient::SetAudioPlayoutDeviceOnFlight(int i) {
  rtc::scoped_refptr<webrtc::AudioState> audio_state =
      channel_manager_->media_engine()->voice().GetAudioState();
  if (audio_state->SetPlayoutDevice(i) == -1)
    return false;
  return true;
}

bool MediaClient::SetAudioPlayoutDevice(int index) {
#ifdef WEBRTC_WIN
  API_LOG(INFO) << "index: " << index;
  CreateAudioDevice();
  EC_CHECK_VALUE((own_adm != nullptr), false);
  int num_devices = own_adm->PlayoutDevices();
  EC_CHECK_VALUE((num_devices != 0), false);
  // Verify that all available recording devices can be set (not enabled yet).
  for (int j = 0; j < num_devices; ++j) {
    int ret = own_adm->SetPlayoutDevice(j);
    EC_CHECK_VALUE((ret != -1), false);
  }

  if (!(own_adm->SetPlayoutDevice(index))) {
    own_adm->InitPlayout();
    return true;
  } else
    return false;
#else
  return true;
#endif
}

int MediaClient::GetCaptureDevice(int index,
                                  char* device_name,
                                  int name_len,
                                  char* unique_name,
                                  int id_len) {
  RTC_DCHECK(vcm_device_info_);
  API_LOG(INFO) << "index: " << index << ", name: " << device_name
                << ", name_len: " << name_len << ", id: " << unique_name
                << ", id_len: " << id_len;
  return vcm_device_info_->GetDeviceName(index, device_name, name_len,
                                         unique_name, id_len);
}

int MediaClient::NumOfCapabilities(const char* id) {
  RTC_DCHECK(vcm_device_info_);

  int num = vcm_device_info_->NumberOfCapabilities(id);
  API_LOG(INFO) << "id: " << id;
  return num;
}

int MediaClient::GetCaptureCapabilities(const char* id,
                                        int index,
                                        webrtc::VideoCaptureCapability& cap) {
  RTC_DCHECK(vcm_device_info_);
  return vcm_device_info_->GetCapability(id, index, cap);
}

int MediaClient::AllocateCaptureDevice(const char* id, int len, int& deviceid) {
  API_LOG(INFO) << "id: " << id << ", len: " << len
                << ", deviceid: " << deviceid;
#ifdef WEBRTC_WIN
  auto it = std::find_if(camera_devices_.begin(), camera_devices_.end(),
                         FindUniqueId(std::string(id)));

  if (it != camera_devices_.end()) {
    deviceid = it->first;
  } else {
    VideoCapturer* video_device = VideoCapturer::CreateCaptureDevice(id, len);
    if (!video_device) {
      deviceid = -1;
      return -1;
    };
    deviceid = camera_devices_.size();
    camera_devices_[deviceid] = std::make_pair(std::string(id), video_device);
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
#endif
  return 0;
}

int MediaClient::ConnectCaptureDevice(int deviceid, int peer_id) {
  API_LOG(INFO) << "deviceid: " << deviceid << ", peer_id " << peer_id;
  RTC_DCHECK_GE(deviceid, 0);
  // TODO
  return -1;
}

int MediaClient::StartCameraCapturer(int deviceid,
                                     webrtc::VideoCaptureCapability& cap) {
  API_LOG(INFO) << "deviceid: " << deviceid << ", width: " << cap.width
                << ", height: " << cap.height << ", maxfps: " << cap.maxFPS;
  // RTC_DCHECK(signaling_thread_->IsCurrent());
  RTC_DCHECK_GE(deviceid, 0);
#if defined(WEBRTC_IOS)
  ObjCCallClient::GetInstance()->StartCapture(deviceid, cap);
  return 0;
#elif defined(WEBRTC_WIN)
  return camera_devices_[deviceid].second->StartCapture(cap);
#endif
  return 0;
}

// note: must call from ui thread
int MediaClient::StopCapturer(int deviceid) {
  API_LOG(INFO) << "deviceid: " << deviceid;
  // RTC_DCHECK_GE(deviceid, 0);
#if defined(WEBRTC_IOS)
  ObjCCallClient::GetInstance()->StopCapture();
  return 0;
#elif defined(WEBRTC_WIN)
  if (camera_devices_.empty()) {
    return 0;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return camera_devices_[deviceid].second->StopCapture();
#endif
  return 0;
}

int MediaClient::StopAllCapturer() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  int ret = 0;
#if defined(WEBRTC_WIN)
  if (camera_devices_.empty()) {
    return 0;
  }

  for (auto camera : camera_devices_) {
    ret = camera.second.second->StopCapture();
  }
#endif

  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return ret;
}

bool MediaClient::StartConnectChannel(int audio_channel_id,
                                      int video_channel_id) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", audio_channel_id: " << audio_channel_id
                << ", video_channel_id: " << video_channel_id;
  bool bOk = false;
  //  MediaClient::GetInstance()->StartConnect(audio_channel_id,
  //  video_channel_id);
  return true;
  /* if (m_pMediaClient) {
    bOk = m_pMediaClient->StartConnect(audio_channel_id, video_channel_id);
   }*/
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " end... ";
  return bOk;
}

int MediaClient::StopLocalRender(int peer_id, int deviceid) {
  return -1;
}

int MediaClient::StopRemoteRender(int peer_id, int deviceid) {
  return -1;
}

int MediaClient::StartMicCapture(int peer_id) {
  return -1;
}

void MediaClient::GetAudioCodecs(cricket::AudioCodecs* audio_codecs) const {}

void MediaClient::GetVideoCodecs(cricket::VideoCodecs* video_codecs) const {}

void MediaClient::SetSendCodecVideo(cricket::VideoCodec* video_codec) {}

void MediaClient::SetReceiveCodecVideo(int peer_id,
                                       cricket::VideoCodec* video_codec) {}

void MediaClient::SetSendCodecAudio(cricket::AudioCodec* audio_codec) {}

void MediaClient::SetReceiveCodecAudio(int peer_id,
                                       cricket::AudioCodec* audio_codec) {}

/***************************A/V send recerve************************/
int MediaClient::StartSendRecv(int peer_id) {
  return 0;
}

int MediaClient::AudioStartReceive(int peer_id) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", peer_id: " << peer_id;
  // RTC_DCHECK(peer_manager_);
  // peer_manager_->SetLocalAndRemoteDescription();
  return 0;
}

int MediaClient::AudioStartSend(int peer_id) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", peer_id: " << peer_id;
  return 0;
}

int MediaClient::VideoStartReceive(int peer_id) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", peer_id: " << peer_id;
  // peer_manager_->SetLocalAndRemoteDescription();
  return 0;
}

int MediaClient::VideoStartSend(int peer_id) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", peer_id: " << peer_id;

  return 0;
}
// wx
int MediaClient::RegisterConferenceParticipantCallback(
    int channel_id,
    ECMedia_ConferenceParticipantCallback* callback) {
  API_LOG(INFO) << "begin...";
  int ret = -1;
  if (mVoiceChannels_[channel_id] &&
      mVoiceChannels_[channel_id]->media_channel()) {
    ret = mVoiceChannels_[channel_id]
              ->media_channel()
              ->Register_ECMedia_ConferenceParticipantCallback(callback);
  } else {
    API_LOG(LS_ERROR) << "Can't find media_voice_channel!";
  }
  return ret;
}

int MediaClient::SetConferenceParticipantCallbackTimeInterVal(
    int channel_id,
    int timeInterVal) {
  int ret = -1;
  if (mVoiceChannels_[channel_id] &&
      mVoiceChannels_[channel_id]->media_channel()) {
    ret = mVoiceChannels_[channel_id]
              ->media_channel()
              ->SetConferenceParticipantCallbackTimeInterVal(timeInterVal);
  }
  return ret;
}
// android interface
#if defined(WEBRTC_ANDROID)
bool MediaClient::SaveLocalVideoTrack(int channelId,
                                      webrtc::VideoTrackInterface* track) {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<bool>(
        RTC_FROM_HERE, [&]() { return SaveLocalVideoTrack(channelId, track); });
  }
  bool bOk = false;
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " begin..."
                << " channelId:" << channelId << " track: " << track;
  if (channelId >= 0 && track) {
    mapLocalVideoTracks[channelId] = track;
    bOk = true;
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end..."
                << " bOk: " << bOk;
  return bOk;
}

webrtc::VideoTrackInterface* MediaClient::GetLocalVideoTrack(int channelId) {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<webrtc::VideoTrackInterface*>(
        RTC_FROM_HERE, [&]() { return GetLocalVideoTrack(channelId); });
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " begin..."
                << " channelId:" << channelId;
  webrtc::VideoTrackInterface* track = nullptr;
  if (mapLocalVideoTracks.size() > 0)
    track = mapLocalVideoTracks.begin()->second;

  if (channelId >= 0) {
    std::map<int, webrtc::VideoTrackInterface*>::iterator it =
        mapLocalVideoTracks.find(channelId);
    if (it != mapLocalVideoTracks.end()) {
      track = it->second;
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end..."
                << " track: " << track;
  return track;
}

bool MediaClient::SaveLocalVideoTrack(int channelId, void* track) {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<bool>(
        RTC_FROM_HERE, [&]() { return SaveLocalVideoTrack(channelId, track); });
  }
  bool bOk = false;
  webrtc::VideoTrackInterface* local_track =
      reinterpret_cast<webrtc::VideoTrackInterface*>(track);
  if (local_track) {
    bOk = SaveLocalVideoTrack(channelId, local_track);
  }
  return bOk;
}

void* MediaClient::GetLocalVideoTrackPtr(int channelId) {
  return reinterpret_cast<void*>(GetLocalVideoTrack(channelId));
}

bool MediaClient::RemoveLocalVideoTrack(int channelId) {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<bool>(
        RTC_FROM_HERE, [&]() { return RemoveLocalVideoTrack(channelId); });
  }
  bool bOk = false;
  if (channelId >= 0) {
    std::map<int, webrtc::VideoTrackInterface*>::iterator it =
        mapLocalVideoTracks.find(channelId);
    if (it != mapLocalVideoTracks.end()) {
      mapLocalVideoTracks.erase(it);
      bOk = true;
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end..."
                << " bOk: " << bOk;
  return bOk;
}

bool MediaClient::SaveRemoteVideoSink(int channelId,
                                      JNIEnv* env,
                                      jobject javaSink) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " begin..."
                << " channelId:" << channelId << " env: " << env
                << " javaSink: " << javaSink;

  bool bOk = false;
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " static_cast env: " << env << " javaSink: " << javaSink;
  if (env && javaSink && channelId >= 0) {
    RTC_LOG(INFO) << __FUNCTION__ << "() "
                  << " JavaToNativeVideoSink before.";
    std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink =
        webrtc::JavaToNativeVideoSink(env, javaSink);
    RTC_LOG(INFO) << __FUNCTION__ << "() "
                  << " JavaToNativeVideoSink after. sink: " << sink;
    if (sink) {
      mapRemoteVideoSinks[channelId] = std::move(sink);
      bOk = true;
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end..."
                << " bOk: " << bOk;

  return bOk;
}

rtc::VideoSinkInterface<webrtc::VideoFrame>* MediaClient::GetRemoteVideoSink(
    int channelId) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " begin..."
                << " channelId:" << channelId;

  rtc::VideoSinkInterface<webrtc::VideoFrame>* sink = nullptr;
  /*if (mapRemoteVideoSinks.size() > 0) {
          sink = mapRemoteVideoSinks.begin()->second.get();
  }
  else*/
  if (channelId >= 0) {
    VideoSinkIterator it = mapRemoteVideoSinks.find(channelId);
    if (it != mapRemoteVideoSinks.end()) {
      sink = it->second.get();
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end..."
                << " sink: " << sink;

  return sink;
}

bool MediaClient::RemoveRemoteVideoSink(int channelId) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " begin..."
                << " channelId:" << channelId;

  bool bOk = false;
  if (channelId >= 0) {
    VideoSinkIterator it = mapRemoteVideoSinks.find(channelId);
    if (it != mapRemoteVideoSinks.end()) {
      mapRemoteVideoSinks.erase(it);
      bOk = true;
    }
  }
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end..."
                << " bOk: " << bOk;

  return bOk;
}

int MediaClient::InitializeJVM() {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " begin...";
  int ret = 0;
  webrtc::JVM::Initialize(webrtc::jni::GetJVM());
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " end..."
                << " ret:" << ret;
  return ret;
}

void* MediaClient::CreateVideoTrack(const char* id,
                                    webrtc::VideoTrackSourceInterface* source) {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<void*>(
        RTC_FROM_HERE, [&]() { return CreateVideoTrack(id, source); });
  }

  RTC_DCHECK(signaling_thread_->IsCurrent());

  rtc::scoped_refptr<webrtc::VideoTrackInterface> track(
      webrtc::VideoTrack::Create(id, source, worker_thread_));

  return track.release();
}

void* MediaClient::CreateAudioTrack(const char* id,
                                    webrtc::AudioSourceInterface* source) {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<void*>(
        RTC_FROM_HERE, [&]() { return CreateAudioTrack(id, source); });
  }

  RTC_DCHECK(signaling_thread_->IsCurrent());
  rtc::scoped_refptr<webrtc::AudioTrackInterface> track(
      webrtc::AudioTrack::Create(id, source));
  return track.release();
}

void* MediaClient::CreateAudioSource() {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<void*>(
        RTC_FROM_HERE, [&]() { return CreateAudioSource(); });
  }

  RTC_DCHECK(signaling_thread_->IsCurrent());

  cricket::AudioOptions options;
  rtc::scoped_refptr<webrtc::LocalAudioSource> source(
      webrtc::LocalAudioSource::Create(&options));
  return source.release();
}

void* MediaClient::CreateVideoSource(JNIEnv* env,
                                     bool is_screencast,
                                     bool align_timestamps) {
  if (!signaling_thread_->IsCurrent()) {
    return signaling_thread_->Invoke<void*>(RTC_FROM_HERE, [&]() {
      return CreateVideoSource(env, is_screencast, align_timestamps);
    });
  }

  rtc::scoped_refptr<webrtc::jni::AndroidVideoTrackSource> source(
      new rtc::RefCountedObject<webrtc::jni::AndroidVideoTrackSource>(
          signaling_thread_, env, is_screencast, align_timestamps));
  return source.release();
}

#endif

///////////////////////////////////TransportControllerObserve/////////////////////////////////
TransportControllerObserve::TransportControllerObserve(
    rtc::Thread* network_thread,
    MediaClient* owner)
    : network_thread_(network_thread), owner_(owner) {}

TransportControllerObserve::~TransportControllerObserve() {}

bool TransportControllerObserve::OnTransportChanged(
    const std::string& mid,
    webrtc::RtpTransportInternal* rtp_transport,
    rtc::scoped_refptr<webrtc::DtlsTransport> dtls_transport,
    webrtc::MediaTransportInterface* media_transport) {
  RTC_DCHECK_RUN_ON(network_thread_);
  EC_CHECK_VALUE(owner_, false);

  bool bOk = true;
  auto base_channel = owner_->GetChannel(mid);
  if (base_channel) {
    bOk = base_channel->SetRtpTransport(rtp_transport);
  }

  return bOk;
}

///////////////////////////////////ChannelGenerator/////////////////////////////////
ChannelGenerator::ChannelGenerator() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  ResetGenerator();
}

ChannelGenerator::~ChannelGenerator() {}

bool ChannelGenerator::GeneratorId(int& id) {
  rtc::CritScope lock(&id_critsect_);
  for (size_t i = kBaseId; i < kMaxId; i++) {
    if (!idBools_[i]) {
      idBools_[i] = true;
      id = i;
      return true;
    }
  }
  return false;
}

void ChannelGenerator::ResetGenerator() {
  rtc::CritScope lock(&id_critsect_);
  memset(idBools_, 0, kMaxId - kBaseId);
}

bool ChannelGenerator::ReturnId(int id) {
  rtc::CritScope lock(&id_critsect_);
  if (id >= kBaseId && id < kMaxId) {
    idBools_[id] = false;
    return true;
  }
  return false;
}

}  // namespace ecmedia_sdk

///////////////////////////////////VideoRenderer/////////////////////////////////

namespace win_render {

#if defined(WEBRTC_WIN)

//VideoRenderer::VideoRenderer(HWND wnd,
//                             int mode,
//                             int width,
//                             int height,
//                             webrtc::VideoTrackInterface* track_to_render)
//    : wnd_(wnd), rendered_track_(track_to_render) {
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << ", wnd: " << wnd << ", width: " << width
//                << ", height: " << height
//                << ", track_to_render: " << track_to_render;
//  mode_ = mode;
//  ::InitializeCriticalSection(&buffer_lock_);
//  ZeroMemory(&bmi_, sizeof(bmi_));
//  bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//  bmi_.bmiHeader.biPlanes = 1;
//  bmi_.bmiHeader.biBitCount = 32;
//  bmi_.bmiHeader.biCompression = BI_RGB;
//  bmi_.bmiHeader.biWidth = width;
//  bmi_.bmiHeader.biHeight = -height;
//  bmi_.bmiHeader.biSizeImage =
//      width * height * (bmi_.bmiHeader.biBitCount >> 3);
//  if (rendered_track_) {
//    rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
//  }
//  if (wnd_) {
//    hDC_ = GetDC((HWND)wnd_);
//  } else {
//    hDC_ = nullptr;
//  }
//}
//
//VideoRenderer::~VideoRenderer() {
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << " begin... ";
//  if (rendered_track_) {
//    rendered_track_->RemoveSink(this);
//  }
//  ::DeleteCriticalSection(&buffer_lock_);
//}
//
//bool VideoRenderer::UpdateVideoTrack(
//    webrtc::VideoTrackInterface* track_to_render) {
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << " track_to_render: " << track_to_render;
//  if (rendered_track_) {
//    rendered_track_->RemoveSink(this);
//  }
//  rendered_track_ = track_to_render;
//  if (rendered_track_) {
//    rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
//    return true;
//  }
//  return false;
//}
//
//void VideoRenderer::SetSize(int width, int height) {
//  // RTC_LOG(INFO) << __FUNCTION__  << "() "<< " begin..."
//  //              << " width:" << width << "height:" << height;
//  AutoLock<VideoRenderer> lock(this);
//
//  if (width == bmi_.bmiHeader.biWidth && height == bmi_.bmiHeader.biHeight) {
//    return;
//  }
//
//  bmi_.bmiHeader.biWidth = width;
//  bmi_.bmiHeader.biHeight = -height;
//  bmi_.bmiHeader.biSizeImage =
//      width * height * (bmi_.bmiHeader.biBitCount >> 3);
//  image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
//}
//
//void VideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame) {
//  // RTC_LOG(INFO) << __FUNCTION__  << "() "<< " begin...";
//  {
//    AutoLock<VideoRenderer> lock(this);
//
//    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
//        video_frame.video_frame_buffer()->ToI420());
//    if (video_frame.rotation() != webrtc::kVideoRotation_0) {
//      buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
//    }
//
//    SetSize(buffer->width(), buffer->height());
//
//    RTC_DCHECK(image_.get() != NULL);
//    //add by ytx_wx begin...
//    bool flag = GetisLocal();
//    if (flag) {
//	  std::unique_ptr<uint8_t[]> mirror_image_;
//      mirror_image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
//      RTC_DCHECK(image_.get() != NULL);
//      libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
//                         buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
//                         mirror_image_.get(),
//                         bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
//                         buffer->width(), buffer->height());
//      libyuv::ARGBMirror(mirror_image_.get(),
//                         bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
//                         image_.get(),
//                         bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
//                         buffer->width(), buffer->height());    
//    }else{
//      libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
//                         buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
//                         image_.get(),
//                         bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
//                         buffer->width(), buffer->height());
//	}     
//	//add by ytx_wx end...
//  }
//
//  Paint();
//}
//
//void VideoRenderer::Paint() {
//  if (image_ != nullptr && handle() != nullptr && hDC_ != nullptr) {
//    int height = abs(bmi_.bmiHeader.biHeight);
//    int width = bmi_.bmiHeader.biWidth;
//
//    RECT rc;
//    ::GetClientRect(handle(), &rc);
//
//    HDC dc_mem = ::CreateCompatibleDC(hDC_);
//    ::SetStretchBltMode(dc_mem, HALFTONE);
//
//    // Set the map mode so that the ratio will be maintained for us.
//    HDC all_dc[] = {hDC_, dc_mem};
//    for (size_t i = 0; i < arraysize(all_dc); ++i) {
//      SetMapMode(all_dc[i], MM_ISOTROPIC);
//      SetWindowExtEx(all_dc[i], width, height, NULL);
//      SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
//    }
//
//    HBITMAP bmp_mem = ::CreateCompatibleBitmap(hDC_, rc.right, rc.bottom);
//    HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);
//
//    POINT logical_area = {rc.right, rc.bottom};
//    DPtoLP(hDC_, &logical_area, 1);
//
//    HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
//    RECT logical_rect = {0, 0, logical_area.x, logical_area.y};
//    ::FillRect(dc_mem, &logical_rect, brush);
//    ::DeleteObject(brush);
//
//    int x = (logical_area.x - width) / 2;
//    int y = (logical_area.y - height) / 2;
//    x = x > 0 ? x : 0;
//    y = y > 0 ? y : 0;
//    /*  RTC_LOG(INFO) << __FUNCTION__ << "()"
//                    << "x:" << x << " y:" << y << "  width:" << width
//                    << " height:" << height << "logica_x:" << logical_area.x
//                    << " logical_area.y: " << logical_area.y;*/
//    if (mode_ == 1) {
//      StretchDIBits(dc_mem, x, y, width, height, 0, 0, width, height,
//                    image_.get(), &bmi_, DIB_RGB_COLORS, SRCCOPY);
//      RTC_LOG(INFO) << "render mode = 1";
//    } else {
//      if (x > 0) {
//        y = x * height / width;
//        x = 0;
//      } else {
//        x = y * width / height;
//        y = 0;
//      }
//      StretchDIBits(dc_mem, 0, 0, logical_area.x, logical_area.y, x, y,
//                    width - x, height - y, image_.get(), &bmi_, DIB_RGB_COLORS,
//                    SRCCOPY);
//    }
//
//    BitBlt(hDC_, 0, 0, logical_area.x, logical_area.y, dc_mem, 0, 0, SRCCOPY);
//
//    ::SelectObject(dc_mem, bmp_old);
//    ::DeleteObject(bmp_mem);
//    ::DeleteDC(dc_mem);
//  }
//}
//=======
//VideoRenderer::VideoRenderer(HWND wnd,
//                             int mode,
//                             int width,
//                             int height,
//                             webrtc::VideoTrackInterface* track_to_render)
//    : wnd_(wnd), rendered_track_(track_to_render) ,clock_(webrtc::Clock::GetRealTimeClock()){
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << ", wnd: " << wnd << ", width: " << width
//                << ", height: " << height
//                << ", track_to_render: " << track_to_render;
//  mode_ = mode;
//  ::InitializeCriticalSection(&buffer_lock_);
//  ZeroMemory(&bmi_, sizeof(bmi_));
//  bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//  bmi_.bmiHeader.biPlanes = 1;
//  bmi_.bmiHeader.biBitCount = 32;
//  bmi_.bmiHeader.biCompression = BI_RGB;
//  bmi_.bmiHeader.biWidth = width;
//  bmi_.bmiHeader.biHeight = -height;
//  bmi_.bmiHeader.biSizeImage =
//      width * height * (bmi_.bmiHeader.biBitCount >> 3);
//  if (rendered_track_) {
//    rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
//  }
//  if (wnd_) {
//    hDC_ = GetDC((HWND)wnd_);
//  } else {
//    hDC_ = nullptr;
//  }
//}
//
//VideoRenderer::~VideoRenderer() {
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << " begin... ";
//  if (rendered_track_) {
//    rendered_track_->RemoveSink(this);
//  }
//  ::DeleteCriticalSection(&buffer_lock_);
//}
//
//bool VideoRenderer::UpdateVideoTrack(
//    webrtc::VideoTrackInterface* track_to_render) {
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << " track_to_render: " << track_to_render;
//  if (rendered_track_) {
//    rendered_track_->RemoveSink(this);
//  }
//  rendered_track_ = track_to_render;
//  if (rendered_track_) {
//    rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
//    return true;
//  }
//  return false;
//}
//
//void VideoRenderer::SetSize(int width, int height) {
//
//  AutoLock<VideoRenderer> lock(this);
//
//  if (width == bmi_.bmiHeader.biWidth && height == bmi_.bmiHeader.biHeight) {
//    return;
//  }
//
//  RTC_LOG(INFO) << __FUNCTION__ << "() "
//                << " begin..."
//                << " width:" << width << " biWidth:" << bmi_.bmiHeader.biWidth
//                << "height:" << height << " biHeight:" << bmi_.bmiHeader.biHeight;
//
//  bmi_.bmiHeader.biWidth = width;
//  bmi_.bmiHeader.biHeight = -height;
//  bmi_.bmiHeader.biSizeImage =
//      width * height * (bmi_.bmiHeader.biBitCount >> 3);
//  image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
//}
//
//void VideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame) {
//  // RTC_LOG(INFO) << __FUNCTION__  << "() "<< " begin...";
//  {
//    AutoLock<VideoRenderer> lock(this);
//
//	
//
//    //if (video_frame.timestamp() > 0) {
//    //  RTC_LOG(INFO) << "hubintest OnFrame 000 ts:" << video_frame.timestamp();
//	//}
//    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
//        video_frame.video_frame_buffer()->ToI420());
//    if (video_frame.rotation() != webrtc::kVideoRotation_0) {
//      buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
//    }
//
//    SetSize(buffer->width(), buffer->height());
//
//	//if (video_frame.timestamp() > 0) {
//    //  RTC_LOG(INFO) << "hubintest OnFrame 111 ts:" << video_frame.timestamp();
//	//}
//    RTC_DCHECK(image_.get() != NULL);
//    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
//                       buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
//                       image_.get(),
//                       bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
//                       buffer->width(), buffer->height());
//  }
//  //if (video_frame.timestamp() > 0) {
//  //  RTC_LOG(INFO) << "hubintest OnFrame 222 ts:" << video_frame.timestamp();
//  //}
//
//  Paint(video_frame.timestamp());
//  //if (video_frame.timestamp() > 0) {
//	//int64_t end_time = clock_->TimeInMilliseconds();
//  //  RTC_LOG(INFO) << "hubintest OnFrame 333 ts:" << video_frame.timestamp() << " diff:" << end_time-start_time;
// // }
//}
//
//void VideoRenderer::Paint(uint32_t ts) {
//
//  if (ts > 0) {
//    RTC_LOG(INFO) << "hubintest OnPaint 000 ts:" << ts;
//  }
//
//  int64_t start_time_0 = clock_->TimeInMilliseconds();
//
//  if (image_ != nullptr && handle() != nullptr && hDC_ != nullptr) {
//    int height = abs(bmi_.bmiHeader.biHeight);
//    int width = bmi_.bmiHeader.biWidth;
//
//    RECT rc;
//    ::GetClientRect(handle(), &rc);
//
//    HDC dc_mem = ::CreateCompatibleDC(hDC_);
//    ::SetStretchBltMode(dc_mem, HALFTONE);
//
//  if (ts > 0) {
//    RTC_LOG(INFO) << "hubintest OnPaint 111 ts:" << ts;
//  }
//
//    // Set the map mode so that the ratio will be maintained for us.
//    HDC all_dc[] = {hDC_, dc_mem};
//    for (size_t i = 0; i < arraysize(all_dc); ++i) {
//      SetMapMode(all_dc[i], MM_ISOTROPIC);
//      SetWindowExtEx(all_dc[i], width, height, NULL);
//      SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
//    }
//
//  if (ts > 0) {
//    RTC_LOG(INFO) << "hubintest OnPaint 222 ts:" << ts;
//  }
//
//    HBITMAP bmp_mem = ::CreateCompatibleBitmap(hDC_, rc.right, rc.bottom);
//    HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);
//
//    POINT logical_area = {rc.right, rc.bottom};
//    DPtoLP(hDC_, &logical_area, 1);
//
//    HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
//    RECT logical_rect = {0, 0, logical_area.x, logical_area.y};
//    ::FillRect(dc_mem, &logical_rect, brush);
//    ::DeleteObject(brush);
//
//
//
//    int x = (logical_area.x - width) / 2;
//    int y = (logical_area.y - height) / 2;
//    x = x > 0 ? x : 0;
//    y = y > 0 ? y : 0;
//
//  if (ts > 0) {
//    RTC_LOG(INFO) << "hubintest OnPaint 333 ts:" << ts << " mode:" << mode_ << "x:" << x << " y:" << y << "  width:" << width << " height:" << height << "logica_x:" << logical_area.x << " logical_area.y: " << logical_area.y;;
//  }
//
//    //RTC_LOG(INFO) << __FUNCTION__ << "()"
//    //              << "x:" << x << " y:" << y << "  width:" << width
//    //              << " height:" << height << "logica_x:" << logical_area.x
//    //              << " logical_area.y: " << logical_area.y;
//    if (mode_ == 1) {
//      StretchDIBits(dc_mem, x, y, width, height, 0, 0, width, height,
//                    image_.get(), &bmi_, DIB_RGB_COLORS, SRCCOPY);
//      //RTC_LOG(INFO) << "render mode = 1";
//    } else {
//      if (x > 0) {
//        y = x * height / width;
//        x = 0;
//      } else {
//        x = y * width / height;
//        y = 0;
//      }
//      StretchDIBits(dc_mem, 0, 0, logical_area.x, logical_area.y, x, y,
//                    width - x, height - y, image_.get(), &bmi_, DIB_RGB_COLORS,
//                    SRCCOPY);
//    }
//
//  if (ts > 0) {
//    RTC_LOG(INFO) << "hubintest OnPaint 444 ts:" << ts;
//  }
//
//    BitBlt(hDC_, 0, 0, logical_area.x, logical_area.y, dc_mem, 0, 0, SRCCOPY);
//
//
//  if (ts > 0) {
//    RTC_LOG(INFO) << "hubintest OnPaint 555 ts:" << ts;
//  }
//
//    ::SelectObject(dc_mem, bmp_old);
//    ::DeleteObject(bmp_mem);
//    ::DeleteDC(dc_mem);
//  }
//
//  if (ts > 0) {
//	int64_t end_time = clock_->TimeInMilliseconds();
//    RTC_LOG(INFO) << "hubintest OnPaint 666 ts:" << ts << " diff:" << end_time-start_time_0;
//  }
//}

/*void VideoRenderer::Paint() {
  // RTC_LOG(INFO) << __FUNCTION__  << "() "<< " begin...";
  if (image_ != nullptr && handle() != nullptr && hDC_ != nullptr) {
    int srcWidth = bmi_.bmiHeader.biWidth;
    int srcHeight = abs(bmi_.bmiHeader.biHeight);

    HBITMAP hBitmap = CreateBitmap(srcWidth, srcHeight, 1, 32, image_.get());
    if (hBitmap) {
      HDC hMemDC = CreateCompatibleDC(hDC_);
      HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

      RECT rectWnd;
      ::GetWindowRect(handle(), &rectWnd);

      int dstWidth = rectWnd.right - rectWnd.left;
      int dstHeight = rectWnd.bottom - rectWnd.top;
      RTC_LOG(INFO) << __FUNCTION__ << "() "<< " dstWidth..." << dstWidth << "
dstHeight:" << dstHeight<<"srcWidth"<<srcWidth<<"srcHeight"<<srcHeight;
      SetStretchBltMode(hDC_, WHITEONBLACK);
      if (dstWidth == srcWidth)
        ::StretchBlt(hDC_, 0, 0, dstWidth, srcHeight, hMemDC, 0, 0, srcWidth,
                   srcHeight, SRCCOPY);
      ::StretchBlt(hDC_, 0, 0, dstWidth, dstHeight, hMemDC, 0, 0, srcWidth,
                   srcHeight, SRCCOPY);

      SelectObject(hMemDC, hOldBitmap);
      DeleteDC(hMemDC);
      DeleteObject(hBitmap);
    }
  }
}*/

///////////////////////////////////RenderWndsManager/////////////////////////////////

RenderWndsManager::RenderWndsManager() {}

RenderWndsManager::~RenderWndsManager() {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... ";
  // localRender_.reset();
  std::map<int, ptr_render>::iterator it = mapLocalRenderWnds.begin();
  while (it != mapLocalRenderWnds.end()) {
    if (it->second != nullptr) {
      it->second.reset(nullptr);
    }
    it++;
  }
  std::map<int, ptr_render>::iterator itR = mapRemoteRenderWnds.begin();
  while (itR != mapRemoteRenderWnds.end()) {
    if (itR->second != nullptr) {
      itR->second.reset(nullptr);
    }
    itR++;
  }
  mapLocalRenderWnds.clear();
  mapRemoteRenderWnds.clear();
}

bool RenderWndsManager::StartLocalRenderer(
    int channelId,
    webrtc::VideoTrackInterface* local_video) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channelId: " << channelId
                << ", local_video: " << local_video << " hubin_render";
  std::map<int, ptr_render>::iterator it = mapLocalRenderWnds.find(channelId);
  if (it != mapLocalRenderWnds.end()) {
    it->second->UpdateVideoTrack(local_video);
    if (local_video)
      return it->second->StartRender();
    else
      return it->second->StopRender();
  }

  return false;
}

bool RenderWndsManager::RemoveLocalRenderer(int channelId) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channelId: " << channelId << " hubin_render";
  std::map<int, ptr_render>::iterator localRender =
      mapLocalRenderWnds.find(channelId);
  if (localRender != mapLocalRenderWnds.end()) {
    if (localRender->second != nullptr) {
      localRender->second.reset(nullptr);
    }
    mapLocalRenderWnds.erase(localRender);
    return true;
  }
  return false;
}


void RenderWndsManager::SetLocalRenderWnd(
    int channelId,
    int render_mode,
    void* winLocal,
    rtc::Thread* worker_thread, 
    webrtc::VideoTrackInterface* track_to_render) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channelId: " << channelId << ", winLocal: " << winLocal
                << ", track_to_render: " << track_to_render << " hubin_render";
  std::map<int, ptr_render>::iterator localRender =
      mapLocalRenderWnds.find(channelId);
  if (localRender != mapLocalRenderWnds.end()) {
    if (localRender->second != nullptr) {
      localRender->second.reset(nullptr);
    }
    mapLocalRenderWnds.erase(localRender);
  }

  ptr_render it;
//<<<<<<< HEAD
//  it.reset(new win_render::VideoRenderer((HWND)winLocal, render_mode, 1, 1,
//                                         track_to_render));
//  
//=======
  it.reset(VideoRenderer::CreateVideoRenderer(winLocal, render_mode, true,
                                              track_to_render, 
                             worker_thread, kRenderWindows));
  mapLocalRenderWnds[channelId] = std::move(it);
}

void RenderWndsManager::AddRemoteRenderWnd(
    int channelId,
    int render_mode,
    void* winRemote,
    rtc::Thread* worker_thread, 
    webrtc::VideoTrackInterface* track_to_render) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channelId: " << channelId << ", winRemote: " << winRemote
                << ", track_to_render: " << track_to_render << " hubin_render";
  std::map<int, ptr_render>::iterator remoteRender =
      mapRemoteRenderWnds.find(channelId);
  if (remoteRender != mapRemoteRenderWnds.end()) {
    if (remoteRender->second != nullptr) {
      remoteRender->second.reset(nullptr);
    }
    mapRemoteRenderWnds.erase(remoteRender);
  }

  ptr_render it;
  it.reset(VideoRenderer::CreateVideoRenderer(
      winRemote, render_mode, false, track_to_render, 
                               worker_thread, kRenderWindows));
  mapRemoteRenderWnds[channelId] = std::move(it);
}
bool RenderWndsManager::StartRemoteRenderer(
    int channelId,
    webrtc::VideoTrackInterface* remote_video) {
  RTC_LOG(INFO) << __FUNCTION__ << "(),"
                << " begin... "
                << ", channelId: " << channelId
                << ", remote_video: " << remote_video << " hubin_render";
  std::map<int, ptr_render>::iterator it = mapRemoteRenderWnds.find(channelId);
  if (it != mapRemoteRenderWnds.end()) {
    it->second->UpdateVideoTrack(remote_video);
	return it->second->StartRender();
  }
  return false;
}

bool RenderWndsManager::UpdateRemoteVideoTrack(
    int channelId,
    webrtc::VideoTrackInterface* track_to_render) {
  RTC_LOG(INFO) << __FUNCTION__ << "(), "
                << " begin... "
                << ", channelId: " << channelId
                << ", track_to_render: " << track_to_render << " hubin_render";
  std::map<int, ptr_render>::iterator it = mapRemoteRenderWnds.find(channelId);
  if (it != mapRemoteRenderWnds.end()) {
    return it->second->UpdateVideoTrack(track_to_render);
  }
  return false;
}

bool RenderWndsManager::RemoveRemoteRenderWnd(int channelId) {
  RTC_LOG(INFO) << __FUNCTION__ << "(), "
                << " begin... "
                << ", channelId: " << channelId << " hubin_render";
  std::map<int, ptr_render>::iterator it = mapRemoteRenderWnds.find(channelId);
  if (it != mapRemoteRenderWnds.end()) {
    if (it->second != nullptr) {
      it->second.reset(nullptr);
    }
    mapRemoteRenderWnds.erase(it);
    return true;
  }
  return false;
}

bool RenderWndsManager::UpdateLocalVideoTrack(
    int channelId,
    webrtc::VideoTrackInterface* track_to_render) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "(), "
                << " begin... "
                << ", channelId: " << channelId
                << ", track_to_render: " << track_to_render << " hubin_render";
  std::map<int, ptr_render>::iterator it = mapLocalRenderWnds.find(channelId);
  if (it != mapLocalRenderWnds.end()) {
    return it->second->UpdateVideoTrack(track_to_render);
  }
  return false;
}

//void* RenderWndsManager::GetRemoteWnd(int channelId) {
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << " begin... "
//                << ", channelId: " << channelId;
//  std::map<int, ptr_render>::iterator it = mapRemoteRenderWnds.find(channelId);
//  if (it != mapRemoteRenderWnds.end()) {
//    return (void*)it->second->handle();
//  }
//  return nullptr;
//}


//std::vector<int> RenderWndsManager::GetAllRemoteChanelIds() {
//  RTC_LOG(INFO) << __FUNCTION__ << "(),"
//                << " begin... ";
//  std::vector<int> vec;
//  std::map<int, ptr_render>::iterator it = mapRemoteRenderWnds.begin();
//  while (it != mapRemoteRenderWnds.end()) {
//    vec.push_back(it->first);
//  }
//  return vec;
//}

#endif

}  // namespace win_render
namespace win_desk {
#if defined(WEBRTC_WIN)
ECDesktopCapture::ECDesktopCapture(
    std::unique_ptr<webrtc::DesktopCapturer> capturer)
    : capturer_(std::move(capturer)) {
  isStartCapture = false;
}

ECDesktopCapture::~ECDesktopCapture() {
  if (capturer_) {
    capturer_.release();
  }
  isStartCapture = false;
}

rtc::scoped_refptr<ECDesktopCapture> ECDesktopCapture::Create(int type) {
  std::unique_ptr<webrtc::DesktopCapturer> capturer = nullptr;
  switch (type) {
    case 0:
      capturer = webrtc::DesktopCapturer::CreateScreenCapturer(
          webrtc::DesktopCaptureOptions::CreateDefault());
      return new rtc::RefCountedObject<ECDesktopCapture>(std::move(capturer));
      break;
    case 1:
      capturer = webrtc::DesktopCapturer::CreateWindowCapturer(
          webrtc::DesktopCaptureOptions::CreateDefault());
      return new rtc::RefCountedObject<ECDesktopCapture>(std::move(capturer));
      break;
    default:
      return nullptr;
      break;
  }
}

bool ECDesktopCapture::is_screencast() const {
  return true;
}
absl::optional<bool> ECDesktopCapture::needs_denoising() const {
  return true;
}

webrtc::MediaSourceInterface::SourceState ECDesktopCapture::state() const {
  return kInitializing;
}

bool ECDesktopCapture::remote() const {
  /*int len = 0;
  const char* ss = NULL;
  ss = GetCaptureSources(len);
  SetSourceID(2);*/
  return true;
}

void ECDesktopCapture::OnCaptureResult(
    webrtc::DesktopCapturer::Result result,
    std::unique_ptr<webrtc::DesktopFrame> desktopframe) {
  if (result != webrtc::DesktopCapturer::Result::SUCCESS)
    return;
  int width = desktopframe->size().width();
  int height = desktopframe->size().height();

  rtc::scoped_refptr<webrtc::I420Buffer> buffer =
      webrtc::I420Buffer::Create(width, height);
  int stride = width;
  uint8_t* yplane = buffer->MutableDataY();
  uint8_t* uplane = buffer->MutableDataU();
  uint8_t* vplane = buffer->MutableDataV();
  libyuv::ConvertToI420(desktopframe->data(), 0, yplane, stride, uplane,
                        (stride + 1) / 2, vplane, (stride + 1) / 2, 0, 0, width,
                        height, width, height, libyuv::kRotate0,
                        libyuv::FOURCC_ARGB);

  webrtc::VideoFrame frame =
      webrtc::VideoFrame(buffer, 0, 0, webrtc::kVideoRotation_0);
  this->OnFrame(frame);
}

const char* ECDesktopCapture::GetCaptureSources(
    webrtc::DesktopCapturer::SourceList& source) {
  capturer_->GetSourceList(&source);
  for (auto it = source.begin(); it != source.end(); it++) {
    sources_id_.push_back(it->id);
  }
  return nullptr;
}

int ECDesktopCapture::SetDesktopSourceID(int index) {
  for (auto it = sources_id_.begin(); it != sources_id_.end(); it++) {
    if ((*it) == index) {
      capturer_->SelectSource(*it);
      return 0;
    }
  }
  return -1;
}

void ECDesktopCapture::Stop() {
  // if (isStartCapture) {
  isStartCapture = false;
  // }
}

void ECDesktopCapture::Start() {
  if (!isStartCapture) {
    isStartCapture = true;
    capturer_->Start(this);
    CaptureFrame();
  }
}

void ECDesktopCapture::OnMessage(rtc::Message* msg) {
  if (msg->message_id == 0 && isStartCapture)
    CaptureFrame();
}
void ECDesktopCapture::CaptureFrame() {
  capturer_->CaptureFrame();
  rtc::Location loc(__FUNCTION__, __FILE__);
  rtc::Thread::Current()->PostDelayed(loc, 60, this, 0);
}
#endif
}  // namespace win_desk
