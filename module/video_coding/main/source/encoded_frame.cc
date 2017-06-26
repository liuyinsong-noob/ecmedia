/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video_coding_defines.h"
#include "encoded_frame.h"
#include "generic_encoder.h"
#include "jitter_buffer_common.h"

namespace cloopenwebrtc {

VCMEncodedFrame::VCMEncodedFrame()
:
cloopenwebrtc::EncodedImage(),
_renderTimeMs(-1),
_payloadType(0),
_missingFrame(false),
_codec(kVideoCodecUnknown),
_fragmentation()
{
    _codecSpecificInfo.codecType = kVideoCodecUnknown;
}

VCMEncodedFrame::VCMEncodedFrame(const cloopenwebrtc::EncodedImage& rhs)
:
cloopenwebrtc::EncodedImage(rhs),
_renderTimeMs(-1),
_payloadType(0),
_missingFrame(false),
_codec(kVideoCodecUnknown),
_fragmentation()
{
    _codecSpecificInfo.codecType = kVideoCodecUnknown;
    _buffer = NULL;
    _size = 0;
    _length = 0;
    if (rhs._buffer != NULL)
    {
        VerifyAndAllocate(rhs._length);
        memcpy(_buffer, rhs._buffer, rhs._length);
    }
}

VCMEncodedFrame::VCMEncodedFrame(const VCMEncodedFrame& rhs)
  :
    cloopenwebrtc::EncodedImage(rhs),
    _renderTimeMs(rhs._renderTimeMs),
    _payloadType(rhs._payloadType),
    _missingFrame(rhs._missingFrame),
    _codecSpecificInfo(rhs._codecSpecificInfo),
    _codec(rhs._codec),
    _fragmentation() {
  _buffer = NULL;
  _size = 0;
  _length = 0;
  if (rhs._buffer != NULL)
  {
      VerifyAndAllocate(rhs._length);
      memcpy(_buffer, rhs._buffer, rhs._length);
      _length = rhs._length;
  }
  _fragmentation.CopyFrom(rhs._fragmentation);
}

VCMEncodedFrame::~VCMEncodedFrame()
{
    Free();
}

void VCMEncodedFrame::Free()
{
    Reset();
    if (_buffer != NULL)
    {
        delete [] _buffer;
        _buffer = NULL;
    }
}

void VCMEncodedFrame::Reset()
{
    _renderTimeMs = -1;
    _timeStamp = 0;
    _payloadType = 0;
    _frameType = kDeltaFrame;
    _encodedWidth = 0;
    _encodedHeight = 0;
    _completeFrame = false;
    _missingFrame = false;
    _length = 0;
    _codecSpecificInfo.codecType = kVideoCodecUnknown;
    _codec = kVideoCodecUnknown;
}

void VCMEncodedFrame::CopyCodecSpecific(const RTPVideoHeader* header)
{
  if (header) {
    switch (header->codec) {
      case kRtpVideoVp8: {
        if (_codecSpecificInfo.codecType != kVideoCodecVP8) {
          // This is the first packet for this frame.
          _codecSpecificInfo.codecSpecific.VP8.pictureId = -1;
          _codecSpecificInfo.codecSpecific.VP8.temporalIdx = 0;
          _codecSpecificInfo.codecSpecific.VP8.layerSync = false;
          _codecSpecificInfo.codecSpecific.VP8.keyIdx = -1;
          _codecSpecificInfo.codecType = kVideoCodecVP8;
        }
        _codecSpecificInfo.codecSpecific.VP8.nonReference =
            header->codecHeader.VP8.nonReference;
        if (header->codecHeader.VP8.pictureId != kNoPictureId) {
          _codecSpecificInfo.codecSpecific.VP8.pictureId =
              header->codecHeader.VP8.pictureId;
        }
        if (header->codecHeader.VP8.temporalIdx != kNoTemporalIdx) {
          _codecSpecificInfo.codecSpecific.VP8.temporalIdx =
              header->codecHeader.VP8.temporalIdx;
          _codecSpecificInfo.codecSpecific.VP8.layerSync =
              header->codecHeader.VP8.layerSync;
        }
        if (header->codecHeader.VP8.keyIdx != kNoKeyIdx) {
          _codecSpecificInfo.codecSpecific.VP8.keyIdx =
              header->codecHeader.VP8.keyIdx;
        }
        break;
      }
      case kRtpVideoH264: {
        _codecSpecificInfo.codecType = kVideoCodecH264;
        break;
      }
      default: {
        _codecSpecificInfo.codecType = kVideoCodecUnknown;
        break;
      }
    }
  }
}

const RTPFragmentationHeader* VCMEncodedFrame::FragmentationHeader() const {
  return &_fragmentation;
}

void VCMEncodedFrame::VerifyAndAllocate(const uint32_t minimumSize)
{
    if(minimumSize > _size)
    {
        // create buffer of sufficient size
        uint8_t* newBuffer = new uint8_t[minimumSize];
        if(_buffer)
        {
            // copy old data
            memcpy(newBuffer, _buffer, _size);
            delete [] _buffer;
        }
        _buffer = newBuffer;
        _size = minimumSize;
    }
}

cloopenwebrtc::FrameType VCMEncodedFrame::ConvertFrameType(VideoFrameType frameType)
{
  switch(frameType) {
    case kKeyFrame:
      return  kVideoFrameKey;
    case kDeltaFrame:
      return kVideoFrameDelta;
    case kSkipFrame:
      return kEmptyFrame;
    default:
      return kVideoFrameDelta;
  }
}

VideoFrameType VCMEncodedFrame::ConvertFrameType(cloopenwebrtc::FrameType frame_type) {
  switch (frame_type) {
    case kVideoFrameKey:
      return kKeyFrame;
    case kVideoFrameDelta:
      return kDeltaFrame;
    default:
      assert(false);
      return kDeltaFrame;
  }
}

void VCMEncodedFrame::ConvertFrameTypes(
    const std::vector<cloopenwebrtc::FrameType>& frame_types,
    std::vector<VideoFrameType>* video_frame_types) {
  assert(video_frame_types);
  video_frame_types->reserve(frame_types.size());
  for (size_t i = 0; i < frame_types.size(); ++i) {
    (*video_frame_types)[i] = ConvertFrameType(frame_types[i]);
  }
}

WebRtc_Word32
	VCMEncodedFrame::Store(VCMFrameStorageCallback& storeCallback) const
{
	EncodedVideoData frameToStore;
	frameToStore.codec = _codec;
	if (_buffer != NULL)
	{
		frameToStore.VerifyAndAllocate(_length);
		memcpy(frameToStore.payloadData, _buffer, _length);
		frameToStore.payloadSize = _length;
	}
	frameToStore.completeFrame = _completeFrame;
	frameToStore.encodedWidth = _encodedWidth;
	frameToStore.encodedHeight = _encodedHeight;
	frameToStore.frameType = ConvertFrameType(_frameType);
	frameToStore.missingFrame = _missingFrame;
	frameToStore.payloadType = _payloadType;
	frameToStore.renderTimeMs = _renderTimeMs;
	frameToStore.timeStamp = _timeStamp;
	storeCallback.StoreReceivedFrame(frameToStore);
	return VCM_OK;
}

}
