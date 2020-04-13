#include "screen_share.h"

//#include "../../base/threading/thread.h "
#include "../../modules/desktop_capture/desktop_capture_options.h"
#include "../../third_party/libyuv/include/libyuv.h"

    DeskCapturer::DeskCapturer() {
  std::vector<cricket::VideoFormat> formats;
  formats.push_back(cricket::VideoFormat(
      800, 600, cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_I420));
//  SetSupportedFormats(formats);
}

DeskCapturer ::~DeskCapturer() {}
    //cricket::CaptureState
//const cricket::VideoFormat& capture_format
void DeskCapturer::Start() {
  cricket::VideoFormat supported;
 /* if (GetBestCaptureFormat(capture_format, &supported))
    SetCaptureFormat(&supported);
  SetCaptureState(cricket::CS_RUNNING);*/

  capturer = webrtc::DesktopCapturer::CreateWindowCapturer(
      webrtc::DesktopCaptureOptions::CreateDefault());
 webrtc:: DesktopCapturer::SourceList sources;
 capturer->Start(this);
 capturer->GetSourceList(&sources);
  

  // Verify that we can select and capture each window.
  for (auto it = sources.begin(); it != sources.end(); ++it) {
    frame_.reset();
    if (capturer->SelectSource(it->id)) {
      capturer->CaptureFrame();
    }
  }

 /* auto options = webrtc::DesktopCaptureOptions::CreateDefault();
  options.set_allow_directx_capturer(true);
  capturer = webrtc::DesktopCapturer::CreateScreenCapturer(options);
  capturer->Start(this);
  CaptureFrame();*/
 // return cricket::CS_RUNNING;
}

void DeskCapturer::Stop() {
 // SetCaptureState(cricket::CS_STOPPED);
 // SetCaptureFormat(NULL);
}

bool DeskCapturer::IsRunning() {
  //return capture_state() == cricket::CS_RUNNING;
  return false;
}

bool DeskCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs) {
  fourccs->push_back(cricket::FOURCC_I420);
  fourccs->push_back(cricket::FOURCC_MJPG);
  return true;
}

void DeskCapturer::OnCaptureResult(
    webrtc::DesktopCapturer::Result result,
                                 std::unique_ptr<webrtc::DesktopFrame> frame) {
  if (result != webrtc::DesktopCapturer::Result::SUCCESS)
    return;

  int width = frame->size().width();
  int height = frame->size().height();

  if (!i420_buffer_.get() ||
      i420_buffer_->width() * i420_buffer_->height() < width * height) {
    i420_buffer_ = webrtc::I420Buffer::Create(width, height);
  }
  libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
                        i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
                        i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
                        i420_buffer_->StrideV(), 0, 0, width, height, width,
                        height, libyuv::kRotate0, libyuv::FOURCC_ARGB);

 // OnFrame(webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0),
 //         width, height);
}

void DeskCapturer::OnMessage(rtc::Message* msg) {
  if (msg->message_id == 0)
    CaptureFrame();
}

void DeskCapturer::CaptureFrame() {
  capturer->CaptureFrame();

  rtc::Location loc(__FUNCTION__, __FILE__);
  rtc::Thread::Current()->PostDelayed(loc, 33, this, 0);
}