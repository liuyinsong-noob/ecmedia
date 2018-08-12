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
                                yuntongxunwebrtc::Clock* clock,
                                yuntongxunwebrtc::RemoteBitrateObserver* observer,
                                yuntongxunwebrtc::test::RtpFileReader** rtp_reader,
                                yuntongxunwebrtc::RtpHeaderParser** parser,
                                yuntongxunwebrtc::RemoteBitrateEstimator** estimator,
                                std::string* estimator_used) {
  *rtp_reader = yuntongxunwebrtc::test::RtpFileReader::Create(
      yuntongxunwebrtc::test::RtpFileReader::kRtpDump, argv[3]);
  if (!*rtp_reader) {
    fprintf(stderr, "Cannot open input file %s\n", argv[3]);
    return false;
  }
  fprintf(stderr, "Input file: %s\n\n", argv[3]);
  yuntongxunwebrtc::RTPExtensionType extension = yuntongxunwebrtc::kRtpExtensionAbsoluteSendTime;

  if (strncmp("tsoffset", argv[1], 8) == 0) {
    extension = yuntongxunwebrtc::kRtpExtensionTransmissionTimeOffset;
    fprintf(stderr, "Extension: toffset\n");
  } else {
    fprintf(stderr, "Extension: abs\n");
  }
  int id = atoi(argv[2]);

  // Setup the RTP header parser and the bitrate estimator.
  *parser = yuntongxunwebrtc::RtpHeaderParser::Create();
  (*parser)->RegisterRtpHeaderExtension(extension, id);
  if (estimator) {
    switch (extension) {
      case yuntongxunwebrtc::kRtpExtensionAbsoluteSendTime: {
          yuntongxunwebrtc::AbsoluteSendTimeRemoteBitrateEstimatorFactory factory;
          *estimator = factory.Create(observer, clock, yuntongxunwebrtc::kAimdControl,
                                      kMinBitrateBps);
          *estimator_used = "AbsoluteSendTimeRemoteBitrateEstimator";
          break;
        }
      case yuntongxunwebrtc::kRtpExtensionTransmissionTimeOffset: {
          yuntongxunwebrtc::RemoteBitrateEstimatorFactory factory;
          *estimator = factory.Create(observer, clock, yuntongxunwebrtc::kAimdControl,
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
