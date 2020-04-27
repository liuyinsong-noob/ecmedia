
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

#include "video_render/video_renderer.h"
#include "video_capturer/video_capturer.h"
#include "media/base/video_common.h"
#include "media/engine/webrtc_video_engine.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "modules/video_capture/video_capture.h"


#include "ec_log.h"

#ifdef WEBRTC_ANDROID
#include <jni.h>
#endif

#include "media/base/adapted_video_track_source.h"
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

namespace win_render {
#if defined(WEBRTC_WIN)
class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  VideoRenderer(HWND wnd,
                int mode,
                int width,
                int height,
                webrtc::VideoTrackInterface* track_to_render);
  virtual ~VideoRenderer();

  void Lock() { ::EnterCriticalSection(&buffer_lock_); }

  void Unlock() { ::LeaveCriticalSection(&buffer_lock_); }

  // VideoSinkInterface implementation
  void OnFrame(const webrtc::VideoFrame& frame) override;
  void Paint();
  bool UpdateVideoTrack(webrtc::VideoTrackInterface* track_to_render);

  const BITMAPINFO& bmi() const { return bmi_; }
  const uint8_t* image() const { return image_.get(); }
  HWND handle() const { return wnd_; }

 protected:
  void SetSize(int width, int height);

  enum {
    SET_SIZE,
    RENDER_FRAME,
  };

 private:
  HWND wnd_;
  HDC hDC_;
  BITMAPINFO bmi_;
  std::unique_ptr<uint8_t[]> image_;
  CRITICAL_SECTION buffer_lock_;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
  int mode_;
};

// A little helper class to make sure we always to proper locking and
// unlocking when working with VideoRenderer buffers.
template <typename T>
class AutoLock {
 public:
  explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
  ~AutoLock() { obj_->Unlock(); }

 protected:
  T* obj_;
};

class RenderWndsManager : public sigslot::has_slots<> {
 public:
  RenderWndsManager();
  ~RenderWndsManager();

  void AddRemoteRenderWnd(int channelId,
                          int render_mode,
                          void* winRemote,
                          webrtc::VideoTrackInterface* track_to_render);
  bool UpdateVideoTrack(int channelId,
                        webrtc::VideoTrackInterface* track_to_render);
  bool UpdateLocalVideoTrack(int channelId,
                             webrtc::VideoTrackInterface* track_to_render);
  void* GetRemoteWnd(int channelId);
  bool RemoveRemoteRenderWnd(int channelId);
  void SetLocalRenderWnd(int channelId,
                         int render_mode,
                         void* winLocal,
                         webrtc::VideoTrackInterface* track_to_render);
  bool StartLocalRenderer(int window_id,
                          webrtc::VideoTrackInterface* local_video);
  bool StartRemoteRenderer(int channelId,
                           webrtc::VideoTrackInterface* remote_video);

  std::vector<int> GetAllRemoteChanelIds();

 private:
  using ptr_render = std::unique_ptr<win_render::VideoRenderer>;
  // ptr_render localRender_ = nullptr;
  std::map<int, ptr_render> mapRemoteRenderWnds;
  std::map<int, ptr_render> mapLocalRenderWnds;
};

#endif
}  // namespace win_render

namespace ecmedia_sdk {

#define EC_CHECK_VALUE(v, r) \
  if (!v)                    \
    return r;

class TransportControllerObserve;
class ChannelGenerator;
enum VIDEO_SOURCE_TYPE { VIDEO_CAMPER = 0, VIDEO_SCREEN, VIDEO_EXTERNAL };
typedef void(ECMedia_ConferenceParticipantCallback)(uint32_t arrOfCSRCs[],
                                                    int count);

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
    int fecType = 1;  // red and ulp fec
    int fecPayload = 97;
    int rtx = 1;
    int rtxPayload = 97;
    int payloadType = 96;
    int width = 1280;
    int height = 720;
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

