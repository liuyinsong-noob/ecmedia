#include "ecmedia.h"
#include "api/rtc_error.h"
#include "api/rtp_sender_interface.h"
#include "api/rtp_transceiver_interface.h"
#include "media_client.h"
#include "modules/video_capture/video_capture_factory.h"

ecmedia_sdk::MediaClient* g_ECMedia = nullptr;

#define ECMEDIA_VERSION "v3.0.8"

ECMEDIA_API bool ECMedia_set_trace(const char* path, const int level) {
  ecmedia_sdk::MediaClient::SetTrace(path, level);
  RTC_LOG(INFO) << "ECMedia SDK " << ECMEDIA_VERSION;
  return true;
}

/******************init**********************************************************/
ECMEDIA_API int ECMedia_init() {
  RTC_LOG(INFO) << "ECMedia SDK " << ECMEDIA_VERSION;
  if (g_ECMedia == nullptr) {
    g_ECMedia = ecmedia_sdk::MediaClient::GetInstance();
    g_ECMedia->Initialize();
    return 0;
  }
  return -1;
}

ECMEDIA_API int ECMedia_uninit() {
  if (g_ECMedia) {
    delete g_ECMedia;
    g_ECMedia = NULL;
  }
  return 0;
}

/************************channel************************************************/
ECMEDIA_API bool ECMedia_generate_channel_id(int& channel_id) {
  RTC_DCHECK(g_ECMedia);
  bool ret = g_ECMedia->GenerateChannelId(channel_id);
  return ret;
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
  return g_ECMedia->StopChannel(channel_id);
}

ECMEDIA_API bool ECMedia_stop_connect() {
  g_ECMedia->DestroyTransport();
  g_ECMedia->UnInitialize();
  g_ECMedia->DestroyInstance();
  return true;
}

ECMEDIA_API bool ECMedia_set_local_audio_mute(int channel_id, bool bMute) {
  return g_ECMedia->SetLocalMute(channel_id, bMute);
}

ECMEDIA_API int ECMedia_set_loudspeaker_status(bool enabled) {
  return g_ECMedia->SetLoudSpeakerStatus(enabled);
}

ECMEDIA_API int ECMedia_get_loudpeaker_status(bool& enabled) {
  return g_ECMedia->GetLoudSpeakerStatus(enabled);
}

ECMEDIA_API bool ECMedia_set_remote_audio_mute(int channel_id, bool bMute) {
  return g_ECMedia->SetRemoteMute(channel_id, bMute);
}

ECMEDIA_API bool ECMedia_request_remote_ssrc(int channel_id,
                                             int flag,
                                             int ssrc) {
  return g_ECMedia->RequestRemoteSsrc(channel_id, flag, ssrc);
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
ECMEDIA_API bool ECMedia_add_local_render(int channel_id,
                                          int mode,
                                          void* video_window) {
  RTC_DCHECK(g_ECMedia);

  return g_ECMedia->SetLocalVideoRenderWindow(channel_id, mode, video_window);
}

ECMEDIA_API bool ECMedia_add_remote_render(int channel_id,
                                           int mode,
                                           void* video_window) {
  RTC_DCHECK(g_ECMedia);

  return g_ECMedia->SetRemoteVideoRenderWindow(channel_id, mode, video_window);
}

/***********************ssrc*********************************************************/
ECMEDIA_API bool ECMedia_video_set_local_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->AddMediaSsrc(true, peer_id, ssrc);
}

ECMEDIA_API bool ECMedia_video_set_remote_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->AddMediaSsrc(false, peer_id, ssrc);
}

ECMEDIA_API bool ECMedia_audio_set_local_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->AddMediaSsrc(true, peer_id, ssrc);
}

ECMEDIA_API bool ECMedia_audio_set_remote_ssrc(int peer_id, unsigned int ssrc) {
  return g_ECMedia->AddMediaSsrc(false, peer_id, ssrc);
}

/***************************track**************************************************/
ECMEDIA_API void* ECMedia_create_audio_track(const char* track_id,
                                             int voice_index) {
  return g_ECMedia->CreateLocalVoiceTrack(track_id);
}

