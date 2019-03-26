/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H264_VIDEO_TOOLBOX_ENCODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H264_VIDEO_TOOLBOX_ENCODER_H_

#include "bitrate_adjuster.h"
//#include "webrtc/common_video/rotation.h"
//#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"

#if defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)

#include <VideoToolbox/VideoToolbox.h>
#include <vector>

#include "video_codec_interface.h"
#include "module_common_types.h"

// This file provides a H264 encoder implementation using the VideoToolbox
// APIs. Since documentation is almost non-existent, this is largely based on
// the information in the VideoToolbox header files, a talk from WWDC 2014 and
// experimentation.

namespace yuntongxunwebrtc {

class H264VideoToolboxEncoder : public VideoEncoder {
 public:
    static H264VideoToolboxEncoder* Create();
    
  H264VideoToolboxEncoder();

  ~H264VideoToolboxEncoder() final;

  virtual WebRtc_Word32 InitEncode(const VideoCodec* codecSettings, int32_t numberOfCores, size_t maxPayloadSize) override;
    
    virtual WebRtc_Word32 Encode(const I420VideoFrame& inputImage,
                                 const CodecSpecificInfo* codecSpecificInfo,
                                 const std::vector<VideoFrameType>* frame_types) override;

  virtual int RegisterEncodeCompleteCallback(EncodedImageCallback* callback) override;

  virtual int SetChannelParameters(uint32_t packet_loss, int64_t rtt) override;

  virtual int SetRates(uint32_t new_bitrate_kbit, uint32_t frame_rate,
                       uint32_t minBitrate_kbit = 0, uint32_t maxBitrate_kbit = 0) override;

  virtual WebRtc_Word32 Release() override;
  virtual WebRtc_Word32 SetPeriodicKeyFrames(bool enable) override;
    
  const char* ImplementationName() const;

  void OnEncodedFrame(OSStatus status,
                      VTEncodeInfoFlags info_flags,
                      CMSampleBufferRef sample_buffer,
                      CodecSpecificInfo codec_specific_info,
                      int32_t width,
                      int32_t height,
                      int64_t render_time_ms,
                      uint32_t timestamp/*,
                      VideoRotation rotation*/);

 private:
  int ResetCompressionSession();
  void ConfigureCompressionSession();
  void DestroyCompressionSession();
  void SetBitrateBps(uint32_t bitrate_bps);
  void SetEncoderBitrateBps(uint32_t bitrate_bps);

  EncodedImageCallback* callback_;
  VTCompressionSessionRef compression_session_;
  BitrateAdjuster bitrate_adjuster_;
  uint32_t target_bitrate_bps_;
  uint32_t encoder_bitrate_bps_;
  int32_t width_;
  int32_t height_;
  uint8_t codec_type_;
    uint8_t count;
};  // H264VideoToolboxEncoder

}  // namespace webrtc

#endif  // defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
#endif  // WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H264_VIDEO_TOOLBOX_ENCODER_H_
