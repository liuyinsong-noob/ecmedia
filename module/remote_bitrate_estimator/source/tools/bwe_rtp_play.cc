/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdio.h>

//#include "webrtc/base/format_macros.h"
#include "remote_bitrate_estimator.h"
#include "bwe_rtp.h"
#include "rtp_header_parser.h"
#include "rtp_payload_registry.h"
#include "scoped_ptr.h"
#include "rtp_file_reader.h"

#pragma comment(lib,"winmm.lib")

class Observer : public cloopenwebrtc::RemoteBitrateObserver {
 public:
  explicit Observer(cloopenwebrtc::Clock* clock) : clock_(clock) {}

  // Called when a receive channel group has a new bitrate estimate for the
  // incoming streams.
  virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs,
                                       unsigned int bitrate) {
    printf("[%u] Num SSRCs: %d, bitrate: %u\n",
           static_cast<uint32_t>(clock_->TimeInMilliseconds()),
           static_cast<int>(ssrcs.size()), bitrate);
  }

  virtual ~Observer() {}

 private:
  cloopenwebrtc::Clock* clock_;
};

int main(int argc, char** argv) {
  if (argc < 4) {
    printf("Usage: bwe_rtp_play <extension type> <extension id> "
           "<input_file.rtp>\n");
    printf("<extension type> can either be:\n"
           "  abs for absolute send time or\n"
           "  tsoffset for timestamp offset.\n"
           "<extension id> is the id associated with the extension.\n");
    return -1;
  }
  cloopenwebrtc::test::RtpFileReader* reader;
  cloopenwebrtc::RemoteBitrateEstimator* estimator;
  cloopenwebrtc::RtpHeaderParser* parser;
  std::string estimator_used;
  cloopenwebrtc::SimulatedClock clock(0);
  Observer observer(&clock);
  if (!ParseArgsAndSetupEstimator(argc, argv, &clock, &observer, &reader,
                                  &parser, &estimator, &estimator_used)) {
    return -1;
  }
  cloopenwebrtc::scoped_ptr<cloopenwebrtc::test::RtpFileReader> rtp_reader(reader);
  cloopenwebrtc::scoped_ptr<cloopenwebrtc::RtpHeaderParser> rtp_parser(parser);
  cloopenwebrtc::scoped_ptr<cloopenwebrtc::RemoteBitrateEstimator> rbe(estimator);

  // Process the file.
  int packet_counter = 0;
  int64_t next_rtp_time_ms = 0;
  int64_t first_rtp_time_ms = -1;
  int abs_send_time_count = 0;
  int ts_offset_count = 0;
  cloopenwebrtc::test::RtpPacket packet;
  if (!rtp_reader->NextPacket(&packet)) {
    printf("No RTP packet found\n");
    return 0;
  }
  first_rtp_time_ms = packet.time_ms;
  packet.time_ms = packet.time_ms - first_rtp_time_ms;
  while (true) {
    if (next_rtp_time_ms <= clock.TimeInMilliseconds()) {
      cloopenwebrtc::RTPHeader header;
      parser->Parse(packet.data, packet.length, &header);
      if (header.extension.hasAbsoluteSendTime)
        ++abs_send_time_count;
      if (header.extension.hasTransmissionTimeOffset)
        ++ts_offset_count;
      size_t packet_length = packet.length;
      // Some RTP dumps only include the header, in which case packet.length
      // is equal to the header length. In those cases packet.original_length
      // usually contains the original packet length.
      if (packet.original_length > 0) {
        packet_length = packet.original_length;
      }
      rbe->IncomingPacket(clock.TimeInMilliseconds(),
                          packet_length - header.headerLength,
                          header);
      ++packet_counter;
      if (!rtp_reader->NextPacket(&packet)) {
        break;
      }
      packet.time_ms = packet.time_ms - first_rtp_time_ms;
      next_rtp_time_ms = packet.time_ms;
    }
    int64_t time_until_process_ms = rbe->TimeUntilNextProcess();
    if (time_until_process_ms <= 0) {
      rbe->Process();
    }
    int64_t time_until_next_event =
        std::min(rbe->TimeUntilNextProcess(),
                 next_rtp_time_ms - clock.TimeInMilliseconds());
    clock.AdvanceTimeMilliseconds(std::max<int64_t>(time_until_next_event, 0));
  }
  printf("Parsed %d packets\nTime passed: %I64d ms\n", packet_counter,
         clock.TimeInMilliseconds());
  printf("Estimator used: %s\n", estimator_used.c_str());
  printf("Packets with absolute send time: %d\n",
         abs_send_time_count);
  printf("Packets with timestamp offset: %d\n",
         ts_offset_count);
  printf("Packets with no extension: %d\n",
         packet_counter - ts_offset_count - abs_send_time_count);
  return 0;
}