ECMEDIA_API void ECMedia_destroy_audio_track(void* track) {
  webrtc::AudioTrackInterface* ptrack = (webrtc::AudioTrackInterface*)track;
  g_ECMedia->DestroyLocalAudioTrack(ptrack);
}

ECMEDIA_API void* ECMedia_create_video_track(const char* track_params) {
  return g_ECMedia->CreateLocalVideoTrack(track_params);
}

ECMEDIA_API void ECMedia_destroy_video_track(void* track) {
  webrtc::VideoTrackInterface* ptrack = (webrtc::VideoTrackInterface*)track;
  g_ECMedia->DestroyLocalVideoTrack(ptrack);
}

ECMEDIA_API bool ECMedia_preview_video_track(int window_id, void* track) {
  return g_ECMedia->PreviewTrack(window_id, track);
}

ECMEDIA_API bool ECMedia_select_video_source(const char* tid,
                                             int channelid,
                                             const char* track_id,
                                             void* video_track,
                                             const char* s_ids) {
  return g_ECMedia->SelectVideoSource(
      channelid, track_id, (webrtc::VideoTrackInterface*)video_track);
}

ECMEDIA_API bool ECMedia_select_audio_source(
    const char* tid,
    int channelid,
    const char* track_id,
    void* audio_track,
    const std::vector<std::string>& stream_ids) {
  return g_ECMedia->SelectVoiceSource(
      channelid, track_id, (webrtc::AudioTrackInterface*)audio_track);
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
  return g_ECMedia->SetKeyFrameRequestCallback(channelId,
                                               (OnRequestKeyFrameCallback)cb);
}

ECMEDIA_API bool ECMedia_set_aec(bool enable) {
  return g_ECMedia->SetAEC(enable);
}

ECMEDIA_API bool ECMedia_set_agc(bool enable) {
  return g_ECMedia->SetAGC(enable);
}

ECMEDIA_API bool ECMedia_set_ns(bool enable) {
  return g_ECMedia->SetNS(enable);
}

ECMEDIA_API void* ECMedia_create_audio_device() {
  // TODO
  return nullptr;
}

ECMEDIA_API int ECMedia_set_speaker_volume(int volumep) {
  return g_ECMedia->SetSpeakerVolume(volumep);
}

ECMEDIA_API int ECMedia_get_speaker_volume(unsigned int& volumep) {
  return g_ECMedia->GetSpeakerVolume(volumep);
}

ECMEDIA_API int ECMedia_save_local_video_snapshot(int channelID,
                                                  const char* fileName) {
  return g_ECMedia->SaveLocalVideoSnapshot(channelID, fileName);
}

ECMEDIA_API bool ECMedia_set_audio_recording_volume(uint32_t vol) {
  return g_ECMedia->SetAudioRecordingVolume(vol);
}

ECMEDIA_API char* ECMedia_get_audio_device_list(int* len) {
  return g_ECMedia->GetAudioDeviceList(len);
}

ECMEDIA_API bool ECMedia_set_audio_recording_device(int index) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "(),"
                << " begins... "
                << ", index: " << index;
  return g_ECMedia->SetAudioRecordingDeviceOnFlight(index);
}

ECMEDIA_API bool ECMedia_set_audio_playout_device(int index) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "(),"
                << " begins... "
                << ", index: " << index;
  return g_ECMedia->SetAudioPlayoutDeviceOnFlight(index);
//  #define TEST_SELECT_CAMERA
#ifdef TEST_SELECT_CAMERA
  {
    std::string track_params0 =
        "{\n\t\"trackId\":\t\"kVedioLabel1\",\n\t\"videoSourceType\":\t0,"
        "\n\t\"deviceIndex\":\t0,\n\t\"width\":\t640,\n\t\"height\":\t480\n}";

    std::string track_params1 =
        "{\n\t\"trackId\":\t\"kVedioLabel1\",\n\t\"videoSourceType\":\t0,"
        "\n\t\"deviceIndex\":\t1,\n\t\"width\":\t640,\n\t\"height\":\t480\n}";

    if (index == 0)
      ECMedia_select_camera_on_flight(1, 0, track_params0);
    else
      ECMedia_select_camera_on_flight(1, 1, track_params1);
  }
  return true;
#endif
}

ECMEDIA_API int ECMedia_create_engine() {
  return 0;
}

/****************video capture********************/

