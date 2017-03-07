/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H
#define WEBRTC_MODULE_VIDEO_PROCESSING_IMPL_H

#include "video_processing.h"
#include "brighten.h"
#include "brightness_detection.h"
#include "color_enhancement.h"
#include "deflickering.h"
#include "frame_preprocessor.h"

namespace cloopenwebrtc {
class CriticalSectionWrapper;

class VideoProcessingModuleImpl : public VideoProcessingModule {
 public:
  VideoProcessingModuleImpl(int32_t id);

  virtual ~VideoProcessingModuleImpl();

  int32_t Id() const;

  virtual int32_t ChangeUniqueId(const int32_t id) OVERRIDE;

  virtual void Reset() OVERRIDE;

  virtual int32_t Deflickering(I420VideoFrame* frame,
                               FrameStats* stats) OVERRIDE;

  virtual int32_t BrightnessDetection(const I420VideoFrame& frame,
                                      const FrameStats& stats) OVERRIDE;

  // Frame pre-processor functions

  // Enable temporal decimation
  virtual void EnableTemporalDecimation(bool enable) OVERRIDE;

  virtual void SetInputFrameResampleMode(
      VideoFrameResampling resampling_mode) OVERRIDE;

  // Enable content analysis
  virtual void EnableContentAnalysis(bool enable) OVERRIDE;

  // Set Target Resolution: frame rate and dimension
  virtual int32_t SetTargetResolution(uint32_t width,
                                      uint32_t height,
                                      uint32_t frame_rate) OVERRIDE;


  // Get decimated values: frame rate/dimension
  virtual uint32_t Decimatedframe_rate() OVERRIDE;
  virtual uint32_t DecimatedWidth() const OVERRIDE;
  virtual uint32_t DecimatedHeight() const OVERRIDE;

  // Preprocess:
  // Pre-process incoming frame: Sample when needed and compute content
  // metrics when enabled.
  // If no resampling takes place - processed_frame is set to NULL.
  virtual int32_t PreprocessFrame(const I420VideoFrame& frame,
                                  I420VideoFrame** processed_frame) OVERRIDE;
  virtual VideoContentMetrics* ContentMetrics() const OVERRIDE;

  void EnableDenoising(bool enable) override;

  virtual void setFrameScaleType(FrameScaleType type);
 private:
  int32_t  id_;
  CriticalSectionWrapper& mutex_;
  VPMDeflickering deflickering_;
  VPMBrightnessDetection brightness_detection_;
  VPMFramePreprocessor  frame_pre_processor_;
};

}  // namespace

#endif
