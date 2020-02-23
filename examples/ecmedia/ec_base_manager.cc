#include "ec_base_manager.h"

#include <algorithm>
#include <memory>
#include "api/media_stream_track_proxy.h"
#include "api/video_track_source_proxy.h"
#include "ec_base_manager.h"
#include "media/base/media_engine.h"
#include "media_client.h"
#include "modules/video_capture/video_capture_factory.h"
#include "pc/audio_rtp_receiver.h"
#include "pc/rtp_sender.h"
#include "pc/video_rtp_receiver.h"
#include "pc/video_track.h"
#include "rtc_base/helpers.h"
#include "rtc_base/logging.h"
#include "video_capturer/capturer_track_source.h"

namespace webrtc {

using ecmedia_sdk::MediaClient;

ECBaseManager* ECBaseManager::g_instance = nullptr;
rtc::CriticalSection ECBaseManager::getIns_crit_;

bool ECBaseManager::SetTrace(const char* path, const int level){
  MediaClient::GetInstance()->SetTrace(path, level);
  return true;
}


ECBaseManager* ECBaseManager::GetInstance() {
  RTC_LOG(INFO) << " begins...";
  rtc::CritScope lock(&getIns_crit_);
  if (g_instance == nullptr)
    g_instance = new ECBaseManager();
  RTC_LOG(INFO) << " end...";
  return g_instance;
}



void ECBaseManager::DestroyInstance() {
  RTC_LOG(INFO) << " begins...";
  rtc::CritScope lock(&getIns_crit_);

  if (g_instance) {
    delete g_instance;
  }
  RTC_LOG(INFO) << " end...";
}

ECBaseManager::ECBaseManager() : local_window_(nullptr) {
  RTC_LOG(INFO) << " begins...";
  vcm_device_info_.reset(webrtc::VideoCaptureFactory::CreateDeviceInfo());
  Init();//创建network_thread_ ；worker_thread_；signaling_thread_三个线程；
  RTC_LOG(INFO) << " end...";
}

ECBaseManager::~ECBaseManager() {
  // note: release local_renderer first, then worker_thread
}

void ECBaseManager::Init() {
  RTC_LOG(INFO) << " begins...";
   MediaClient::GetInstance()->Initialize();
}

int ECBaseManager::CreateMediaEngine() {
  RTC_LOG(INFO) << " begins...";
  return 0;
}

int ECBaseManager::DeleteBaseManager(int peer_id) {
  RTC_LOG(INFO) << " begins...";
  return 0;
}
int ECBaseManager::CreateChannelManager() {
  RTC_LOG(INFO) << " begins...";
  rtc::CritScope lock(&getIns_crit_);
  return 0;
}

/**********************************************************************************/
///////////////////////////////////////////////////////////////////////////////////
bool ECBaseManager::GenerateChannelId(int& id) {
  RTC_LOG(INFO) << " begins..." << "id:" << id;
  bool bOk = true;
 
  bOk = MediaClient::GetInstance()->GenerateChannelId(&id);
  RTC_LOG(INFO) << " end..."
                << "bOK:" << bOk;
  return bOk;
}

bool ECBaseManager::ReleaseChannelId(int id) {
  RTC_LOG(INFO) << " begins..."
                << "id:" << id;
  bool bOk = false;
  bOk = MediaClient::GetInstance()->ReleaseChannelId(id);
  RTC_LOG(INFO) << " end..."
                << "bOK:" << bOk;
  return bOk;
}

bool ECBaseManager::CreateTransport(const char* l_addr,
                                    int l_port,
                                    const char* r_addr,
                                    int r_port,
                                    const char* tid) {
  RTC_LOG(INFO) << " begin..."
                << "l_addr:" << l_addr << "l_port:" << l_port
                << "r_addr:" << r_addr << "r_port:" << r_port << "tid:" << tid;
  return MediaClient::GetInstance()->CreateTransport(l_addr, l_port, r_addr,
                                                     r_port, tid);
}

bool ECBaseManager::CreateChannel(const std::string& settings,
                                  int channel_id,
                                  bool is_video) {
  RTC_LOG(INFO) << " begin..."
                << "settings:" << settings << "channel_id:" << channel_id
                << "is_video:" << is_video;
  return MediaClient::GetInstance()->CreateChannel(settings, channel_id,
                                                   is_video);
}

void ECBaseManager::DestroyChannel(int channel_id, bool is_video) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "is_video:" << is_video;
  return MediaClient::GetInstance()->DestroyChannel(channel_id, is_video);
}

