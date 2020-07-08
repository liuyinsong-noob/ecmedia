#include "video_renderer_impl.h"

#include "win_d3d9_render.h"
#include "win_gdi_render.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/clock.h"

// TODO : STANDARD_RENDERING is platform relevant
#define STANDARD_RENDERING kRenderWindows

VideoRenderer* VideoRenderer::CreateVideoRenderer(
    int channelid,
    void* windows,
    int render_mode,
	bool mirror,
    webrtc::VideoTrackInterface* track_to_render,
    rtc::Thread* worker_thread,
    VideoRenderType type,
    rtc::VideoSinkWants wants) {
  VideoRenderType render_type = type;
  if (render_type == kRenderDefault) {
    render_type = STANDARD_RENDERING;
  }

  RTC_LOG(LS_INFO) << " CreateVideoRenderer "
                   << " hubin_render";
  return new VideoRenderImpl(channelid, windows, render_mode, mirror, track_to_render,
                             worker_thread,
                             render_type, wants);
}


VideoRenderImpl::VideoRenderImpl(int channelid,
                                void* windows,
                                 int render_mode,
	                             bool mirror,
                                 webrtc::VideoTrackInterface* track_to_render,
                                 rtc::Thread* worker_thread,
                                 VideoRenderType render_type,
                                 rtc::VideoSinkWants wants)
    : channelid_(channelid),
      worker_thread_(worker_thread),
      rendered_track_(track_to_render),
      video_window_(windows) {
  switch (render_type) {
    case kRenderWindows: {
      RTC_LOG(LS_INFO) << " new WinD3d9Render begins " << (long)this
                       << " hubin_render";
      _ptrRenderer = new WinD3d9Render(windows, render_mode, mirror);
	  RTC_LOG(LS_INFO) << " CreateVideoRenderer ends " << (long)this << " render:" << _ptrRenderer
                       << " hubin_render";
      break;
    }
      // TODO: other Platform render implementor
    default:
      break;
  }
 
  if (_ptrRenderer) {
    if (_ptrRenderer->Init() == -1) {
      delete _ptrRenderer;
      _ptrRenderer = nullptr;
       RTC_LOG(LS_ERROR) << __FUNCTION__ << " _ptrRenderer->Init() failed.";
      _ptrRenderer = new WinGdiRender(windows, render_mode, mirror);
       RTC_LOG(INFO) << "create gdi Renderer...";
      return;
    }
  }
  if (rendered_track_) {
    // note: crash here because of thread_check
    worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
      return rendered_track_->AddOrUpdateSink(this, wants);
    });
  }
}

VideoRenderImpl::~VideoRenderImpl() {

 RTC_LOG(LS_INFO) << " ~VideoRenderImpl begins " << (long)this
                   << " hubin_render";
  if (rendered_track_) {
	worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
		return rendered_track_->RemoveSink(this); });
  }
  if (_ptrRenderer) {
    VideoRenderType videoRenderType = _ptrRenderer->RenderType();
    switch (videoRenderType) {
      case kRenderWindows: {
        //modify by ytx_wx begin...
        /*WinD3d9Render* ptrRenderer =
            reinterpret_cast<WinD3d9Render*>(_ptrRenderer);
        _ptrRenderer = nullptr;
        delete ptrRenderer;*/
        delete _ptrRenderer;
        _ptrRenderer = nullptr;
		//modify by ytx_wx end...
		RTC_LOG(LS_INFO) << " ~VideoRenderImpl begins " << (long)this
                         << "render " << _ptrRenderer
                         << " hubin_render";
        break;
      }
      default:
        break;
    }
  }
}

int VideoRenderImpl::StartRender() {
  // windows platform render start is create a render thread.
  if (_ptrRenderer)
    _ptrRenderer->StartRenderInternal();

  return 0;
}

int VideoRenderImpl::StopRender() {
  // windows platform render start is create a render thread.
  if (_ptrRenderer)
    _ptrRenderer->StopRenderInternal();

  return 0;
}
int VideoRenderImpl::UpdateVideoTrack(
    webrtc::VideoTrackInterface* track_to_render,
    rtc::VideoSinkWants wants) {

	worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
    if (rendered_track_) {
      rendered_track_->RemoveSink(this);
    }
    rendered_track_ = track_to_render;
    if (rendered_track_) {
      rendered_track_->AddOrUpdateSink(this, wants);
    }
	});
  return 0;
}
void VideoRenderImpl::OnFrame(const webrtc::VideoFrame& frame) {
  if(last_width_ != frame.width() || last_height_ != frame.height()){
   last_width_ = frame.width();
   last_height_ = frame.height();
   if(callback_){
      callback_(channelid_, last_width_, last_height_ );
    }
  }
  if (_ptrRenderer) {
    if (!_ptrRenderer->IsRunning())
      return;
    VideoRenderType render_type = _ptrRenderer->RenderType();
    switch (render_type) {
      case kRenderWindows:
        _ptrRenderer->RenderFrame(frame);
        break;
      case kRenderiOS:
        break;
      default:
        break;
    }
  }
}
 bool VideoRenderImpl::RegisterRemoteVideoResoluteCallback(
      ECMedia_FrameSizeChangeCallback* callback){
   return  callback_ = callback;
}