/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 *  WEBRTC VP8 wrapper interface
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_INCLUDE_VP8_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_INCLUDE_VP8_H_

#include "video_codec_interface.h"

namespace yuntongxunwebrtc {

class VP8Encoder : public VideoEncoder {
 public:
  static VP8Encoder* Create();
  VP8Encoder() {};
  virtual ~VP8Encoder() {};

  virtual int32_t InitEncode(const VideoCodec* codec_settings,
                             int32_t number_of_cores,
                             size_t max_payload_size) = 0;
  virtual int32_t RegisterEncodeCompleteCallback(EncodedImageCallback* callback) = 0;

  virtual int32_t Release() = 0;

  virtual int32_t Encode(const I420VideoFrame& frame,
                         const CodecSpecificInfo* codec_specific_info,
                         const std::vector<VideoFrameType>* frame_types) = 0;

  virtual int32_t SetChannelParameters(uint32_t packet_loss, int64_t rtt) = 0;

  virtual int32_t SetRates(uint32_t bitrate, uint32_t framerate,
                           uint32_t minBitrate_kbit = 0, uint32_t maxBitrate_kbit = 0) = 0;

};  // end of VP8Encoder class


class VP8Decoder : public VideoDecoder {
 public:
  static VP8Decoder* Create();
  virtual ~VP8Decoder() {};

  virtual int32_t InitDecode(const VideoCodec* codecSettings,
						   int32_t numberOfCores) = 0;

  virtual int32_t Decode(const EncodedImage& inputImage,
					   bool missingFrames,
					   const RTPFragmentationHeader* fragmentation,
					   const CodecSpecificInfo* codecSpecificInfo = NULL,
					   int64_t renderTimeMs = -1) = 0;

  virtual int32_t RegisterDecodeCompleteCallback(
	DecodedImageCallback* callback) = 0;

  virtual int32_t Release() = 0;
  virtual int32_t Reset() = 0;

};  // end of VP8Decoder class
}  // namespace webrtc

#endif // WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_INCLUDE_VP8_H_