bool ECBaseManager::StartChannel(int channel_id) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id ;
  return MediaClient::GetInstance()->StartChannel(channel_id);
}

bool ECBaseManager::StopChannel(int channel_id, bool is_video) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "is_video:" << is_video;
  return MediaClient::GetInstance()->StopChannel(channel_id);
}

bool ECBaseManager::StopConnectChannel() {
  RTC_LOG(INFO) << " begin...";
  MediaClient::GetInstance()->DestroyTransport();
  MediaClient::GetInstance()->UnInitialize();
  MediaClient::DestroyInstance();
  RTC_LOG(INFO) << " end...";
  return true;
}
/***************************render************************/
bool ECBaseManager::AddLocalRender(int window_id, void* video_window) {
  RTC_LOG(INFO) << " begin...";
  MediaClient::GetInstance()->SetLocalVideoRenderWindow(window_id,
                                                        video_window);
  return true;
}

bool ECBaseManager::AddRemoteRender(int peer_id, void* video_window) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id;
  MediaClient::GetInstance()->SetRemoteVideoRenderWindow(peer_id, video_window);
  return true;
}
/***************************ssrc************************/
bool ECBaseManager::SetVideoLocalSsrc(int channel_id, unsigned int ssrc) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "ssrc:" << ssrc;
  return MediaClient::GetInstance()->AddMediaSsrc(true, channel_id, ssrc);
}

bool ECBaseManager::SetVideoRemoteSsrc(int channel_id, unsigned int ssrc) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "ssrc:" << ssrc;
  return MediaClient::GetInstance()->AddMediaSsrc(false, channel_id, ssrc);
}

bool ECBaseManager::SetAudioLocalSsrc(int channel_id, unsigned int ssrc) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "ssrc:" << ssrc;
  return MediaClient::GetInstance()->AddMediaSsrc(true, channel_id, ssrc);
}

bool ECBaseManager::SetAudioRemoteSsrc(int channel_id, unsigned int ssrc) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "ssrc:" << ssrc;
  return MediaClient::GetInstance()->AddMediaSsrc(false, channel_id, ssrc);
}

void* ECBaseManager::CreateVideoTrack(std::string& track_params) {
  RTC_LOG(INFO) << " begin...";
  rtc::scoped_refptr<webrtc::VideoTrackInterface> track(
      MediaClient::GetInstance()->CreateLocalVideoTrack(track_params));
  return track;
}

bool ECBaseManager::PreviewTrack(int window_id, void* track) {
  RTC_LOG(INFO) << " begin..."
                << "window_id:" << window_id;
  return MediaClient::GetInstance()->PreviewTrack(window_id, track);
}

void ECBaseManager::DestroyVideoTrack(void* track) {
  RTC_LOG(INFO) << " begin..." ;
  webrtc::VideoTrackInterface* ptrack = (webrtc::VideoTrackInterface*)track;
  MediaClient::GetInstance()->DestroyLocalVideoTrack(ptrack);
}

void ECBaseManager::DestroyAudioTrack(void* track) {
  RTC_LOG(INFO) << " begin...";
  webrtc::AudioTrackInterface* ptrack = (webrtc::AudioTrackInterface*)track;
  MediaClient::GetInstance()->DestroyLocalAudioTrack(ptrack);
}

