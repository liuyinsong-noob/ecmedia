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

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H264_VIDEO_TOOLBOX_DECODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H264_VIDEO_TOOLBOX_DECODER_H_

#if defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
#include "video_codec_interface.h"
#include "typedefs.h"
#include <VideoToolbox/VideoToolbox.h>
#include <list>

// This file provides a H264 encoder implementation using the VideoToolbox
// APIs. Since documentation is almost non-existent, this is largely based on
// the information in the VideoToolbox header files, a talk from WWDC 2014 and
// experimentation.

namespace cloopenwebrtc {
const int MaxPicOrderCntLsb = 64;
class H264VideoToolboxDecoder : public VideoDecoder {
 public:
    
    static H264VideoToolboxDecoder *Create();
  H264VideoToolboxDecoder();

  virtual ~H264VideoToolboxDecoder() override;

  virtual WebRtc_Word32 InitDecode(const VideoCodec* codecSettings, WebRtc_Word32 /*numberOfCores*/) override;
  virtual WebRtc_Word32 SetCodecConfigParameters (const WebRtc_UWord8* /*buffer*/, WebRtc_Word32 /*size*/){return WEBRTC_VIDEO_CODEC_OK;};
    
  virtual WebRtc_Word32 Decode(const EncodedImage& inputImage,
                                 bool missingFrames,
                                 const RTPFragmentationHeader* fragmentation,
                                 const CodecSpecificInfo* /*codecSpecificInfo*/,
                                 WebRtc_Word64 /*renderTimeMs*/) override;

  virtual WebRtc_Word32 RegisterDecodeCompleteCallback(DecodedImageCallback* callback) override;

  virtual WebRtc_Word32 Release() override;
    virtual WebRtc_Word32 Reset() override;
  const char* ImplementationName() const;

 private:
  int ResetDecompressionSession();
  void ConfigureDecompressionSession();
  void DestroyDecompressionSession();
  void SetVideoFormat(CMVideoFormatDescriptionRef video_format);

  DecodedImageCallback* callback_;
  CMVideoFormatDescriptionRef video_format_;
  VTDecompressionSessionRef decompression_session_;
    int							_numberOfCores;
    
    uint64_t                         prevPicOrderCntLsb = 0;
    uint64_t                         prevPicOrderCntMsb = 64;
#define DEBUG_H264 0
#if DEBUG_H264
    FILE*                       debug_h264_;
#endif
public:
    std::list<I420VideoFrame *> decodedList;
    int                         threshhold;
};  // H264VideoToolboxDecoder

}  // namespace webrtc

#endif  // defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
#endif  // WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H264_VIDEO_TOOLBOX_DECODER_H_
