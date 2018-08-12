/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENCODER_H_
#define WEBRTC_VIDEO_ENCODER_H_

#include <vector>

#include "common_types.h"
#include "typedefs.h"
#include "video_frame.h"

namespace yuntongxunwebrtc {

class RTPFragmentationHeader;
// TODO(pbos): Expose these through a public (root) header or change these APIs.
struct CodecSpecificInfo;
class VideoCodec;

class EncodedImageCallback {
 public:
  virtual ~EncodedImageCallback() {}

  // Callback function which is called when an image has been encoded.
  // TODO(pbos): Remove default arguments.
  virtual int32_t Encoded(
      const EncodedImage& encoded_image,
      const CodecSpecificInfo* codec_specific_info = NULL,
      const RTPFragmentationHeader* fragmentation = NULL) = 0;
};

class VideoEncoder {
 public:
  enum EncoderType {
    kVp8,
    kVp9,
  };

  static VideoEncoder* Create(EncoderType codec_type);

  static VideoCodecVP8 GetDefaultVp8Settings();
//  static VideoCodecVP9 GetDefaultVp9Settings();
  static VideoCodecH264 GetDefaultH264Settings();
  static VideoCodecH264High GetH264HighSettings();

  virtual ~VideoEncoder() {}

  // Initialize the encoder with the information from the codecSettings
  //
  // Input:
  //          - codec_settings    : Codec settings
  //          - number_of_cores   : Number of cores available for the encoder
  //          - max_payload_size  : The maximum size each payload is allowed
  //                                to have. Usually MTU - overhead.
  //
  // Return value                  : Set bit rate if OK
  //                                 <0 - Errors:
  //                                  WEBRTC_VIDEO_CODEC_ERR_PARAMETER
  //                                  WEBRTC_VIDEO_CODEC_ERR_SIZE
  //                                  WEBRTC_VIDEO_CODEC_LEVEL_EXCEEDED
  //                                  WEBRTC_VIDEO_CODEC_MEMORY
  //                                  WEBRTC_VIDEO_CODEC_ERROR
  virtual int32_t InitEncode(const VideoCodec* codec_settings,
                             int32_t number_of_cores,
                             size_t max_payload_size) = 0;

  // Register an encode complete callback object.
  //
  // Input:
  //          - callback         : Callback object which handles encoded images.
  //
  // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int32_t RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback) = 0;

  // Free encoder memory.
  // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int32_t Release() = 0;

  // Encode an I420 image (as a part of a video stream). The encoded image
  // will be returned to the user through the encode complete callback.
  //
  // Input:
  //          - frame             : Image to be encoded
  //          - frame_types       : Frame type to be generated by the encoder.
  //
  // Return value                 : WEBRTC_VIDEO_CODEC_OK if OK
  //                                <0 - Errors:
  //                                  WEBRTC_VIDEO_CODEC_ERR_PARAMETER
  //                                  WEBRTC_VIDEO_CODEC_MEMORY
  //                                  WEBRTC_VIDEO_CODEC_ERROR
  //                                  WEBRTC_VIDEO_CODEC_TIMEOUT
  virtual int32_t Encode(const I420VideoFrame& frame,
                         const CodecSpecificInfo* codec_specific_info,
                         const std::vector<VideoFrameType>* frame_types) = 0;

  // Inform the encoder of the new packet loss rate and the round-trip time of
  // the network.
  //
  // Input:
  //          - packet_loss : Fraction lost
  //                          (loss rate in percent = 100 * packetLoss / 255)
  //          - rtt         : Round-trip time in milliseconds
  // Return value           : WEBRTC_VIDEO_CODEC_OK if OK
  //                          <0 - Errors: WEBRTC_VIDEO_CODEC_ERROR
  virtual int32_t SetChannelParameters(uint32_t packet_loss, int64_t rtt) = 0;

  // Inform the encoder about the new target bit rate.
  //
  // Input:
  //          - bitrate         : New target bit rate
  //          - framerate       : The target frame rate
  //
  // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
  virtual int32_t SetRates(uint32_t bitrate, uint32_t framerate) = 0;

  virtual int32_t SetPeriodicKeyFrames(bool enable) { return -1; }
  virtual int32_t CodecConfigParameters(uint8_t* /*buffer*/, int32_t /*size*/) {
    return -1;
  }
};

}  // namespace webrtc
#endif  // WEBRTC_VIDEO_ENCODER_H_