bool ECBaseManager::SelectVideoSource(
    const char* tid,
    int channelid,
    const std::string& track_id,
    void* video_track,
    const std::vector<std::string>& stream_ids) {
  RTC_LOG(INFO) << " begin..."
                << "channelid:" << channelid
                << "video_track:" << video_track;
  bool bOk = false;
  webrtc::VideoTrackInterface* track =
      (webrtc::VideoTrackInterface*)video_track;
  bOk =
      MediaClient::GetInstance()->SelectVideoSource(channelid, track_id, track);
  RTC_LOG(INFO) << " end..." << "bOK:" << bOk;
  return bOk;
}

bool ECBaseManager::SelectVoiceSource(
    const char* tid,
    int channelid,
    const std::string& track_id,
    void* audio_track,
    const std::vector<std::string>& stream_ids) {
  RTC_LOG(INFO) << " begin..."
                << "channelid:" << channelid
                << "audio_track:" << audio_track;
  bool bOk = false;
  webrtc::AudioTrackInterface* track =
      (webrtc::AudioTrackInterface*)audio_track;
  bOk =
      MediaClient::GetInstance()->SelectVoiceSource(channelid, track_id, track);
  RTC_LOG(INFO) << " end..." << "bOK:" << bOk;
  return bOk;
}

void* ECBaseManager::CreateAudioTrack(const char* track_id, int voice_index) {
  RTC_LOG(INFO) << " begin..."
                << "voice_index:" << voice_index;
  return (void*)MediaClient::GetInstance()->CreateLocalVoiceTrack(track_id);
}

bool ECBaseManager::SetLocalMute(int channel_id, bool bMute) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "bMute:" << bMute;
  return MediaClient::GetInstance()->SetLocalMute(channel_id, bMute);
}

bool ECBaseManager::SetRemoteMute(int channel_id, bool bMute) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "bMute:" << bMute;
  return MediaClient::GetInstance()->SetRemoteMute(channel_id, bMute);
}

bool ECBaseManager::RequestRemoteSsrc(int channel_id, int32_t ssrc) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id << "ssrc:" << ssrc;
  return MediaClient::GetInstance()->RequestRemoteSsrc(channel_id, ssrc);
}

bool ECBaseManager::GetVideoCodecs(char* jsonVideoCodecInfos, int* length) {
  RTC_LOG(INFO) << " begin...";
  MediaClient::GetInstance()->Initialize();
  return MediaClient::GetInstance()->GetVideoCodecs(jsonVideoCodecInfos,
                                                    length);
}

bool ECBaseManager::GetAudioCodecs(char* jsonAudioCodecInfos, int* length) {
  RTC_LOG(INFO) << " begin...";
  MediaClient::GetInstance()->Initialize();
  return MediaClient::GetInstance()->GetAudioCodecs(jsonAudioCodecInfos,
                                                    length);
}

bool ECBaseManager::SetVideoNackStatus(const int channelId,
                                       const bool enable_nack) {
  RTC_LOG(INFO) << " begin..."
                << "channelId:" << channelId << "enable_nack:" << enable_nack;
  return MediaClient::GetInstance()->SetVideoNackStatus(channelId, enable_nack);
}

bool ECBaseManager::SetVideoUlpFecStatus(const int channelId,
                                         const bool enable,
                                         const uint8_t payloadtype_red,
                                         const uint8_t payloadtype_fec) {
  RTC_LOG(INFO) << " begin..."
                << "channelId:" << channelId << "enable:" << enable
                << "payloadtype_red:" << payloadtype_red
                << "payloadtype_fec:" << payloadtype_fec;
  return MediaClient::GetInstance()->SetVideoUlpFecStatus(
      channelId, enable, payloadtype_red, payloadtype_fec);
}

bool ECBaseManager::SetVideoDegradationMode(
    const int channelId,
    const webrtc::DegradationPreference mode) {
  RTC_LOG(INFO) << " begin..."
                << "channelId:" << channelId << "mode:" << mode;
  return MediaClient::GetInstance()->SetVideoDegradationMode(channelId, mode);
}

