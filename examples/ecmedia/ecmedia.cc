#include "ecmedia.h"

#include "ec_base_manager.h"

#include "api/rtc_error.h"
#include "api/rtp_sender_interface.h"
#include "api/rtp_transceiver_interface.h"

webrtc::ECBaseManager* g_ECMedia = nullptr;

/******************init**********************************************************/
ECMEDIA_API int ECMedia_init() {
  if (g_ECMedia == nullptr) {
    g_ECMedia = webrtc::ECBaseManager::GetInstance();
    g_ECMedia->Init();
    return 0;
  }
  return -1;
}

ECMEDIA_API int ECMedia_uninit() {
  webrtc::ECBaseManager::DestroyInstance();
  return 0;
}

/************************channel************************************************/
ECMEDIA_API bool ECMedia_generate_channel_id(int& channel_id) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->GenerateChannelId(channel_id);
}

ECMEDIA_API bool ECMedia_release_channel_id(int channel_id) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->ReleaseChannelId(channel_id);
}

ECMEDIA_API bool ECMedia_create_transport(const char* l_addr,
                                          int l_port,
                                          const char* r_addr,
                                          int r_port,
                                          const char* tid) {
  return g_ECMedia->CreateTransport(l_addr, l_port, r_addr, r_port, tid);
}

ECMEDIA_API bool ECMedia_create_channel(const char* tid,
                                        int& channel_id,
                                        bool is_video) {
  return g_ECMedia->CreateChannel(tid, channel_id, is_video);
}

ECMEDIA_API void ECMedia_destroy_channel(int& channel_id, bool is_video) {
  g_ECMedia->DestroyChannel(channel_id, is_video);
}

ECMEDIA_API bool ECMedia_start_channel(int channel_id) {
  return g_ECMedia->StartChannel(channel_id);
}

ECMEDIA_API bool ECMedia_stop_channel(int channel_id, bool is_video) {
  return g_ECMedia->StopChannel(channel_id, is_video);
}

ECMEDIA_API bool ECMedia_stop_connect() {
  return g_ECMedia->StopConnectChannel();
}

ECMEDIA_API bool ECMedia_set_local_audio_mute(int channel_id, bool bMute) {
  return g_ECMedia->SetLocalMute(channel_id, bMute);
}

ECMEDIA_API bool ECMedia_set_remote_audio_mute(int channel_id, bool bMute) {
  return g_ECMedia->SetRemoteMute(channel_id, bMute);
}

ECMEDIA_API bool ECMedia_request_remote_ssrc(int channel_id, int ssrc) {
  return g_ECMedia->RequestRemoteSsrc(channel_id, ssrc);
}

ECMEDIA_API bool ECMedia_get_video_codecs(char* jsonVideoCodecInfos,
                                          int* length) {
  return g_ECMedia->GetVideoCodecs(jsonVideoCodecInfos, length);
}

ECMEDIA_API bool ECMedia_get_audio_codecs(char* jsonAudioCodecInfos,
                                          int* length) {
  return g_ECMedia->GetAudioCodecs(jsonAudioCodecInfos, length);
}
/**********************************render********************************************/
ECMEDIA_API bool ECMedia_add_local_render(int channel_id, void* video_window) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->AddLocalRender(channel_id, video_window);
}

ECMEDIA_API bool ECMedia_add_remote_render(int channel_id, void* video_window) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->AddRemoteRender(channel_id, video_window);
}

/***********************ssrc*********************************************************/
ECMEDIA_API bool ECMedia_video_set_local_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->SetVideoLocalSsrc(peer_id, ssrc);
}

ECMEDIA_API bool ECMedia_video_set_remote_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->SetVideoRemoteSsrc(peer_id, ssrc);
}

ECMEDIA_API bool ECMedia_audio_set_local_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->SetAudioLocalSsrc(peer_id, ssrc);
}

ECMEDIA_API bool ECMedia_audio_set_remote_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->SetAudioRemoteSsrc(peer_id, ssrc);
}

/***************************track**************************************************/
ECMEDIA_API void* ECMedia_create_audio_track(const char* track_id,
                                             int voice_index) {
  return g_ECMedia->CreateAudioTrack(track_id, 0);
}

ECMEDIA_API void ECMedia_destroy_audio_track(void* track) {
  g_ECMedia->DestroyAudioTrack(track);
}

ECMEDIA_API void* ECMedia_create_video_track(const char* track_params) {
  /*ECMEDIA_API void* ECMedia_create_video_track(int video_mode,
                                              const char* track_id,
                                              int camera_index) {*/
  std::string ss = track_params;

  return g_ECMedia->CreateVideoTrack(ss);
}