  bool SetTrace(const char* path, int min_sev);
  /***************************************************************************/
  /*** 函数名: 初始化                                                      ***/
  /*** 功能:   获取媒体库实例                                              ***/
  /*** 返回值: 类型  bool                                                   ***/
  /*** 函数参数: 无                                                        ***/
  /***************************************************************************/
#if defined(WEBRTC_ANDROID)
  bool Initialize(JNIEnv* env,
                  jobject jencoder_factory,
                  jobject jdecoder_factory);
#else
  bool Initialize();
#endif

  /****************************************************************************/
  /*** 函数名: 反初始化                                                     ***/
  /*** 功能:   释放媒体库实例                                               ***/
  /*** 返回值: 类型  bool                                                   ***/
  /*** 函数参数: 无                                                         ***/
  /****************************************************************************/
  void UnInitialize();

  /****************************************************************************/
  /*** 函数名: 生成媒体通道id                                               ***/
  /*** 功能:   媒体库初始化并生成通道ID                                     ***/
  /*** 返回值: 类型    bool   true  成功      false  失败                   ***/
  /*** 函数参数: 名称  channel_id    类型   int                             ***/
  /****************************************************************************/
  bool GenerateChannelId(int& channelId);

  /****************************************************************************/
  /*** 函数名: 释放媒体通道id                                               ***/
  /*** 功能:   释放通道ID                                                   ***/
  /*** 返回值: 类型    bool   true  成功      false  失败                   ***/
  /*** 函数参数: 名称  channel_id    类型   int                             ***/
  /****************************************************************************/
  bool ReleaseChannelId(int channelId);

  /****************************************************************************/
  /*** 函数名: 创建传输                                                     ***/
  /*** 功能:   创建媒体流传输                                               ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    l_addr本地地址       类型   const char*           ***/
  /*** 函数参数2: 名称    l_port本地端口       类型   int                   ***/
  /*** 函数参数3: 名称    r_addr远端地址       类型   const char*           ***/
  /*** 函数参数4: 名称    r_port远端端口       类型   int                   ***/
  /*** 函数参数5: 名称    tid 传输ID            类型   const char*          ***/
  /****************************************************************************/
  bool CreateTransport(const char* local_addr,
                       int local_port,
                       const char* remote_addr,
                       int remote_port,
                       const std::string& tid,
                       bool bUdp = true);

  /****************************************************************************/
  /*** 函数名: 释放传输                                                     ***/
  /*** 功能:   创建媒体流传输                                               ***/
  /*** 返回值: 类型  void                                                   ***/
  /****************************************************************************/
  void DestroyTransport();

  /****************************************************************************/
  /*** 函数名: 创建通道                                                     ***/
  /*** 功能:   媒体库创建逻辑通道                                           ***/
  /*** 返回值: 类型  bool   true  成功      false  失败                     ***/
  /*** 函数参数1: 名称    tid                  类型   int                   ***/
  /*** 函数参数2: 名称    channel_id           类型   int                   ***/
  /*** 函数参数3: 名称    is_video             类型   bool                  ***/
  /****************************************************************************/
  bool CreateChannel(const std::string& settings,
                     int channel_id,
                     bool is_video = true);

  /****************************************************************************/
  /*** 函数名: 释放通道                                                     ***/
  /*** 功能:   媒体库释放逻辑通道                                           ***/
  /*** 返回值: 类型  void                                                   ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    is_video             类型   bool                  ***/
  /****************************************************************************/
  void DestroyChannel(int channel_id, bool is_video = true);

  /****************************************************************************/
  /*** 函数名: 创建本地音频track                                            ***/
  /*** 功能:   创建本地音频源track                                          ***/
  /*** 返回值: 类型  void*    返回创建的track指针                           ***/
  /*** 函数参数1: 名称    track_id             类型   const char            ***/
  /*** 函数参数2: 名称    voice_index          类型   int                   ***/
  /****************************************************************************/
  rtc::scoped_refptr<webrtc::AudioTrackInterface> CreateLocalVoiceTrack(
      const std::string& trace_id);

  /****************************************************************************/
  /*** 函数名: 释放本地音频track                                            ***/
  /*** 功能:   释放本地音频源track                                          ***/
  /*** 返回值: 类型  void                                                   ***/
  /*** 函数参数1: 名称    track               类型   void*                  ***/
  /****************************************************************************/
  void DestroyLocalAudioTrack(
      rtc::scoped_refptr<webrtc::AudioTrackInterface> track);