bool ECBaseManager::SendKeyFrame(const int channelId) {
  RTC_LOG(INFO) << " begin..."
                << "channelId:" << channelId;
  return MediaClient::GetInstance()->SendKeyFrame(channelId);
}

bool ECBaseManager::SetKeyFrameRequestCallback(const int channelId, void* cb) {
  RTC_LOG(INFO) << " begin..."
                << "channelId:" << channelId << "cb:" << cb;
  OnRequestKeyFrameCallback callback = (OnRequestKeyFrameCallback)cb;
  return MediaClient::GetInstance()->SetKeyFrameRequestCallback(channelId,
                                                                callback);
}

bool ECBaseManager::SetAEC(bool enable) {
  RTC_LOG(INFO) << " begin..."
                << "enable:" << enable;
  return MediaClient::GetInstance()->SetAEC(enable);
}

bool ECBaseManager::SetAGC(bool enable) {
  RTC_LOG(INFO) << " begin..."
                << "enable:" << enable;
  return MediaClient::GetInstance()->SetAGC(enable);
}

bool ECBaseManager::SetNS(bool enable) {
  RTC_LOG(INFO) << " begin..."
                << "enable:" << enable;
  return MediaClient::GetInstance()->SetNS(enable);
}

void* ECBaseManager::CreateAudioDevice() {
  RTC_LOG(INFO) << " begin...";
  return (void*)MediaClient::GetInstance()->CreateAudioDevice();
}

bool ECBaseManager::SetAudioRecordingVolume(uint32_t vol) {
  RTC_LOG(INFO) << " begin..." << "vol:" << vol;
  return MediaClient::GetInstance()->SetAudioRecordingVolume(vol);
}

bool ECBaseManager::GetAudioDeviceList(char* json, int* len) {
  RTC_LOG(INFO) << " begin..."
                << "json:" << json << "len:" << len;
  return MediaClient::GetInstance()->GetAudioDeviceList(json, len);
}

bool ECBaseManager::SetAudioRecordingDevice(int index) {
  RTC_LOG(INFO) << " begin..."
                << "index:" << index;
  return MediaClient::GetInstance()->SetAudioRecordingDevice(index);
}
int ECBaseManager::NumOfCaptureDevices() {
  RTC_LOG(INFO) << " begin...";
  return MediaClient::GetInstance()->GetNumberOfVideoDevices();
}
bool ECBaseManager::GetVideoDevices(char* devices, int* len) {
  RTC_LOG(INFO) << " begin..."
                << "devices:" << devices << "len:" << len;
  return MediaClient::GetInstance()->GetVideoDevices(devices, len);
}
    /***************************************************************************************/
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
rtc::scoped_refptr<ECPeerManager> ECBaseManager::GetPeerManagerById(int id) {
  RTC_LOG(INFO) << " begin..." << "id:" << id;
  auto it = peer_managers_.find(id);
  return (it == peer_managers_.end()) ? nullptr : it->second.get();
}

rtc::scoped_refptr<ECPeerManager> ECBaseManager::GetFirstPeerManager() const {
  RTC_LOG(INFO) << " begin...";
  for (auto it = peer_managers_.begin(); it != peer_managers_.end(); ++it) {
    return it->second.get();
  }
  RTC_LOG(INFO) << " end...";
  return nullptr;
}

/***************************camera************************/

int ECBaseManager::NumOfCapabilities(const char* id, int id_len) {
  RTC_LOG(INFO) << " begin..."
                << "id:" << id << "id_len:" << id_len;
  RTC_DCHECK(vcm_device_info_);
  RTC_LOG(INFO) << " end...";
  return vcm_device_info_->NumberOfCapabilities(id);
}

