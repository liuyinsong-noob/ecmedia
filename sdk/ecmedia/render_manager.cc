
#include "render_manager.h"


#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/call/call_factory_interface.h"
#include "api/fec_controller.h"
#include "api/media_stream_proxy.h"
#include "api/media_stream_track_proxy.h"
#include "api/rtp_sender_interface.h"
#include "api/transport/network_control.h"
#include "api/video/i420_buffer.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_track_source_proxy.h"

#include "media/base/codec.h"
#include "media/base/media_engine.h"
#include "media/base/rtp_data_engine.h"
#include "media/engine/webrtc_media_engine.h"
#include "media/engine/webrtc_voice_engine.h"
#include "media/sctp/sctp_transport.h"

#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"

#include "rtc_base/bind.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/strings/json.h"
#include "rtc_base/system/file_wrapper.h"

#include "pc/audio_track.h"
#include "pc/local_audio_source.h"
#include "pc/media_stream.h"
#include "pc/rtp_parameters_conversion.h"
#include "pc/video_track.h"
#include "pc/video_track_source.h"

#include "third_party/abseil-cpp/absl/memory/memory.h"
#include "third_party/abseil-cpp/absl/strings/ascii.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"

#include "logging/rtc_event_log/rtc_event_log_factory.h"
#include "system_wrappers/include/field_trial.h"
#include "video_capturer/capturer_track_source.h"

#define API_LOG(sev) RTC_LOG(sev) << "[API] " << __FUNCTION__ << " "
#if defined(WEBRTC_IOS)
#include "sdk/ecmedia/ios/objc_client.h"
#endif
///////////////////////////////////VideoRenderer/////////////////////////////////

RenderManager::RenderManager() {}
RenderManager::~RenderManager() {
  RTC_LOG(INFO) << __FUNCTION__ << "(), " << " this:" << this
                << " begin... ";
  //
  std::map<int, render_list>::iterator it = map_video_renders_.begin();
  while (it != map_video_renders_.end()) {
    if (it->second.size() > 0) {
      std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
      while (renderIter != it->second.end()) {
        delete *renderIter;
        renderIter++;
      }
      it->second.clear();
    }
    it++;
  }
  map_video_renders_.clear();
}

bool RenderManager::AttachVideoRender(int channelId,
                                      void* videoView,
                                      int render_mode,
                                      int mirror_mode,
                                      rtc::Thread* worker_thread,
                                      rtc::VideoSinkWants wants) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << ", channelId: " << channelId << ", videoView: " << videoView
                << ", render_mode: " << render_mode
                << ", mirror_mode: " << mirror_mode;

  std::map<int, render_list>::iterator it = map_video_renders_.begin();
  while (it != map_video_renders_.end()) {
    if (it->second.size() > 0) {
      std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
      while (renderIter != it->second.end()) {
        if ((*renderIter)->WindowPtr() == videoView) {
          //RTC_LOG(LS_ERROR)
          //    << __FUNCTION__ << "(), failed, videoView:" << videoView
          //    << " already attach a render.";
          //return false;
          delete *renderIter;
          it->second.erase(renderIter);
          break;
        }
        renderIter++;
      }
    }
    it++;
  }
#if defined(WEBRTC_LINUX_ONLY)
VideoRenderer* render = VideoRenderer::CreateVideoRenderer(
	channelId,videoView, render_mode, mirror_mode, nullptr, worker_thread, kRenderX11,wants);
#else 
   VideoRenderer* render = VideoRenderer::CreateVideoRenderer(
     channelId, videoView, render_mode, mirror_mode, nullptr, worker_thread, kRenderWindows, wants);
#endif  
  std::map<int, render_list>::iterator iter = map_video_renders_.find(channelId);
  if (iter == map_video_renders_.end()) {
    render_list _renderList;
    _renderList.push_back(render);
    map_video_renders_[channelId] = _renderList;
  } else {
    iter->second.push_back(render);
  }

  return true;
}

bool RenderManager::DetachVideoRender(int channelId, void* videoView) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " this:" << this
                << ", channelId: " << channelId << ", videoView: " << videoView;

  if (videoView == nullptr) {
    RTC_LOG(LERROR) << __FUNCTION__ << "() "
                    << " failed, channelId: " << channelId
                    << ", videoView is null";
    return false;
  }

  std::map<int, render_list>::iterator it = map_video_renders_.find(channelId);
  if (it == map_video_renders_.end()) {
    RTC_LOG(LERROR) << __FUNCTION__ << "() can't find channelId:" << channelId;
    return false;
  }

  //删除指定view的render
  std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
  while (renderIter != it->second.end()) {
    if ((*renderIter)->WindowPtr() == videoView) {
      delete *renderIter;
      it->second.erase(renderIter);
      break;
    }
    renderIter++;
  }
  //如果render列表已经空了，就从map表中删除
  if (it->second.size() == 0) {
    map_video_renders_.erase(it);
  }

  return true;
}

