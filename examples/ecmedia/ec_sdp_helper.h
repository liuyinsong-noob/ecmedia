#ifndef EC_SDP_HELPER_H
#define EC_SDP_HELPER_H

#include "api/jsep_session_description.h"
#include "pc/session_description.h"
#include "pc/webrtc_sdp.h"
#include<vector>

namespace ecmedia_sdk {

class ECSDPHelper {
public:
  ECSDPHelper();
  ~ECSDPHelper();

  std::unique_ptr<webrtc::SessionDescriptionInterface> CreateSessionDescription(
      webrtc::SdpType type,
      const std::string& sdp);

  std::unique_ptr<webrtc::SessionDescriptionInterface> CreateSessionDescription(
      webrtc::SdpType type,
      const std::string& sdp,
      webrtc::SdpParseError* error_out);

  
  void GetAudioCodecs(cricket::AudioCodecs* audio_codecs,
                      webrtc::SessionDescriptionInterface* desc);
  void GetVideoCodecs(cricket::VideoCodecs* video_codecs,
                      webrtc::SessionDescriptionInterface* desc);

  void SetSDPAVCodec(
	  cricket::AudioCodec* audio_codec,
	  cricket::VideoCodec* video_codec,             
                   webrtc::SessionDescriptionInterface* desc);


  void SetSDPCname(const std::string& audio,
                                       const std::string& video,
                   webrtc::SessionDescriptionInterface* desc);

  void SetSDPSsrcs(uint32_t audio,
                   uint32_t video,
                   webrtc::SessionDescriptionInterface* desc);
  
  void SetSDPSsrcs(uint32_t audio,
                          std::vector<uint32_t> video,
                   webrtc::SessionDescriptionInterface* desc);

private:


};

}  // namespace webrtc

#endif  //
