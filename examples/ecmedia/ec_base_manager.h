#ifndef ECMEDIA_IMPL_H
#define ECMEDIA_IMPL_H

#include <map>
#include <memory>
#include <thread>

#include "modules/video_capture/video_capture.h"
#include "pc/session_description.h"

#include "api/rtc_error.h"
#include "api/rtp_sender_interface.h"
#include "api/rtp_transceiver_interface.h"
#include "ec_peer_manager.h"
#include "ec_log.h"
#include "ec_peer_config.h"
#include "pc/rtp_transceiver.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/synchronization/sequence_checker.h"
#include "rtc_base/thread.h"
#include "video_capturer/video_capturer.h"
#include "video_render/video_renderer.h"
#ifdef WIN32
#include "rtc_base/win32_socket_init.h"
#endif
#ifdef WEBRTC_ANDROID
#include <jni.h>
#endif

namespace webrtc {


// ECBaseManager is singleton
class ECBaseManager : public sigslot::has_slots<> {
 public:
  static ECBaseManager* GetInstance();
  static void DestroyInstance();

  ~ECBaseManager();
  void Init();
  bool SetTrace(const char* path, const int level);

  bool GenerateChannelId(int& id);

  bool ReleaseChannelId(int id);

  bool CreateTransport(const char* l_addr,
                       int l_port,
                       const char* r_addr,
                       int r_port,
                       const char* id);

  bool CreateChannel(const std::string& settings,
                     int channel_id,
                     bool is_video = true);

  void DestroyChannel(int channel_id, bool is_video);

  bool StartChannel(int channel_id);

  bool StopChannel(int channel_id, bool is_video = true);

  bool StopConnectChannel();

  bool AddLocalRender(int window_id, void* video_window);

  bool AddRemoteRender(int peer_id, void* video_window);

  bool SetVideoLocalSsrc(int peer_id, unsigned int ssrc);

  bool SetVideoRemoteSsrc(int peer_id, unsigned int ssrc);

  bool SetAudioLocalSsrc(int peer_id, unsigned int ssrc);

  bool SetAudioRemoteSsrc(int peer_id, unsigned int ssrc);

  void* CreateAudioTrack(const char* track_id, int voice_index);

  void DestroyAudioTrack(void* track);

  void* CreateVideoTrack(std::string& track_params);

  void DestroyVideoTrack(void* track);

  bool PreviewTrack(int window_id, void* track);

  bool SelectVideoSource(const char* tid,
                         int channelid,
                         const std::string& track_id,
                         void* video_track,
                         const std::vector<std::string>& stream_ids);

  bool SelectVoiceSource(const char* tid,
                         int channelid,
                         const std::string& track_id,
                         void* video_track,
                         const std::vector<std::string>& stream_ids);

  bool SetLocalMute(int channel_id, bool bMute);

  bool SetRemoteMute(int channel_id, bool bMute);

  bool RequestRemoteSsrc(int channel_id, int32_t ssrc);

  bool GetVideoCodecs(char* jsonVideoCodecInfos, int* length);

  bool GetAudioCodecs(char* jsonAudioCodecInfos, int* length); 

  bool SetVideoNackStatus(const int channelId, const bool enable_nack);

  bool SetVideoUlpFecStatus(const int channelId,const bool enable,const uint8_t payloadtype_red, const uint8_t payloadtype_fec);

  bool SetVideoDegradationMode(const int channelId, const webrtc::DegradationPreference mode);

  bool SendKeyFrame(const int channelId);

  bool SetKeyFrameRequestCallback(const int channelId, void* cb);

  bool SetAEC(bool enable);

  bool SetAGC(bool a);

  bool SetNS(bool a);

  void* CreateAudioDevice();

  bool SetAudioRecordingVolume(uint32_t vol);

  char* GetAudioDeviceList(int* len);

  bool SetAudioRecordingDevice(int index);
  bool SetAudioRecordingDeviceOnFlight(int index);
  bool SetAudioPlayoutDeviceOnFlight(int index);

  bool SetAudioPlayoutDevice(int index);

  bool GetVideoDevices(char* devices, int* len);
 ////////////////////////////////////////////////////////////////////////////////////////
  /*************************************************************************************/


  int CreateMediaEngine();
  int CreateChannelManager();
  int DeleteBaseManager(int peer_id);
  int NumOfCaptureDevices();
  int NumOfCapabilities(const char* id, int id_len);
  int GetCaptureDevice(int index,
                       char* name,
                       int name_len,
                       char* id,
                       int id_len);
  int GetCaptureCapabilities(const char* id,
                             int index,
                             webrtc::VideoCaptureCapability& cap);
  int AllocateCaptureDevice(const char* id, int len, int& deviceid);
  int ConnectCaptureDevice(int deviceid, int peer_id);
  int StartCameraCapturer(int deviceid, webrtc::VideoCaptureCapability& cap);
  int StopCapturer(int deviceid);
  int StopAllCapturer();
  int StopLocalRender(int peer_id, int deviceid);
  int StopRemoteRender(int peer_id, int deviceid);


  bool StartConnectChannel(int audio_channel_id, int video_channel_id);

  // int ResetRemoteView(int peer_id, void* video_window);
  // int AudioStartPlayout(int peer_id);
  // int AudioStopPlayout(int peer_id);

  int StartMicCapture(int peer_id);

  int StartSendRecv(int peer_id);

