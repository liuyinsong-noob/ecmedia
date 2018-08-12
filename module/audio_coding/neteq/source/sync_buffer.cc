/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#include <algorithm>  // Access to min.

#include "sync_buffer.h"

namespace yuntongxunwebrtc {
// Returns the number of samples yet to play out form the buffer.
size_t SyncBuffer::FutureLength() const {
  return Size() - next_index_;
}

// Adds the contents of |append_this| to the back of the SyncBuffer. Removes
// the same number of samples from the beginning of the SyncBuffer, to
// maintain a constant buffer size. The |next_index_| is updated to reflect
// the move of the beginning of "future" data.

void SyncBuffer::PushBack(const AudioMultiVector& append_this) {
  size_t samples_added = append_this.Size();
  AudioMultiVector::PushBack(append_this);
  AudioMultiVector::PopFront(samples_added);
  if (samples_added <= next_index_) {
      size_t tmp ;
	  tmp = next_index_;
    next_index_ -= samples_added;
  } else {
    // This means that we are pushing out future data that was never used.
//    assert(false);
    // TODO(hlundin): This assert must be disabled to support 60 ms frames.
    // This should not happen even for 60 ms frames, but it does. Investigate
    // why.
    next_index_ = 0;
  }
  dtmf_index_ -= std::min(dtmf_index_, samples_added);
}
    
// Adds |length| zeros to the beginning of each channel. Removes
// the same number of samples from the end of the SyncBuffer, to
// maintain a constant buffer size. The |next_index_| is updated to reflect
// the move of the beginning of "future" data.
// Note that this operation may delete future samples that are waiting to
// be played.
void SyncBuffer::PushFrontZeros(size_t length) {
  InsertZerosAtIndex(length, 0);
}
    
// Inserts |length| zeros into each channel at index |position|. The size of
// the SyncBuffer is kept constant, which means that the last |length|
// elements in each channel will be purged.
void SyncBuffer::InsertZerosAtIndex(size_t length, size_t position) {
  position = std::min(position, Size());
  length = std::min(length, Size() - position);
  AudioMultiVector::PopBack(length);
  for (size_t channel = 0; channel < Channels(); ++channel) {
    channels_[channel]->InsertZerosAt(length, position);
  }
  if (next_index_ >= position) {
    // We are moving the |next_index_| sample.
    set_next_index(next_index_ + length);  // Overflow handled by subfunction.
  }
  if (dtmf_index_ > 0 && dtmf_index_ >= position) {
    // We are moving the |dtmf_index_| sample.
    set_dtmf_index(dtmf_index_ + length);  // Overflow handled by subfunction.
  }
}

void SyncBuffer::ReplaceAtIndex(const AudioMultiVector& insert_this,
                                size_t length,
                                size_t position) {
  position = std::min(position, Size());  // Cap |position| in the valid range.
  length = std::min(length, Size() - position);
  AudioMultiVector::OverwriteAt(insert_this, length, position);
}

void SyncBuffer::ReplaceAtIndex(const AudioMultiVector& insert_this,
                                size_t position) {
  ReplaceAtIndex(insert_this, insert_this.Size(), position);
}

    // Reads |requested_len| samples from each channel and writes them interleaved
    // into |output|. The |next_index_| is updated to point to the sample to read
    // next time.
size_t SyncBuffer::GetNextAudioInterleaved(size_t requested_len,
                                           int16_t* output) {
  if (!output) {
    assert(false);
    return 0;
  }
  size_t samples_to_read = std::min(FutureLength(), requested_len);
  ReadInterleavedFromIndex(next_index_, samples_to_read, output);
  next_index_ += samples_to_read;
  return samples_to_read;
}

void SyncBuffer::IncreaseEndTimestamp(uint32_t increment) {
  end_timestamp_ += increment;
}

void SyncBuffer::Flush() {
  Zeros(Size());
  next_index_ = Size();
  end_timestamp_ = 0;
  dtmf_index_ = 0;
}

void SyncBuffer::set_next_index(size_t value) {
  // Cannot set |next_index_| larger than the size of the buffer.
  next_index_ = std::min(value, Size());
//    printf("sean set_next_index next_index_ %zu\n",next_index_);
}

void SyncBuffer::set_dtmf_index(size_t value) {
  // Cannot set |dtmf_index_| larger than the size of the buffer.
  dtmf_index_ = std::min(value, Size());
}

}  // namespace yuntongxunwebrtc
