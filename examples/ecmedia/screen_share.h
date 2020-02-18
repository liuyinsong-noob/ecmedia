#pragma once

#include "../../api/scoped_refptr.h"
#include "../../api/video/i420_buffer.h"
//#include "../../media/base/videocapturer.h"
#include "../../modules/video_capture/video_capture.h"
#include "../../media/base/video_common.h"
#include "../../modules/desktop_capture/desktop_capturer.h"
#include "../../modules/desktop_capture/desktop_frame.h"
#include  "../../rtc_base/message_handler.h"
#include "../../rtc_base/thread.h"

class DeskCapturer :  // public VideoCapturer,
                   public rtc::MessageHandler,
                   public webrtc::DesktopCapturer::Callback {
 public:
  DeskCapturer();
  ~DeskCapturer();

  void CaptureFrame();

  //virtual cricket::CaptureState 
	void  Start( );
  //const cricket::VideoFormat& capture_format

  virtual void Stop();
  virtual bool IsRunning();
  virtual bool IsScreencast() const { return true; }
  virtual void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                               std::unique_ptr<webrtc::DesktopFrame> frame);
  virtual void OnMessage(rtc::Message* msg);

 protected:
  virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs);

 private:
  std::unique_ptr<webrtc::DesktopCapturer> capturer;
  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;

   std::unique_ptr<webrtc::DesktopFrame> frame_;
};