  int AudioStartReceive(int peer_id);
  // int AudioStopReceive(int peer_id);
  int AudioStartSend(int peer_id);
  // int AudioStopSend(int peer_id);
  int VideoStartReceive(int peer_id);
  // int VideoStopReceive(int peer_id);
  int VideoStartSend(int peer_id);
  // int VideoStopSend(int peer_id);

  // int SetSendCodecVideo(int peer_id, webrtc::VideoCodec& videoCodec);
  // int GetSendCodecVideo(int peer_id, webrtc::VideoCodec& videoCodec);
  // int SetReceiveCodecVideo(int peer_id, webrtc::VideoCodec& videoCodec);
  // int GetReceiveCodecVideo(int peer_id, webrtc::VideoCodec& videoCodec);

  // int get_send_codec_audio(int peer_id, webrtc::CodecInst& audioCodec);
  // int set_send_codec_audio(int peer_id, webrtc::CodecInst& audioCodec);
  // int set_receive_playloadType_audio(int peer_id,
  //                                   webrtc::CodecInst& audioCodec);
  // int get_receive_playloadType_audio(int peer_id,
  //                                   webrtc::CodecInst& audioCodec);

  rtc::Thread* signaling_thread() const { return signaling_thread_.get(); }
  rtc::Thread* worker_thread() const { return worker_thread_.get(); }



  void GetAudioCodecs(cricket::AudioCodecs* audio_codecs) const;
  void GetVideoCodecs(cricket::VideoCodecs* video_codecs) const;

  void SetSendCodecVideo(cricket::VideoCodec* video_codec);
  void SetReceiveCodecVideo(int peer_id, cricket::VideoCodec* video_codec);
  void SetSendCodecAudio(cricket::AudioCodec* audio_codec);
  void SetReceiveCodecAudio(int peer_id, cricket::AudioCodec* audio_codec);

  void SetSendDestination(int peer_id,
                          const char* r_addr,
                          const char* l_addr,
                          int vrtp_port);

#if defined(WEBRTC_ANDROID)
  bool SaveLocalVideoTrack(int channelId, void* track);
  void* GetLocalVideoTrackPtr(int channelId);
  bool RemoveLocalVideoTrack(int channelId);

  bool SaveRemoteVideoSink(int channelId, JNIEnv* env, jobject javaSink);
  bool RemoveRemoteVideoSink(int channelId);

  int InitializeJVM();

#endif

 private:
  typedef std::pair<std::string, VideoCapturer*> UniqueIdVideoCapturerPair;
  class FindUniqueId {
   public:
    FindUniqueId(std::string unique_id) : unique_id_(unique_id) {}
    bool operator()(
        std::map<int, UniqueIdVideoCapturerPair>::value_type& pair) {
      return pair.second.first == unique_id_;
    }

   private:
    std::string unique_id_;
  };

  rtc::scoped_refptr<ECPeerManager> GetPeerManagerById(int id);
  rtc::scoped_refptr<ECPeerManager> GetFirstPeerManager() const;

  // Internal implementation for AddTransceiver family of methods. If
  // |fire_callback| is set, fires OnRenegotiationNeeded callback if
  // successful.
  RTCErrorOr<rtc::scoped_refptr<RtpTransceiverInterface>> AddTransceiver(
      cricket::MediaType media_type,
      rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
      const webrtc::RtpTransceiverInit& init,
      bool fire_callback = true) RTC_RUN_ON(signaling_thread());

  // Return the RtpSender with the given id, or null if none exists.
  rtc::scoped_refptr<RtpSenderProxyWithInternal<RtpSenderInternal>>
  FindSenderById(const std::string& sender_id) const
      RTC_RUN_ON(signaling_thread());

  rtc::scoped_refptr<RtpSenderInterface> CreateSender(
      const std::string& kind,
      const std::string& stream_id);

  rtc::scoped_refptr<RtpReceiverProxyWithInternal<RtpReceiverInternal>>
  CreateReceiver(cricket::MediaType media_type, const std::string& receiver_id);

  static rtc::CriticalSection getIns_crit_;
  static ECBaseManager* g_instance;
  ECBaseManager();
  ECBaseManager(ECBaseManager&) = delete;
  ECBaseManager& operator=(ECBaseManager&) = delete;

  

  rtc::scoped_refptr<webrtc::VideoTrackInterface> local_video_track_;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> remote_video_track_;

  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> vcm_device_info_;
  std::unique_ptr<VideoCapturer> local_camera_;
  std::unique_ptr<VideoRenderer> local_renderer_;

  //std::unique_ptr<VideoRenderer> remote_renderer_;  // TODO: modify to vector
  // void* remote_window_;  // TODO: modify to vector
  void* local_window_;
  bool is_call_add_remote_render_;  // TODO: yuck, need fix.

  std::map<int, UniqueIdVideoCapturerPair> camera_devices_;

  std::unique_ptr<rtc::Thread> signaling_thread_;
  std::unique_ptr<rtc::Thread> worker_thread_;
  std::unique_ptr<rtc::Thread> network_thread_;
  
  std::map<uint32_t, rtc::scoped_refptr<ECPeerManager>> peer_managers_;
  uint32_t peer_manager_ids_;
  std::map<uint32_t, std::unique_ptr<ECPeerConfig>> peer_config_m_;



  std::vector<
      rtc::scoped_refptr<RtpTransceiverProxyWithInternal<RtpTransceiver>>>
      transceivers_;  // TODO(bugs.webrtc.org/9987): Accessed on both signaling
                      // and network thread.

#ifdef WIN32
  rtc::WinsockInitializer winsock_init;
#endif

  std::unique_ptr<ECLog> ec_log_;
};

}  // namespace webrtc

#endif