  /****************************************************************************/
  /*** 函数名: 创建本地视频track                                            ***/
  /*** 功能:   创建本地视频源track                                          ***/
  /*** 返回值: 类型  VideoTrackInterface*    返回创建的track指针            ***/
  /*** 函数参数1: 名称    video_mode           类型   int                   ***/
  /*** 函数参数2: 名称    track_id             类型   const char            ***/
  /*** 函数参数3: 名称    camera_index         类型   int                   ***/
  /****************************************************************************/
  rtc::scoped_refptr<webrtc::VideoTrackInterface> CreateLocalVideoTrack(
      const std::string& track_params);  //------modify

  /****************************************************************************/
  /*** 函数名: 释放本地视频track                                            ***/
  /*** 功能:   释放本地视频源track                                          ***/
  /*** 返回值: 类型  void                                                   ***/
  /*** 函数参数1: 名称    track               类型   void*                  ***/
  /****************************************************************************/
  void DestroyLocalVideoTrack(
      rtc::scoped_refptr<webrtc::VideoTrackInterface> track);

  /****************************************************************************/
  /*** 函数名: 预览本地视频track                                            ***/
  /*** 功能:   开启预览本地视频源track                                      ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    window_id           类型   int                    ***/
  /*** 函数参数2: 名称    video_track         类型   void*                  ***/
  /****************************************************************************/
  bool PreviewTrack(int window_id, void* video_track);

  /****************************************************************************/
  /*** 函数名: 选择视频源                                                   ***/
  /*** 功能:   绑定视频源track与逻辑通道channel_id                          ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    tid                  类型   const char*           ***/
  /*** 函数参数2: 名称    channel_id           类型   int                   ***/
  /*** 函数参数3: 名称    track_id             类型   const char*           ***/
  /*** 函数参数4: 名称    video_track          类型   VideoTrackInterface*  ***/
  /*** 函数参数5: 名称    stream_ids           类型 std::vector<std::string>***/
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
  /*** 函数名: 选择音频源                                                   ***/
  /*** 功能:   绑定音频源track与逻辑通道channel_id                          ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    tid                  类型   const char*           ***/
  /*** 函数参数2: 名称    channel_id           类型   int                   ***/
  /*** 函数参数3: 名称    track_id             类型   const char*           ***/
  /*** 函数参数4: 名称    audio_track          类型   AudioTrackInterface*  ***/
  /*** 函数参数5: 名称    stream_ids           类型 std::vector<std::string>***/
  /****************************************************************************/
  bool SelectVoiceSource(
      int channelid,
      const std::string& track_id,
      rtc::scoped_refptr<webrtc::AudioTrackInterface> audiotrack);

  /****************************************************************************/
  /*** 函数名: 开始通道                                                     ***/
  /*** 功能:   开始channel_id逻辑                                           ***/
  /*** 返回值: 类型  bool     true  成功         false  失败                ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    is_video             类型   bool                  ***/
  /****************************************************************************/
  bool StartChannel(int channel_id);  //------modify

  /****************************************************************************/
  /*** 函数名: 停止通道                                                     ***/
  /*** 功能:   停止channel_id逻辑                                           ***/
  /*** 返回值: 类型  bool   true  成功      false  失败                     ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    is_video             类型   bool                  ***/
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
  /*** 函数名: 设置本地视频流ssrc                                           ***/
  /*** 功能:   设置通道channel_id本地媒体流ssrc                             ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    ssrc                 类型   unsigned int          ***/
  /****************************************************************************/
  bool AddMediaSsrc(bool is_local, int channelId, uint32_t ssrc);

  /****************************************************************************/
  /*** 函数名: 增加本地渲染窗口                                             ***/
  /*** 功能:   增加本地视频渲染窗口                                         ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    window_id           类型   int                    ***/
  /*** 函数参数2: 名称    view                类型   void*                  ***/
  /****************************************************************************/
  bool SetLocalVideoRenderWindow(int window_id, int render_mode, void* view);

