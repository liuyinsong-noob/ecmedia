
#ifndef MEDIA_CLIENT_H_
#define MEDIA_CLIENT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "api/media_stream_interface.h"
#include "api/media_transport_interface.h"
#include "api/peer_connection_interface.h"
#include "api/rtp_parameters.h"
#include "api/scoped_refptr.h"

#include "logging/rtc_event_log/rtc_event_log.h"

#include "pc/audio_rtp_receiver.h"
#include "pc/channel_manager.h"
#include "pc/dtls_srtp_transport.h"
#include "pc/general_transport_controller.h"
#include "pc/media_session.h"
#include "pc/rtc_stats_collector.h"
#include "pc/rtc_stats_traversal.h"
#include "pc/rtp_receiver.h"
#include "pc/rtp_transceiver.h"
#include "pc/rtp_transport.h"
#include "pc/stats_collector.h"
#include "pc/stream_collection.h"
#include "pc/video_rtp_receiver.h"

#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/thread.h"

#include "media/base/stream_params.h"
#include "media/sctp/sctp_transport_internal.h"

#include "sdk/ecmedia/video_renderer.h"
#include "sdk/ecmedia/win/video_capturer/video_capturer.h"
#include "media/base/video_common.h"
#include "media/engine/webrtc_video_engine.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "modules/video_capture/video_capture.h"
#include "sdk/ecmedia/render_manager.h"

#include "ec_log.h"

#ifdef WEBRTC_ANDROID
#include <jni.h>
#endif

#include "media/base/adapted_video_track_source.h"
#include "sdk_common.h"
//#include "third_party/protobuf/src/google/protobuf/message_lite.h"

// class myclass : public ::google::protobuf::MessageLite {

//};
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*OnDetroyChannel)(int channel_id);
#ifdef __cplusplus
}
#endif
namespace win_desk {
#if defined(WEBRTC_WIN)
class ECDesktopCapture : public rtc::AdaptedVideoTrackSource,
                         public rtc::MessageHandler,
                         public webrtc::DesktopCapturer::Callback {
 public:
  ~ECDesktopCapture();
  static rtc::scoped_refptr<ECDesktopCapture> Create(
      int type);  // type=0 screen  type=1 window

  const char* GetCaptureSources(webrtc::DesktopCapturer::SourceList& source);

  int SetDesktopSourceID(int index);

  void OnCaptureResult(
      webrtc::DesktopCapturer::Result result,
      std::unique_ptr<webrtc::DesktopFrame> desktopframe) override;

  bool is_screencast() const override;

  absl::optional<bool> needs_denoising() const override;

  webrtc::MediaSourceInterface::SourceState state() const override;
  void OnMessage(rtc::Message* msg) override;

  bool remote() const override;
  void Start();
  void Stop();
  void CaptureFrame();
  bool GetCaptureState();

 public:
  bool isStartCapture;

 protected:
  explicit ECDesktopCapture(std::unique_ptr<webrtc::DesktopCapturer> capturer);

  webrtc::DesktopCapturer::SourceList sources;

 protected:
  std::vector<int> sources_id_;

 private:
  std::unique_ptr<webrtc::DesktopCapturer> capturer_;
};
#endif
}  // namespace win_desk

//namespace win_render {
//#if defined(WEBRTC_WIN)

//class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
// public:
//  VideoRenderer(HWND wnd,
//                int mode,
//                int width,
//                int height,
//                webrtc::VideoTrackInterface* track_to_render);
//  virtual ~VideoRenderer();
//
//  void Lock() { ::EnterCriticalSection(&buffer_lock_); }
//
//  void Unlock() { ::LeaveCriticalSection(&buffer_lock_); }
//
//  // VideoSinkInterface implementation
//  void OnFrame(const webrtc::VideoFrame& frame) override;
//  void Paint();
//  bool UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render);
//
//  const BITMAPINFO& bmi() const { return bmi_; }
//  const uint8_t* image() const { return image_.get(); }
//  HWND handle() const { return wnd_; }
//  //add by ytx_wx begin...
//  bool GetisLocal (){ return isLocal_;}
//  void SetLocal(bool islocal) { isLocal_ = islocal; }
//  //add by ytx_wx_end ...
// protected:
//  void SetSize(int width, int height);
//
//  enum {
//    SET_SIZE,
//    RENDER_FRAME,
//  };
//
// private:
//  HWND wnd_;
//  bool isLocal_ = false;  // add by ytx_wx
//  HDC hDC_;
//  BITMAPINFO bmi_;
//  std::unique_ptr<uint8_t[]> image_;
//  CRITICAL_SECTION buffer_lock_;
//  rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
//  int mode_;
//};

//class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
// public:
//  VideoRenderer(HWND wnd,
//                int mode,
//                int width,
//                int height,
//                webrtc::VideoTrackInterface* track_to_render);
//  virtual ~VideoRenderer();
//
//  void Lock() { ::EnterCriticalSection(&buffer_lock_); }
//
//  void Unlock() { ::LeaveCriticalSection(&buffer_lock_); }
//
//  // VideoSinkInterface implementation
//  void OnFrame(const webrtc::VideoFrame& frame) override;
//  void Paint(uint32_t ts);
//  bool UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render);
//
//  const BITMAPINFO& bmi() const { return bmi_; }
//  const uint8_t* image() const { return image_.get(); }
//  HWND handle() const { return wnd_; }
//
// protected:
//  void SetSize(int width, int height);
//
//  enum {
//    SET_SIZE,
//    RENDER_FRAME,
//  };
//
// private:
//  HWND wnd_;
//  HDC hDC_;
//  BITMAPINFO bmi_;
//  std::unique_ptr<uint8_t[]> image_;
//  CRITICAL_SECTION buffer_lock_;
//  rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
//  int mode_;
//
//  webrtc::Clock* clock_;
//};

// A little helper class to make sure we always to proper locking and
// unlocking when working with VideoRenderer buffers.
//template <typename T>
//class AutoLock {
// public:
//  explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
//  ~AutoLock() { obj_->Unlock(); }
//
// protected:
//  T* obj_;
//};