int ECBaseManager::GetCaptureDevice(int index,
                                    char* device_name,
                                    int name_len,
                                    char* unique_name,
                                    int id_len) {
  RTC_LOG(INFO) << " begin..."
                << "index:" << index << "device_name:" << device_name
                << "name_len:" << name_len << "unique_name:" << unique_name
                << "id_len:" << id_len;
  RTC_DCHECK(vcm_device_info_);
  return vcm_device_info_->GetDeviceName(index, device_name, name_len,
                                         unique_name, id_len);
}

int ECBaseManager::GetCaptureCapabilities(const char* id,
                                          int index,
                                          webrtc::VideoCaptureCapability& cap) {
  RTC_LOG(INFO) << " begin..."
                << "id:" << id << "index:" << index
                << "cap height:" << cap.height << "cap width:" << cap.width
                << "cap maxFPS:" << cap.maxFPS;
  RTC_DCHECK(vcm_device_info_);
  return vcm_device_info_->GetCapability(id, index, cap);
}

// note: must call from ui thread
int ECBaseManager::AllocateCaptureDevice(const char* id,
                                         int len,
                                         int& deviceid) {
  RTC_LOG(INFO) << " begin..."
                << "id:" << id << "len:" << len << "deviceid:" << deviceid;
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
  RTC_LOG(INFO) << " end...";
  return 0;
}

// note: must call from ui thread
int ECBaseManager::ConnectCaptureDevice(int deviceid, int peer_id) {
  RTC_LOG(INFO) << " begin..."
                << "deviceid:" << deviceid << "peer_id:" << peer_id;
  RTC_DCHECK_GE(deviceid, 0);
  auto it = camera_devices_.find(deviceid);
  if (it != camera_devices_.end()) {
    VideoCapturer* video_device = it->second.second;

    rtc::scoped_refptr<CapturerTrackSource> capTrack(
        new rtc::RefCountedObject<CapturerTrackSource>(false, video_device));

    // rtc::scoped_refptr<webrtc::VideoTrackInterface> local_video_track(
    //    webrtc::VideoTrack::Create(kVideoLabel, capTrack,
    //                               worker_thread_.get()));

    // local_video_track_ = local_video_track;

    // add videoTrackSource to pc
    // RTC_DCHECK(peer_manager_);
    rtc::scoped_refptr<ECPeerManager> peer_manager =
        GetPeerManagerById(peer_id);
    if (!peer_manager) {

      return -1;
    }
    local_video_track_ = peer_manager->AddVideoTrack(capTrack);
    RTC_LOG(INFO) << " end...";
    return 0;
  } else {
    RTC_LOG(LS_ERROR) << "Can't find deviceid.";
    return -1;
  }
}

// note: must call from ui thread
int ECBaseManager::StartCameraCapturer(int deviceid,
                                       webrtc::VideoCaptureCapability& cap) {
  RTC_LOG(INFO) << " begin..."
                << "deviceid:" << deviceid
                << "cap height:" << cap.height << "cap width:" << cap.width
                << "cap maxFPS:" << cap.maxFPS;
  // RTC_DCHECK(signaling_thread_->IsCurrent());
  RTC_DCHECK_GE(deviceid, 0);

  return camera_devices_[deviceid].second->StartCapture(cap);
}

// note: must call from ui thread
int ECBaseManager::StopCapturer(int deviceid) {
  RTC_LOG(INFO) << " begin..."
                << "deviceid:" << deviceid;
  // RTC_DCHECK_GE(deviceid, 0);

  if (camera_devices_.empty()) {
    return 0;
  }
  RTC_LOG(INFO) << " end...";
  return camera_devices_[deviceid].second->StopCapture();
}

int ECBaseManager::StopAllCapturer() {
  RTC_LOG(INFO) << " begin...";
  int ret = 0;

  if (camera_devices_.empty()) {
    return 0;
  }

  for (auto camera : camera_devices_) {
    ret = camera.second.second->StopCapture();
  }

  RTC_LOG(INFO) << " end...";
  return ret;
}

