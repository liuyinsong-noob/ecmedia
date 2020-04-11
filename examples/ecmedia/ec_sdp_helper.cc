#include "ec_sdp_helper.h"

namespace ecmedia_sdk {

ECSDPHelper::ECSDPHelper() {}

ECSDPHelper::~ECSDPHelper() {}

std::unique_ptr<webrtc::SessionDescriptionInterface>
ECSDPHelper::CreateSessionDescription(webrtc::SdpType type,
                                      const std::string& sdp) {
  return CreateSessionDescription(type, sdp, nullptr);
}

std::unique_ptr<webrtc::SessionDescriptionInterface>
ECSDPHelper::CreateSessionDescription(webrtc::SdpType type,
                                      const std::string& sdp,
                                      webrtc::SdpParseError* error_out) {
  auto jsep_desc = absl::make_unique<webrtc::JsepSessionDescription>(type);
  if (!SdpDeserialize(sdp, jsep_desc.get(), error_out)) {
    return nullptr;
  }
  return std::move(jsep_desc);
}

void ECSDPHelper::SetSDPCname(const std::string& audio,
                              const std::string& video,
                              webrtc::SessionDescriptionInterface* desc) {
  for (cricket::ContentInfos::iterator content =
           desc->description()->contents().begin();
       content != desc->description()->contents().end(); ++content) {
    cricket::MediaContentDescription* media_desc = content->media_description();

    if (media_desc->type() == cricket::MEDIA_TYPE_AUDIO) {
      for (cricket::StreamParamsVec::iterator stream =
               media_desc->mutable_streams().begin();
           stream != media_desc->mutable_streams().end(); stream++) {
        stream->cname = audio;
      }
    } else if (media_desc->type() == cricket::MEDIA_TYPE_VIDEO) {
      for (cricket::StreamParamsVec::iterator stream =
               media_desc->mutable_streams().begin();
           stream != media_desc->mutable_streams().end(); stream++) {
        stream->cname = video;
      }
    }
  }

  return;
}

void ECSDPHelper::GetAudioCodecs(cricket::AudioCodecs* audio_codecs,
                                 webrtc::SessionDescriptionInterface* desc) {
  for (cricket::ContentInfos::iterator content =
           desc->description()->contents().begin();
       content != desc->description()->contents().end(); ++content) {
    cricket::MediaContentDescription* media_desc = content->media_description();

    if (media_desc->type() == cricket::MEDIA_TYPE_AUDIO) {
      cricket::AudioContentDescription* audio_desc = media_desc->as_audio();

	  for (auto codec : audio_desc->codecs()) {
        audio_codecs->push_back(codec);
      }
    }
  }

  return;
}

void ECSDPHelper::GetVideoCodecs(cricket::VideoCodecs* video_codecs,
                                 webrtc::SessionDescriptionInterface* desc) {
  for (cricket::ContentInfos::iterator content =
           desc->description()->contents().begin();
       content != desc->description()->contents().end(); ++content) {
    cricket::MediaContentDescription* media_desc = content->media_description();

    if (media_desc->type() == cricket::MEDIA_TYPE_VIDEO) {
      cricket::VideoContentDescription* video_desc = media_desc->as_video();

      for (auto codec : video_desc->codecs()) {
        video_codecs->push_back(codec);
      }
    }
  }

  return;
}

void ECSDPHelper::SetSDPAVCodec(cricket::AudioCodec* audio_codec,
                                cricket::VideoCodec* video_codec,
                                webrtc::SessionDescriptionInterface* desc) {
  for (cricket::ContentInfos::iterator content =
           desc->description()->contents().begin();
       content != desc->description()->contents().end(); ++content) {
    cricket::MediaContentDescription* media_desc = content->media_description();

    if (media_desc->type() == cricket::MEDIA_TYPE_AUDIO) {
      if (!audio_codec) {
        continue;
      }

      cricket::AudioContentDescription* audio_desc = media_desc->as_audio();
      for (auto iter = audio_desc->codecs().begin();
           iter != audio_desc->codecs().end(); ++iter) {
        if (iter->id != audio_codec->id) {
			//lzm------------
      //    audio_desc->codecs().erase(iter);
      //    audio_desc->codecs().insert(audio_desc->codecs().begin(), *iter);
        }
      }

    } else if (media_desc->type() == cricket::MEDIA_TYPE_VIDEO) {
      if (!video_codec) {
        continue;
      }

      cricket::VideoContentDescription* video_desc = media_desc->as_video();
      for (auto iter = video_desc->codecs().begin();
           iter != video_desc->codecs().end(); ++iter) {
        if (iter->id != video_codec->id) {
			//lzm-----
         // video_desc->codecs().erase(iter);
         // video_desc->codecs().insert(video_desc->codecs().begin(), *iter);
        }
      }
    }
  }

  return;
}

void ECSDPHelper::SetSDPSsrcs(uint32_t audio,
                              uint32_t video,
                              webrtc::SessionDescriptionInterface* desc) {
  for (cricket::ContentInfos::iterator content =
           desc->description()->contents().begin();
       content != desc->description()->contents().end(); ++content) {
    cricket::MediaContentDescription* media_desc = content->media_description();

    if (media_desc->type() == cricket::MEDIA_TYPE_AUDIO) {
      if (audio == 0) {
        continue;
      }

      for (cricket::StreamParamsVec::iterator stream =
               media_desc->mutable_streams().begin();
           stream != media_desc->mutable_streams().end(); stream++) {
        stream->ssrcs = {audio};

        // Add the ssrc group to the track.
        for (cricket::SsrcGroup& ssrc_group : stream->ssrc_groups) {
          if (ssrc_group.ssrcs.empty()) {
            continue;
          }
          ssrc_group.ssrcs = {audio, audio};
        }
      }
    } else if (media_desc->type() == cricket::MEDIA_TYPE_VIDEO) {
      if (video == 0) {
        continue;
      }

      for (cricket::StreamParamsVec::iterator stream =
               media_desc->mutable_streams().begin();
           stream != media_desc->mutable_streams().end(); stream++) {
        std::vector<uint32_t> temp_ssrcs;
        for (uint32_t i = 0; i < stream->ssrcs.size(); i++) {
          temp_ssrcs.push_back(video);

          // TODO: huping. Use one video ssrc to disable Simulcast.
          break;
        }
        stream->ssrcs = temp_ssrcs;

        // Add the ssrc group to the track.
        for (cricket::SsrcGroup& ssrc_group : stream->ssrc_groups) {
          if (ssrc_group.ssrcs.empty()) {
            continue;
          }
          ssrc_group.ssrcs = temp_ssrcs;
        }
      }
    }
  }

  return;
}

void ECSDPHelper::SetSDPSsrcs(uint32_t audio,
                              std::vector<uint32_t> video,
                              webrtc::SessionDescriptionInterface* desc) {
  for (cricket::ContentInfos::iterator content =
           desc->description()->contents().begin();
       content != desc->description()->contents().end(); ++content) {
    cricket::MediaContentDescription* media_desc = content->media_description();

    if (media_desc->type() == cricket::MEDIA_TYPE_AUDIO) {
      for (cricket::StreamParamsVec::iterator stream =
               media_desc->mutable_streams().begin();
           stream != media_desc->mutable_streams().end(); stream++) {
        stream->add_ssrc(audio);
      }
    } else if (media_desc->type() == cricket::MEDIA_TYPE_VIDEO) {
      uint32_t i = 0;
      cricket::StreamParamsVec::iterator stream;
      for (stream = media_desc->mutable_streams().begin(), i = 0;
           stream != media_desc->mutable_streams().end() && i < video.size();
           stream++, i++) {
        stream->add_ssrc(video.at(i));
      }
    }
  }

  /*for (cricket::ContentInfos::iterator content =
           desc_ptr->description()->contents().begin();
       content != desc_ptr->description()->contents().end(); ++content) {
    cricket::MediaContentDescription* media_desc = content->media_description();

    for (auto local_content :
         current_local_description_->description()->contents()) {
      cricket::MediaContentDescription* local_media_desc =
          local_content.media_description();

      if (local_media_desc->type() == media_desc->type()) {
        media_desc->set_direction(local_media_desc->direction());

        for (auto stream : local_media_desc->streams()) {
          media_desc->AddStream(stream);
          break;
        }
      }
    }
  }*/
}

}  // namespace webrtc