//}


namespace ecmedia_sdk {

#define EC_CHECK_VALUE(v, r) \
  if (!v)                    \
    return r;

class TransportControllerObserve;
class ChannelGenerator;
enum VIDEO_SOURCE_TYPE { VIDEO_CAMPER = 0, VIDEO_SCREEN, VIDEO_EXTERNAL };
typedef void(ECMedia_ConferenceParticipantCallback)(uint32_t arrOfCSRCs[],
                                                    int count);

#ifdef __cplusplus
extern "C" {
#endif
#if defined(WEBRTC_ANDROID)
typedef int (*OnGetVideoHardwareEncoderFactory)(void** env,
                                                void** jencoder_factory,
                                                void** jdecoder_factory);
typedef int (*OnGetAudioHardwareEncoderFactoryAndAdm)(void** jencoder_factory,
                                                      void** jdecoder_factory,
                                                      void** adm);
#endif

#ifdef __cplusplus
}
#endif

class MediaClient : public sigslot::has_slots<> {
 public:
  enum AV_TYPE {
    AV_NONE = 0,
    AV_AUDIO = 1 << 0,
    AV_VIDEO = 1 << 1,
    AV_AUDIO_VIDEO = AV_AUDIO | AV_VIDEO,
    AV_MAX
  };
  enum STATUS_CONNECT {
    SC_UNCONNECTED = 0,
    SC_CONNECTING = 1 << 0,
    SC_DISCONNECTING = 1 << 1,
    SC_CONNECTED = 1 << 2,
    SC_MAX
  };

  enum class VideoCodecMode { kRealtimeVideo, kScreensharing };

  // Video codec types
  enum VideoCodecType {
    // There are various memset(..., 0, ...) calls in the code that rely on
    // kVideoCodecGeneric being zero.
    kVideoCodecGeneric = 0,
    kVideoCodecVP8,
    kVideoCodecVP9,
    kVideoCodecH264,
    kVideoCodecMultiplex,
  };

  enum class FecMechanism {
    RED,
    RED_AND_ULPFEC,
    FLEXFEC,
  };

  struct VideoDeviceConfig {
    int width = 640;
    int height = 480;
    int deviceIndex = 0;
    int maxFramerate = 30;
    int videoSourceType = 0;  // 0 is kRealtimeVideo, 1 is kScreensharing.
    std::string trackId;
    std::string transportId;
  };

  struct AudioCodecConfig {
    int channels = 1;
    int payloadType = 0;
    int codecType = 0;
    int clockRate = 1600;
    int minBitrateKps = 10;
    int startBitrateKps = 30;
    int maxBitrateKps = 50;
    std::string codecName = "opus";
    std::string trackId;
    std::string transportId;
  };

  struct VideoStreamConfig {
    std::string name = "H264";
    int payloadType = 96;
    int new_payloadType = 104;
    int associated_payloadType = new_payloadType - 1;
  };

  struct VideoCodecConfig {
    bool red = true;
    bool nack = true;
    bool isScreenShare = false;
	bool isSimulcast = false;
	bool isRecvOnly = false;
    int fecType = 1;  // red and ulp fec
    int fecPayload = 97;
    int rtx = 1;
    int rtxPayload = 97;
    int payloadType = 96;
    int width = 640;
    int height = 480;
    int minBitrateKps = 300;
    int startBitrateKps = 600;
    int maxBitrateKps = 1700;
    int maxFramerate = 15;
    int codecType = 1;  // kVideoCodecVP8;
    int maxQp = 56;
    std::vector<VideoStreamConfig> video_stream_configs;
    std::string codecName = "VP8";
    std::string trackId;
    std::string transportId;
  };

  friend class TransportControllerObserve;

 public:
  static MediaClient* GetInstance();
  static void DestroyInstance();

  virtual ~MediaClient();

