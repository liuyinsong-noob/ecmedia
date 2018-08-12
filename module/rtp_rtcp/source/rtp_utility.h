/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_UTILITY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_UTILITY_H_

#include <map>

#include "../base/deprecation.h"
#include "../module/rtp_rtcp/include/receive_statistics.h"
#include "../module/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "../module/rtp_rtcp/source/rtp_header_extension.h"
#include "../module/rtp_rtcp/source/rtp_rtcp_config.h"
#include "../module/typedefs.h"

namespace yuntongxunwebrtc {

const uint8_t kRtpMarkerBitMask = 0x80;

RtpData* NullObjectRtpData();
RtpFeedback* NullObjectRtpFeedback();
ReceiveStatistics* NullObjectReceiveStatistics();
    
    
namespace RtpUtility {

	struct Payload {
		char name[RTP_PAYLOAD_NAME_SIZE];
		bool audio;
		PayloadUnion typeSpecific;
	};

bool StringCompare(const char* str1, const char* str2, const uint32_t length);

    void AssignUWord32ToBuffer(uint8_t* dataBuffer, uint32_t value);
    void AssignUWord24ToBuffer(uint8_t* dataBuffer, uint32_t value);
    void AssignUWord16ToBuffer(uint8_t* dataBuffer, uint16_t value);

    /**
     * Converts a network-ordered two-byte input buffer to a host-ordered value.
     * \param[in] dataBuffer Network-ordered two-byte buffer to convert.
     * \return Host-ordered value.
     */
    uint16_t BufferToUWord16(const uint8_t* dataBuffer);

    /**
     * Converts a network-ordered three-byte input buffer to a host-ordered value.
     * \param[in] dataBuffer Network-ordered three-byte buffer to convert.
     * \return Host-ordered value.
     */
    uint32_t BufferToUWord24(const uint8_t* dataBuffer);

    /**
     * Converts a network-ordered four-byte input buffer to a host-ordered value.
     * \param[in] dataBuffer Network-ordered four-byte buffer to convert.
     * \return Host-ordered value.
     */
    uint32_t BufferToUWord32(const uint8_t* dataBuffer);

// Round up to the nearest size that is a multiple of 4.
size_t Word32Align(size_t size);

class RtpHeaderParser {
 public:
  RtpHeaderParser(const uint8_t* rtpData, size_t rtpDataLength);
  ~RtpHeaderParser();

        bool RTCP() const;
        bool ParseRtcp(RTPHeader* header) const;
        bool Parse(RTPHeader* parsedPacket,
                   RtpHeaderExtensionMap* ptrExtensionMap = NULL) const;
        int setECMedia_ConferenceParticipantCallback(ECMedia_ConferenceParticipantCallback *cb);
    private:
        void ParseOneByteExtensionHeader(
            RTPHeader* parsedPacket,
            const RtpHeaderExtensionMap* ptrExtensionMap,
            const uint8_t* ptrRTPDataExtensionEnd,
            const uint8_t* ptr) const;

        uint8_t ParsePaddingBytes(
            const uint8_t* ptrRTPDataExtensionEnd,
            const uint8_t* ptr) const;

        const uint8_t* const _ptrRTPDataBegin;
        const uint8_t* const _ptrRTPDataEnd;

    };
}  // namespace RtpUtility
}  // namespace yuntongxunwebrtc

#endif  // WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_UTILITY_H_
