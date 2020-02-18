#include "video_renderer.h"

#include "win_d3d9_render.h"

// TODO : STANDARD_RENDERING is platform relevant
#define STANDARD_RENDERING kRenderWindows

VideoRenderer* VideoRenderer::CreateVideoRenderer(
    void* windows,
    bool full_screen,
    webrtc::VideoTrackInterface* track_to_render,
    rtc::Thread* worker_thread,
    VideoRenderType type) {
  VideoRenderType render_type = type;
  if (render_type == kRenderDefault) {
    render_type = STANDARD_RENDERING;
  }

  return new VideoRenderImpl(windows, full_screen, track_to_render, worker_thread,
                             render_type);
}


VideoRenderImpl::VideoRenderImpl(void* windows,
                                 bool full_screen,
                                 webrtc::VideoTrackInterface* track_to_render,
                                 rtc::Thread* worker_thread,
                                 VideoRenderType render_type)
    : worker_thread_(worker_thread), rendered_track_(track_to_render) {
  switch (render_type) {
    case kRenderWindows: {
      _ptrRenderer = new WinD3d9Render(windows, full_screen);
      break;
    }
      // TODO: other Platform render implementor
    default:
      break;
  }

  if (_ptrRenderer) {
    if (_ptrRenderer->Init() == -1) {
      // WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id,
      //             "%s: _ptrRenderer->Init() failed.", __FUNCTION__);
    }
  }

  // note: crash here because of thread_check
  worker_thread_->Invoke<void>(RTC_FROM_HERE, 
      [&] {return rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
  });
}

VideoRenderImpl::~VideoRenderImpl() {
  worker_thread_->Invoke<void>(RTC_FROM_HERE, [&] {
    return rendered_track_->RemoveSink(this); });
  if (_ptrRenderer) {
    VideoRenderType videoRenderType = _ptrRenderer->RenderType();
    switch (videoRenderType) {
      case kRenderWindows: {
        WinD3d9Render* ptrRenderer =
            reinterpret_cast<WinD3d9Render*>(_ptrRenderer);
        _ptrRenderer = nullptr;
        delete ptrRenderer;
        break;
      }
      default:
        break;
    }
  }
}

int VideoRenderImpl::StartRender(int channel_id) {
  // windows platform render start is create a render thread.
  if (_ptrRenderer)
    _ptrRenderer->StartRenderInternal();

  return 0;
}

int VideoRenderImpl::StopRender(int channel_id) {
  // windows platform render start is create a render thread.
  if (_ptrRenderer)
    _ptrRenderer->StopRenderInternal();

  return 0;
}

void VideoRenderImpl::OnFrame(const webrtc::VideoFrame& frame) {
  if (_ptrRenderer) {
    if (!_ptrRenderer->IsRunning())
      return;
    VideoRenderType render_type = _ptrRenderer->RenderType();
    switch (render_type) {
      case kRenderWindows:
        _ptrRenderer->RenderFrame(frame);
        break;
      default:
        break;
    }
  }
}