ECMEDIA_API void ECMedia_destroy_video_track(void* track) {
  g_ECMedia->DestroyVideoTrack(track);
}

ECMEDIA_API bool ECMedia_preview_video_track(int window_id, void* track) {
  return g_ECMedia->PreviewTrack(window_id, track);
}

ECMEDIA_API bool ECMedia_select_video_source(const char* tid,
                                             int channelid,
                                             const char* track_id,
                                             void* video_track,
                                             const char* s_ids) {
  std::vector<std::string> stream_ids;
  std::string ss = s_ids;
  size_t n = 0;
  while ((n = ss.find(',')) != ss.npos) {
    std::string st = ss.substr(0, n);
    ss = ss.substr(n + 1);
    stream_ids.push_back(st);
    // return true;
  }
  stream_ids.push_back(ss);

  return g_ECMedia->SelectVideoSource(tid, channelid, track_id, video_track,
                                      stream_ids);
}

ECMEDIA_API bool ECMedia_select_audio_source(
    const char* tid,
    int channelid,
    const char* track_id,
    void* audio_track,
    const std::vector<std::string>& stream_ids) {
  return g_ECMedia->SelectVoiceSource(tid, channelid, track_id, audio_track,
                                      stream_ids);
}

ECMEDIA_API bool ECMedia_set_video_nack_status(const int channelId,
                                               const bool enable_nack) {
  return g_ECMedia->SetVideoNackStatus(channelId, enable_nack);
}

ECMEDIA_API bool ECMedia_set_video_ulpfec_status(
    const int channelId,
    const bool enable,
    const uint8_t payloadtype_red,
    const uint8_t payloadtype_fec) {
  return g_ECMedia->SetVideoUlpFecStatus(channelId, enable, payloadtype_red,
                                         payloadtype_fec);
}

ECMEDIA_API bool ECMedia_set_video_degradation_mode(const int channelId,
                                                    int mode) {
  webrtc::DegradationPreference video_mode;
  switch (mode) {
    case 0:
      video_mode = webrtc::DegradationPreference::DISABLED;
      break;
    case 1:
      video_mode = webrtc::DegradationPreference::MAINTAIN_FRAMERATE;
      break;
    case 2:
      video_mode = webrtc::DegradationPreference::MAINTAIN_RESOLUTION;
      break;
    case 3:
    default:
      video_mode = webrtc::DegradationPreference::BALANCED;
      break;
  }
  return g_ECMedia->SetVideoDegradationMode(channelId, video_mode);
}

ECMEDIA_API bool ECMedia_send_key_frame(const int channelId) {
  return g_ECMedia->SendKeyFrame(channelId);
}

ECMEDIA_API bool ECMedia_set_key_frame_request_callback(const int channelId,
                                                        void* cb) {
  return g_ECMedia->SetKeyFrameRequestCallback(channelId, cb);
}

ECMEDIA_API bool ECMedia_set_aec(bool enable) {
  return g_ECMedia->SetAEC(enable);
}

ECMEDIA_API bool ECMedia_set_agc(bool enable) {
  return g_ECMedia->SetAEC(enable);
}

ECMEDIA_API bool ECMedia_set_ns(bool enable) {
  return g_ECMedia->SetAEC(enable);
}

ECMEDIA_API void* ECMedia_create_audio_device() {
  return g_ECMedia->CreateAudioDevice();
}

ECMEDIA_API bool ECMedia_set_audio_recording_volume(uint32_t vol) {
  return g_ECMedia->SetAudioRecordingVolume(vol);
}

ECMEDIA_API bool ECMedia_get_audio_device_list(char* json, int* len) {
  return g_ECMedia->GetAudioDeviceList(json, len);
}

ECMEDIA_API bool ECMedia_set_audio_recording_device(int index) {
  return g_ECMedia->SetAudioRecordingDevice(index);
}

///////////////////////////////////////////////////////////////////////////////////////
/**************************************************************************************/

// ECMEDIA_API int ECMedia_init_audio() {
//  if (g_ECMedia == nullptr) {
//    g_ECMedia = webrtc::ECBaseManager::GetInstance();
//  }
//
//  return 0;
//}
//
// ECMEDIA_API int ECMedia_uninit_audio() {
//  webrtc::ECBaseManager::DestroyInstance();
//  return 0;
//}
//
// ECMEDIA_API int ECMedia_init_video() {
//  if (g_ECMedia == nullptr) {
//    g_ECMedia = webrtc::ECBaseManager::GetInstance();
//  }
//
//  return 0;
//}
//
// ECMEDIA_API int ECMedia_uninit_video() {
//  webrtc::ECBaseManager::DestroyInstance();
//  return 0;
//}

