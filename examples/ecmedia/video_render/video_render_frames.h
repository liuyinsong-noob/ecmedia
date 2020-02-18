#ifndef VIDEO_RENDER_FRAMES_H
#define VIDEO_RENDER_FRAMES_H

#include <list>
#include <vector>
#include "api/video/video_frame.h"

class VideoRenderFrames {
 public:
  VideoRenderFrames();
  ~VideoRenderFrames();

  int AddFrame(const webrtc::VideoFrame* frame);
  // Get a frame for rendering, if it's time to render.
  webrtc::VideoFrame* FrameToRender();
  int ReturnFrame(webrtc::VideoFrame* frame);
  int ReleaseAllFrames();

 private:
  // 10 seconds for 30 fps.
  enum { KMaxNumberOfFrames = 300 };
  // Don't render frames with timestamp older than 500ms from now.
  enum { KOldRenderTimestampMS = 500 };
  // Don't render frames with timestamp more than 10s into the future.
  enum { KFutureRenderTimestampMS = 10000 };
  typedef std::list<webrtc::VideoFrame*> FrameList;

  
  FrameList incoming_frames_;
  FrameList empty_frames_;

  std::vector<webrtc::VideoFrame> buffer_;
};

#endif  // VIDEO_RENDER_FRAMES_H
