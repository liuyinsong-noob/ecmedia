/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_VIDEO_INCLUDE_INCOMING_VIDEO_STREAM_H_
#define WEBRTC_COMMON_VIDEO_INCLUDE_INCOMING_VIDEO_STREAM_H_

#include "../base/race_checker.h"
#include "../base/task_queue.h"
#include "../common_video/video_render_frames.h"
#include "../media/base/videosinkinterface.h"

namespace cloopenwebrtc {

class IncomingVideoStream : public cloopenwebrtc::VideoSinkInterface<VideoFrame> {
 public:
  IncomingVideoStream(int32_t delay_ms,
                      rtc::VideoSinkInterface<VideoFrame>* callback);
  ~IncomingVideoStream() override;

 private:
  void OnFrame(const VideoFrame& video_frame) override;
  void Dequeue();

  // Fwd decl of a QueuedTask implementation for carrying frames over to the TQ.
  class NewFrameTask;

  cloopenwebrtc::ThreadChecker main_thread_checker_;
  cloopenwebrtc::RaceChecker decoder_race_checker_;

  VideoRenderFrames render_buffers_;  // Only touched on the TaskQueue.
  cloopenwebrtc::VideoSinkInterface<VideoFrame>* const callback_;
  cloopenwebrtc::TaskQueue incoming_render_queue_;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_COMMON_VIDEO_INCLUDE_INCOMING_VIDEO_STREAM_H_