/****************video engine********************/

ECMEDIA_API int ECMedia_create_engine() {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->CreateMediaEngine();
}

/****************video capture********************/

ECMEDIA_API bool ECMedia_get_video_devices(char* devices, int* len) {
  return g_ECMedia->GetVideoDevices(devices, len);
}

ECMEDIA_API int ECMedia_delete_channel(int peer_id) {
  return g_ECMedia->DeleteBaseManager(peer_id);
}

ECMEDIA_API int ECMdeia_num_of_capture_devices() {
  RTC_DCHECK(g_ECMedia);
  int camera_nums = g_ECMedia->NumOfCaptureDevices();
  return camera_nums;
}

ECMEDIA_API int ECMedia_get_capture_device(int index,
                                           char* name,
                                           int name_len,
                                           char* id,
                                           int id_len) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->GetCaptureDevice(index, name, name_len, id, id_len);
}

ECMEDIA_API int ECMedia_num_of_capabilities(const char* id, int id_len) {
  RTC_DCHECK(g_ECMedia);
  int num = g_ECMedia->NumOfCapabilities(id, id_len);
  return num;
}

ECMEDIA_API int ECMedia_get_capture_capability(const char* id,
                                               int id_len,
                                               int index,
                                               CameraCapability& capability) {
  RTC_DCHECK(g_ECMedia);
  webrtc::VideoCaptureCapability cap;
  g_ECMedia->GetCaptureCapabilities(id, index, cap);
  capability.height = cap.height;
  capability.width = cap.width;
  capability.maxfps = cap.maxFPS;
  return 0;
}

ECMEDIA_API int ECMedia_allocate_capture_device(const char* id,
                                                int len,
                                                int& deviceid) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->AllocateCaptureDevice(id, len, deviceid);
}

ECMEDIA_API int ECMedia_connect_capture_device(int deviceid, int peer_id) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->ConnectCaptureDevice(deviceid, peer_id);
}

// ECMEDIA_API int ECMedia_set_local_video_window(int deviceid,
//                                               void* video_window) {
//  RTC_DCHECK(g_ECMedia);
//  return g_ECMedia->SetLocalVideoWindow(deviceid, video_window);
//}

ECMEDIA_API int ECMedia_start_capture(int deviceid, CameraCapability cam) {
  RTC_DCHECK(g_ECMedia);
  webrtc::VideoCaptureCapability cap;
  cap.width = cam.width;
  cap.height = cam.height;
  cap.maxFPS = cam.maxfps;
  cap.videoType = webrtc::VideoType::kI420;

  return g_ECMedia->StartCameraCapturer(deviceid, cap);
}

ECMEDIA_API int ECMedia_stop_capture(int deviceid) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->StopCapturer(deviceid);
}

/****************video render********************/

ECMEDIA_API int ECMedia_stop_local_render(int peer_id, int deviceid) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->StopLocalRender(peer_id, deviceid);
}

ECMEDIA_API int ECMedia_stop_remote_render(int peer_id, int deviceid) {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->StopRemoteRender(peer_id, deviceid);
}

/*******************audio capture************************/
ECMEDIA_API int ECMedia_start_mic(int peer_id, int deviceid) {
  return g_ECMedia->StartMicCapture(deviceid);
}

/****************video receive send********************/
ECMEDIA_API int ECMedia_start_sendrecv(int peer_id) {
  RTC_DCHECK(g_ECMedia);
  g_ECMedia->VideoStartSend(peer_id);

  return 0;
}

ECMEDIA_API int ECMedia_video_start_receive(int peer_id) {
  return int();
}

ECMEDIA_API int ECMedia_video_stop_receive(int peer_id) {
  return int();
}

ECMEDIA_API int ECMedia_video_start_send(int peer_id) {
  RTC_DCHECK(g_ECMedia);
  g_ECMedia->VideoStartSend(peer_id);
  return int();
}

ECMEDIA_API int ECMedia_video_stop_send(int peer_id) {
  return g_ECMedia->StopAllCapturer();
}

/*********************audio receive send*****************/
ECMEDIA_API int ECMedia_audio_start_receive(int peer_id) {
  return 0;
}

ECMEDIA_API int ECMedia_audio_stop_receive(int peer_id) {
  return 0;
}

ECMEDIA_API int ECMedia_audio_start_send(int peer_id) {
  return g_ECMedia->AudioStartSend(peer_id);
}

