/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "media/base/video_source_base.h"

#include "absl/algorithm/container.h"
#include "rtc_base/checks.h"

namespace rtc {

VideoSourceBase::VideoSourceBase() = default;
VideoSourceBase::~VideoSourceBase() = default;

void VideoSourceBase::AddOrUpdateSink(
    VideoSinkInterface<webrtc::VideoFrame>* sink,
    const VideoSinkWants& wants) {
  RTC_DCHECK(sink != nullptr);

  SinkPair* sink_pair = FindSinkPair(sink);
  if (!sink_pair) {
	  //ytx_add by dxf begin
    if (wants.fixed_resolution)
      sinks_no_scale_.push_back(SinkPair(sink, wants));
    else
      sinks_.push_back(SinkPair(sink, wants));
	//ytx_add end
  } else {
    sink_pair->wants = wants;
  }
}

void VideoSourceBase::RemoveSink(VideoSinkInterface<webrtc::VideoFrame>* sink) {
  RTC_DCHECK(sink != nullptr);
  RTC_DCHECK(FindSinkPair(sink));
  sinks_.erase(std::remove_if(sinks_.begin(), sinks_.end(),
                              [sink](const SinkPair& sink_pair) {
                                return sink_pair.sink == sink;
                              }),
               sinks_.end());
  // ytx_add by dxf begin
  sinks_.erase(std::remove_if(sinks_no_scale_.begin(), sinks_no_scale_.end(),
                              [sink](const SinkPair& sink_pair) {
                                return sink_pair.sink == sink;
                              }),
               sinks_no_scale_.end());
  // ytx_add end
}

VideoSourceBase::SinkPair* VideoSourceBase::FindSinkPair(
    const VideoSinkInterface<webrtc::VideoFrame>* sink) {
  auto sink_pair_it = absl::c_find_if(
      sinks_,
      [sink](const SinkPair& sink_pair) { return sink_pair.sink == sink; });
  if (sink_pair_it != sinks_.end()) {
    return &*sink_pair_it;
  }
  // ytx_add by dxf begin
  sink_pair_it = absl::c_find_if(
      sinks_no_scale_,
      [sink](const SinkPair& sink_pair) { return sink_pair.sink == sink; });
  if (sink_pair_it != sinks_no_scale_.end()) {
    return &*sink_pair_it;
  }
  // ytx_add end
  return nullptr;
}

}  // namespace rtc
