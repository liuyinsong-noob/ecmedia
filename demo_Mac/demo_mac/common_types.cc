/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../module/common_types.h"

#include <limits>
#include <string.h>

#include "../base/checks.h"
#include "../base/stringutils.h"

namespace yuntongxunwebrtc {

const uint32_t BitrateAllocation::kMaxBitrateBps =
    std::numeric_limits<uint32_t>::max();

BitrateAllocation::BitrateAllocation() : sum_(0), bitrates_{} {}

bool BitrateAllocation::SetBitrate(size_t spatial_index,
                                   size_t temporal_index,
                                   uint32_t bitrate_bps) {
  CHECK_LT(spatial_index, kMaxSpatialLayers);
  CHECK_LT(temporal_index, kMaxTemporalStreams);
  CHECK_LE(bitrates_[spatial_index][temporal_index], sum_);
  uint64_t new_bitrate_sum_bps = sum_;
  new_bitrate_sum_bps -= bitrates_[spatial_index][temporal_index];
  new_bitrate_sum_bps += bitrate_bps;
  if (new_bitrate_sum_bps > kMaxBitrateBps)
    return false;

  bitrates_[spatial_index][temporal_index] = bitrate_bps;
  sum_ = static_cast<uint32_t>(new_bitrate_sum_bps);
  return true;
}

uint32_t BitrateAllocation::GetBitrate(size_t spatial_index,
                                       size_t temporal_index) const {
  CHECK_LT(spatial_index, kMaxSpatialLayers);
  CHECK_LT(temporal_index, kMaxTemporalStreams);
  return bitrates_[spatial_index][temporal_index];
}

// Get the sum of all the temporal layer for a specific spatial layer.
uint32_t BitrateAllocation::GetSpatialLayerSum(size_t spatial_index) const {
  CHECK_LT(spatial_index, kMaxSpatialLayers);
  uint32_t sum = 0;
  for (int i = 0; i < kMaxTemporalStreams; ++i)
    sum += bitrates_[spatial_index][i];
  return sum;
}

}  // namespace yuntongxunwebrtc