ECMEDIA_API bool ECMedia_get_video_devices(char* devices, int* len) {
  return g_ECMedia->GetVideoDevices(devices, len);
}

ECMEDIA_API int ECMedia_delete_channel(int peer_id) {
  RTC_LOG(INFO) << " peer_id: " << peer_id;
  return 0;
}

ECMEDIA_API int ECMdeia_num_of_capture_devices() {
  RTC_DCHECK(g_ECMedia);
  return g_ECMedia->GetNumberOfVideoDevices();
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
  return g_ECMedia->NumOfCapabilities(id);
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
  RTC_LOG(INFO) << ", id: " << id << ", id_len: " << id_len
                << ", index: " << index << ", width: " << capability.width
                << ", height: " << capability.height
                << ", maxfps: " << capability.maxfps;
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

ECMEDIA_API void* ECMedia_select_camera_on_flight(
    int channelid,
    int device_idx,
    const std::string& track_params) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "(),"
                << " begins..."
                << ", index: " << device_idx;
  return g_ECMedia->SelectVideoSourceOnFlight(channelid, device_idx,
                                              track_params);
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
  RTC_LOG(INFO) << ", video_codecs: " << video_codecs;
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
  RTC_LOG(INFO) << ", audio_codecs id: " << audio_codecs;
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
  return 0;
}

ECMEDIA_API int ECMedia_audio_set_send_destination(int peer_id,
                                                   const char* r_addr,
                                                   const char* l_addr,
                                                   int port) {
  return 0;
}

/*ECMEDIA_API void ECMedia_add_tracks() {

         g_ECMedia->AddTracks();
}*/
ECMEDIA_API int ECMedia_set_video_protect_mode(int mode) {
  RTC_LOG(INFO) << " mode: " << mode;
  return 0;
}

ECMEDIA_API bool ECMedia_attach_video_render(int channelId,
                                             void* videoView,
                                             int render_mode,
                                             int mirror_mode) {
  return g_ECMedia->AttachVideoRender(channelId, videoView, render_mode,
                                      mirror_mode, nullptr);
}
ECMEDIA_API bool ECMedia_detach_video_render(int channelId, void* winRemote) {
  return g_ECMedia->DetachVideoRender(channelId, winRemote);
}
ECMEDIA_API void ECMedia_remove_all_video_render(int channelId) {
  return g_ECMedia->RemoveAllVideoRender(channelId);
}

ECMEDIA_API bool ECMedia_update_or_add_video_track(int channelId,
                                                   void* track_to_render) {
  return g_ECMedia->UpdateOrAddVideoTrack(channelId, track_to_render);
}
ECMEDIA_API bool ECMedia_start_render(int channelId, void* videoView) {
  return g_ECMedia->StartRender(channelId, videoView);
}
ECMEDIA_API bool ECMedia_stop_render(int channelId, void* videoView) {
  return g_ECMedia->StopRender(channelId, videoView);
}

ECMEDIA_API void ECMedia_set_render_mode(bool isLocal,
                                         int renderMode,
                                         bool mirrorMode) {
  g_ECMedia->SetRenderMode(isLocal, renderMode, mirrorMode);
}

#if defined(WEBRTC_WIN)
ECMEDIA_API int ECMedia_set_render_gdi(bool isGdi) {
  g_ECMedia->SetRenderGdi(isGdi);
  return 0;
}
#endif

#if defined(WEBRTC_IOS)
ECMEDIA_API int ECMedia_get_orientation(int deviceid,
                                        ECMediaRotateCapturedFrame& tr) {
  return g_ECMedia->GetOrientation(deviceid, tr);
}
ECMEDIA_API int ECMedia_set_rotate_captured_frames(
    int deviceid,
    ECMediaRotateCapturedFrame tr) {
  return g_ECMedia->SetRotateCapturedFrames(deviceid, tr);
}
int ECMedia_audio_set_microphone_gain(int channelId, float gain) {
  return g_ECMedia->SetMicrophoneGain(channelId, gain);
}
#endif

#if defined(WEBRTC_ANDROID)
ECMEDIA_API bool ECMedia_SaveLocalVideoTrack(int channelId, void* track) {
  bool bOk = g_ECMedia->SaveLocalVideoTrack(channelId, track);
  RTC_LOG(INFO) << " channelId: " << channelId << " track: " << track
                << " bOk: " << bOk;
  return bOk;
}