  /****************************************************************************/
  /*** 函数名: 增加远端渲染窗口                                             ***/
  /*** 功能:   增加远端接收视频渲染窗口                                     ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    video_window         类型   void*                 ***/
  /****************************************************************************/
  bool SetRemoteVideoRenderWindow(int channelId, int render_mode, void* view);

  /****************************************************************************/
  /*** 函数名: 设置本地静音                                                 ***/
  /*** 功能:   本地静音                                                     ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    bMute                类型   bool                  ***/
  /****************************************************************************/
  bool SetLocalMute(int channel_id, bool bMute);  //------add

  /****************************************************************************/
  /*** ∫???√?: …?÷√±????≤“?                                                 ***/
  /*** π???:   ±????≤“?                                                     ***/
  /*** ∑???÷?: ??–?  bool    true  ≥…π?      false  ??∞?                    ***/
  /*** ∫???≤???1: √?≥?    channel_id           ??–?   int                   ***/
  /*** ∫???≤???2: √?≥?    bMute                ??–?   bool                  ***/
  /****************************************************************************/
  bool SetLoudSpeakerStatus(bool enabled);  //------add

  /****************************************************************************/
  /*** ∫???√?: …?÷√±????≤“?                                                 ***/
  /*** π???:   ±????≤“?                                                     ***/
  /*** ∑???÷?: ??–?  bool    true  ≥…π?      false  ??∞?                    ***/
  /*** ∫???≤???1: √?≥?    channel_id           ??–?   int                   ***/
  /*** ∫???≤???2: √?≥?    bMute                ??–?   bool                  ***/
  /****************************************************************************/
  bool GetLoudSpeakerStatus(bool& enabled);  //------add

  /****************************************************************************/
  /*** 函数名: 设置远端静音                                                 ***/
  /*** 功能:   指定通道的远端静音                                           ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    bMute                类型   bool                  ***/
  /****************************************************************************/
  bool SetRemoteMute(int channel_id, bool bMute);  //------add

  /****************************************************************************/
  /*** 函数名: RequestRemoteVideo                                           ***/
  /*** 功能:   请求远端视频                                                 ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id           类型   int                   ***/
  /*** 函数参数2: 名称    remote_ssrc          类型   int_32_t              ***/
  /****************************************************************************/
  bool RequestRemoteVideo(int channel_id, int32_t remote_ssrc);

  /****************************************************************************/
  /*** 函数名: RequestRemoteSsrc                                            ***/
  /*** 功能:   请求远端ssrc                                                 ***/
  /*** 返回值: 类型  bool        true   成功   false  失败                  ***/
  /*** 函数参数1: 类型  int       channel_id                                ***/
  /*** 函数参数2: 类型  int32_t       ssrc                                  ***/
  /****************************************************************************/
  bool RequestRemoteSsrc(int channel_id, int flag, int32_t ssrc);
  /****************************************************************************/
  /*** 函数名: GetNumberOfVideoDevices                                      ***/
  /*** 功能:   获取当前系统所有视频设备                                     ***/
  /*** 返回值: 类型  uint32_t    视频设备个数                               ***/
  /****************************************************************************/
  uint32_t GetNumberOfVideoDevices();

  /****************************************************************************/
  /*** 函数名: GetVideoDevices                                ***/
  /*** 功能:   获取当前系统所有视频设备信息                                 ***/
  /*** 返回值: 类型  bool true  成功   false  失败时返回需要缓存大小        ***/
  /*** 函数参数1: 名称 jsonDeviceInfos  类型   char*  json格式的信息字符串  ***/
  /*** 函数参数2: 名称  ength           类型   int*  [in out]输入输出参数   ***/
  /****************************************************************************/
  bool GetVideoDevices(char* jsonDeviceInfos, int* length);

  /****************************************************************************/
  /*** 函数名: GetVideoCodecs                                ***/
  /*** 功能:   获取当前系统所有视频设备信息                                 ***/
  /*** 返回值: 类型  bool true  成功   false  失败时返回需要缓存大小        ***/
  /*** 函数参数1:名称 jsonVideoCodecInfos 类型 char*  json格式的信息字符串  ***/
  /*** 函数参数2:名称 length            类型   int*  [in out]输入输出参数   ***/
  /****************************************************************************/
  bool GetVideoCodecs(char* jsonVideoCodecInfos, int* length);

