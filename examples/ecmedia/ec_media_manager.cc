#include "ec_media_manager.h"


#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
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
#include "pc/video_track_source.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "media/engine/webrtc_media_engine.h"

ECMediaManager::ECMediaManager(rtc::Thread* worker_thread,
                               rtc::Thread* network_thread,
                               rtc::Thread* signal_thread)
    : worker_thread_(worker_thread),
      network_thread_(network_thread),
      signaling_thread_(signal_thread) {
          #if defined(WEBRTC_WIN)

    media_engine_ = cricket::WebRtcMediaEngineFactory::Create(nullptr /* default_adm */,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
      nullptr /* audio_processing */);
        
#endif
}

ECMediaManager::~ECMediaManager(){
  worker_thread_->Invoke<void>(RTC_FROM_HERE, [&]{ media_engine_.reset();});
  network_thread_->Invoke<void>(RTC_FROM_HERE, [&] { media_engine_.reset(); });

  signaling_thread_->Invoke<void>(RTC_FROM_HERE,
                                  [&] { media_engine_.reset(); });
}