  static bool SetTrace(const char* path, int min_sev);
  /***************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B3\F5ʼ\BB\AF                                                      ***/
  /*** \B9\A6\C4\DC:   \BB\F1ȡý\CC\E5\BF\E2ʵ\C0\FD                                              ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool                                                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD: \CE\DE                                                        ***/
  /***************************************************************************/
  bool Initialize();

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B7\B4\B3\F5ʼ\BB\AF                                                     ***/
  /*** \B9\A6\C4\DC:   \CAͷ\C5ý\CC\E5\BF\E2ʵ\C0\FD                                               ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool                                                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD: \CE\DE                                                         ***/
  /****************************************************************************/
  void UnInitialize();

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\FA\B3\C9ý\CC\E5ͨ\B5\C0id                                               ***/
  /*** \B9\A6\C4\DC:   ý\CC\E5\BF\E2\B3\F5ʼ\BB\AF\B2\A2\C9\FA\B3\C9ͨ\B5\C0ID                                     ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD    bool   true  \B3ɹ\A6      false  ʧ\B0\DC                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD: \C3\FB\B3\C6  channel_id    \C0\E0\D0\CD   int                             ***/
  /****************************************************************************/
  bool GenerateChannelId(int& channelId);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \CAͷ\C5ý\CC\E5ͨ\B5\C0id                                               ***/
  /*** \B9\A6\C4\DC:   \CAͷ\C5ͨ\B5\C0ID                                                   ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD    bool   true  \B3ɹ\A6      false  ʧ\B0\DC                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD: \C3\FB\B3\C6  channel_id    \C0\E0\D0\CD   int                             ***/
  /****************************************************************************/
  bool ReleaseChannelId(int channelId);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B4\B4\BD\A8\B4\AB\CA\E4                                                     ***/
  /*** \B9\A6\C4\DC:   \B4\B4\BD\A8ý\CC\E5\C1\F7\B4\AB\CA\E4                                               ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    l_addr\B1\BE\B5ص\D8ַ       \C0\E0\D0\CD   const char*           ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    l_port\B1\BE\B5ض˿\DA       \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD3: \C3\FB\B3\C6    r_addrԶ\B6˵\D8ַ       \C0\E0\D0\CD   const char*           ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD4: \C3\FB\B3\C6    r_portԶ\B6˶˿\DA       \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD5: \C3\FB\B3\C6    tid \B4\AB\CA\E4ID            \C0\E0\D0\CD   const char*          ***/
  /****************************************************************************/
  bool CreateTransport(const char* local_addr,
                       int local_port,
                       const char* remote_addr,
                       int remote_port,
                       const std::string& tid,
                       bool bUdp = true);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \CAͷŴ\AB\CA\E4                                                     ***/
  /*** \B9\A6\C4\DC:   \B4\B4\BD\A8ý\CC\E5\C1\F7\B4\AB\CA\E4                                               ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  void                                                   ***/
  /****************************************************************************/
  void DestroyTransport();

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B4\B4\BD\A8ͨ\B5\C0                                                     ***/
  /*** \B9\A6\C4\DC:   ý\CC\E5\BFⴴ\BD\A8\C2߼\ADͨ\B5\C0                                           ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool   true  \B3ɹ\A6      false  ʧ\B0\DC                     ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    tid                  \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD3: \C3\FB\B3\C6    is_video             \C0\E0\D0\CD   bool                  ***/
  /****************************************************************************/
  bool CreateChannel(const std::string& settings,
                     int channel_id,
                     bool is_video = true);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \CAͷ\C5ͨ\B5\C0                                                     ***/
  /*** \B9\A6\C4\DC:   ý\CC\E5\BF\E2\CAͷ\C5\C2߼\ADͨ\B5\C0                                           ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  void                                                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    is_video             \C0\E0\D0\CD   bool                  ***/
  /****************************************************************************/
  void DestroyChannel(int channel_id, bool is_video = true);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B4\B4\BD\A8\B1\BE\B5\D8\D2\F4Ƶtrack                                            ***/
  /*** \B9\A6\C4\DC:   \B4\B4\BD\A8\B1\BE\B5\D8\D2\F4ƵԴtrack                                          ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  void*    \B7\B5\BBش\B4\BD\A8\B5\C4trackָ\D5\EB                           ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    track_id             \C0\E0\D0\CD   const char            ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    voice_index          \C0\E0\D0\CD   int                   ***/
  /****************************************************************************/
  rtc::scoped_refptr<webrtc::AudioTrackInterface> CreateLocalVoiceTrack(
      const std::string& trace_id);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \CAͷű\BE\B5\D8\D2\F4Ƶtrack                                            ***/
  /*** \B9\A6\C4\DC:   \CAͷű\BE\B5\D8\D2\F4ƵԴtrack                                          ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  void                                                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    track               \C0\E0\D0\CD   void*                  ***/
  /****************************************************************************/
  void DestroyLocalAudioTrack(
      rtc::scoped_refptr<webrtc::AudioTrackInterface> track);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B4\B4\BD\A8\B1\BE\B5\D8\CA\D3Ƶtrack                                            ***/
  /*** \B9\A6\C4\DC:   \B4\B4\BD\A8\B1\BE\B5\D8\CA\D3ƵԴtrack                                          ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  VideoTrackInterface*    \B7\B5\BBش\B4\BD\A8\B5\C4trackָ\D5\EB            ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    video_mode           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    track_id             \C0\E0\D0\CD   const char            ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD3: \C3\FB\B3\C6    camera_index         \C0\E0\D0\CD   int                   ***/
  /****************************************************************************/
  rtc::scoped_refptr<webrtc::VideoTrackInterface> CreateLocalVideoTrack(
      const std::string& track_params);  //------modify

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \CAͷű\BE\B5\D8\CA\D3Ƶtrack                                            ***/
  /*** \B9\A6\C4\DC:   \CAͷű\BE\B5\D8\CA\D3ƵԴtrack                                          ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  void                                                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    track               \C0\E0\D0\CD   void*                  ***/
  /****************************************************************************/
  void DestroyLocalVideoTrack(
      rtc::scoped_refptr<webrtc::VideoTrackInterface> track);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: Ԥ\C0\C0\B1\BE\B5\D8\CA\D3Ƶtrack                                            ***/
  /*** \B9\A6\C4\DC:   \BF\AA\C6\F4Ԥ\C0\C0\B1\BE\B5\D8\CA\D3ƵԴtrack                                      ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    window_id           \C0\E0\D0\CD   int                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    video_track         \C0\E0\D0\CD   void*                  ***/
  /****************************************************************************/
  bool PreviewTrack(int window_id, void* video_track);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: ѡ\D4\F1\CA\D3ƵԴ                                                   ***/
  /*** \B9\A6\C4\DC:   \B0\F3\B6\A8\CA\D3ƵԴtrack\D3\EB\C2߼\ADͨ\B5\C0channel_id                          ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    tid                  \C0\E0\D0\CD   const char*           ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD3: \C3\FB\B3\C6    track_id             \C0\E0\D0\CD   const char*           ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD4: \C3\FB\B3\C6    video_track          \C0\E0\D0\CD   VideoTrackInterface*  ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD5: \C3\FB\B3\C6    stream_ids           \C0\E0\D0\CD std::vector<std::string>***/
  /****************************************************************************/
  bool SelectVideoSource(
      int channelid,
      const std::string& track_id,
      rtc::scoped_refptr<webrtc::VideoTrackInterface> videotrack);

  rtc::scoped_refptr<webrtc::VideoTrackInterface> SelectVideoSourceOnFlight(
      int channelid,
      int device_index,
      const std::string& track_params);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: ѡ\D4\F1\D2\F4ƵԴ                                                   ***/
  /*** \B9\A6\C4\DC:   \B0\F3\B6\A8\D2\F4ƵԴtrack\D3\EB\C2߼\ADͨ\B5\C0channel_id                          ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    tid                  \C0\E0\D0\CD   const char*           ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD3: \C3\FB\B3\C6    track_id             \C0\E0\D0\CD   const char*           ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD4: \C3\FB\B3\C6    audio_track          \C0\E0\D0\CD   AudioTrackInterface*  ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD5: \C3\FB\B3\C6    stream_ids           \C0\E0\D0\CD std::vector<std::string>***/
  /****************************************************************************/
  bool SelectVoiceSource(
      int channelid,
      const std::string& track_id,
      rtc::scoped_refptr<webrtc::AudioTrackInterface> audiotrack);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \BF\AAʼͨ\B5\C0                                                     ***/
  /*** \B9\A6\C4\DC:   \BF\AAʼchannel_id\C2߼\AD                                           ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool     true  \B3ɹ\A6         false  ʧ\B0\DC                ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    is_video             \C0\E0\D0\CD   bool                  ***/
  /****************************************************************************/
  bool StartChannel(int channel_id);  //------modify

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: ֹͣͨ\B5\C0                                                     ***/
  /*** \B9\A6\C4\DC:   ֹͣchannel_id\C2߼\AD                                           ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool   true  \B3ɹ\A6      false  ʧ\B0\DC                     ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    is_video             \C0\E0\D0\CD   bool                  ***/
  /****************************************************************************/
  bool StopChannel(int channel_id);

