/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_VIDEO_INCLUDE_VIDEO_FRAME_BUFFER_H_
#define WEBRTC_COMMON_VIDEO_INCLUDE_VIDEO_FRAME_BUFFER_H_

#include <memory>

#include "../module/video_coding/codecs/h264_hardcode/video_frame_buffer.h"
// TODO(nisse): For backwards compatibility, files including this file
// expect it to declare I420Buffer. Delete after callers are updated.
//#include "./api/video/i420_buffer.h"
#include "../base/callback.h"
#include "../base/scoped_ref_ptr.h"

namespace cloopenwebrtc {

// Base class for native-handle buffer is a wrapper around a |native_handle|.
// This is used for convenience as most native-handle implementations can share
// many VideoFrame implementations, but need to implement a few others (such
// as their own destructors or conversion methods back to software I420).
class NativeHandleBuffer : public VideoFrameBuffer {
 public:
  NativeHandleBuffer(void* native_handle, int width, int height);

  int width() const override;
  int height() const override;
  const uint8_t* DataY() const override;
  const uint8_t* DataU() const override;
  const uint8_t* DataV() const override;
  int StrideY() const override;
  int StrideU() const override;
  int StrideV() const override;

  void* native_handle() const override;

 protected:
  void* native_handle_;
  const int width_;
  const int height_;
};

class WrappedI420Buffer : public cloopenwebrtc::VideoFrameBuffer {
 public:
  WrappedI420Buffer(int width,
                    int height,
                    const uint8_t* y_plane,
                    int y_stride,
                    const uint8_t* u_plane,
                    int u_stride,
                    const uint8_t* v_plane,
                    int v_stride,
                    const cloopenwebrtc::Callback0<void>& no_longer_used);
  int width() const override;
  int height() const override;

  const uint8_t* DataY() const override;
  const uint8_t* DataU() const override;
  const uint8_t* DataV() const override;
  int StrideY() const override;
  int StrideU() const override;
  int StrideV() const override;

  void* native_handle() const override;

  cloopenwebrtc::scoped_refptr<VideoFrameBuffer> NativeToI420Buffer() override;

 private:
  friend class cloopenwebrtc::RefCountedObject<WrappedI420Buffer>;
  ~WrappedI420Buffer() override;

  const int width_;
  const int height_;
  const uint8_t* const y_plane_;
  const uint8_t* const u_plane_;
  const uint8_t* const v_plane_;
  const int y_stride_;
  const int u_stride_;
  const int v_stride_;
  cloopenwebrtc::Callback0<void> no_longer_used_cb_;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_COMMON_VIDEO_INCLUDE_VIDEO_FRAME_BUFFER_H_
