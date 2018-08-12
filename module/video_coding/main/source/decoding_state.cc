/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "decoding_state.h"

#include "module_common_types.h"
#include "frame_buffer.h"
#include "jitter_buffer_common.h"
#include "packet.h"

namespace yuntongxunwebrtc {

VCMDecodingState::VCMDecodingState()
    : sequence_num_(0),
      time_stamp_(0),
      picture_id_(kNoPictureId),
      temporal_id_(kNoTemporalIdx),
      tl0_pic_id_(kNoTl0PicIdx),
      full_sync_(true),
      in_initial_state_(true) {}

VCMDecodingState::~VCMDecodingState() {}

void VCMDecodingState::Reset() {
  // TODO(mikhal): Verify - not always would want to reset the sync
  sequence_num_ = 0;
  time_stamp_ = 0;
  picture_id_ = kNoPictureId;
  temporal_id_ = kNoTemporalIdx;
  tl0_pic_id_ = kNoTl0PicIdx;
  full_sync_ = true;
  in_initial_state_ = true;
}

uint32_t VCMDecodingState::time_stamp() const {
  return time_stamp_;
}

uint16_t VCMDecodingState::sequence_num() const {
  return sequence_num_;
}

bool VCMDecodingState::IsOldFrame(const VCMFrameBuffer* frame) const {
  assert(frame != NULL);
  if (in_initial_state_)
    return false;
  return !IsNewerTimestamp(frame->TimeStamp(), time_stamp_);
}

bool VCMDecodingState::IsOldPacket(const VCMPacket* packet) const {
  assert(packet != NULL);
  if (in_initial_state_)
    return false;
  return !IsNewerTimestamp(packet->timestamp, time_stamp_);
}

void VCMDecodingState::SetState(const VCMFrameBuffer* frame) {
  assert(frame != NULL && frame->GetHighSeqNum() >= 0);
  UpdateSyncState(frame);
  sequence_num_ = static_cast<uint16_t>(frame->GetHighSeqNum());
  time_stamp_ = frame->TimeStamp();
  picture_id_ = frame->PictureId();
  temporal_id_ = frame->TemporalId();
  tl0_pic_id_ = frame->Tl0PicId();
  in_initial_state_ = false;
}

void VCMDecodingState::CopyFrom(const VCMDecodingState& state) {
  sequence_num_ = state.sequence_num_;
  time_stamp_ = state.time_stamp_;
  picture_id_ = state.picture_id_;
  temporal_id_ = state.temporal_id_;
  tl0_pic_id_ = state.tl0_pic_id_;
  full_sync_ = state.full_sync_;
  in_initial_state_ = state.in_initial_state_;
}

bool VCMDecodingState::UpdateEmptyFrame(const VCMFrameBuffer* frame) {
  bool empty_packet = frame->GetHighSeqNum() == frame->GetLowSeqNum();
  if (in_initial_state_ && empty_packet) {
    // Drop empty packets as long as we are in the initial state.
    return true;
  }
  if ((empty_packet && ContinuousSeqNum(frame->GetHighSeqNum())) ||
      ContinuousFrame(frame)) {
    // Continuous empty packets or continuous frames can be dropped if we
    // advance the sequence number.
    sequence_num_ = frame->GetHighSeqNum();
    time_stamp_ = frame->TimeStamp();
    return true;
  }
  return false;
}

void VCMDecodingState::UpdateOldPacket(const VCMPacket* packet) {
  assert(packet != NULL);
  if (packet->timestamp == time_stamp_) {
    // Late packet belonging to the last decoded frame - make sure we update the
    // last decoded sequence number.
    sequence_num_ = LatestSequenceNumber(packet->seqNum, sequence_num_);
  }
}

void VCMDecodingState::SetSeqNum(uint16_t new_seq_num) {
  sequence_num_ = new_seq_num;
}

bool VCMDecodingState::in_initial_state() const {
  return in_initial_state_;
}

bool VCMDecodingState::full_sync() const {
  return full_sync_;
}

void VCMDecodingState::UpdateSyncState(const VCMFrameBuffer* frame) {
  if (in_initial_state_)
    return;
  if (frame->TemporalId() == kNoTemporalIdx ||
      frame->Tl0PicId() == kNoTl0PicIdx) {
    full_sync_ = true;
  } else if (frame->FrameType() == kVideoFrameKey || frame->LayerSync()) {
    full_sync_ = true;
  } else if (full_sync_) {
    // Verify that we are still in sync.
    // Sync will be broken if continuity is true for layers but not for the
    // other methods (PictureId and SeqNum).
    if (UsingPictureId(frame)) {
      // First check for a valid tl0PicId.
      if (frame->Tl0PicId() - tl0_pic_id_ > 1) {
        full_sync_ = false;
      } else {
        full_sync_ = ContinuousPictureId(frame->PictureId());
      }
    } else {
      full_sync_ = ContinuousSeqNum(static_cast<uint16_t>(
          frame->GetLowSeqNum()));
    }
  }
}

bool VCMDecodingState::ContinuousFrame(const VCMFrameBuffer* frame) const {
  // Check continuity based on the following hierarchy:
  // - Temporal layers (stop here if out of sync).
  // - Picture Id when available.
  // - Sequence numbers.
  // Return true when in initial state.
  // Note that when a method is not applicable it will return false.
  assert(frame != NULL);
  // A key frame is always considered continuous as it doesn't refer to any
  // frames and therefore won't introduce any errors even if prior frames are
  // missing.
  if (frame->FrameType() == kVideoFrameKey)
    return true;
  // When in the initial state we always require a key frame to start decoding.
  if (in_initial_state_)
    return false;
  if (ContinuousLayer(frame->TemporalId(), frame->Tl0PicId()))
    return true;
  // tl0picId is either not used, or should remain unchanged.
  if (frame->Tl0PicId() != tl0_pic_id_)
    return false;
  // Base layers are not continuous or temporal layers are inactive.
  // In the presence of temporal layers, check for Picture ID/sequence number
  // continuity if sync can be restored by this frame.
  if (!full_sync_ && !frame->LayerSync())
    return false;
  if (UsingPictureId(frame)) {
    return ContinuousPictureId(frame->PictureId());
  } else {
    return ContinuousSeqNum(static_cast<uint16_t>(frame->GetLowSeqNum()));
  }
}

bool VCMDecodingState::ContinuousPictureId(int picture_id) const {
  int next_picture_id = picture_id_ + 1;
  if (picture_id < picture_id_) {
    // Wrap
    if (picture_id_ >= 0x80) {
      // 15 bits used for picture id
      return ((next_picture_id & 0x7FFF) == picture_id);
    } else {
      // 7 bits used for picture id
      return ((next_picture_id & 0x7F) == picture_id);
    }
  }
  // No wrap
  return (next_picture_id == picture_id);
}

bool VCMDecodingState::ContinuousSeqNum(uint16_t seq_num) const {
  return seq_num == static_cast<uint16_t>(sequence_num_ + 1);
}

bool VCMDecodingState::ContinuousLayer(int temporal_id,
                                       int tl0_pic_id) const {
  // First, check if applicable.
  if (temporal_id == kNoTemporalIdx || tl0_pic_id == kNoTl0PicIdx)
    return false;
  // If this is the first frame to use temporal layers, make sure we start
  // from base.
  else if (tl0_pic_id_ == kNoTl0PicIdx && temporal_id_ == kNoTemporalIdx &&
           temporal_id == 0)
    return true;

  // Current implementation: Look for base layer continuity.
  if (temporal_id != 0)
    return false;
  return (static_cast<uint8_t>(tl0_pic_id_ + 1) == tl0_pic_id);
}

bool VCMDecodingState::UsingPictureId(const VCMFrameBuffer* frame) const {
  return (frame->PictureId() != kNoPictureId && picture_id_ != kNoPictureId);
}

}  // namespace webrtc