  /* SetTrackCodec(trackid, codec_cofnig);

   videoTrack = CreateLocalVideoTrack(channelid,   transportid, string
   videosourceId,  ssrc);

   

   remotetrackid = AddRemoteTrack(transportid, ssrc, payload, is_video)

   SelectVideoSource(videotrack, videosourceId)
   SelectAudioSource(audiotrack, audiosourceid);

   StartChannel(channelid)

   bool AddMediaSsrc(bool is_local, int channelId, uint32_t ssrc);*/

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6ñ\BE\B5\D8\CA\D3Ƶ\C1\F7ssrc                                           ***/
  /*** \B9\A6\C4\DC:   \C9\E8\D6\C3ͨ\B5\C0channel_id\B1\BE\B5\D8ý\CC\E5\C1\F7ssrc                             ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    ssrc                 \C0\E0\D0\CD   unsigned int          ***/
  /****************************************************************************/
  bool AddMediaSsrc(bool is_local, int channelId, uint32_t ssrc);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \D4\F6\BCӱ\BE\B5\D8\E4\D6Ⱦ\B4\B0\BF\DA                                             ***/
  /*** \B9\A6\C4\DC:   \D4\F6\BCӱ\BE\B5\D8\CA\D3Ƶ\E4\D6Ⱦ\B4\B0\BF\DA                                         ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    window_id           \C0\E0\D0\CD   int                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    view                \C0\E0\D0\CD   void*                  ***/
  /****************************************************************************/
  bool SetLocalVideoRenderWindow(int window_id, int render_mode, void* view);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \D4\F6\BC\D3Զ\B6\CB\E4\D6Ⱦ\B4\B0\BF\DA                                             ***/
  /*** \B9\A6\C4\DC:   \D4\F6\BC\D3Զ\B6˽\D3\CA\D5\CA\D3Ƶ\E4\D6Ⱦ\B4\B0\BF\DA                                     ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    video_window         \C0\E0\D0\CD   void*                 ***/
  /****************************************************************************/
  bool SetRemoteVideoRenderWindow(int channelId, int render_mode, void* view);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6ñ\BE\B5ؾ\B2\D2\F4                                                 ***/
  /*** \B9\A6\C4\DC:   \B1\BE\B5ؾ\B2\D2\F4                                                     ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    bMute                \C0\E0\D0\CD   bool                  ***/
  /****************************************************************************/
  bool SetLocalMute(int channel_id, bool bMute);  //------add

  /****************************************************************************/
  /*** \A1\D2???\A1\CC?: \A1\AD?\A1¡̡\C0????\A1ܡ\B0?                                                 ***/
  /*** \A6\D0???:   \A1\C0????\A1ܡ\B0?                                                     ***/
  /*** \A1\C6???\A1\C2?: ??\A8C?  bool    true  \A1ݡ\AD\A6\D0?      false  ??\A1\DE?                    ***/
  /*** \A1\D2???\A1\DC???1: \A1\CC?\A1\DD?    channel_id           ??\A8C?   int                   ***/
  /*** \A1\D2???\A1\DC???2: \A1\CC?\A1\DD?    bMute                ??\A8C?   bool                  ***/
  /****************************************************************************/
  bool SetLoudSpeakerStatus(bool enabled);  //------add