bool ECBaseManager::StartConnectChannel(int audio_channel_id,
                                        int video_channel_id) {
  RTC_LOG(INFO) << " begin..."
                << "audio_channel_id:" << audio_channel_id
                << "video_channel_id:" << video_channel_id;
  bool bOk = false;
  //  MediaClient::GetInstance()->StartConnect(audio_channel_id,
  //  video_channel_id);
  return true;
  /* if (m_pMediaClient) {
    bOk = m_pMediaClient->StartConnect(audio_channel_id, video_channel_id);
   }*/
  RTC_LOG(INFO) << " end...";
  return bOk;
}

int ECBaseManager::StopLocalRender(int peer_id, int deviceid) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id << "deviceid:" << deviceid;
  if (!local_renderer_) {
    return 0;
  }
  RTC_LOG(INFO) << " end...";
  return local_renderer_->StopRender(0);
}

int ECBaseManager::StopRemoteRender(int peer_id, int deviceid) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id << "deviceid:" << deviceid;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetPeerManagerById(peer_id);
  if (!peer_manager) {
    return 0;
  }
  RTC_LOG(INFO) << " end...";
  return peer_manager->StopRemoteRender();
}

// int ECPeerManager::CreateChannel(int& peer_id, bool is_video) {
//  if (is_video) {
//    // AddVideoTrack When ConnectCaptureDevice
//  } else if (!is_video) {
//    peer_manager_->AddAudioTrack();
//  }
//  return 0;
//}

/**************************auido capture*******************************/
// note: must call from ui thread
int ECBaseManager::StartMicCapture(int peer_id) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetPeerManagerById(peer_id);
  if (!peer_manager) {
    return -1;
  }
  peer_manager->AddAudioTrack();
  RTC_LOG(INFO) << " end...";
  return 0;
}

/***************************A/V send recerve************************/
int ECBaseManager::StartSendRecv(int peer_id) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetPeerManagerById(peer_id);
  if (!peer_manager) {
    return -1;
  }

  peer_manager->SetLocalAndRemoteDescription();
  RTC_LOG(INFO) << " end...";
  return 0;
}

int ECBaseManager::AudioStartReceive(int peer_id) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id;
  // RTC_DCHECK(peer_manager_);
  // peer_manager_->SetLocalAndRemoteDescription();
  return 0;
}

int ECBaseManager::AudioStartSend(int peer_id) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id;
  return 0;
}

int ECBaseManager::VideoStartReceive(int peer_id) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id;
  // peer_manager_->SetLocalAndRemoteDescription();
  return 0;
}

int ECBaseManager::VideoStartSend(int peer_id) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetPeerManagerById(peer_id);
  if (!peer_manager) {
    return -1;
  }
  peer_manager->SetLocalAndRemoteDescription();
  RTC_LOG(INFO) << " end...";
  return 0;
}

/***************************codec************************/
void ECBaseManager::GetAudioCodecs(cricket::AudioCodecs* audio_codecs) const {
  RTC_LOG(INFO) << " begin..."
                << "audio_codecs:" << audio_codecs;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetFirstPeerManager();
  if (!peer_manager) {
    return;
  }
  peer_manager->GetAudioCodecs(audio_codecs);
  RTC_LOG(INFO) << " end...";
}

void ECBaseManager::GetVideoCodecs(cricket::VideoCodecs* video_codecs) const {
  RTC_LOG(INFO) << " begin..."
                << "video_codecs:" << video_codecs;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetFirstPeerManager();
  if (!peer_manager) {
    return;
  }
  peer_manager->GetVideoCodecs(video_codecs);
  RTC_LOG(INFO) << " end...";
}

void ECBaseManager::SetSendCodecVideo(cricket::VideoCodec* video_codec) {
  RTC_LOG(INFO) << " begin..."
                << "video_codec:" << video_codec;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetFirstPeerManager();
  if (!peer_manager) {
    return;
  }
  peer_manager->SetSendCodecVideo(video_codec);
  RTC_LOG(INFO) << " end...";
}

