#include "video_capturer.h"

#include "api/video/i420_buffer.h"
#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/logging.h"

VideoCapturer* VideoCapturer::CreateCaptureDevice(
    const char* device_unique_idUTF8,
    const uint32_t device_unique_idUTF8Length) {
  std::unique_ptr<VideoCapturer> video_capturer(new VideoCapturer());

  if (!video_capturer->Init(device_unique_idUTF8, device_unique_idUTF8Length)) {
    RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer";
    return nullptr;
  }

  return video_capturer.release();
}

VideoCapturer* VideoCapturer::CreateCaptureDevice(
	 int width,int height, int  fps,int index)  {
  std::unique_ptr<VideoCapturer> video_capturer(new VideoCapturer());


  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());

  index = index >= (int)device_info->NumberOfDevices()
              ? (int)device_info->NumberOfDevices()-1
                 : index;
  char device_name[256];
  char unique_name[256];
  if (device_info->GetDeviceName(static_cast<uint32_t>(index),
                                 device_name, sizeof(device_name), unique_name,
                                 sizeof(unique_name)) != 0) {
    return nullptr;
  }

  if (!video_capturer->Init(unique_name, sizeof(unique_name))) {
    RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer";
    return nullptr;
  }
  return video_capturer.release();
}

VideoCapturer::VideoCapturer() : vcm_(nullptr) {
  capability_.width = 640;
  capability_.height = 480;
  capability_.maxFPS = 30;
  capability_.videoType = webrtc::VideoType::kI420;
}

VideoCapturer::~VideoCapturer() {
  Destroy();
}

bool VideoCapturer::Init(const char* device_unique_idUTF8,
                         const uint32_t device_unique_idUTF8Length) {
  // note: VideoCaptureFactory now only support windows, what about android and
  // ios??
  vcm_ = webrtc::VideoCaptureFactory::Create(device_unique_idUTF8);
  if (!vcm_) {
    return false;
  }
  vcm_->RegisterCaptureDataCallback(this);
  return true;
}

void VideoCapturer::Destroy() {
  if (!vcm_)
    return;

  vcm_->StopCapture();
  vcm_->DeRegisterCaptureDataCallback();
  // Release reference to VCM.
  vcm_ = nullptr;
}
bool VideoCapturer::Init(int width,
	int height,
	int fps,
	const char* device_unique_idUTF8,
	const uint32_t device_unique_idUTF8Length) {
  capability_.width = width;
  capability_.height = height;
  capability_.maxFPS = fps;
  capability_.videoType = webrtc::VideoType::kI420;
  return Init(device_unique_idUTF8, device_unique_idUTF8Length);
}
int VideoCapturer::StartCapture(webrtc::VideoCaptureCapability& capability) {
  capability_ = capability;
  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return -1;
  }
  RTC_CHECK(vcm_->CaptureStarted());
  return 0;
}

int VideoCapturer::StopCapture() {
  if (vcm_->StopCapture() != 0) {
    return -1;
  }
  return 0;
}

void VideoCapturer::AddOrUpdateSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
  broadcaster_.AddOrUpdateSink(sink, wants);
  UpdateVideoAdapter();
}

void VideoCapturer::RemoveSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
  broadcaster_.RemoveSink(sink);
  UpdateVideoAdapter();
}

void VideoCapturer::UpdateVideoAdapter() {
  rtc::VideoSinkWants wants = broadcaster_.wants();
  video_adapter_.OnResolutionFramerateRequest(
      wants.target_pixel_count, wants.max_pixel_count, wants.max_framerate_fps);
}

void VideoCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  int cropped_width = 0;
  int cropped_height = 0;
  int out_width = 0;
  int out_height = 0;

  if (!video_adapter_.AdaptFrameResolution(
          frame.width(), frame.height(), frame.timestamp_us() * 1000,
          &cropped_width, &cropped_height, &out_width, &out_height)) {
    // Drop frame in order to respect frame rate constraint.
    return;
  }

  broadcaster_.OnFixedFrame(frame);

  if (out_height != frame.height() || out_width != frame.width()) {
    // Video adapter has requested a down-scale. Allocate a new buffer and
    // return scaled version.
    rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
        webrtc::I420Buffer::Create(out_width, out_height);
    scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());
    broadcaster_.OnFrame(webrtc::VideoFrame::Builder()
                             .set_video_frame_buffer(scaled_buffer)
                             .set_rotation(webrtc::kVideoRotation_0)
                             .set_timestamp_us(frame.timestamp_us())
                             .set_id(frame.id())
                             .build());
  } else {
    // No adaptations needed, just return the frame as is.
    broadcaster_.OnFrame(frame);
  }
}