  /****************************************************************************/
  /*** \A1\D2???\A1\CC?: \A1\AD?\A1¡̡\C0????\A1ܡ\B0?                                                 ***/
  /*** \A6\D0???:   \A1\C0????\A1ܡ\B0?                                                     ***/
  /*** \A1\C6???\A1\C2?: ??\A8C?  bool    true  \A1ݡ\AD\A6\D0?      false  ??\A1\DE?                    ***/
  /*** \A1\D2???\A1\DC???1: \A1\CC?\A1\DD?    channel_id           ??\A8C?   int                   ***/
  /*** \A1\D2???\A1\DC???2: \A1\CC?\A1\DD?    bMute                ??\A8C?   bool                  ***/
  /****************************************************************************/
  bool GetLoudSpeakerStatus(bool& enabled);  //------add

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3Զ\B6˾\B2\D2\F4                                                 ***/
  /*** \B9\A6\C4\DC:   ָ\B6\A8ͨ\B5\C0\B5\C4Զ\B6˾\B2\D2\F4                                           ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    bMute                \C0\E0\D0\CD   bool                  ***/
  /****************************************************************************/
  bool SetRemoteMute(int channel_id, bool bMute);  //------add

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: RequestRemoteVideo                                           ***/
  /*** \B9\A6\C4\DC:   \C7\EB\C7\F3Զ\B6\CB\CA\D3Ƶ                                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id           \C0\E0\D0\CD   int                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    remote_ssrc          \C0\E0\D0\CD   int_32_t              ***/
  /****************************************************************************/
  bool RequestRemoteVideo(int channel_id, int32_t remote_ssrc);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: RequestRemoteSsrc                                            ***/
  /*** \B9\A6\C4\DC:   \C7\EB\C7\F3Զ\B6\CBssrc                                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool        true   \B3ɹ\A6   false  ʧ\B0\DC                  ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C0\E0\D0\CD  int       channel_id                                ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C0\E0\D0\CD  int32_t       ssrc                                  ***/
  /****************************************************************************/
  bool RequestRemoteSsrc(int channel_id, int flag, int32_t ssrc);
  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: GetNumberOfVideoDevices                                      ***/
  /*** \B9\A6\C4\DC:   \BB\F1ȡ\B5\B1ǰϵͳ\CB\F9\D3\D0\CA\D3Ƶ\C9豸                                     ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  uint32_t    \CA\D3Ƶ\C9豸\B8\F6\CA\FD                               ***/
  /****************************************************************************/
  uint32_t GetNumberOfVideoDevices();

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: GetVideoDevices                                ***/
  /*** \B9\A6\C4\DC:   \BB\F1ȡ\B5\B1ǰϵͳ\CB\F9\D3\D0\CA\D3Ƶ\C9豸\D0\C5Ϣ                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool true  \B3ɹ\A6   false  ʧ\B0\DCʱ\B7\B5\BB\D8\D0\E8Ҫ\BB\BA\B4\E6\B4\F3С        ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6 jsonDeviceInfos  \C0\E0\D0\CD   char*  json\B8\F1ʽ\B5\C4\D0\C5Ϣ\D7ַ\FB\B4\AE  ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6  ength           \C0\E0\D0\CD   int*  [in out]\CA\E4\C8\EB\CA\E4\B3\F6\B2\CE\CA\FD   ***/
  /****************************************************************************/
  bool GetVideoDevices(char* jsonDeviceInfos, int* length);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: GetVideoCodecs                                ***/
  /*** \B9\A6\C4\DC:   \BB\F1ȡ\B5\B1ǰϵͳ\CB\F9\D3\D0\CA\D3Ƶ\C9豸\D0\C5Ϣ                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool true  \B3ɹ\A6   false  ʧ\B0\DCʱ\B7\B5\BB\D8\D0\E8Ҫ\BB\BA\B4\E6\B4\F3С        ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1:\C3\FB\B3\C6 jsonVideoCodecInfos \C0\E0\D0\CD char*  json\B8\F1ʽ\B5\C4\D0\C5Ϣ\D7ַ\FB\B4\AE  ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2:\C3\FB\B3\C6 length            \C0\E0\D0\CD   int*  [in out]\CA\E4\C8\EB\CA\E4\B3\F6\B2\CE\CA\FD   ***/
  /****************************************************************************/
  bool GetVideoCodecs(char* jsonVideoCodecInfos, int* length);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: GetAudioCodecs                                ***/
  /*** \B9\A6\C4\DC:   \BB\F1ȡ\B5\B1ǰϵͳ\CB\F9\D3\D0\CA\D3Ƶ\C9豸\D0\C5Ϣ                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool true  \B3ɹ\A6   false  ʧ\B0\DCʱ\B7\B5\BB\D8\D0\E8Ҫ\BB\BA\B4\E6\B4\F3С        ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1:\C3\FB\B3\C6 jsonAudioCodecInfos \C0\E0\D0\CD char*  json\B8\F1ʽ\B5\C4\D0\C5Ϣ\D7ַ\FB\B4\AE  ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2:\C3\FB\B3\C6 length            \C0\E0\D0\CD   int*  [in out]\CA\E4\C8\EB\CA\E4\B3\F6\B2\CE\CA\FD   ***/
  /****************************************************************************/
  bool GetAudioCodecs(char* jsonAudioCodecInfos, int* length);

  //////////////////////////ylr interface start////////////////////////////////
  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3\CA\D3Ƶ\B5\C4Nack\B9\A6\C4\DC                                           ***/
  /*** \B9\A6\C4\DC:   \BF\AA\C6\F4\BB\F2\B9ر\D5Nack\B9\A6\C4\DC ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id       \C0\E0\D0\CD   int                       ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    enable_nack	   \C0\E0\D0\CD   bool                      ***/
  /****************************************************************************/
  bool SetVideoNackStatus(const int channelId, const bool enable_nack);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3\CA\D3Ƶ\B5\C4Fec\B9\A6\C4\DC                                            ***/
  /*** \B9\A6\C4\DC:   \BF\AA\C6\F4\BB\F2\B9ر\D5\CA\D3ƵFec\B9\A6\C4\DC                                        ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id       \C0\E0\D0\CD   int                       ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    enable    	   \C0\E0\D0\CD   bool                      ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD3: \C3\FB\B3\C6    payloadtype_red  \C0\E0\D0\CD   uint8_t                   ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD4: \C3\FB\B3\C6    payloadtype_fec  \C0\E0\D0\CD   uint8_t                   ***/
  /****************************************************************************/
  bool SetVideoUlpFecStatus(const int channelId,
                            const bool enable,
                            const uint8_t payloadtype_red,
                            const uint8_t payloadtype_fec);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3\CA\D3Ƶ\B5\C4DegradationMode\B9\A6\C4\DC                                ***/
  /*** \B9\A6\C4\DC:   \BF\AA\C6\F4\BB\F2\B9ر\D5\CA\D3ƵDegradationMode\B9\A6\C4\DC                            ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id       \C0\E0\D0\CD   int                       ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD2: \C3\FB\B3\C6    mode      	   \C0\E0\D0\CD   DegradationPreference     ***/
  /****************************************************************************/
  bool SetVideoDegradationMode(const int channelId,
                               const webrtc::DegradationPreference mode);
  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B9ؼ\FC֡\B7\A2\CB\CD                                                   ***/
  /*** \B9\A6\C4\DC:   \B7\A2\CB͹ؼ\FC֡                                                   ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id       \C0\E0\D0\CD   int                       ***/
  /****************************************************************************/
  bool SendKeyFrame(const int channelId);
  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6ùؼ\FC֡\BBص\F7\C7\EB\C7\F3                                           ***/
  /*** \B9\A6\C4\DC:   \C9\E8\D6\C3\C7\EB\C7\F3\B9ؼ\FC֡\BBص\F7                                           ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    channel_id       \C0\E0\D0\CD   int                       ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    cb        \C0\E0\D0\CD  OnRequestKeyFrameCallback\BA\AF\CA\FDָ\D5\EB ***/
  /****************************************************************************/
  bool SetKeyFrameRequestCallback(const int channelId,
                                  OnRequestKeyFrameCallback cb);
  //////////////////////////ylr interface end////////////////////////////////

