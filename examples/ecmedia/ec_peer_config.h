#ifndef ECM_INTERFACE_H
#define ECM_INTERFACE_H

#include "rtc_base/thread.h"

namespace webrtc {

class ECPeerConfig {
 public:
  ECPeerConfig(); 
  ~ECPeerConfig();

  int local_video_ssrc;
  int remote_video_ssrc;
  int local_audio_ssrc;
  int remote_audio_ssrc;

  rtc::Thread* signaling_thread;
  rtc::Thread* worker_thread;
  rtc::Thread* network_thread;

};



}  // namespace webrtc

#endif