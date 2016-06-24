/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "bwe_rtp.h"

#include <stdio.h>
#include <string>

#include "remote_bitrate_estimator.h"
#include "rtp_header_parser.h"
#include "rtp_payload_registry.h"
#include "rtp_file_reader.h"

const int kMinBitrateBps = 30000;

bool ParseArgsAndSetupEstimator(int argc,
                                char** argv,
                                cloopenwebrtc::Clock* clock,
                                cloopenwebrtc::RemoteBitrateObserver* observer,
                                cloopenwebrtc::test::RtpFileReader** rtp_reader,
                                cloopenwebrtc::RtpHeaderParser** parser,
                                cloopenwebrtc::RemoteBitrateEstimator** estimator,
                                std::string* estimator_used) {
  *rtp_reader = cloopenwebrtc::test::RtpFileReader::Create(
      cloopenwebrtc::test::RtpFileReader::kRtpDump, argv[3]);
  if (!*rtp_reader) {
    fprintf(stderr, "Cannot open input file %s\n", argv[3]);
    return false;
  }
  fprintf(stderr, "Input file: %s\n\n", argv[3]);
  cloopenwebrtc::RTPExtensionType extension = cloopenwebrtc::kRtpExtensionAbsoluteSendTime;

  if (strncmp("tsoffset", argv[1], 8) == 0) {
    extension = cloopenwebrtc::kRtpExtensionTransmissionTimeOffset;
    fprintf(stderr, "Extension: toffset\n");
  } else {
    fprintf(stderr, "Extension: abs\n");
  }
  int id = atoi(argv[2]);

  // Setup the RTP header parser and the bitrate estimator.
  *parser = cloopenwebrtc::RtpHeaderParser::Create();
  (*parser)->RegisterRtpHeaderExtension(extension, id);
  if (estimator) {
    switch (extension) {
      case cloopenwebrtc::kRtpExtensionAbsoluteSendTime: {
          cloopenwebrtc::AbsoluteSendTimeRemoteBitrateEstimatorFactory factory;
          *estimator = factory.Create(observer, clock, cloopenwebrtc::kAimdControl,
                                      kMinBitrateBps);
          *estimator_used = "AbsoluteSendTimeRemoteBitrateEstimator";
          break;
        }
      case cloopenwebrtc::kRtpExtensionTransmissionTimeOffset: {
          cloopenwebrtc::RemoteBitrateEstimatorFactory factory;
          *estimator = factory.Create(observer, clock, cloopenwebrtc::kAimdControl,
                                      kMinBitrateBps);
          *estimator_used = "RemoteBitrateEstimator";
          break;
        }
      default:
        assert(false);
    }
  }
  return true;
}
