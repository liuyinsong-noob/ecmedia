/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_encoder_isac.h"

#include "audio_encoder_isac_t_impl.h"

namespace yuntongxunwebrtc{

// Explicit instantiation of AudioEncoderDecoderIsacT<IsacFloat>, a.k.a.
// AudioEncoderDecoderIsac.
template class AudioEncoderDecoderIsacT<IsacFloat>;

// Explicit instantiation of AudioEncoderDecoderIsacT<IsacRed>, a.k.a.
// AudioEncoderDecoderIsacRed.
template class AudioEncoderDecoderIsacT<IsacRed>;

}  // namespace webrtc
