#include "video_render_frames.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/logging.h"

VideoRenderFrames::VideoRenderFrames() {}

VideoRenderFrames::~VideoRenderFrames() {
  empty_frames_.clear();
  incoming_frames_.clear();
  buffer_.clear();
}

int VideoRenderFrames::AddFrame(const webrtc::VideoFrame* new_frame) {

  const int64_t time_now = rtc::TimeMillis();
  // Drop old frames only when there are other frames in the queue, otherwise, a
  // really slow system never renders any frames.
  if (!incoming_frames_.empty() &&
      new_frame->render_time_ms() + KOldRenderTimestampMS < time_now) {

    RTC_LOG(LS_WARNING)
        << __FUNCTION__
        << ": too old frame, timestamp=" << new_frame->timestamp();
    return -1;
  }

  if (new_frame->render_time_ms() > time_now + KFutureRenderTimestampMS) {
    RTC_LOG(LS_WARNING) 
                        << "frame too long into the future, timestamp="
                        << new_frame->timestamp();
    return -1;
  }

  // Get an empty frame
  webrtc::VideoFrame* frame_to_add = NULL;
  if (!empty_frames_.empty()) {
    frame_to_add = empty_frames_.front();
    empty_frames_.pop_front();
  }
  if (!frame_to_add) {
    if (empty_frames_.size() + incoming_frames_.size() > KMaxNumberOfFrames) {
     
      RTC_LOG(LS_WARNING) << " too many frames. ";
      return -1;
    }

    // Allocate new memory.
    RTC_LOG(LS_INFO) << "allocating render buffer: "
                     << empty_frames_.size() + incoming_frames_.size();

    // copy new_frame to buffer_
    buffer_.push_back(*new_frame);
    frame_to_add = &buffer_.back();

    if (!frame_to_add) {
      RTC_LOG(LS_ERROR) << " could not create new frame for render.";
      return -1;
    }
  }

  // note: copy new incoming frame to the local vector (_buffer)
  *frame_to_add = *new_frame;
  incoming_frames_.push_back(frame_to_add);

  RTC_LOG(LS_INFO) << "new frame for render. size is : " << incoming_frames_.size();

  return static_cast<int32_t>(incoming_frames_.size());
}

webrtc::VideoFrame* VideoRenderFrames::FrameToRender() {
  webrtc::VideoFrame* render_frame = NULL;
  FrameList::iterator iter = incoming_frames_.begin();
  uint32_t render_delay_ms = 0;
  while (iter != incoming_frames_.end()) {
    webrtc::VideoFrame* oldest_frame_in_list = *iter;
    if (oldest_frame_in_list->render_time_ms() <=
        rtc::TimeMillis() + render_delay_ms) {
      // This is the oldest one so far and it's OK to render.
      if (render_frame) {
        // This one is older than the newly found frame, remove this one.
        ReturnFrame(render_frame);
      }
      render_frame = oldest_frame_in_list;
      iter = incoming_frames_.erase(iter);
    } else {
      // We can't release this one yet, we're done here.
      break;
    }
  }

  if (render_frame)
  {
    int i = 0;
    i = 1; 
  }
  return render_frame;
}

int VideoRenderFrames::ReturnFrame(webrtc::VideoFrame* old_frame) {
  // No need to reuse texture frames because they do not allocate memory.

  empty_frames_.push_back(old_frame);

  return 0;
}

int VideoRenderFrames::ReleaseAllFrames() {
  for (FrameList::iterator iter = incoming_frames_.begin();
       iter != incoming_frames_.end(); ++iter) {
    delete *iter;
  }
  incoming_frames_.clear();

  for (FrameList::iterator iter = empty_frames_.begin();
       iter != empty_frames_.end(); ++iter) {
    delete *iter;
  }
  empty_frames_.clear();
  return 0;
}