ECMEDIA_API void* ECMedia_GetLocalVideoTrackPtr(int channelId) {
  void* track = g_ECMedia->GetLocalVideoTrackPtr(channelId);
  RTC_LOG(INFO) << " channelId: " << channelId << " track: " << track;
  return track;
}

ECMEDIA_API bool ECMedia_RemoveLocalVideoTrack(int channelId) {
  bool bOk = g_ECMedia->RemoveLocalVideoTrack(channelId);
  RTC_LOG(INFO) << " channelId: " << channelId << " bOk: " << bOk;
  return bOk;
}

ECMEDIA_API bool ECMedia_SaveRemoteVideoSink(int channelId,
                                             JNIEnv* env,
                                             jobject javaSink) {
  bool bOk = g_ECMedia->SaveRemoteVideoSink(channelId, env, javaSink);
  RTC_LOG(INFO) << " channelId: " << channelId << " env: " << env
                << " javaSink: " << javaSink << " bOk: " << bOk;
  return bOk;
}

ECMEDIA_API bool ECMedia_RemoveRemoteVideoSink(int channelId) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "() "
                << " begin..."
                << " channelId: " << channelId;

  bool bOk = g_ECMedia->RemoveRemoteVideoSink(channelId);
  RTC_LOG(INFO) << " channelId: " << channelId << " bOk: " << bOk;
  return bOk;
}

ECMEDIA_API int ECMedia_InitializeJVM() {
  int ret = g_ECMedia->InitializeJVM();
  RTC_LOG(INFO) << " ret: " << ret;
  return ret;
}

ECMEDIA_API void* CreateAudioSource() {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "() "
                << " begin...";

  return g_ECMedia->CreateAudioSource();
}

ECMEDIA_API void* CreateVideoSource(JNIEnv* env,
                                    bool is_screencast,
                                    bool align_timestamps) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "() "
                << " begin...";

  return g_ECMedia->CreateVideoSource(env, is_screencast, align_timestamps);
}

ECMEDIA_API void* CreateVideoTrack(const char* id, void* source) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "() "
                << " begin...";

  return g_ECMedia->CreateVideoTrack(
      id, (webrtc::VideoTrackSourceInterface*)source);
}

ECMEDIA_API void* CreateAudioTrack(const char* id, void* source) {
  RTC_LOG(INFO) << "[ECMEDIA3.0]" << __FUNCTION__ << "() "
                << " begin...";
  return g_ECMedia->CreateAudioTrack(id, (webrtc::AudioSourceInterface*)source);
}

ECMEDIA_API void ECMedia_SetVideoHardwareEncoderFactoryCallback(
    ECMedia_OnGetVideoHardwareEncoderFactory callback) {
  ecmedia_sdk::MediaClient::SetVideoHardwareEncoderFactoryCallback(
      (ecmedia_sdk::OnGetVideoHardwareEncoderFactory)callback);
}

ECMEDIA_API void ECMedia_SetAudioHardwareEncoderFactoryAndAdmCallback(
    ECMedia_OnGetAudioHardwareEncoderFactoryAndAdm callback) {
  ecmedia_sdk::MediaClient::SetAudioHardwareEncoderFactoryAndAdmCallback(
      (ecmedia_sdk::OnGetAudioHardwareEncoderFactoryAndAdm)callback);
}

#endif

ECMEDIA_API int ECMedia_set_desktop_capture_source(int type, int id) {
  return 0;
  //  return g_ECMedia->SetDesktopSourceID(type,id);
}

ECMEDIA_API int ECMedia_create_desktop_capture(int type) {
  return g_ECMedia->CreateDesktopCapture(type);
}

ECMEDIA_API int ECMedia_get_screen_list(int type, ScreenID** screenList) {
  if (!screenList) {
    return -1;
  } else {
    webrtc::DesktopCapturer::SourceList sources;
    g_ECMedia->GetWindowsList(type, sources);
    int num = sources.size();
    ScreenID* m_pScreenlist = new ScreenID[num];
    ScreenID* temp = m_pScreenlist;

    for (auto it = sources.begin(); it != sources.end(); ++it) {
      (*temp) = it->id;
      temp++;
    }
    *screenList = m_pScreenlist;
    return num;
  }
}

