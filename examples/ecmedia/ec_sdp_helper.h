#ifndef EC_SDP_HELPER_H
#define EC_SDP_HELPER_H

#include "api/jsep_session_description.h"
#include "pc/session_description.h"
#include "pc/webrtc_sdp.h"
#include<vector>

namespace webrtc {

class ECSDPHelper {
public:
  ECSDPHelper();
  ~ECSDPHelper();

  std::unique_ptr<SessionDescriptionInterface> CreateSessionDescription(
      SdpType type,
      const std::string& sdp);

  std::unique_ptr<SessionDescriptionInterface> CreateSessionDescription(
      SdpType type,
      const std::string& sdp,
      SdpParseError* error_out);

  
  void GetAudioCodecs(cricket::AudioCodecs* audio_codecs,
                      SessionDescriptionInterface* desc) ;
  void GetVideoCodecs(cricket::VideoCodecs* video_codecs,
                      SessionDescriptionInterface* desc) ;

  void SetSDPAVCodec(
	  cricket::AudioCodec* audio_codec,
	  cricket::VideoCodec* video_codec,             
                   SessionDescriptionInterface* desc);


  void SetSDPCname(const std::string& audio,
                                       const std::string& video,
                                       SessionDescriptionInterface* desc);

  void SetSDPSsrcs(uint32_t audio,
                   uint32_t video,
                   SessionDescriptionInterface* desc);
  
  void SetSDPSsrcs(uint32_t audio,
                          std::vector<uint32_t> video,
                          SessionDescriptionInterface* desc);

  

private:


};

}  // namespace webrtc

#endif  //
