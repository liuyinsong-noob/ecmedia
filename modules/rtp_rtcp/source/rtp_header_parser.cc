/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "modules/rtp_rtcp/include/rtp_header_parser.h"

#include <string.h>

#include "modules/rtp_rtcp/include/rtp_header_extension_map.h"
#include "modules/rtp_rtcp/source/rtp_utility.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/thread_annotations.h"
#include "logging/rtc_event_log/rtc_event_log.h"

namespace webrtc {

class RtpHeaderParserImpl : public RtpHeaderParser {
 public:
  RtpHeaderParserImpl();
  ~RtpHeaderParserImpl() override = default;

 bool Parse(const uint8_t* packet,
              size_t length,
              RTPHeader* header) const override;

  bool RegisterRtpHeaderExtension(RTPExtensionType type, uint8_t id) override;
  bool RegisterRtpHeaderExtension(RtpExtension extension) override;

  bool DeregisterRtpHeaderExtension(RTPExtensionType type) override;
  bool DeregisterRtpHeaderExtension(RtpExtension extension) override;

   //ytx_begin by wx ÓïÒô¼¤Àø»Øµ÷º¯Êý
   int setECMediaConferenceParticipantCallback(ECMedia_ConferenceParticipantCallback* cb) override;
   void setECMediaConferenceParticipantCallbackTimeInterVal(int timeInterVal) override;
   //ytx_end
 private:
  rtc::CriticalSection critical_section_;
  RtpHeaderExtensionMap rtp_header_extension_map_
      RTC_GUARDED_BY(critical_section_);
  //ytx_begin added by wx.
  ECMedia_ConferenceParticipantCallback* _participantCallback;  
  int callConferenceParticipantCallbacktimeInterVal_;
  mutable uint8_t last_arr_csrc_count_;
  mutable uint32_t last_arrOfCSRCs_[kRtpCsrcSize];
  mutable int64_t base_time;
  //ytx_end
};

RtpHeaderParser* RtpHeaderParser::Create() {
  return new RtpHeaderParserImpl;
}
//ytx_begin by wx
RtpHeaderParserImpl::RtpHeaderParserImpl():_participantCallback(nullptr),\
callConferenceParticipantCallbacktimeInterVal_(3),last_arr_csrc_count_(0),base_time(0){
  memset(last_arrOfCSRCs_, 0, sizeof(uint32_t) * kRtpCsrcSize);
}
//ytx_end

bool RtpHeaderParser::IsRtcp(const uint8_t* packet, size_t length) {
  RtpUtility::RtpHeaderParser rtp_parser(packet, length);
  return rtp_parser.RTCP();
}

absl::optional<uint32_t> RtpHeaderParser::GetSsrc(const uint8_t* packet,
                                                  size_t length) {
  RtpUtility::RtpHeaderParser rtp_parser(packet, length);
  RTPHeader header;
  if (rtp_parser.Parse(&header, nullptr)) {
    return header.ssrc;
  }
  return absl::nullopt;
}

bool RtpHeaderParserImpl::Parse(const uint8_t* packet,
                                size_t length,
                                RTPHeader* header) const{
  RtpUtility::RtpHeaderParser rtp_parser(packet, length);
  *header = RTPHeader();

  RtpHeaderExtensionMap map;
  {
    rtc::CritScope cs(&critical_section_);
    map = rtp_header_extension_map_;
  }

  const bool valid_rtpheader = rtp_parser.Parse(header, &map);
  if (!valid_rtpheader) {
    return false;
  }
  /****  callback conference csrc array when csrc array changed, added by wx ******/
  //ytx_begin
  bool should_send = false;
  {
    int64_t current_time = Clock::GetRealTimeClock()->TimeInMilliseconds();
    should_send = (current_time - base_time) >=
                  callConferenceParticipantCallbacktimeInterVal_ * 1000;
    if (should_send) {
      base_time = Clock::GetRealTimeClock()->TimeInMilliseconds();
    }
  }
 
  if (_participantCallback && should_send) {
    if (last_arr_csrc_count_ != header->numCSRCs) {
      for (int i = 0; i < header->numCSRCs; i++) {
        RTC_LOG(INFO) << "audio csrc count changed, participantCallback i:" << i
                      << " ssrc:" << header->arrOfCSRCs[i];
      }
      _participantCallback(header->arrOfCSRCs, header->numCSRCs);
      last_arr_csrc_count_ = header->numCSRCs;
      for (int i = 0; i < header->numCSRCs; i++) {
        last_arrOfCSRCs_[i] = header->arrOfCSRCs[i];
      }
    } else if (header->numCSRCs != 0) {
      bool need_callback = true;
      for (int i = 0; i < header->numCSRCs; i++) {
        // callback only once, when csrc list change
        if (header->arrOfCSRCs[i] != last_arrOfCSRCs_[i]) {
          if (need_callback) {
            for (int i = 0; i < header->numCSRCs; i++) {
              RTC_LOG(INFO)<< "audio csrc ssrc changed, participantCallback i:" << i
                  << " ssrc:" << header->arrOfCSRCs[i];
            }
            _participantCallback(header->arrOfCSRCs, header->numCSRCs);
            need_callback = false;
          }
        }
        // copy current csrs arr.
        last_arrOfCSRCs_[i] = header->arrOfCSRCs[i];
      }
      // store last arr count.
      last_arr_csrc_count_ = header->numCSRCs;
    }
  }
  //ytx_end
  return true;
}
bool RtpHeaderParserImpl::RegisterRtpHeaderExtension(RtpExtension extension) {
  rtc::CritScope cs(&critical_section_);
  return rtp_header_extension_map_.RegisterByUri(extension.id, extension.uri);
}

bool RtpHeaderParserImpl::RegisterRtpHeaderExtension(RTPExtensionType type,
                                                     uint8_t id) {
  rtc::CritScope cs(&critical_section_);
  return rtp_header_extension_map_.RegisterByType(id, type);
}

bool RtpHeaderParserImpl::DeregisterRtpHeaderExtension(RtpExtension extension) {
  rtc::CritScope cs(&critical_section_);
  return rtp_header_extension_map_.Deregister(
      rtp_header_extension_map_.GetType(extension.id));
}

bool RtpHeaderParserImpl::DeregisterRtpHeaderExtension(RTPExtensionType type) {
  rtc::CritScope cs(&critical_section_);
  return rtp_header_extension_map_.Deregister(type) == 0;
}
//ytx_begin by wx
int RtpHeaderParserImpl::setECMediaConferenceParticipantCallback(ECMedia_ConferenceParticipantCallback* cb) {
  if (cb == nullptr) {
    RTC_LOG(LS_ERROR) << "setECMediaConferenceParticipantCallback is null.";
    return -1;
  }

  if (_participantCallback == cb) {
    RTC_LOG(LS_WARNING)<< "setECMediaConferenceParticipantCallback already have been set.";
    return 0;
  }

  _participantCallback =cb;
  return 0;
}

void RtpHeaderParserImpl::setECMediaConferenceParticipantCallbackTimeInterVal(int timeInterVal) {
  if (timeInterVal < 0) {
    RTC_LOG(LS_ERROR)<< "setECMediaConferenceParticipantCallback timeInterVal is 0.";
    timeInterVal =1;
  }

  callConferenceParticipantCallbacktimeInterVal_ = timeInterVal;
}
//ytx_end
}  // namespace webrtc