ECMEDIA_API bool ECMedia_select_screen(int type, ScreenID screeninfo) {
  int ret = g_ECMedia->SetDesktopSourceID(type, screeninfo);
  return ret == 0 ? true : false;
}

ECMEDIA_API int ECMedia_release_window_list(WindowShare** windowList) {
  if ((*windowList) == NULL) {
    return -1;
  }
  delete[](*windowList);
  (*windowList) = NULL;
  return 0;
}
ECMEDIA_API int ECMedia_get_window_list(int type, WindowShare** windowList) {
  if (!windowList) {
    return -1;
  }
  webrtc::DesktopCapturer::SourceList sources;
  g_ECMedia->GetWindowsList(type, sources);
  int num = sources.size();
  WindowShare* m_pWindowlist = NULL;
  m_pWindowlist = new WindowShare[num];
  if (m_pWindowlist == NULL) {
    RTC_LOG(INFO) << "new operate fail!";
    return -1;
  }
  WindowShare* temp = m_pWindowlist;
  for (auto it = sources.begin(); it != sources.end(); ++it) {
    (*temp).id = it->id;
    (*temp).type = 0;
#if defined(WEBRTC_WIN)
    int len = it->title.length();
    if (len >= kTitleLength)
      len = kTitleLength - 1;
    memcpy_s((*temp).title, len, it->title.c_str(), len);
#endif
    temp++;
  }
  *windowList = m_pWindowlist;
  return num;
}

ECMEDIA_API int ECMedia_release_screen_list(ScreenID** screen) {
  if ((*screen) == NULL) {
    return -1;
  }
  delete[](*screen);
  (*screen) = NULL;
  return 0;
}

ECMEDIA_API int ECMedia_start_screen_share(int type, int channelId) {
  return g_ECMedia->StartScreenShare(type, channelId);
}

ECMEDIA_API int ECMedia_stop_screen_share(int type, int channelId) {
  return g_ECMedia->StopScreenShare(type, channelId);
}
ECMEDIA_API int ECMedia_crop_desktop_capture(int type,
                                             int x,
                                             int y,
                                             int width,
                                             int height) {
  return g_ECMedia->CropDesktopCapture(type, x, y, width, height);
}
ECMEDIA_API void ECMedia_get_audio_channel_volume_level(int audioid,
                                                        int level) {
  return;
}

ECMEDIA_API void ECMedia_set_video_show_window_mode(int mode) {
  return;
}
/**************************************************Voice
 * excitation**************************************************************/
// wx begin
int ECMedia_setECMedia_ConferenceParticipantCallback(
    int channelid,
    ECMedia_ConferenceParticipantCallback* callback) {
  return g_ECMedia->RegisterConferenceParticipantCallback(channelid, callback);
}

int ECMedia_setECMedia_ConferenceParticipantCallbackTimeInterVal(
    int channelid,
    int timeInterVal) {
  return g_ECMedia->SetConferenceParticipantCallbackTimeInterVal(channelid,
                                                                 timeInterVal);
}
int ECMedia_get_stats(char* statistics, int length) {
  return g_ECMedia->GetCallStats(statistics, length);
}
bool ECMedia_get_StreamStats(char* statistics, int length, int channel_id) {
  if (channel_id == 0) {
    return g_ECMedia->GetVoiceStreamStats(statistics, length, channel_id);
  } else {
    return g_ECMedia->GetVideoStreamStats(statistics, length, channel_id);
  }
}
bool ECMedia_set_remote_video_resolute_callback(
    int channelid,
    ECMedia_FrameSizeChangeCallback* callback) {
  return g_ECMedia->RegisterRemoteVideoResoluteCallback(channelid, callback);
}
int ECMedia_set_media_packet_timeout_callback(
    int channelid,
    ECMedia_PacketTimeout* media_timeout_cb) {
  return g_ECMedia->RegisterMediaPacketTimeoutCallback(channelid,
                                                       media_timeout_cb);
}
int ECMedia_set_packet_timeout_noti(int channel, int timeout_ms) {
  return g_ECMedia->SetPacketTimeoutNotification(channel, timeout_ms);
}
