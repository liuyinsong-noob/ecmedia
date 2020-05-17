
#ifndef RENDER_MANAGER_H_
#define RENDER_MANAGER_H_

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

#include "ec_log.h"


#include "media/base/adapted_video_track_source.h"

//};

class RenderManager{
 public:
  RenderManager();
  ~RenderManager();
  
  bool AttachVideoRender(int channelId,
                      void* videoView,
                      int render_mode,
                      int mirror_mode,
                      rtc::Thread* worker_thread);
  bool DetachVideoRender(int channelId, void* winRemote);
  void RemoveAllVideoRender(int channelId);

  bool UpdateOrAddVideoTrack(int channelId,
                             webrtc::VideoTrackInterface* track_to_render);
  bool StartRender(int channelId, void* videoView);
  bool StopRender(int channelId, void* videoView);

 private:
  using ptr_render = std::unique_ptr<VideoRenderer>;
  using render_list = std::list<VideoRenderer*>;
  std::map<int, render_list> map_video_renders_;
};

#endif  // MEDIA_CLIENT_H_
