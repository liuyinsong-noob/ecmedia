/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "neteq.h"

#include "accelerate.h"
#include "buffer_level_filter.h"
#include "decoder_database.h"
#include "delay_manager.h"
#include "delay_peak_detector.h"
#include "dtmf_buffer.h"
#include "dtmf_tone_generator.h"
#include "expand.h"
#include "neteq_impl.h"
#include "packet_buffer.h"
#include "payload_splitter.h"
#include "preemptive_expand.h"
#include "timestamp_scaler.h"

namespace cloopenwebrtc {

// Creates all classes needed and inject them into a new NetEqImpl object.
// Return the new object.
NetEq* NetEq::Create(const NetEq::Config& config) {
  BufferLevelFilter* buffer_level_filter = new BufferLevelFilter;
  DecoderDatabase* decoder_database = new DecoderDatabase;
  DelayPeakDetector* delay_peak_detector = new DelayPeakDetector;
  DelayManager* delay_manager =
      new DelayManager(config.max_packets_in_buffer, delay_peak_detector);
  delay_manager->SetMaximumDelay(config.max_delay_ms);
  DtmfBuffer* dtmf_buffer = new DtmfBuffer(config.sample_rate_hz);
  DtmfToneGenerator* dtmf_tone_generator = new DtmfToneGenerator;
  PacketBuffer* packet_buffer = new PacketBuffer(config.max_packets_in_buffer);
  PayloadSplitter* payload_splitter = new PayloadSplitter;
  TimestampScaler* timestamp_scaler = new TimestampScaler(*decoder_database);
  AccelerateFactory* accelerate_factory = new AccelerateFactory;
  ExpandFactory* expand_factory = new ExpandFactory;
  PreemptiveExpandFactory* preemptive_expand_factory =
      new PreemptiveExpandFactory;
  return new NetEqImpl(config,
                       buffer_level_filter,
                       decoder_database,
                       delay_manager,
                       delay_peak_detector,
                       dtmf_buffer,
                       dtmf_tone_generator,
                       packet_buffer,
                       payload_splitter,
                       timestamp_scaler,
                       accelerate_factory,
                       expand_factory,
                       preemptive_expand_factory);
}

}  // namespace cloopenwebrtc