  /****************************************************************************/
  /*** 函数名: GetAudioCodecs                                ***/
  /*** 功能:   获取当前系统所有视频设备信息                                 ***/
  /*** 返回值: 类型  bool true  成功   false  失败时返回需要缓存大小        ***/
  /*** 函数参数1:名称 jsonAudioCodecInfos 类型 char*  json格式的信息字符串  ***/
  /*** 函数参数2:名称 length            类型   int*  [in out]输入输出参数   ***/
  /****************************************************************************/
  bool GetAudioCodecs(char* jsonAudioCodecInfos, int* length);

  //////////////////////////ylr interface start////////////////////////////////
  /****************************************************************************/
  /*** 函数名: 设置视频的Nack功能                                           ***/
  /*** 功能:   开启或关闭Nack功能 ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id       类型   int                       ***/
  /*** 函数参数2: 名称    enable_nack	   类型   bool                      ***/
  /****************************************************************************/
  bool SetVideoNackStatus(const int channelId, const bool enable_nack);

  /****************************************************************************/
  /*** 函数名: 设置视频的Fec功能                                            ***/
  /*** 功能:   开启或关闭视频Fec功能                                        ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id       类型   int                       ***/
  /*** 函数参数2: 名称    enable    	   类型   bool                      ***/
  /*** 函数参数3: 名称    payloadtype_red  类型   uint8_t                   ***/
  /*** 函数参数4: 名称    payloadtype_fec  类型   uint8_t                   ***/
  /****************************************************************************/
  bool SetVideoUlpFecStatus(const int channelId,
                            const bool enable,
                            const uint8_t payloadtype_red,
                            const uint8_t payloadtype_fec);

  /****************************************************************************/
  /*** 函数名: 设置视频的DegradationMode功能                                ***/
  /*** 功能:   开启或关闭视频DegradationMode功能                            ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id       类型   int                       ***/
  /*** 函数参数2: 名称    mode      	   类型   DegradationPreference     ***/
  /****************************************************************************/
  bool SetVideoDegradationMode(const int channelId,
                               const webrtc::DegradationPreference mode);
  /****************************************************************************/
  /*** 函数名: 关键帧发送                                                   ***/
  /*** 功能:   发送关键帧                                                   ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id       类型   int                       ***/
  /****************************************************************************/
  bool SendKeyFrame(const int channelId);
  /****************************************************************************/
  /*** 函数名: 设置关键帧回调请求                                           ***/
  /*** 功能:   设置请求关键帧回调                                           ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    channel_id       类型   int                       ***/
  /*** 函数参数1: 名称    cb        类型  OnRequestKeyFrameCallback函数指针 ***/
  /****************************************************************************/
  bool SetKeyFrameRequestCallback(const int channelId,
                                  OnRequestKeyFrameCallback cb);
  //////////////////////////ylr interface end////////////////////////////////

  //////////////////////////zjy interface start///////////////////////////////

  /****************************************************************************/
  /*** 函数名: 设置回声消除                                                 ***/
  /*** 功能:   音频通话回音消除                                             ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    enable       类型   bool                          ***/
  /*** 此函数需要在CreateChannel之前使用                                    ***/
  /****************************************************************************/
  bool SetAEC(bool enable);
  /****************************************************************************/
  /*** 函数名: 设置语音自动增益功能                                         ***/
  /*** 功能:   语音自动增益                                                 ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    enable       类型   bool                          ***/
  /*** 此函数需要在CreateChannel之前使用                                    ***/
  /****************************************************************************/
  bool SetAGC(bool enable);
  /****************************************************************************/
  /*** 函数名: 设置语音噪声抑制功能                                         ***/
  /*** 功能:   语音噪声抑制                                                 ***/
  /*** 返回值: 类型  bool    true  成功      false  失败                    ***/
  /*** 函数参数1: 名称    enable       类型   bool                          ***/
  /*** 此函数需要在CreateChannel之前使用                                    ***/
  /****************************************************************************/
  bool SetNS(bool enable);