  //////////////////////////zjy interface start///////////////////////////////

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6û\D8\C9\F9\CF\FB\B3\FD                                                 ***/
  /*** \B9\A6\C4\DC:   \D2\F4Ƶͨ\BB\B0\BB\D8\D2\F4\CF\FB\B3\FD                                             ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    enable       \C0\E0\D0\CD   bool                          ***/
  /*** \B4˺\AF\CA\FD\D0\E8Ҫ\D4\DACreateChannel֮ǰʹ\D3\C3                                    ***/
  /****************************************************************************/
  bool SetAEC(bool enable);
  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3\D3\EF\D2\F4\D7Զ\AF\D4\F6\D2湦\C4\DC                                         ***/
  /*** \B9\A6\C4\DC:   \D3\EF\D2\F4\D7Զ\AF\D4\F6\D2\E6                                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    enable       \C0\E0\D0\CD   bool                          ***/
  /*** \B4˺\AF\CA\FD\D0\E8Ҫ\D4\DACreateChannel֮ǰʹ\D3\C3                                    ***/
  /****************************************************************************/
  bool SetAGC(bool enable);
  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3\D3\EF\D2\F4\D4\EB\C9\F9\D2\D6\D6ƹ\A6\C4\DC                                         ***/
  /*** \B9\A6\C4\DC:   \D3\EF\D2\F4\D4\EB\C9\F9\D2\D6\D6\C6                                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool    true  \B3ɹ\A6      false  ʧ\B0\DC                    ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6    enable       \C0\E0\D0\CD   bool                          ***/
  /*** \B4˺\AF\CA\FD\D0\E8Ҫ\D4\DACreateChannel֮ǰʹ\D3\C3                                    ***/
  /****************************************************************************/
  bool SetNS(bool enable);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \B4\B4\BD\A8\D2\F4Ƶ\C9豸\B6\D4\CF\F3                                             ***/
  /*** \B9\A6\C4\DC:   \BB\F1ȡ\B5ײ\E3AudioDeviceModule\B6\D4\CF\F3                                ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  scoped_refptr<webrtc::AudioDeviceModule>               ***/
  /****************************************************************************/
  bool CreateAudioDevice();

  // int GetAudioRecordingDevice(const int a);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3¼\D2\F4\D2\F4\C1\BF                                                 ***/
  /*** \B9\A6\C4\DC:   \C9\E8\D6\C3¼\D2\F4\C9豸¼\D2\F4\D2\F4\C1\BF                                         ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool        true  \B3ɹ\A6      false   ʧ\B0\DC               ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6   vol                \C0\E0\D0\CD    uint32_t                ***/
  /****************************************************************************/
  bool SetAudioRecordingVolume(uint32_t vol);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \BB\F1ȡ¼\D2\F4\C9豸\C1б\ED ***/
  /*** \B9\A6\C4\DC:   \BB\F1ȡ¼\D2\F4\C9豸\C1б\ED\D7ַ\FB\B4\AE ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  char* ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6   length                 \C0\E0\D0\CD    int* ***/
  /*****************************************************************************/
  char* GetAudioDeviceList(int* length);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6\C3¼\D2\F4\C9豸 ***/
  /*** \B9\A6\C4\DC:   \B8\F9\BE\DD\CB\F7\D2\FDѡ\D4\F1\D0\E8Ҫʹ\D3õ\C4¼\D2\F4\C9豸 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool        true  \B3ɹ\A6      false   ʧ\B0\DC ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6   index                   \C0\E0\D0\CD    int ***/
  /*****************************************************************************/
  bool SetAudioRecordingDevice(int index);

  bool SetAudioRecordingDeviceOnFlight(int index);
  bool SetAudioPlayoutDeviceOnFlight(int index);

  /****************************************************************************/
  /*** \BA\AF\CA\FD\C3\FB: \C9\E8\D6ò\A5\B7\C5\C9豸                                                 ***/
  /*** \B9\A6\C4\DC:   \B8\F9\BE\DD\CB\F7\D2\FDѡ\D4\F1\D0\E8Ҫʹ\D3ò\A5\B7\C5\C9豸                                 ***/
  /*** \B7\B5\BB\D8ֵ: \C0\E0\D0\CD  bool        true  \B3ɹ\A6      false   ʧ\B0\DC               ***/
  /*** \BA\AF\CA\FD\B2\CE\CA\FD1: \C3\FB\B3\C6   index                   \C0\E0\D0\CD    int                ***/
  /*****************************************************************************/
  bool SetAudioPlayoutDevice(int index);
  ///////////////////////// zjy interface end//////////////////////////////////
  //#if defined(WEBRTC_WIN)
  int CreateDesktopCapture(int type);
  int SetDesktopSourceID(int type, int id);
  int GetWindowsList(int type, webrtc::DesktopCapturer::SourceList& source);
  int StartScreenShare(int type, int channelId);
  int StopScreenShare(int type, int channelId);

  int GetCaptureDevice(int index,
                       char* device_name,
                       int name_len,
                       char* unique_name,
                       int id_len);
  int AllocateCaptureDevice(const char* id, int len, int& deviceid);
  int ConnectCaptureDevice(int deviceid, int peer_id);
  int NumOfCapabilities(const char* id);
  int GetCaptureCapabilities(const char* id,
                             int index,
                             webrtc::VideoCaptureCapability& cap);
  int StartCameraCapturer(int deviceid, webrtc::VideoCaptureCapability& cap);
  int StopCapturer(int deviceid);
  int StopAllCapturer();
  int StopLocalRender(int peer_id, int deviceid);
  int StopRemoteRender(int peer_id, int deviceid);

  bool StartConnectChannel(int audio_channel_id, int video_channel_id);

  int StartMicCapture(int peer_id);

  int StartSendRecv(int peer_id);