ECMEDIA_API int ECMedia_audio_stop_send(int peer_id) {
  return 0;
}

/***********************begin connect audio video
 * channel***************************/

ECMEDIA_API bool ECMedia_start_connect(int audio_channel_id,
                                       int video_channel_id) {
  return g_ECMedia->StartConnectChannel(audio_channel_id, video_channel_id);
}

/*********************codec**********************/
ECMEDIA_API int ECMedia_get_supported_codecs_video(
    std::vector<ecmedia::VideoCodec>* video_codecs) {
  cricket::VideoCodecs cricket_video_codecs;
  g_ECMedia->GetVideoCodecs(&cricket_video_codecs);

  for (auto cr : cricket_video_codecs) {
    ecmedia::VideoCodec ec(cr.id, cr.name);
    video_codecs->push_back(ec);
  }

  return 0;
}

ECMEDIA_API int ECMedia_get_supported_codecs_audio(
    std::vector<ecmedia::AudioCodec>* audio_codecs) {
  cricket::AudioCodecs cricket_audio_codecs;
  g_ECMedia->GetAudioCodecs(&cricket_audio_codecs);

  for (auto cr : cricket_audio_codecs) {
    ecmedia::AudioCodec ec(cr.id, cr.name, cr.clockrate, cr.bitrate,
                           cr.channels);
    audio_codecs->push_back(ec);
  }

  return 0;
}

ECMEDIA_API int ECMedia_set_send_codec_video(int peer_id,
                                             ecmedia::VideoCodec* video_codec) {
  cricket::VideoCodec cricket_video_codec(video_codec->id, video_codec->name);

  g_ECMedia->SetSendCodecVideo(&cricket_video_codec);

  return 0;
}

ECMEDIA_API int ECMedia_set_receive_codec_video(
    int peer_id,
    ecmedia::VideoCodec* video_codec) {
  cricket::VideoCodec cricket_video_codec(video_codec->id, video_codec->name);

  g_ECMedia->SetReceiveCodecVideo(peer_id, &cricket_video_codec);

  return 0;
}

ECMEDIA_API int ECMedia_set_send_codec_audio(int peer_id,
                                             ecmedia::AudioCodec* audio_codec) {
  cricket::AudioCodec cricket_audio_codec(
      audio_codec->id, audio_codec->name, audio_codec->clockrate,
      audio_codec->bitrate, audio_codec->channels);
  g_ECMedia->SetSendCodecAudio(&cricket_audio_codec);

  return 0;
}

ECMEDIA_API int ECMedia_set_receive_playloadType_audio(
    int peer_id,
    ecmedia::AudioCodec* audio_codec) {
  cricket::AudioCodec cricket_audio_codec(
      audio_codec->id, audio_codec->name, audio_codec->clockrate,
      audio_codec->bitrate, audio_codec->channels);

  g_ECMedia->SetReceiveCodecAudio(peer_id, &cricket_audio_codec);

  return 0;
}

/***************************ip port************************/
ECMEDIA_API int ECMedia_video_set_send_destination(int peer_id,
                                                   const char* r_addr,
                                                   const char* l_addr,
                                                   int port) {
  // g_ECMedia->SetSendDestination(peer_id, r_addr, l_addr,port);

  return 0;
}

ECMEDIA_API int ECMedia_audio_set_send_destination(int peer_id,
                                                   const char* r_addr,
                                                   const char* l_addr,
                                                   int port) {
  // g_ECMedia->SetSendDestination(peer_id, r_addr, l_addr, port);
  return 0;
}

/*ECMEDIA_API void ECMedia_add_tracks() {

         g_ECMedia->AddTracks();
}*/
ECMEDIA_API int ECMedia_set_video_protect_mode(int mode) {
  return 0;
}
//wwx
ECMEDIA_API bool ECMedia_set_remote_ssrc_video(char* setting, int channel_id) {
  return g_ECMedia->SetSendSsrcVideo(setting, channel_id);
}

ECMEDIA_API bool ECMedia_set_remote_ssrc_audio(char* setting, int channel_id) {
  return g_ECMedia->SetSendSsrcAudio(setting, channel_id);
}
ECMEDIA_API bool ECMedia_SetRemoteSsrcAfterSelectAudioSource(int channelId){
  return g_ECMedia->SetRemoteSsrcAfterSelectAudioSource(channelId);
}

ECMEDIA_API bool ECMedia_SetRemoteSsrcAfterSelectVideoSource(int channelId) {
  return g_ECMedia->SetRemoteSsrcAfterSelectVideoSource(channelId);
}