void RenderManager::RemoveAllVideoRender(int channelId) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << " this:" << this
                << ", channelId: " << channelId;

  std::map<int, render_list>::iterator it = map_video_renders_.find(channelId);
  if (it == map_video_renders_.end()) {
    RTC_LOG(LERROR) << __FUNCTION__ << "() can't find channelId:" << channelId;
    return;
  }
  // channel所有的render都释放
  std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
  while (renderIter != it->second.end()) {
    delete *renderIter;
    renderIter++;
  }
  it->second.clear();
  map_video_renders_.erase(it);
  return;
}


bool RenderManager::UpdateOrAddVideoTrack(
    int channelId,
    webrtc::VideoTrackInterface* track_to_render,
    rtc::VideoSinkWants wants) {
  RTC_LOG(INFO) << __FUNCTION__ << "() begin "
                << ", channelId: " << channelId << " track:" << track_to_render;
  std::map<int, render_list>::iterator it = map_video_renders_.find(channelId);
  if (it == map_video_renders_.end()) {
    RTC_LOG(LERROR) << __FUNCTION__ << "() can't find channelId:" << channelId;
    return false;
  }

  std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
  while (renderIter != it->second.end()) {
    (*renderIter)->UpdateVideoTrack(track_to_render, wants);
    renderIter++;
  }
 RTC_LOG(INFO) << __FUNCTION__ << "() end "
 << ", channelId: " << channelId << " track:" << track_to_render;
  return true;
}
int RenderManager::SaveVideoSnapshot(int channelID, const char* fileName)
{
#if defined(WEBRTC_WIN)
	std::map<int, render_list>::iterator it = map_video_renders_.find(channelID);
	if (it == map_video_renders_.end()) {
		RTC_LOG(LERROR) << __FUNCTION__ << "() can't find channelId:" << channelID;
		return -1;
	}
	
	std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
	if (renderIter != it->second.end()) {
		(*renderIter)->SaveVideoSnapshot(fileName);
	}
#endif	
	return 0;
}
bool RenderManager::StartRender(int channelId, void* videoView) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << ", channelId: " << channelId << " videoView:" << videoView;

  std::map<int, render_list>::iterator it = map_video_renders_.find(channelId);
  if (it == map_video_renders_.end()) {
    RTC_LOG(LERROR) << __FUNCTION__ << "() can't find channelId:" << channelId;
    return false;
  }
  if (videoView == nullptr) {
    std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
    while (renderIter != it->second.end()) {
      (*renderIter)->StartRender();
      renderIter++;
    }
    return true;
  } else {
    std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
    while (renderIter != it->second.end()) {
      if ((*renderIter)->WindowPtr() == videoView) {
        (*renderIter)->StartRender();
        return true;
      }
      renderIter++;
    }
    return false;
  }

}


bool RenderManager::StopRender(int channelId, void* videoView) {
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << ", channelId: " << channelId << " videoView:" << videoView;

  std::map<int, render_list>::iterator it = map_video_renders_.find(channelId);
  if (it == map_video_renders_.end()) {
    RTC_LOG(LERROR) << __FUNCTION__ << "() can't find channelId:" << channelId;
    return false;
  }

  if (videoView == nullptr) {
    std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
    while (renderIter != it->second.end()) {
      (*renderIter)->StopRender();
      renderIter++;
    }
    return true;
  } else {
    std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
    while (renderIter != it->second.end()) {
      if ((*renderIter)->WindowPtr() == videoView) {
        (*renderIter)->StopRender();
        return true;
      }
      renderIter++;
    }
    return false;
  }
  
}
bool RenderManager::RegisterRemoteVideoResoluteCallback(
      int channelid,
      ECMedia_FrameSizeChangeCallback* callback){
  RTC_LOG(INFO) << __FUNCTION__ << "() "
                << ", channelId: " << channelid;
   std::map<int, render_list>::iterator it = map_video_renders_.find(channelid);
  if (it == map_video_renders_.end()) {
     RTC_LOG(LERROR) << __FUNCTION__ << "() can't find channelId:" << channelid;
    return false;
  }

  std::list<VideoRenderer*>::iterator renderIter = it->second.begin();
  while (renderIter != it->second.end()) {
    (*renderIter)->RegisterRemoteVideoResoluteCallback(callback);
    renderIter++;
  }
  return true;
 }