  int AudioStartReceive(int peer_id);
  int AudioStartSend(int peer_id);
  int VideoStartReceive(int peer_id);
  int VideoStartSend(int peer_id);
  //#endif
  void GetAudioCodecs(cricket::AudioCodecs* audio_codecs) const;
  void GetVideoCodecs(cricket::VideoCodecs* video_codecs) const;
  void SetSendCodecVideo(cricket::VideoCodec* video_codec);
  void SetReceiveCodecVideo(int peer_id, cricket::VideoCodec* video_codec);
  void SetSendCodecAudio(cricket::AudioCodec* audio_codec);
  void SetReceiveCodecAudio(int peer_id, cricket::AudioCodec* audio_codec);
  // wx begin
  int RegisterConferenceParticipantCallback(
      int channelid,
      ECMedia_ConferenceParticipantCallback* callback);
  int SetConferenceParticipantCallbackTimeInterVal(int channelid,
                                                   int timeInterVal);
  //ytx_wx add
  int GetCallStats(char* statistics, int length);
  bool GetVideoStreamStats(char* jsonVideoStats, int length,int channel_id);
  bool GetVoiceStreamStats(char* jsonAudioStats,
                               int length,
                               int channel_id);
  //wx end
  
  
  bool AttachVideoRender(int channelId,
                       void* videoView,
                       int render_mode,
                       int mirror_mode,
                       rtc::Thread* worker_thread);
   bool DetachVideoRender(int channelId, void* winRemote);
   void RemoveAllVideoRender(int channelId);

   bool UpdateOrAddVideoTrack(int channelId,
                              void* track_to_render);
   bool StartRender(int channelId, void* videoView);
   bool StopRender(int channelId, void* videoView);
  
#if defined(WEBRTC_IOS)
 int GetOrientation(int deviceid, ECMediaRotateCapturedFrame &tr);
 int SetRotateCapturedFrames(int deviceid, ECMediaRotateCapturedFrame tr);
#endif

#if defined(WEBRTC_ANDROID)
  bool SaveLocalVideoTrack(int channelId, webrtc::VideoTrackInterface* track);
  webrtc::VideoTrackInterface* GetLocalVideoTrack(int channelId);
  bool SaveLocalVideoTrack(int channelId, void* track);
  void* GetLocalVideoTrackPtr(int channelId);
  bool RemoveLocalVideoTrack(int channelId);

  bool SaveRemoteVideoSink(int channelId, JNIEnv* env, jobject javaSink);
  rtc::VideoSinkInterface<webrtc::VideoFrame>* GetRemoteVideoSink(
      int channelId);
  bool RemoveRemoteVideoSink(int channelId);

  int InitializeJVM();

  void* CreateVideoTrack(const char* id,
                         webrtc::VideoTrackSourceInterface* source);

  void* CreateAudioTrack(const char* id, webrtc::AudioSourceInterface* source);

  void* CreateAudioSource();

  void* CreateVideoSource(JNIEnv* env,
                          bool is_screencast,
                          bool align_timestamps);

  static void SetVideoHardwareEncoderFactoryCallback(
      OnGetVideoHardwareEncoderFactory callback);
  static void SetAudioHardwareEncoderFactoryAndAdmCallback(
      OnGetAudioHardwareEncoderFactoryAndAdm callback);

#endif

 private:
  MediaClient();

  bool CreateThreads();
  bool CreateRtcEventLog();
#if defined(WEBRTC_ANDROID)
  bool CreateChannelManager(
      std::unique_ptr<webrtc::VideoEncoderFactory> encoderFactory,
      std::unique_ptr<webrtc::VideoDecoderFactory> decoderFactory);
  bool AndroidInitialize();
#else
  bool CreateChannelManager();
#endif
  bool CreateCall(webrtc::RtcEventLog* event_log);
  bool CreateTransportController(bool disable_encryp = true);
  bool CreateVideoChannel(const std::string& settings, int channel_id);
  bool CreateVoiceChannel(const std::string& settingsvvv, int channel_id);
  bool DisposeConnect();
  bool InitRenderWndsManager();
  void DestroyAllChannels();
  bool GetMediaSsrc(bool is_local, int channelId, std::vector<uint32_t>& ssrcs);
  int GetMaxVideoBitrateKbps(int width, int height, bool is_screenshare);
  void DestroyTransceiverChannel(
      rtc::scoped_refptr<
          webrtc::RtpTransceiverProxyWithInternal<webrtc::RtpTransceiver>>
          transceiver);

  void DestroyChannelInterface(cricket::ChannelInterface* channel);

  cricket::ChannelInterface* GetChannel(const std::string& mid);

  cricket::RtpDataChannel* rtp_data_channel() const;

  void OnSentPacket(const rtc::SentPacket& sent_packet);
  std::string GetMidFromChannelId(int channelId);

  bool FilterAudioCodec(const AudioCodecConfig& config,
                        std::vector<cricket::AudioCodec>& vec);

  bool FilterVideoCodec(const VideoCodecConfig& config,
                        std::vector<cricket::VideoCodec>& vec);
  bool FilterVideoCodec(std::vector<VideoStreamConfig>* configs,
                        std::vector<cricket::VideoCodec>& vec);
  bool ParseVideoDeviceSetting(const char* videoDeviceSettings,
                               VideoDeviceConfig* config);
  bool ParseAudioCodecSetting(const char* audioCodecSettings,
                              AudioCodecConfig* config);
  bool ParseVideoCodecSetting(const char* videoCodecSettings,
                              VideoCodecConfig* config);
  bool ParseVideoCodecSetting(const char* videoCodecSettings,
                              std::vector<VideoStreamConfig>* configs);
  bool GetStringJsonString(const char* json,
                           const std::string& key,
                           std::string* value);
  bool GetIntJsonString(const char* json, const std::string& key, int* value);
  cricket::WebRtcVideoChannel* GetInternalVideoChannel(const int channelId);
  bool InsertVideoCodec(cricket::VideoCodecs& input_codecs,
                        const std::string& codec_name,
                        uint8_t payload_type);
  bool RemoveVideoCodec(cricket::VideoCodecs& input_codecs,
                        const std::string& codec_name);

