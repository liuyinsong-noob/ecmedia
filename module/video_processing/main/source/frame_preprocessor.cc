/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "frame_preprocessor.h"

namespace cloopenwebrtc {

VPMFramePreprocessor::VPMFramePreprocessor()
    : id_(0),
      content_metrics_(NULL),
      resampled_frame_(),
      enable_ca_(false),
      frame_cnt_(0) {
  spatial_resampler_ = new VPMSimpleSpatialResampler();
  ca_ = new VPMContentAnalysis(true);
  vd_ = new VPMVideoDecimator();
  EnableDenoising(false);
  denoised_frame_toggle_ = 0;
}

VPMFramePreprocessor::~VPMFramePreprocessor() {
  Reset();
  delete spatial_resampler_;
  delete ca_;
  delete vd_;
}

int32_t VPMFramePreprocessor::ChangeUniqueId(const int32_t id) {
  id_ = id;
  return VPM_OK;
}

void  VPMFramePreprocessor::Reset() {
  ca_->Release();
  vd_->Reset();
  content_metrics_ = NULL;
  spatial_resampler_->Reset();
  enable_ca_ = false;
  frame_cnt_ = 0;
}


// void  VPMFramePreprocessor::set()
    
    
void  VPMFramePreprocessor::EnableTemporalDecimation(bool enable) {
  vd_->EnableTemporalDecimation(enable);
}

void VPMFramePreprocessor::EnableContentAnalysis(bool enable) {
  enable_ca_ = enable;
}

void  VPMFramePreprocessor::SetInputFrameResampleMode(
    VideoFrameResampling resampling_mode) {
  spatial_resampler_->SetInputFrameResampleMode(resampling_mode);
}

int32_t VPMFramePreprocessor::SetTargetResolution(
    uint32_t width, uint32_t height, uint32_t frame_rate) {
  if ( (width == 0) || (height == 0) || (frame_rate == 0)) {
    return VPM_PARAMETER_ERROR;
  }
  int32_t ret_val = 0;
  ret_val = spatial_resampler_->SetTargetFrameSize(width, height);

  if (ret_val < 0) return ret_val;

  ret_val = vd_->SetTargetFramerate(frame_rate);
  if (ret_val < 0) return ret_val;

  return VPM_OK;
}

void VPMFramePreprocessor::UpdateIncomingframe_rate() {
  vd_->UpdateIncomingframe_rate();
}

uint32_t VPMFramePreprocessor::Decimatedframe_rate() {
  return vd_->Decimatedframe_rate();
}


uint32_t VPMFramePreprocessor::DecimatedWidth() const {
  return spatial_resampler_->TargetWidth();
}


uint32_t VPMFramePreprocessor::DecimatedHeight() const {
  return spatial_resampler_->TargetHeight();
}

void VPMFramePreprocessor::setFrameScaleType(FrameScaleType type) {
    spatial_resampler_->setFrameScaleType(type);
}


int32_t VPMFramePreprocessor::PreprocessFrame(const I420VideoFrame& frame,
    I420VideoFrame** processed_frame) {
  if (frame.IsZeroSize()) {
    return VPM_PARAMETER_ERROR;
  }

  vd_->UpdateIncomingframe_rate();

  if (vd_->DropFrame()) {
    return 1;  // drop 1 frame
  }

  const  I420VideoFrame* current_frame = &frame;
  if (denoiser_) {
	  I420VideoFrame* denoised_frame = &denoised_frame_[0];
	  I420VideoFrame* denoised_frame_prev = &denoised_frame_[1];
	  // Swap the buffer to save one memcpy in DenoiseFrame.
	  if (denoised_frame_toggle_) {
		  denoised_frame = &denoised_frame_[1];
		  denoised_frame_prev = &denoised_frame_[0];
	  }
	  // Invert the flag.
	  denoised_frame_toggle_ ^= 1;
	  denoiser_->DenoiseFrame(*current_frame, denoised_frame, denoised_frame_prev,
		  true);
	  current_frame = denoised_frame;
  }

  // Resizing incoming frame if needed. Otherwise, remains NULL.
  // We are not allowed to resample the input frame (must make a copy of it).
  *processed_frame = NULL;
  if (spatial_resampler_->ApplyResample(current_frame->width(), current_frame->height()))  {
    int32_t ret = spatial_resampler_->ResampleFrame(*current_frame, &resampled_frame_);
    if (ret != VPM_OK) return ret;
    current_frame = &resampled_frame_;
  }

  // Perform content analysis on the frame to be encoded.
  if (enable_ca_) {
    // Compute new metrics every |kSkipFramesCA| frames, starting with
    // the first frame.
    if (frame_cnt_ % kSkipFrameCA == 0) {
      if (*processed_frame == NULL)  {
        content_metrics_ = ca_->ComputeContentMetrics(*current_frame);
      } else {
        content_metrics_ = ca_->ComputeContentMetrics(resampled_frame_);
      }
    }
    ++frame_cnt_;
  }

  *processed_frame = const_cast<I420VideoFrame*>(current_frame);
  return VPM_OK;
}

VideoContentMetrics* VPMFramePreprocessor::ContentMetrics() const {
  return content_metrics_;
}

void VPMFramePreprocessor::EnableDenoising(bool enable) {
	if (enable) {
		denoiser_.reset(new VideoDenoiser(true));
	}
	else {
		denoiser_.reset();
	}
}
}  // namespace