void ECBaseManager::SetReceiveCodecVideo(int peer_id,
                                         cricket::VideoCodec* video_codec) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id << "video_codec:" << video_codec;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetPeerManagerById(peer_id);
  if (!peer_manager) {
    return;
  }

  peer_manager->SetReceiveCodecVideo(video_codec);
  RTC_LOG(INFO) << " end...";
}

void ECBaseManager::SetSendCodecAudio(cricket::AudioCodec* audio_codec) {
  RTC_LOG(INFO) << " begin..."
                << "audio_codec:" << audio_codec;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetFirstPeerManager();
  if (!peer_manager) {
    return;
  }

  peer_manager->SetSendCodecAudio(audio_codec);
  RTC_LOG(INFO) << " end...";
}

void ECBaseManager::SetReceiveCodecAudio(int peer_id,
                                         cricket::AudioCodec* audio_codec) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id
                << "audio_codec:" << audio_codec;
  rtc::scoped_refptr<ECPeerManager> peer_manager = GetPeerManagerById(peer_id);
  if (!peer_manager) {
    return;
  }

  peer_manager->SetReceiveCodecAudio(audio_codec);
  RTC_LOG(INFO) << " end...";
}

/***************************ip port************************/
void ECBaseManager::SetSendDestination(int peer_id,
                                       const char* r_addr,
                                       const char* l_addr,
                                       int vrtp_port) {
  RTC_LOG(INFO) << " begin..."
                << "peer_id:" << peer_id << "r_addr:" << r_addr
                << "l_addr:" << l_addr << "vrtp_port:" << vrtp_port;
  // MediaClient::GetInstance()->SetSocketAddress(l_addr, r_addr, 7078,9078);
  // MediaClient::GetInstance()->SetSendDestination(peer_id, r_addr, l_addr,
  //                                                vrtp_port);
  // if (m_pMediaClient) {
  //	m_pMediaClient->SetSocketAddress(l_addr, r_addr, 7078, 9078);
  // m_pMediaClient->SetSendDestination(peer_id,r_addr,l_addr,vrtp_port);
  //}

  /* rtc::scoped_refptr<ECPeerManager> peer_manager =
       GetPeerManagerById(peer_id);
   if (!peer_manager) {
     return;
   }

   peer_manager->SetSendDestination(rtp_addr, rtp_port);*/
}

/*void ECBaseManager::AddTracks() {
  if (m_pMediaClient) {
    m_pMediaClient->AddTracks();
  }
}*/
bool ECBaseManager::SetSendSsrcVideo(const std::string& settings,
                                     int channel_id) {
  RTC_LOG(INFO) << " begin..."
                << "channel_id:" << channel_id;
  return MediaClient::GetInstance()->SetRemoteSsrcAfterCreatedVideoChannel(settings,
                                                                    channel_id);
}

bool ECBaseManager::SetSendSsrcAudio(const std::string& settings,
                                     int channel_id) {
  RTC_LOG(INFO) << " begin..."
                << "settings:" << settings << "channel_id:" << channel_id;
  return MediaClient::GetInstance()->SetRemoteSsrcAfterCreatedAudioChannel(settings,
                                                                    channel_id);
}
bool ECBaseManager::SetRemoteSsrcAfterSelectAudioSource(int channelId) {
  RTC_LOG(INFO) << " begin..."
                << "channelId:" << channelId;
 return MediaClient::GetInstance()->SetRemoteSsrcAfterSelectAudioSource(
      channelId);
}

bool ECBaseManager::SetRemoteSsrcAfterSelectVideoSource(int channelId) {
  RTC_LOG(INFO) << " begin..."
                << "channelId:" << channelId;
 return MediaClient::GetInstance()->SetRemoteSsrcAfterSelectVideoSource(
      channelId);
}
}  // namespace webrtc