 private:
  static MediaClient* m_pInstance;
#if defined(WEBRTC_WIN)
  int m_screenshareID;
  bool m_screenshareStart;
#endif
  static rtc::CriticalSection m_critical;
  bool isCreateCall;
  char* pAudioDevice;
  // std::unique_ptr<rtc::LogSink*> ec_log_ = nullptr;
  //rtc::LogSink* ec_log_ = nullptr;
  //bool bfirst = true;
  static rtc::LogSink* ec_log_;
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_tracks_[100];

  std::vector<rtc::scoped_refptr<webrtc::VideoTrackInterface>> video_tracks_;
  
  std::map<int, rtc::scoped_refptr<webrtc::VideoTrackInterface>> remote_tracks_;

  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;

  int asum_;

  webrtc::RtcEventLog* event_log_ptr_ = nullptr;

  std::unique_ptr<webrtc::RtcEventLog> event_log_;

  webrtc::Call* call_ptr_ = nullptr;

  std::unique_ptr<webrtc::Call> call_;

  cricket::RtpDataChannel* rtp_data_channel_ = nullptr;

  std::unique_ptr<rtc::BasicPacketSocketFactory> default_socket_factory_;

  std::unique_ptr<cricket::ChannelManager> channel_manager_;

  rtc::Thread* network_thread_ = nullptr;
  rtc::Thread* worker_thread_ = nullptr;
  rtc::Thread* signaling_thread_ = nullptr;
  std::unique_ptr<rtc::Thread> owned_network_thread_;
  std::unique_ptr<rtc::Thread> owned_worker_thread_;
  std::unique_ptr<rtc::Thread> owned_signaling_thread_;

  const std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;

  std::map<std::string, std::vector<int>> transports_;

  std::map<int, cricket::VideoChannel*> mVideoChannels_;

  std::map<int, cricket::VoiceChannel*> mVoiceChannels_;

  std::vector<rtc::scoped_refptr<
      webrtc::RtpTransceiverProxyWithInternal<webrtc::RtpTransceiver>>>
      transceivers_;

  std::map<int, rtc::scoped_refptr<webrtc::RtpSenderInterface>> RtpSenders_;
  std::map<int, rtc::scoped_refptr<webrtc::VideoTrackInterface>> TrackChannels_;

  struct cricket::MediaConfig media_config_;

  cricket::AudioOptions audio_options_;

  cricket::VideoOptions video_options_;

  rtc::UniqueRandomIdGenerator ssrc_generator_;

  std::unique_ptr<webrtc::GeneralTransportController> transport_controller_;

  std::unique_ptr<TransportControllerObserve> transportControllerObserve_;

  std::unique_ptr<ChannelGenerator> channelGenerator_;

  std::unique_ptr<webrtc::DesktopCapturer> capturer_;
  // rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;

  std::unique_ptr<webrtc::DesktopFrame> frame_;

  rtc::scoped_refptr<webrtc::AudioDeviceModule> own_adm;  // zjy

  webrtc::AudioDeviceModule::AudioLayer audio_layer_ =
      webrtc::AudioDeviceModule::kPlatformDefaultAudio;  // zjy
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> vcm_device_info_;
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
  std::map<int, UniqueIdVideoCapturerPair> camera_devices_;
#if defined WEBRTC_WIN
  rtc::scoped_refptr<win_desk::ECDesktopCapture> desktop_device_;

  std::map<int, rtc::scoped_refptr<win_desk::ECDesktopCapture>>
      desktop_devices_;
#endif
#if defined WEBRTC_WIN || defined WEBRTC_IOS
  std::unique_ptr<RenderManager> renderWndsManager_;
#endif
 // std::unique_ptr<VideoRenderer> local_renderer_;
  bool m_bInitialized;
  bool m_bControll;
  uint32_t m_nConnected;

  /* struct ChannelSsrc {
       uint32_t ssrcLocal = 0;
       uint32_t ssrcRemote = 0;
       std::string mid;  // the media stream id of channelId
     

   };*/
  struct ChannelSsrcs {
    std::vector<uint32_t> ssrcLocal;
    std::vector<uint32_t> ssrcRemote;
    std::string mid;  // the media stream id of channelId
  };
  std::map<int, ChannelSsrcs> mapChannelSsrcs_;

  std::map<int, rtc::scoped_refptr<webrtc::VideoTrackInterface>>
      cameraId_videoTrack_pairs_;
#if defined(WEBRTC_ANDROID)
  std::map<int, webrtc::VideoTrackInterface*> mapLocalVideoTracks;
  std::map<int, std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>>
      mapRemoteVideoSinks;
  using VideoSinkIterator = std::map<
      int,
      std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>>::iterator;

  static OnGetVideoHardwareEncoderFactory onGetVideoHardwareEncoderFactory_;
  static OnGetAudioHardwareEncoderFactoryAndAdm
      onGetAudioHardwareEncoderFactoryAndAdm_;
#endif
  int m_MaxBandwidthBps_;
#if defined(WEBRTC_WIN)
  int m_MaxBitrateScreen_;
#endif
};

class TransportControllerObserve
    : public webrtc::GeneralTransportController::Observer,
      public sigslot::has_slots<> {
 public:
  TransportControllerObserve(rtc::Thread* network_thread, MediaClient* owner);
  ~TransportControllerObserve();

  virtual bool OnTransportChanged(
      const std::string& mid,
      webrtc::RtpTransportInternal* rtp_transport,
      rtc::scoped_refptr<webrtc::DtlsTransport> dtls_transport,
      webrtc::MediaTransportInterface* media_transport) override;

 private:
  rtc::Thread* network_thread_ = nullptr;
  MediaClient* owner_ = nullptr;
};

class ChannelGenerator {
 public:
  const static int kBaseId = 0;
  const static int kMaxId = 255;
  ChannelGenerator();
  ~ChannelGenerator();

  bool GeneratorId(int& id);
  bool ReturnId(int id);

  // private:
  void ResetGenerator();

 private:
  bool idBools_[kMaxId - kBaseId] = {false};
  rtc::CriticalSection id_critsect_;
};

}  // namespace ecmedia_sdk

#endif  // MEDIA_CLIENT_H_