  /****************************************************************************/
  /*** 函数名: 创建音频设备对象                                             ***/
  /*** 功能:   获取底层AudioDeviceModule对象                                ***/
  /*** 返回值: 类型  scoped_refptr<webrtc::AudioDeviceModule>               ***/
  /****************************************************************************/
  bool CreateAudioDevice();

  // int GetAudioRecordingDevice(const int a);

  /****************************************************************************/
  /*** 函数名: 设置录音音量                                                 ***/
  /*** 功能:   设置录音设备录音音量                                         ***/
  /*** 返回值: 类型  bool        true  成功      false   失败               ***/
  /*** 函数参数1: 名称   vol                类型    uint32_t                ***/
  /****************************************************************************/
  bool SetAudioRecordingVolume(uint32_t vol);

  /****************************************************************************/
  /*** 函数名: 获取录音设备列表 ***/
  /*** 功能:   获取录音设备列表字符串 ***/
  /*** 返回值: 类型  char* ***/
  /*** 函数参数1: 名称   length                 类型    int* ***/
  /*****************************************************************************/
  char* GetAudioDeviceList(int* length);

  /****************************************************************************/
  /*** 函数名: 设置录音设备 ***/
  /*** 功能:   根据索引选择需要使用的录音设备 ***/
  /*** 返回值: 类型  bool        true  成功      false   失败 ***/
  /*** 函数参数1: 名称   index                   类型    int ***/
  /*****************************************************************************/
  bool SetAudioRecordingDevice(int index);

  bool SetAudioRecordingDeviceOnFlight(int index);
  bool SetAudioPlayoutDeviceOnFlight(int index);

  /****************************************************************************/
  /*** 函数名: 设置播放设备                                                 ***/
  /*** 功能:   根据索引选择需要使用播放设备                                 ***/
  /*** 返回值: 类型  bool        true  成功      false   失败               ***/
  /*** 函数参数1: 名称   index                   类型    int                ***/
  /*****************************************************************************/
  bool SetAudioPlayoutDevice(int index);
  ///////////////////////// zjy interface end//////////////////////////////////
  //#if defined(WEBRTC_WIN)
  int CreateDesktopCapture(int type);
  int SetDesktopSourceID(int type, int id);
  int GetWindowsList(int type, webrtc::DesktopCapturer::SourceList& source);
  int StartScreenShare();
  int StopScreenShare();
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

#endif

 private:
  MediaClient();

  bool CreateThreads();
  bool CreateRtcEventLog();
#if defined(WEBRTC_ANDROID)
  bool CreateChannelManager(
      std::unique_ptr<webrtc::VideoEncoderFactory> encoderFactory,
      std::unique_ptr<webrtc::VideoDecoderFactory> decoderFactory);
  bool AndroidInitialize(JNIEnv* env,
                         jobject jencoder_factory,
                         jobject jdecoder_factory);
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

  void DestroyTransceiverChannel(
      rtc::scoped_refptr<
          webrtc::RtpTransceiverProxyWithInternal<webrtc::RtpTransceiver>>
          transceiver);

  void DestroyChannelInterface(cricket::ChannelInterface* channel);

  cricket::ChannelInterface* GetChannel(const std::string& mid);

  cricket::RtpDataChannel* rtp_data_channel() const;

  void OnSentPacket_Voice(const rtc::SentPacket& sent_packet);
  void OnSentPacket_Video(const rtc::SentPacket& sent_packet);
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

  static rtc::CriticalSection m_critical;
  bool isCreateCall;
  char* pAudioDevice;
  // std::unique_ptr<rtc::LogSink*> ec_log_ = nullptr;
  rtc::LogSink* ec_log_ = nullptr;
  bool bfirst = true;
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_tracks_[5];

  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_tracks_[20];

  int vsum_;
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

  std::unique_ptr<win_render::RenderWndsManager> renderWndsManager_;
#endif
  std::unique_ptr<VideoRenderer> local_renderer_;
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
